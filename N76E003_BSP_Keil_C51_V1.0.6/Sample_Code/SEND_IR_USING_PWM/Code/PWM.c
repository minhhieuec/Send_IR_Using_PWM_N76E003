#include "N76E003.h"
#include "Common.h"
#include "Delay.h"
#include "SFR_Macro.h"
#include "Function_define.h"

#define TRUE 1
#define FALSE 0

#define delay_ms Timer0_Delay1ms
#define delay_100us Timer0_Delay100us
#define delay_10us Timer3_Delay10us

#define EN_PWM_INTERRUPT

#define IR_PIN P03
#define Enable_IR_Pin() (IR_PIN = 1)
#define Disable_IR_Pin() (IR_PIN = 0)

#define MAX_BYTES_IR_SEND 8
#define TIME_9MS 345    //  26.1 * 345 = 9ms
#define TIME_4_5MS 173  //  26.1 * 173 = 4.5ms
#define TIME_2_2_MS 86  //  26.1 * 86 = 2.2ms
#define TIME_1_1MS 43   //  26.1 * 43 = 1.12ms
#define TIME_560US 21   //  26.1 * 21 = 560us
#define TIME_END_FRAME 3000

typedef enum {
  START_9MS = 0,
  AGC_BURST_4500US,
  SEND_IR_ADDR_DATA,
  SEND_STOP,
} Ir_Protocol_t;

uint8_t currentDataBit = 1;
uint8_t bisRaise = 1;
uint8_t i, j, tmp;
uint8_t is_ir_off = 0;

uint8_t ir_data[8] = {0xAA, 0x0A, 0x48, 0xD0, 0x30, 0x22, 0x59, 0x71};// 674 Mhz 

uint8_t ir_pin_stt = 0;
uint8_t cnt_bit_shift = 0;
uint8_t is_send_bit_finish = FALSE;

uint8_t tmp = 0, cur_pos = 0;

uint8_t ir_pro_step = START_9MS;
// uint8_t ir_pro_step = SEND_IR_ADDR_DATA;

uint32_t cnt_time_us = 0;

void enable_ir_pin(uint8_t is_enable) {
  if (is_enable) {
    if (is_ir_off == 1) {
      Enable_IR_Pin();
      PWM5_P03_OUTPUT_ENABLE;
      is_ir_off = 0;
    }
  } else {
    is_ir_off = 1;
    PWM5_P03_OUTPUT_DISABLE;
    Disable_IR_Pin();
  }
}

/*
 * @brief: turn on pwm out if turn off pwm before
 */
void enable_ir_pwm_out() {
  if (is_ir_off) {
    PWM5_P03_OUTPUT_ENABLE;
    is_ir_off = 0;
  }
}

/*
 * @brief: turn off pwm if pwm out is enable before
 */
void disable_ir_pwm_out() {
  if (is_ir_off == 0) {
    PWM5_P03_OUTPUT_DISABLE;
    is_ir_off = 1;
  }
}

