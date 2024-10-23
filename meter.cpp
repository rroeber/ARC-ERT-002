/*
 * OTAA Encoder Reader device
*/


#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/sleep.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "tusb.h"
#include "lorawan.h"
#include "meter.h"
#include "porting_functions.h"

#include "encoder.h"
#include "battery.h"
#include "rtc_rv3028c7.h"
#include "serif_i2c.h"



// Definitions for setting the real time clock
#define RTC_SECOND (uint8_t) 0
#define RTC_MINUTE (uint8_t) 3
#define RTC_HOUR (uint8_t) 15
#define RTC_DAY (uint8_t) 2
#define RTC_DATE (uint8_t) 21
#define RTC_MONTH (uint8_t) 10
#define RTC_YEAR (int) 2024
#define EPOCHTIME_INITIAL (uint32_t) 1729523015

//#define INITIAL_START // When true, clock initialization is executed
#undef INITIAL_START // When true, clock initialization is executed

#define MAX_JOIN_ATTEMPTS (uint8_t) 5
#define ALARM_MINUTE (uint8_t) 10  // RTC alarm minute



// ARC-ERT-002 LoraWAN keys
const char *devEui = "9082319c5e7fb97d";
const char *appEui = "6081F9710187ECA0";  // Not used, but set anyway
const char *appKey = "18a9af55aa8904cdd00bd297624ececa";



volatile bool lora_interrupt = false;
char xmit_buf[52];

const sRFM_pins RFM_pins = {
  .CS = RFM_CS,
  .RST = RFM_RESET,
  .DIO0 = RFM_DIO0,
  .DIO1 = RFM_DIO1,
};


void send_data(char* sequence, char* timestamp, uint32_t epoch_time, float volts, char* serial_number, char* odometer)
{
  uint8_t i = 0;              // Loop counter
  uint8_t index = 0;          // Index counter for the transmit buffer
  uint8_t high_byte = 0;      // Most significant byte of a uint16
  uint8_t low_byte = 0;       // Least significant byte of a uint16

  const char hex[] = "0123456789abcdef";
  char buf[8];

  // Add the sequence number to the transmit buffer - four ascii characters
  xmit_buf[index++] = sequence[0];   
  xmit_buf[index++] = sequence[1];
  xmit_buf[index++] = sequence[2];
  xmit_buf[index++] = sequence[3];
  xmit_buf[index++] = sequence[4];

  xmit_buf[index++] = FIELD_SEPARATOR;


  // Add the epoch_time to the transmit buffer - a uint32 using 8 hexadecimal digits
  buf[0] = hex[epoch_time & 0x0f];
  buf[1] = hex[(epoch_time >> 4) & 0x0f];
  buf[2] = hex[(epoch_time >> 8) & 0x0f];
  buf[3] = hex[(epoch_time >> 12) & 0x0f];
  buf[4] = hex[(epoch_time >> 16) & 0x0f];
  buf[5] = hex[(epoch_time >> 20) & 0x0f];
  buf[6] = hex[(epoch_time >> 24) & 0x0f];
  buf[7] = hex[epoch_time >> 28];

  xmit_buf[index++] = buf[7];
  xmit_buf[index++] = buf[6];
  xmit_buf[index++] = buf[5];
  xmit_buf[index++] = buf[4];
  xmit_buf[index++] = buf[3];
  xmit_buf[index++] = buf[2];
  xmit_buf[index++] = buf[1];
  xmit_buf[index++] = buf[0];

  xmit_buf[index++] = FIELD_SEPARATOR;

  // Add the battery voltage to the transmit buffer - a float value converted to ascii with 3 decimal points
  sprintf(buf, "%5.3f", volts);
  for(i=0; i<5; i++)
  {
      xmit_buf[index++] = buf[i];
  }

  xmit_buf[index++] = FIELD_SEPARATOR;

  // Add the meter serial number to the transmit buffer
  for(i=0; i<SERIAL_NUMBER_LENGTH; i++)
  {
      xmit_buf[index++] = serial_number[i];
  }

  xmit_buf[index++] = FIELD_SEPARATOR;

  // Add the meter odometer reading to the transmit buffer
  for(i=0; i<ODOMETER_LENGTH; i++)
  {
      xmit_buf[index++] = odometer[i];
  }

  printf("     Sending: %s", xmit_buf);
  lora.sendUplink(xmit_buf, strlen(xmit_buf), 0, 1);
  printf("    ... complete\n");

  
  // Check Lora RX
  //printf("Update xmit/rcv cycle ");
  lora.update();   // TODO: uncomment this after testing

  //Switch RFM to sleep
  //DON'T USE Switch mode function
  RFM_Write(RFM_REG_OP_MODE, RFM_MODE_SLEEP);  // TODO: Uncomment this after testing
}

