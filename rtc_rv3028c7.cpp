/*
 * rtc_rv3028c7.c
 *
 *  Created on: Aug 12, 2020
 *      Author: r2
 */

/******************************************************************************
RV-3028-C7.h
RV-3028-C7 Arduino Library
Constantin Koch
July 25, 2019
https://github.com/constiko/RV-3028_C7-Arduino_Library

Development environment specifics:
Arduino IDE 1.8.9

This code is released under the [MIT License](http://opensource.org/licenses/MIT).
Please review the LICENSE.md file included with this example. If you have any questions
or concerns with licensing, please contact constantinkoch@outlook.com.
Distributed as-is; no warranty is given.
******************************************************************************/
#include <stdio.h>
#include "pico/stdlib.h"
//#include "ticker.h"  // needed for EEPROM timeout
#include "hardware/i2c.h"
#include "serif_i2c.h"
#include "rtc_rv3028c7.h"

//****************************************************************************//
//
//  Settings and configuration
//
//****************************************************************************//

// Parse the __DATE__ predefined macro to generate date defaults:
// __Date__ Format: MMM DD YYYY (First D may be a space if <10)
// <MONTH>
#define BUILD_MONTH_JAN ((__DATE__[0] == 'J') && (__DATE__[1] == 'a')) ? 1 : 0
#define BUILD_MONTH_FEB (__DATE__[0] == 'F') ? 2 : 0
#define BUILD_MONTH_MAR ((__DATE__[0] == 'M') && (__DATE__[1] == 'a') && (__DATE__[2] == 'r')) ? 3 : 0
#define BUILD_MONTH_APR ((__DATE__[0] == 'A') && (__DATE__[1] == 'p')) ? 4 : 0
#define BUILD_MONTH_MAY ((__DATE__[0] == 'M') && (__DATE__[1] == 'a') && (__DATE__[2] == 'y')) ? 5 : 0
#define BUILD_MONTH_JUN ((__DATE__[0] == 'J') && (__DATE__[1] == 'u') && (__DATE__[2] == 'n')) ? 6 : 0
#define BUILD_MONTH_JUL ((__DATE__[0] == 'J') && (__DATE__[1] == 'u') && (__DATE__[2] == 'l')) ? 7 : 0
#define BUILD_MONTH_AUG ((__DATE__[0] == 'A') && (__DATE__[1] == 'u')) ? 8 : 0
#define BUILD_MONTH_SEP (__DATE__[0] == 'S') ? 9 : 0
#define BUILD_MONTH_OCT (__DATE__[0] == 'O') ? 10 : 0
#define BUILD_MONTH_NOV (__DATE__[0] == 'N') ? 11 : 0
#define BUILD_MONTH_DEC (__DATE__[0] == 'D') ? 12 : 0
#define BUILD_MONTH BUILD_MONTH_JAN | BUILD_MONTH_FEB | BUILD_MONTH_MAR | \
BUILD_MONTH_APR | BUILD_MONTH_MAY | BUILD_MONTH_JUN | \
BUILD_MONTH_JUL | BUILD_MONTH_AUG | BUILD_MONTH_SEP | \
BUILD_MONTH_OCT | BUILD_MONTH_NOV | BUILD_MONTH_DEC
// <DATE>
#define BUILD_DATE_0 ((__DATE__[4] == ' ') ? 0 : (__DATE__[4] - 0x30))
#define BUILD_DATE_1 (__DATE__[5] - 0x30)
#define BUILD_DATE ((BUILD_DATE_0 * 10) + BUILD_DATE_1)
// <YEAR>
#define BUILD_YEAR (((__DATE__[7] - 0x30) * 1000) + ((__DATE__[8] - 0x30) * 100) + \
((__DATE__[9] - 0x30) * 10)  + ((__DATE__[10] - 0x30) * 1))

// Parse the __TIME__ predefined macro to generate time defaults:
// __TIME__ Format: HH:MM:SS (First number of each is padded by 0 if <10)
// <HOUR>
#define BUILD_HOUR_0 ((__TIME__[0] == ' ') ? 0 : (__TIME__[0] - 0x30))
#define BUILD_HOUR_1 (__TIME__[1] - 0x30)
#define BUILD_HOUR ((BUILD_HOUR_0 * 10) + BUILD_HOUR_1)
// <MINUTE>
#define BUILD_MINUTE_0 ((__TIME__[3] == ' ') ? 0 : (__TIME__[3] - 0x30))
#define BUILD_MINUTE_1 (__TIME__[4] - 0x30)
#define BUILD_MINUTE ((BUILD_MINUTE_0 * 10) + BUILD_MINUTE_1)
// <SECOND>
#define BUILD_SECOND_0 ((__TIME__[6] == ' ') ? 0 : (__TIME__[6] - 0x30))
#define BUILD_SECOND_1 (__TIME__[7] - 0x30)
#define BUILD_SECOND ((BUILD_SECOND_0 * 10) + BUILD_SECOND_1)


