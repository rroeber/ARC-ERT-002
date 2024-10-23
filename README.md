# Encoder_Link

## Operation
When the RV3028 RTC interrupts on an alarm time match it pulls the RTC INT pin (pin 2) LOW.  This LOW signal pulls the TCK108AF control voltage (pin 3) LOW.  The TCK108AF is Active Low so a LOW control input causes a HIGH V_OUT.  This HIGH signal pulls the TPS61028 PMIC enable pin HIGH and the PMIC boosts the battery voltage to 5V to power the circuit.  Devices that require 3.3V are powered by the PICO 3.3V voltage regulator.

The TMS61028 provides power until the RTC Alarm Interrupt Flag (AF) is cleared. Once the encoder reader reads and transmits data it clears the AF. Clearing the AF disables the RV3028 INT output.  The external pullup resistor on the INT pin pulls the signal HIGH which in turn disables the TPS61028 PMIC and powers off the Encoder Reader.

The RTC is powered directly from the LiFePO4 battery and can use the battery's full voltage range.

## Backup Switchover Mode
### Direct Switching Mode (DSM)

When the V_DD power source is less than the backup power source the RTC switches to backup power.  This mode is most suitable to situations in which V_DD is normally greater than V_backup.  When the encoder reader is powered up V_DD is 3.3V.  V_backup is approximately 3.5-3.6V when the battery is completely charged.  However, over the battery lifetime, V_backup is mostly below 3.3V which makes DSM a good choice.

### Level Swiching Mode (LSM)

This is the mode used by the encoder reader.  When V_DD is greater than the threshold voltage the RTC is powered by V_DD.  Since V_DD is 3.3V and the RTC threshold voltage is internally set to 2.0V the RTC will use V_DD when the encoder reader is operating.  When the encoder reader is off, the RTC will use V_backup.  Simple.  Slightly higher power consumption than DSM but the encoder reader is operating very infrequently.