cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(encoder_reader_project C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(meter meter.cpp)
pico_add_extra_outputs(meter)

pico_enable_stdio_usb(meter 1)
pico_enable_stdio_uart(meter 0)

target_link_libraries(meter
	pico_stdlib
	hardware_sleep
	hardware_adc
	hardware_uart
	hardware_pwm
	hardware_spi
	hardware_rtc
	hardware_i2c
	hardware_sleep)

target_sources(meter PRIVATE
	AES-128.cpp
	Conversions.cpp
	Encrypt.cpp
	LoRaMAC.cpp
	lorawan-arduino-rfm.cpp
	RFM95.cpp
	porting_functions.cpp

	encoder.cpp
	rtc_rv3028c7.cpp
	serif_i2c.cpp
	battery.cpp	
)