uint8_t rtc_time[TIME_ARRAY_LENGTH];
uint8_t rtc_alarm[3];  // TODO: This is only needed for testing.  Remove for production along with functions that use it.

/*
bool begin(TwoWire &wirePort, bool set_24Hour, bool disable_TrickleCharge, bool set_LevelSwitchingMode)
{
	//We require caller to begin their I2C port, with the speed of their choice
	//external to the library
	//_i2cPort->begin();
	_i2cPort = &wirePort;

	delay(1);
	if (set_24Hour) { set24Hour(); delay(1); }
	if (disable_TrickleCharge) { disableTrickleCharge(); delay(1); }

	return((set_LevelSwitchingMode ? setBackupSwitchoverMode(3) : true) && writeRVRegister(RV3028_STATUS, 0x00));
}
*/

bool setTime_(uint8_t sec, uint8_t min, uint8_t hour, uint8_t weekday, uint8_t date, uint8_t month, uint16_t year)
{
	rtc_time[TIME_SECONDS] = DECtoBCD(sec);
	rtc_time[TIME_MINUTES] = DECtoBCD(min);
	rtc_time[TIME_HOURS] = DECtoBCD(hour);
	rtc_time[TIME_WEEKDAY] = DECtoBCD(weekday);
	rtc_time[TIME_DATE] = DECtoBCD(date);
	rtc_time[TIME_MONTH] = DECtoBCD(month);
	rtc_time[TIME_YEAR] = DECtoBCD(year - 2000);

	bool status = false;

	if (is12Hour())
	{
		set24Hour();
		status = setTime(rtc_time, TIME_ARRAY_LENGTH);
		set12Hour();
	}
	else
	{
		status = setTime(rtc_time, TIME_ARRAY_LENGTH);
	}
	return status;
}

// setTime -- Set time and date/day registers of RV3028 (using data array)
bool setTime(uint8_t *time, uint8_t len)
{
	if (len != TIME_ARRAY_LENGTH)
		return false;

	writeMultipleRegisters(RV3028_SECONDS, time, len);

	// TODO: Check for error when writing multiple registers
    return true;
}

bool setSeconds(uint8_t value)
{
	rtc_time[TIME_SECONDS] = DECtoBCD(value);
	return setTime(rtc_time, TIME_ARRAY_LENGTH);
}

bool setMinutes(uint8_t value)
{
	rtc_time[TIME_MINUTES] = DECtoBCD(value);
	return setTime(rtc_time, TIME_ARRAY_LENGTH);
}

bool setHours(uint8_t value)
{
	rtc_time[TIME_HOURS] = DECtoBCD(value);
	return setTime(rtc_time, TIME_ARRAY_LENGTH);
}

bool setWeekday(uint8_t value)
{
	rtc_time[TIME_WEEKDAY] = DECtoBCD(value);
	return setTime(rtc_time, TIME_ARRAY_LENGTH);
}

bool setDate(uint8_t value)
{
	rtc_time[TIME_DATE] = DECtoBCD(value);
	return setTime(rtc_time, TIME_ARRAY_LENGTH);
}

bool setMonth(uint8_t value)
{
	rtc_time[TIME_MONTH] = DECtoBCD(value);
	return setTime(rtc_time, TIME_ARRAY_LENGTH);
}

bool setYear(uint16_t value)
{
	rtc_time[TIME_YEAR] = DECtoBCD(value - 2000);
	return setTime(rtc_time, TIME_ARRAY_LENGTH);
}