bool join_network()
{
  uint8_t join_count = 0;


  if(!lora.init()){
    printf("RFM95 not detected\n");
    sleep_ms(5000);
    return false;
  }

  // Set LoRaWAN Class change CLASS_A or CLASS_C
  lora.setDeviceClass(CLASS_A);

  // Set Data Rate
  lora.setDataRate(SF9BW125);

  // set channel to random
  lora.setChannel(MULTI);
  
  // Put OTAA Key and DevAddress here
  lora.setDevEUI(devEui);
  lora.setAppEUI(appEui);
  lora.setAppKey(appKey);


  for(join_count = 0; join_count < MAX_JOIN_ATTEMPTS; join_count++)
  {
    printf("     Attempt %u of %u tries\n", join_count+1, MAX_JOIN_ATTEMPTS);
    if(lora.join())
    {
      printf("     Joined network\n");
      return true;  // successfully joined the network
    }
    else
    {
      sleep_ms(1000);
    }
  }

  return false;  // Reached maximum attempts without joining the network
}

// Print the RV3028 RTC STATUS, CONTROL REG 1 and CONTROL REG 2 values to the console
void print_rv_regs()
{
  uint8_t reg_value = 0;

  // Print the status register
  reg_value = readRVRegister(RV3028_STATUS);
  printf("     Status: %08b\n", reg_value);

  // Print Control Register 1
  reg_value = readRVRegister(RV3028_CTRL1);
  printf("     Ctrl1:  %08b\n", reg_value);

  // Print Control Register 1
  reg_value = readRVRegister(RV3028_CTRL2);
  printf("     Ctrl2:  %08b\n", reg_value);
}


void setup()
{
  // If INITIAL_START is defined, compile the code to initialize the RTC
  #ifdef INITIAL_START

    // Initialize the external Real Time Clock - NOT the Pico onboard RTC
    if(setBackupSwitchoverMode(3))  // 3-> Level Switching Mode
    {
      printf("RTC Level Switching Mode configured\n");
    }

    // Set the time
    set24Hour();
    setTime_(RTC_SECOND, RTC_MINUTE, RTC_HOUR, RTC_DAY, RTC_DATE, RTC_MONTH, RTC_YEAR);
    setUNIX(EPOCHTIME_INITIAL);  // Set the epoch time

    updateTime();
    printf("Date: %s %s\n", stringDateUSA(), stringTime());

    // Set the alarm and print the next alarm time

    // min: <alarm minute>
    // hour: <alarm hour>
    // date_or_weekday: <calendar date or day-of-week depending on the next parameter
    // setWeekdayAlarm_not_Date: Use weekday or use date
    // mode: <see rtc_rv30287.c for mode descriptions> (e.g., 6 means alarm when minutes match, or once hourly on the set minute)
    // enable_clock_output:
    clearBit(RV3028_STATUS, STATUS_AF);  // Clear possible pending interrupt
    setAlarmInterrupt(ALARM_MINUTE, 10, 3, false, 6, false);  // This function also enables alarm interrupt
    printf("Alarm set to: Weekday: %x  Hours: %x  Minutes: %x\n", getAlarmWeekday(), getAlarmHours(), getAlarmMinutes());  

    while(1);  // Stop here after setting the clock
  #endif
}


