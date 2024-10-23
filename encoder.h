#ifndef ENCODER_H_
#define ENCODERH_

void encoder_uart_init();
uint encoder_pwm_init();
void encoder_read(uint8_t *meter_data);



#endif  // ENCODER_H_