/*
//Takes the time from the last build and uses it as the current time
//Works very well as an arduino sketch
bool setToCompilerTime()
{
	rtc_time[TIME_SECONDS] = DECtoBCD(BUILD_SECOND);
	rtc_time[TIME_MINUTES] = DECtoBCD(BUILD_MINUTE);
	rtc_time[TIME_HOURS] = DECtoBCD(BUILD_HOUR);

	//Build_Hour is 0-23, convert to 1-12 if needed
	if (is12Hour())
	{
		uint8_t hour = BUILD_HOUR;

		bool pm = false;

		if (hour == 0)
			hour += 12;
		else if (hour == 12)
			pm = true;
		else if (hour > 12)
		{
			hour -= 12;
			pm = true;
		}

		rtc_time[TIME_HOURS] = DECtoBCD(hour); //Load the modified hours

		if (pm == true) rtc_time[TIME_HOURS] |= (1 << HOURS_AM_PM); //Set AM/PM bit if needed
	}

	// Calculate weekday (from here: http://stackoverflow.com/a/21235587)
	// 0 = Sunday, 6 = Saturday
	uint16_t d = BUILD_DATE;
	uint16_t m = BUILD_MONTH;
	uint16_t y = BUILD_YEAR;
	uint16_t weekday = (d += m < 3 ? y-- : y - 2, 23 * m / 9 + d + 4 + y / 4 - y / 100 + y / 400) % 7 + 1;
	rtc_time[TIME_WEEKDAY] = DECtoBCD(weekday);

	rtc_time[TIME_DATE] = DECtoBCD(BUILD_DATE);
	rtc_time[TIME_MONTH] = DECtoBCD(BUILD_MONTH);
	rtc_time[TIME_YEAR] = DECtoBCD(BUILD_YEAR - 2000); //! Not Y2K (or Y2.1K)-proof :(

	return setTime(rtc_time, TIME_ARRAY_LENGTH);
}
*/

//Move the hours, mins, sec, etc registers from RV-3028-C7 into the rtc_time array
//Needs to be called before printing time or date
//We do not protect the GPx registers. They will be overwritten. The user has plenty of RAM if they need it.
bool updateTime()
{
	if (readMultipleRegisters(RV3028_SECONDS, rtc_time, TIME_ARRAY_LENGTH) == false)
		return(false); //Something went wrong

	if (is12Hour()) rtc_time[TIME_HOURS] &= ~(1 << HOURS_AM_PM); //Remove this bit from value

	return true;
}

//Returns a pointer to array of chars that are the date in mm/dd/yyyy format because they're weird
char* stringDateUSA()
{
	static char date[11]; //Max of mm/dd/yyyy with \0 terminator
	sprintf(date, "%02hhu/%02hhu/20%02hhu", BCDtoDEC(rtc_time[TIME_MONTH]), BCDtoDEC(rtc_time[TIME_DATE]), BCDtoDEC(rtc_time[TIME_YEAR]));
	return(date);
}

//Returns a pointer to array of chars that are the date in dd/mm/yyyy format
char*  stringDate()
{
	static char date[11]; //Max of dd/mm/yyyy with \0 terminator
	sprintf(date, "%02hhu/%02hhu/20%02hhu", BCDtoDEC(rtc_time[TIME_DATE]), BCDtoDEC(rtc_time[TIME_MONTH]), BCDtoDEC(rtc_time[TIME_YEAR]));
	return(date);
}

//Returns a pointer to array of chars that represents the time in hh:mm:ss format
//Adds AM/PM if in 12 hour mode
char* stringTime()
{
	static char time[11]; //Max of hh:mm:ssXM with \0 terminator

	if (is12Hour() == true)
	{
		char half = 'A';
		if (isPM()) half = 'P';

		sprintf(time, "%02hhu:%02hhu:%02hhu%cM", BCDtoDEC(rtc_time[TIME_HOURS]), BCDtoDEC(rtc_time[TIME_MINUTES]), BCDtoDEC(rtc_time[TIME_SECONDS]), half);
	}
	else
		sprintf(time, "%02hhu:%02hhu:%02hhu", BCDtoDEC(rtc_time[TIME_HOURS]), BCDtoDEC(rtc_time[TIME_MINUTES]), BCDtoDEC(rtc_time[TIME_SECONDS]));

	return(time);
}

char* stringTimeStamp()
{
	static char timeStamp[25]; //Max of yyyy-mm-ddThh:mm:ss.ss with \0 terminator

	if (is12Hour() == true)
	{
		char half = 'A';
		if (isPM()) half = 'P';

		sprintf(timeStamp, "20%02hhu-%02hhu-%02hhuT%02hhu:%02hhu:%02hhu%cM", BCDtoDEC(rtc_time[TIME_YEAR]), BCDtoDEC(rtc_time[TIME_MONTH]), BCDtoDEC(rtc_time[TIME_DATE]), BCDtoDEC(rtc_time[TIME_HOURS]), BCDtoDEC(rtc_time[TIME_MINUTES]), BCDtoDEC(rtc_time[TIME_SECONDS]), half);
	}
	else
		sprintf(timeStamp, "20%02hhu-%02hhu-%02hhuT%02hhu:%02hhu:%02hhu", BCDtoDEC(rtc_time[TIME_YEAR]), BCDtoDEC(rtc_time[TIME_MONTH]), BCDtoDEC(rtc_time[TIME_DATE]), BCDtoDEC(rtc_time[TIME_HOURS]), BCDtoDEC(rtc_time[TIME_MINUTES]), BCDtoDEC(rtc_time[TIME_SECONDS]));

	return(timeStamp);
}