int main()
{
  uint8_t i = 0;                                  // loop counter
  uint8_t lora_rxbuffer[34] = {0};                // Receive buffer for commands from handheld
  uint8_t meter_data[NUM_ENCODER_CHARS+1] = {0};  // Array for encoder data plus 1 byte for NULL
  uint8_t minute = 0;
  uint8_t hour = 0;
  uint32_t epoch_time = 0;                         // Epoch time variable
  char date_time[24];                             // Array for holding the date time value
  char sequence_num[5];                           // Array to hold the ascii representation of the sequence number
  char odometer[7];                                // Array for storing the meter odometer value
  char serial_number[10];                         // Array for storing the serial number
  uint16_t sequence = 0;
  float volts = 0.0;


  // initialize stdio and wait for USB CDC connect
  stdio_init_all();

  // A delay when not using tud_cdc_connected()
  for(uint8_t i = 0; i<3; i++)
  {
      puts("About to begin...");
      sleep_ms(1000);
  }
  printf("\nPico LoRaWAN - JRG Meter Encoder Reader\n\n");


  //gpio_init(RTC_INT_PIN);     // Initialize the pin as gpio, input
  battery_voltage_init();     // Initialize the battery voltage ADC and GPIO

  // Initialize I2C module
  i2c_init(I2C_PORT, 100 * 1000);
  gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
  gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
  gpio_pull_up(I2C_SDA_PIN);
  gpio_pull_up(I2C_SCL_PIN);

  setup();


  while(1)
  {
    // Get the RTC time
    updateTime();
    epoch_time = getUNIX();
    minute = getMinutes();
    hour = getHours();

    sprintf(date_time, "%s", stringTimeStamp());
    printf("Date: %s %s\n", stringDateUSA(), stringTime());

    print_rv_regs();  // Print the RTC registers

    encoder_read(meter_data);    // Read the encoder data

    // Extract the meter serial number
    for(i=0; i<9; i++)
    {
        serial_number[i] = meter_data[i+13];
    }

    // Extract the meter odometer reading
    for(i=0; i<6; i++)
    {
        odometer[i] = meter_data[i+4];
    }

    // Get the battery voltage
    volts = get_battery_voltage();

    // Set the sequence number
    sprintf(sequence_num, "%05u", sequence);  // Convert the uint16 sequence to char array sequence_num

    // Join the LoRaWAN network and send the data
    
    if(join_network())
    {
      send_data(sequence_num, date_time, epoch_time, volts, serial_number, odometer);
    }
    else
    {
      printf("     Did not join the network, no data sent\n");
    }

    sequence++;   // TODO: The sequence number is useful only in testing.  Repurpose this as an error code.

    // The RTC will interrupt every hour at 15 past.  
    minute = 15;

    // With RV3028 mode 6 (setAlarmInterrupt() minutes match), the hour is not used.  Increment
    // it so that when the next alarm time is sent to the console the hour will be correctly shown.
    hour += 1;
    if(hour > 23)
    {
      hour = 0;
    }


    // Ensure that the BSIE is not causing an interrupt
    //  TODO: This instruction may not be needed but does not interfere.
    //         A condition ocurred during testing in which the BSIE (EEPROM Backup bit 6) was set.
    //uint8_t bsie = readRVRegister(0x37);
    //printf("BSIE: %x\n", bsie);
    clearBit(0x37, 6);
    //bsie = readRVRegister(0x37);
    //printf("BSIE: %x\n", bsie);

    setAlarmInterrupt(minute, hour, 1, false, 6, false);


    printf("     Next alarm:  ");
    printf("%02x:%02x\n", getAlarmHours(), getAlarmMinutes());

    enableAlarmInterrupt();   //enable the alarm interrupt

    clearInterrupts();

    print_rv_regs();
    printf("     Power down...\n");

    clearAlarmInterruptFlag();  // This will power off the PMIC


    // If program execution reaches here it is because the RTC did not turn
    // off the PMIC.  This can happen if the reed switch is keeping the PMIC
    // on.  Delay and continue the while loop.
    sleep_ms(500);
    printf("     \nShould be powered down.\n\n");

    sleep_ms(100000);
    printf("Woke up!\n");  // Should never reach this
  }

  return 0;
}