#ifdef EN_PWM_INTERRUPT
void PWM_ISR(void) interrupt 13 {
  clr_PWMF;  // clear PWM interrupt flag

#if 1
  cnt_time_us += 1;  //  26us = 1/38000

  switch (ir_pro_step) {
    case START_9MS:
      enable_ir_pwm_out();

      if (cnt_time_us >= TIME_9MS) {
        ir_pro_step = AGC_BURST_4500US;
        cnt_time_us = 0;
      }
      break;

    case AGC_BURST_4500US:
      disable_ir_pwm_out();

      if (cnt_time_us >= TIME_4_5MS) {
        cnt_time_us = 0;
        ir_pro_step = SEND_IR_ADDR_DATA;
        // enable to test send start 9ms + wait 4.5ms
        // ir_pro_step = START_9MS;
      }
      break;

    case SEND_IR_ADDR_DATA:
      if (cnt_bit_shift == 0) {
        tmp = ir_data[cur_pos];
        // tmp = ir_data[0];
      }

      if (tmp & 0x80) {
        if (cnt_time_us < TIME_560US) {
          enable_ir_pwm_out();
        } else {
          if (cnt_time_us > TIME_2_2_MS) {
            is_send_bit_finish = TRUE;
          } else {
            disable_ir_pwm_out();
          }
        }
      } else {
        if (cnt_time_us < TIME_560US) {
          enable_ir_pwm_out();
        } else {
          if (cnt_time_us > TIME_1_1MS) {
            is_send_bit_finish = TRUE;
          } else {
            disable_ir_pwm_out();
          }
        }
      }

      if (is_send_bit_finish) {
        is_send_bit_finish = FALSE;

        tmp = tmp << 1;
        cnt_bit_shift += 1;

        if (cnt_bit_shift == 8) {
          cnt_bit_shift = 0;

          cur_pos += 1;  //  for next byte data

          //  send all 4 byte data
          if (cur_pos >= MAX_BYTES_IR_SEND) {
            cur_pos = 0;
            ir_pro_step = SEND_STOP;
            // ir_pro_step = SEND_IR_ADDR_DATA;
          }
        }

        cnt_time_us = 0;
      }
      break;

    case SEND_STOP:
      if (cnt_time_us < TIME_560US) {
        enable_ir_pwm_out();
      } else {
        if (cnt_time_us < TIME_END_FRAME)  //  76
          disable_ir_pwm_out();
        else {
          cnt_time_us = 0;
          ir_pro_step = START_9MS;
        }
      }
      break;

    default:
      break;
  }
#else
  cnt_time_us += 26;  //  26us = 1/38000

  switch (ir_pro_step) {
    case START_9MS:
      enable_ir_pwm_out();

      if (cnt_time_us >= 9000) {
        ir_pro_step = AGC_BURST_4500US;
        cnt_time_us = 0;
      }
      break;

    case AGC_BURST_4500US:
      disable_ir_pwm_out();

      if (cnt_time_us > 4500) {
        cnt_time_us = 0;
        ir_pro_step = SEND_IR_ADDR_DATA;
        // enable to test send start 9ms + wait 4.5ms
        // ir_pro_step = START_9MS;
      }
      break;

    case SEND_IR_ADDR_DATA:
      if (cnt_bit_shift == 0) {
        tmp = ir_data[cur_pos];
        // tmp = ir_data[0];
      }

      if (tmp & 0x80) {
        if (cnt_time_us < 560) {
          enable_ir_pwm_out();
        } else {
          if (cnt_time_us > 2250) {
            is_send_bit_finish = TRUE;
          } else {
            disable_ir_pwm_out();
          }
        }
      } else {
        if (cnt_time_us < 560) {
          enable_ir_pwm_out();
        } else {
          if (cnt_time_us > 1120) {
            is_send_bit_finish = TRUE;
          } else {
            disable_ir_pwm_out();
          }
        }
      }

      if (is_send_bit_finish) {
        is_send_bit_finish = FALSE;

        tmp = tmp << 1;
        cnt_bit_shift += 1;

        if (cnt_bit_shift == 8) {
          cnt_bit_shift = 0;

          cur_pos += 1;  //  for next byte data

          //  send all 4 byte data
          if (cur_pos > 3) {
            cur_pos = 0;
            ir_pro_step = SEND_STOP;
            // ir_pro_step = SEND_IR_ADDR_DATA;
          }
        }

        cnt_time_us = 0;
      }
      break;

    case SEND_STOP:
      if (cnt_time_us < 560) {
        enable_ir_pwm_out();
      } else {
        if (cnt_time_us < 2000)
          disable_ir_pwm_out();
        else {
          cnt_time_us = 0;
          ir_pro_step = START_9MS;
        }
      }
      break;

    default:
      break;
  }
#endif
}
#endif

void delay_us(uint16_t x) {
  while (x--) {
    _nop_();  //	200us
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
    _nop_();
  }
}

/************************************************************************************************************
 *    Main function
 ************************************************************************************************************/
void main(void) {
  Set_All_GPIO_Quasi_Mode;

  Disable_IR_Pin();

  /**********************************************************************
    PWM frequency = Fpwm/((PWMPH,PWMPL) + 1) <Fpwm = Fsys/PWM_CLOCK_DIV>
    = (16MHz/8)/(0x7CF + 1) = 1KHz (1ms)
  ***********************************************************************/
#ifdef EN_PWM_INTERRUPT
  PWM5_P03_OUTPUT_ENABLE;
  PWM_INT_PWM5;
  PWM_RISING_INT;  // PWM_FALLING_INT; //Setting Interrupt happen when PWM0
                   // falling signal

  PWM_CLOCK_DIV_4;

  PWMPH = 0x00;  //	38Khz
  PWMPL = 0x68;  //	38Khz 68 | 67 38k5

  set_SFRPAGE;  // PWM4 and PWM5 duty seting is in SFP page 1
  PWM5H = 0x0;
  PWM5L = 0x40;
  clr_SFRPAGE;

  set_EPWM;  // Enable PWM interrupt
  set_EA;
  set_LOAD;
  set_PWMRUN;
#endif

  while (1) {
  }
}