uint8_t getSeconds()
{
	return BCDtoDEC(rtc_time[TIME_SECONDS]);
}

uint8_t getMinutes()
{
	return BCDtoDEC(rtc_time[TIME_MINUTES]);
}

uint8_t getHours()
{
	return BCDtoDEC(rtc_time[TIME_HOURS]);
}

uint8_t getWeekday()
{
	return BCDtoDEC(rtc_time[TIME_WEEKDAY]);
}

uint8_t getDate()
{
	return BCDtoDEC(rtc_time[TIME_DATE]);
}

uint8_t getMonth()
{
	return BCDtoDEC(rtc_time[TIME_MONTH]);
}

uint16_t getYear()
{
	return BCDtoDEC(rtc_time[TIME_YEAR]) + 2000;
}

//Returns true if RTC has been configured for 12 hour mode
bool is12Hour()
{
	uint8_t controlRegister2 = readRVRegister(RV3028_CTRL2);
	return(controlRegister2 & (1 << CTRL2_12_24));
}

//Returns true if RTC has PM bit set and 12Hour bit set
bool isPM()
{
	uint8_t hourRegister = readRVRegister(RV3028_HOURS);
	if (is12Hour() && (hourRegister & (1 << HOURS_AM_PM)))
		return(true);
	return(false);
}

//Configure RTC to output 1-12 hours
//Converts any current hour setting to 12 hour
void set12Hour()
{
	//Do we need to change anything?
	if (is12Hour() == false)
	{
		uint8_t hour = BCDtoDEC(readRVRegister(RV3028_HOURS)); //Get the current hour in the RTC

															 //Set the 12/24 hour bit
		uint8_t setting = readRVRegister(RV3028_CTRL2);
		setting |= (1 << CTRL2_12_24);
		writeRVRegister(RV3028_CTRL2, setting);

		//Take the current hours and convert to 12, complete with AM/PM bit
		bool pm = false;

		if (hour == 0)
			hour += 12;
		else if (hour == 12)
			pm = true;
		else if (hour > 12)
		{
			hour -= 12;
			pm = true;
		}

		hour = DECtoBCD(hour); //Convert to BCD

		if (pm == true) hour |= (1 << HOURS_AM_PM); //Set AM/PM bit if needed

		writeRVRegister(RV3028_HOURS, hour); //Record this to hours register
	}
}

//Configure RTC to output 0-23 hours
//Converts any current hour setting to 24 hour
void set24Hour()
{
	//Do we need to change anything?
	if (is12Hour() == true)
	{
		//Not sure what changing the CTRL2 register will do to hour register so let's get a copy
		uint8_t hour = readRVRegister(RV3028_HOURS); //Get the current 12 hour formatted time in BCD
		bool pm = false;
		if (hour & (1 << HOURS_AM_PM)) //Is the AM/PM bit set?
		{
			pm = true;
			hour &= ~(1 << HOURS_AM_PM); //Clear the bit
		}

		//Change to 24 hour mode
		uint8_t setting = readRVRegister(RV3028_CTRL2);
		setting &= ~(1 << CTRL2_12_24); //Clear the 12/24 hr bit
		writeRVRegister(RV3028_CTRL2, setting);

		//Given a BCD hour in the 1-12 range, make it 24
		hour = BCDtoDEC(hour); //Convert core of register to DEC

		if (pm == true) hour += 12; //2PM becomes 14
		if (hour == 12) hour = 0; //12AM stays 12, but should really be 0
		if (hour == 24) hour = 12; //12PM becomes 24, but should really be 12

		hour = DECtoBCD(hour); //Convert to BCD

		writeRVRegister(RV3028_HOURS, hour); //Record this to hours register
	}
}

//ATTENTION: Real Time and UNIX Time are INDEPENDENT!
bool setUNIX(uint32_t value)
{
	uint8_t unix_reg[4];

	unix_reg[0] = value;
	unix_reg[1] = value >> 8;
	unix_reg[2] = value >> 16;
	unix_reg[3] = value >> 24;

	writeMultipleRegisters(RV3028_UNIX_TIME0, unix_reg, 4);

	// TODO: Check for error when writing multiple registers
    return true;
}

