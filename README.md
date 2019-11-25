# Send_IR_Using_PWM_N76E003
Using PWM to generate 38KHz Pulse to send IR data.

This sample code in N76E003_BSP_Keil_C51_V1.0.6/Sample_Code/SEND_IR_USING_PWM

## IR Protocol
![IR Protocol](https://github.com/minhhieuec/Send_IR_Using_PWM_N76E003/blob/master/N76E003_BSP_Keil_C51_V1.0.6/IR_PROTOCOL.PNG)
Reference: https://www.sbprojects.net/knowledge/ir/nec.php

## Schematic
![shematic of send ir with pwm](https://github.com/minhhieuec/Send_IR_Using_PWM_N76E003/blob/master/N76E003_BSP_Keil_C51_V1.0.6/shematic.PNG)

Using P0.3 of N76E003 as PWM5 mode.

NPN: C1815.

### Config PWM5
```c
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
```
