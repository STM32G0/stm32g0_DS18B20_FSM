#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stm32g071xx.h>
#include "delay.h"

//#define MCU_CLK 16000000    /* 16 MHz */
//#define DELAY_TIM_FREQUENCY 1000000 /* = 1MHZ -> timer runs in microseconds */

/*
void delay_us(uint16_t us) {
  uint16_t start = TIM6->CNT;
  while((uint16_t)(TIM6->CNT - start) <= us);
  }
*/
void delay_us(uint16_t us){
  TIM6->CNT = 0;
 while (TIM6->CNT < us);
}

void delay_ms(uint16_t ms)
{
	int i;
	for (i=0; i < ms; i++){
		delay_us(1000);
	}
}