//ATTENTION: Real Time and UNIX Time are INDEPENDENT!
uint32_t getUNIX()
{
	uint32_t unix_time = 0;
	uint8_t unix_reg[4];

	readMultipleRegisters(RV3028_UNIX_TIME0, unix_reg, 4);


	unix_time = ((uint32_t)unix_reg[3] << 24) | ((uint32_t)unix_reg[2] << 16) | ((uint32_t)unix_reg[1] << 8) | unix_reg[0];

	return unix_time;
}

/*********************************
Set the alarm mode in the following way:
0: When minutes, hours and weekday/date match (once per weekday/date)
1: When hours and weekday/date match (once per weekday/date)
2: When minutes and weekday/date match (once per hour per weekday/date)
3: When weekday/date match (once per weekday/date)
4: When hours and minutes match (once per day)
5: When hours match (once per day)
6: When minutes match (once per hour)
7: All disabled � Default value
If you want to set a weekday alarm (setWeekdayAlarm_not_Date = true), set 'date_or_weekday' from 0 (Sunday) to 6 (Saturday)
********************************/
void setAlarmInterrupt(uint8_t min, uint8_t hour, uint8_t date_or_weekday, bool setWeekdayAlarm_not_Date, uint8_t mode, bool enable_clock_output)
{
	//disable Alarm Interrupt to prevent accidental interrupts during configuration
	//disableAlarmInterrupt();

	//clearAlarmInterruptFlag();  // Do not clear the interrupt flag

	//ENHANCEMENT: Add Alarm in 12 hour mode
	set24Hour();

	//Set WADA bit (Weekday/Date Alarm)
	if (setWeekdayAlarm_not_Date)
		clearBit(RV3028_CTRL1, CTRL1_WADA);
	else
		setBit(RV3028_CTRL1, CTRL1_WADA);

	//Write alarm settings in registers 0x07 to 0x09
	uint8_t alarmTime[3];
	alarmTime[0] = DECtoBCD(min);				//minutes
	alarmTime[1] = DECtoBCD(hour);				//hours
	alarmTime[2] = DECtoBCD(date_or_weekday);	//date or weekday
	//shift alarm enable bits
	if (mode > 0b111) mode = 0b111; //0 to 7 is valid
	if (mode & 0b001)
		alarmTime[0] |= 1 << MINUTESALM_AE_M;
	if (mode & 0b010)
		alarmTime[1] |= 1 << HOURSALM_AE_H;
	if (mode & 0b100)
		alarmTime[2] |= 1 << DATE_AE_WD;
	//Write registers
	writeMultipleRegisters(RV3028_MINUTES_ALM, alarmTime, 3);
	uint8_t stored_minutes = 0;
	stored_minutes = getAlarmMinutes();

	//enable Alarm Interrupt
	// enableAlarmInterrupt();  // don't enable the alarm interrupt

	//Clock output?
	if (enable_clock_output)
		setBit(RV3028_INT_MASK, IMT_MASK_CAIE);
	else
		clearBit(RV3028_INT_MASK, IMT_MASK_CAIE);
}

void enableAlarmInterrupt()
{
	setBit(RV3028_CTRL2, CTRL2_AIE);
}

//Only disables the interrupt (not the alarm flag)
void disableAlarmInterrupt()
{
	clearBit(RV3028_CTRL2, CTRL2_AIE);
}

bool readAlarmInterruptFlag()
{
	return readBit(RV3028_STATUS, STATUS_AF);
}

void clearAlarmInterruptFlag()
{
	clearBit(RV3028_STATUS, STATUS_AF);
}

bool updateAlarm()
{
	//Write registers
	if(readMultipleRegisters(RV3028_MINUTES_ALM, rtc_alarm, 3) == false);
		return(false); //Something went wrong

	//if (is12Hour()) rtc_time[TIME_HOURS] &= ~(1 << HOURS_AM_PM); //Remove this bit from value
	return true;
}

uint8_t getAlarmMinutes()
{
	uint8_t minutes = 0;
	readMultipleRegisters(RV3028_MINUTES_ALM, &minutes, (uint8_t) 1);
	minutes = minutes & 0x7f;
	return minutes;
}

uint8_t getAlarmHours()
{
	uint8_t hours = 0;
	readMultipleRegisters(RV3028_HOURS_ALM, &hours, (uint8_t) 1);
	return hours & 0x3f;
}

uint8_t getAlarmWeekday()
{
	uint8_t date = 0;
	readMultipleRegisters(RV3028_DATE_ALM, &date, (uint8_t) 1);
	return date;
}







/*********************************
Countdown Timer Interrupt
********************************/
void setTimer(bool timer_repeat, uint16_t timer_frequency, uint16_t timer_value, bool set_interrupt, bool start_timer, bool enable_clock_output)
{
	disableTimer();
	disableTimerInterrupt();
	clearTimerInterruptFlag();

	writeRVRegister(RV3028_TIMERVAL_0, timer_value & 0xff);
	writeRVRegister(RV3028_TIMERVAL_1, timer_value >> 8);

	uint8_t ctrl1_val = readRVRegister(RV3028_CTRL1);
	if (timer_repeat)
	{
		ctrl1_val |= 1 << CTRL1_TRPT;
	}
	else
	{
		ctrl1_val &= ~(1 << CTRL1_TRPT);
	}
	switch (timer_frequency)
	{
	case 4096:		// 4096Hz (default)		// up to 122us error on first time
		ctrl1_val &= ~3; // Clear both the bits
		break;

	case 64:		// 64Hz					// up to 7.813ms error on first time
		ctrl1_val &= ~3; // Clear both the bits
		ctrl1_val |= 1;
		break;

	case 1:			// 1Hz					// up to 7.813ms error on first time
		ctrl1_val &= ~3; // Clear both the bits
		ctrl1_val |= 2;
		break;

	case 60000:		// 1/60Hz				// up to 7.813ms error on first time
		ctrl1_val |= 3; // Set both bits
		break;

	}

	if (set_interrupt)
	{
		enableTimerInterrupt();
	}
	if (start_timer)
	{
		ctrl1_val |= (1 << CTRL1_TE);
	}
	writeRVRegister(RV3028_CTRL1, ctrl1_val);

	//Clock output?
	if (enable_clock_output)
		setBit(RV3028_INT_MASK, IMT_MASK_CTIE);
	else
		clearBit(RV3028_INT_MASK, IMT_MASK_CTIE);
}


void enableTimerInterrupt()
{
	setBit(RV3028_CTRL2, CTRL2_TIE);
}

void disableTimerInterrupt()
{
	clearBit(RV3028_CTRL2, CTRL2_TIE);
}

bool readTimerInterruptFlag()
{
	return readBit(RV3028_STATUS, STATUS_TF);
}

void clearTimerInterruptFlag()
{
	clearBit(RV3028_STATUS, STATUS_TF);
}

void enableTimer()
{
	setBit(RV3028_CTRL1, CTRL1_TE);
}

void disableTimer()
{
	clearBit(RV3028_CTRL1, CTRL1_TE);
}

/*********************************
Periodic Time Update Interrupt
********************************/
void enablePeriodicUpdateInterrupt(bool every_second, bool enable_clock_output)
{
	disablePeriodicUpdateInterrupt();
	clearPeriodicUpdateInterruptFlag();

	if (every_second)
	{
		clearBit(RV3028_CTRL1, CTRL1_USEL);
	}
	else
	{	// every minute
		setBit(RV3028_CTRL1, CTRL1_USEL);
	}

	setBit(RV3028_CTRL2, CTRL2_UIE);

	//Clock output?
	if (enable_clock_output)
		setBit(RV3028_INT_MASK, IMT_MASK_CUIE);
	else
		clearBit(RV3028_INT_MASK, IMT_MASK_CUIE);
}

void disablePeriodicUpdateInterrupt()
{
	clearBit(RV3028_CTRL2, CTRL2_UIE);
}

bool readPeriodicUpdateInterruptFlag()
{
	return readBit(RV3028_STATUS, STATUS_UF);
}

void clearPeriodicUpdateInterruptFlag()
{
	clearBit(RV3028_STATUS, STATUS_UF);
}

/*********************************
Enable the Trickle Charger and set the Trickle Charge series resistor (default is 15k)
TCR_3K  =  3kOhm
TCR_5K  =  5kOhm
TCR_9K  =  9kOhm
TCR_15K = 15kOhm
*********************************/
void enableTrickleCharge(uint8_t tcr)
{
	if (tcr > 3) return;

	//Read EEPROM Backup Register (0x37)
	uint8_t EEPROMBackup = readConfigEEPROM_RAMmirror(EEPROM_Backup_Register);
	//Set TCR Bits (Trickle Charge Resistor)
	EEPROMBackup &= EEPROMBackup_TCR_CLEAR;		//Clear TCR Bits
	EEPROMBackup |= tcr << EEPROMBackup_TCR_SHIFT;	//Shift values into EEPROM Backup Register
	//Write 1 to TCE Bit
	EEPROMBackup |= 1 << EEPROMBackup_TCE_BIT;
	//Write EEPROM Backup Register
	writeConfigEEPROM_RAMmirror(EEPROM_Backup_Register, EEPROMBackup);
}

void disableTrickleCharge()
{
	//Read EEPROM Backup Register (0x37)
	uint8_t EEPROMBackup = readConfigEEPROM_RAMmirror(EEPROM_Backup_Register);
	//Write 0 to TCE Bit
	EEPROMBackup &= ~(1 << EEPROMBackup_TCE_BIT);
	//Write EEPROM Backup Register
	writeConfigEEPROM_RAMmirror(EEPROM_Backup_Register, EEPROMBackup);
}


/*********************************
0 = Switchover disabled
1 = Direct Switching Mode
2 = Standby Mode
3 = Level Switching Mode
*********************************/
bool setBackupSwitchoverMode(uint8_t val)
{
	if (val > 3)
		return false;

	bool success = true;

	// Read EEPROM Backup Register (0x37)
	uint8_t EEPROMBackup = readConfigEEPROM_RAMmirror(EEPROM_Backup_Register);
	uint8_t switchoverMode = EEPROMBackup & 0b00011100; // preserve bit 4 (FEDE bit) and bits 3:2 (Backup Switchover Mode, BSM bits)
	if (EEPROMBackup == 0xFF)
		return false;

	else if (switchoverMode == 0x1C)  // Return immediately if switchoverMode == 0b00011100
		return true;


	else
	{
		// Ensure FEDE Bit is set to 1
		EEPROMBackup |= 1 << EEPROMBackup_FEDE_BIT;

		// Set BSM Bits (Backup Switchover Mode)
		EEPROMBackup &= EEPROMBackup_BSM_CLEAR;		//Clear BSM Bits of EEPROM Backup Register

		EEPROMBackup |= val << EEPROMBackup_BSM_SHIFT;	//Shift values into EEPROM Backup Register

		// Write EEPROM Backup Register
		if (!writeConfigEEPROM_RAMmirror(EEPROM_Backup_Register, EEPROMBackup))
			success = false;

		return success;
	}
}


/*********************************
Clock Out functions
********************************/
void enableClockOut(uint8_t freq)
{
	if (freq > 7) return; // check out of bounds
	//Read EEPROM CLKOUT Register (0x35)
	uint8_t EEPROMClkout = readConfigEEPROM_RAMmirror(EEPROM_Clkout_Register);
	//Ensure CLKOE Bit is set to 1
	EEPROMClkout |= 1 << EEPROMClkout_CLKOE_BIT;
	//Shift values into EEPROM Backup Register
	EEPROMClkout |= freq << EEPROMClkout_FREQ_SHIFT;
	//Write EEPROM Backup Register
	writeConfigEEPROM_RAMmirror(EEPROM_Clkout_Register, EEPROMClkout);
}

void enableInterruptControlledClockout(uint8_t freq)
{
	if (freq > 7) return; // check out of bounds
	//Read EEPROM CLKOUT Register (0x35)
	uint8_t EEPROMClkout = readConfigEEPROM_RAMmirror(EEPROM_Clkout_Register);
	//Shift values into EEPROM Backup Register
	EEPROMClkout |= freq << EEPROMClkout_FREQ_SHIFT;
	//Write EEPROM Backup Register
	writeConfigEEPROM_RAMmirror(EEPROM_Clkout_Register, EEPROMClkout);

	//Set CLKIE Bit
	setBit(RV3028_CTRL2, CTRL2_CLKIE);
}

void disableClockOut()
{
	//Read EEPROM CLKOUT Register (0x35)
	uint8_t EEPROMClkout = readConfigEEPROM_RAMmirror(EEPROM_Clkout_Register);
	//Clear CLKOE Bit
	EEPROMClkout &= ~(1 << EEPROMClkout_CLKOE_BIT);
	//Write EEPROM CLKOUT Register
	writeConfigEEPROM_RAMmirror(EEPROM_Clkout_Register, EEPROMClkout);

	//Clear CLKIE Bit
	clearBit(RV3028_CTRL2, CTRL2_CLKIE);
}

bool readClockOutputInterruptFlag()
{
	return readBit(RV3028_STATUS, STATUS_CLKF);
}

void clearClockOutputInterruptFlag()
{
	clearBit(RV3028_STATUS, STATUS_CLKF);
}


//Returns the status byte
uint8_t status(void)
{
	return(readRVRegister(RV3028_STATUS));
}

void clearInterrupts() //Read the status register to clear the current interrupt flags
{
	writeRVRegister(RV3028_STATUS, 0);
}





/*********************************
FOR INTERNAL USE
********************************/
uint8_t BCDtoDEC(uint8_t val)
{
	return ((val / 0x10) * 10) + (val % 0x10);
}

// BCDtoDEC -- convert decimal to binary-coded decimal (BCD)
uint8_t DECtoBCD(uint8_t val)
{
	return ((val / 10) * 0x10) + (val % 10);
}

uint8_t readRVRegister(uint8_t slave_reg)
{
	uint8_t register_value = 0;

	serif_i2c_read_reg(RV3028_ADDR, slave_reg, &register_value, (uint8_t) 1);

	return register_value;
}

bool writeRVRegister(uint8_t addr, uint8_t val)
{
	serif_i2c_write_reg(RV3028_ADDR, addr, &val, (uint8_t) 1);

	// TODO:  check for "false" condition
	return(true);
}

bool readMultipleRegisters(uint8_t addr, uint8_t *dest, uint8_t len)
{

	serif_i2c_read_reg(RV3028_ADDR, addr, dest, len);

	return(true);
}

bool writeMultipleRegisters(uint8_t addr, uint8_t *values, uint8_t len)
{
	serif_i2c_write_reg(RV3028_ADDR, addr, values, len);

	return(true);
}

bool writeConfigEEPROM_RAMmirror(uint8_t eepromaddr, uint8_t val)
{
	bool success = waitforEEPROM();

	//Disable auto refresh by writing 1 to EERD control bit in CTRL1 register
	uint8_t ctrl1 = readRVRegister(RV3028_CTRL1);
	ctrl1 |= 1 << CTRL1_EERD;
	if (!writeRVRegister(RV3028_CTRL1, ctrl1)) success = false;
	//Write Configuration RAM Register
	writeRVRegister(eepromaddr, val);
	//Update EEPROM (All Configuration RAM -> EEPROM)
	writeRVRegister(RV3028_EEPROM_CMD, EEPROMCMD_First);
	writeRVRegister(RV3028_EEPROM_CMD, EEPROMCMD_Update);
	if (!waitforEEPROM()) success = false;
	//Reenable auto refresh by writing 0 to EERD control bit in CTRL1 register
	ctrl1 = readRVRegister(RV3028_CTRL1);
	if (ctrl1 == 0x00)success = false;
	ctrl1 &= ~(1 << CTRL1_EERD);
	writeRVRegister(RV3028_CTRL1, ctrl1);
	if (!waitforEEPROM()) success = false;

	return success;
}

uint8_t readConfigEEPROM_RAMmirror(uint8_t eepromaddr)
{
	bool success = waitforEEPROM();

	//Disable auto refresh by writing 1 to EERD control bit in CTRL1 register
	uint8_t ctrl1 = readRVRegister(RV3028_CTRL1);
	ctrl1 |= 1 << CTRL1_EERD;
	if (!writeRVRegister(RV3028_CTRL1, ctrl1)) success = false;
	//Read EEPROM Register
	writeRVRegister(RV3028_EEPROM_ADDR, eepromaddr);
	writeRVRegister(RV3028_EEPROM_CMD, EEPROMCMD_First);
	writeRVRegister(RV3028_EEPROM_CMD, EEPROMCMD_ReadSingle);
	if (!waitforEEPROM()) success = false;
	uint8_t eepromdata = readRVRegister(RV3028_EEPROM_DATA);
	if (!waitforEEPROM()) success = false;
	//Reenable auto refresh by writing 0 to EERD control bit in CTRL1 register
	ctrl1 = readRVRegister(RV3028_CTRL1);
	if (ctrl1 == 0x00)success = false;
	ctrl1 &= ~(1 << CTRL1_EERD);
	writeRVRegister(RV3028_CTRL1, ctrl1);

	if (!success) return 0xFF;
	return eepromdata;
}

//True if success, false if timeout occurred
bool waitforEEPROM()
{
	/*
	//uint32_t timeout = millis() + 500;  // TODO: use the Pico command
	while ((readRVRegister(RV3028_STATUS) & 1 << STATUS_EEBUSY) && millis() < timeout);

	return millis() < timeout;
	*/
	return true;
}


void reset()
{
	setBit(RV3028_CTRL2, CTRL2_RESET);
}


void setBit(uint8_t reg_addr, uint8_t bit_num)
{
	uint8_t value = readRVRegister(reg_addr);
	value |= (1 << bit_num); //Set the bit
	writeRVRegister(reg_addr, value);
}

void clearBit(uint8_t reg_addr, uint8_t bit_num)
{
	uint8_t value = readRVRegister(reg_addr);
	value &= ~(1 << bit_num); //Clear the bit
	writeRVRegister(reg_addr, value);
}

bool readBit(uint8_t reg_addr, uint8_t bit_num)
{
	uint8_t value = readRVRegister(reg_addr);
	value &= (1 << bit_num);
	return value;

}
