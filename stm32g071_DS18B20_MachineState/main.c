/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************

-------------------------- END-OF-HEADER -----------------------------

File    : main.c
Purpose : Generic application start

*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stm32g071xx.h>
#include "ds18b20.h"
#include "delay.h"

#define LED_SetHigh()     (GPIOA->BSRR |= GPIO_BSRR_BS8) 
#define LED_SetLow()      (GPIOA->BSRR |= GPIO_BSRR_BR8) 
#define LED_Toggle()      ((GPIOA->ODR & GPIO_ODR_OD8)  ? (GPIOA->BSRR |= GPIO_BSRR_BR8) : (GPIOA->BSRR |= GPIO_BSRR_BS8))

#define MCU_CLK 16000000    /* 16 MHz */
#define DELAY_TIM_FREQUENCY 1000000 /* = 1MHZ -> timer runs in microseconds */

bool ResetPulseFSM(void);
bool WriteBitFSM(bool bit);
bool ReadBitFSM(void);

/* Machine State for DS18B20 */
typedef enum {state0 = 0,state1,state2,state3,stateEnd} state_t; //for interrupt TIM6_DAC_LPTIM1_IRQHandler
volatile state_t STATE_DS18B20_Reset = stateEnd ; //for interrupt TIM6_DAC_LPTIM1_IRQHandler
volatile state_t STATE_DS18B20_WriteBit = stateEnd ; //for interrupt TIM6_DAC_LPTIM1_IRQHandler	
volatile state_t STATE_DS18B20_ReadBit = stateEnd ; //for interrupt TIM6_DAC_LPTIM1_IRQHandler		

volatile bool bit ;
uint8_t idx = 0;

int main(void) {
SystemInit();
  /* zwoka na ustabilizowanie si zegarow / koniecznie musi by */
for (uint32_t i = 0; i < 5000; i++) {asm("nop");}
RCC->IOPENR |= RCC_IOPENR_GPIOAEN; //Open clock for GPIOA
RCC->IOPENR |= RCC_IOPENR_GPIOCEN; //Open clock for GPIOC
RCC->APBENR1 |= RCC_APBENR1_TIM6EN;  // Enable the timer6 clock for 1-Wire/DS18B20
/* PC6 set Output */
GPIOC->MODER |=  GPIO_MODER_MODE6_0; //MODE6 -> 0b01
GPIOC->MODER &= ~GPIO_MODER_MODE6_1; //MODE6 -> 0b01
/* PC6 set High */
GPIOC->BSRR |= GPIO_BSRR_BS6;
/* PA8 set Output for LED */
GPIOA->MODER |=  GPIO_MODER_MODE8_0; //MODE8 -> 0b01
GPIOA->MODER &= ~GPIO_MODER_MODE8_1; //MODE8 -> 0b01

NVIC_SetPriority(TIM6_DAC_LPTIM1_IRQn, 1);
NVIC_EnableIRQ(TIM6_DAC_LPTIM1_IRQn);
TIM6->DIER |= TIM_SR_UIF ;

/* Timer6 init */
TIM6->PSC = (MCU_CLK / DELAY_TIM_FREQUENCY) - 1;  // 16MHz/16 = 1 MHz ~~ 1 uS per tick counter
TIM6->ARR = 0x0FFF; 
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
while (!(TIM6->SR & TIM_SR_UIF));  // UIF: Update interrupt flag..  This bit is set by hardware when the registers are updated


  
while (1) {
  
asm("NOP"); 
     
   };
}



//obsługa przerwania dla Timer6
void TIM6_DAC_LPTIM1_IRQHandler(void){

TIM6->CR1 &= ~TIM_CR1_CEN; // disable Counter
TIM6->SR  &=  ~TIM_SR_UIF ; //clear interrupt flag
TIM6->CNT = 0;
/*********************** Machine STATE RESET ***************************/

ResetPulseFSM();

/*********************** Machine STATE Write Bit ***************************/

WriteBitFSM(bit);

/*********************** Machine STATE Read Bit ***************************/

ReadBitFSM();
        }
        


bool ResetPulseFSM(void){ // resetujemy magistrale , czekamy na impuls PRESENCE

bool result = false;

switch (STATE_DS18B20_Reset) {

case state0: 
SET_Low_Wire2() ;
TIM6->CR1 &= ~TIM_CR1_CEN; // Disable the Counter
TIM6->ARR = (480 - 1);  // ARR value 1 us per tick , for 480 us ARR = (480 - 1)
TIM6->CNT = 0;
STATE_DS18B20_Reset = state1 ;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
break;

case state1: 
SET_High_Wire2();
TIM6->CR1 &= ~TIM_CR1_CEN; // Disable the Counter
TIM6->ARR = (70 - 1);  // ARR value 1 us per tick , for 70 us ARR = (70- 1) 
TIM6->CNT = 0;
STATE_DS18B20_Reset = state2 ;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
break;


case state2: 
result = false ;
if (TEST_Input_Wire2() == 0) { // 0 - Slave OK, 1 - Slave not response
result = true ;
}
TIM6->CR1 &= ~TIM_CR1_CEN; // Disable the Counter
TIM6->ARR = (410 - 1);  // ARR value 1 us per tick , for 410 us ARR = (410 - 1)
TIM6->CNT = 0;
STATE_DS18B20_Reset = stateEnd ;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
return result ;  // 1 - Slave OK, 0 - Slave not response
break;

case stateEnd: 
/* do nothing */
break;
  }
}


bool WriteBitFSM(bool bit){

bool result = false;

switch (STATE_DS18B20_WriteBit) {


case state0: 
if(bit){
SET_Low_Wire2() ;
TIM6->ARR = (6 - 1);  // ARR value 1 us per tick , for 6 us ARR = (6 - 1)
TIM6->CNT = 0;
STATE_DS18B20_WriteBit = state1 ;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter

}
else
{
SET_Low_Wire2() ;
TIM6->ARR = (60 - 1);  // ARR value 1 us per tick , for 60 us ARR = (60 - 1)
TIM6->CNT = 0;
STATE_DS18B20_WriteBit = state2 ;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
} 
break;

case state1: 
SET_High_Wire2();
TIM6->ARR = (64 - 1);  // ARR value 1 us per tick , for 10 us ARR = (10 - 1)
TIM6->CNT = 0;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
result = true ;
STATE_DS18B20_WriteBit = stateEnd ;
return result ;  
break;

case state2: 
SET_High_Wire2();
TIM6->ARR = (10 - 1);  // ARR value 1 us per tick , for 10 us ARR = (10 - 1)
TIM6->CNT = 0;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
result = true ;
STATE_DS18B20_WriteBit = stateEnd ;
return result ;  
break;

case stateEnd: 
/* do nothing */
break;
}


}


bool ReadBitFSM(void){
bool result;

switch (STATE_DS18B20_ReadBit) {

case state0: 
SET_Low_Wire2() ;
TIM6->ARR = (6 - 1);  // ARR value 1 us per tick , for 10 us ARR = (10 - 1)
TIM6->CNT = 0;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
STATE_DS18B20_ReadBit = state1 ;
break;

case state1: 
SET_High_Wire2();
TIM6->ARR = (9 - 1);  // ARR value 1 us per tick , for 10 us ARR = (10 - 1)
TIM6->CNT = 0;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
STATE_DS18B20_ReadBit = state2 ;
break;

case state2: 
result = TEST_Input_Wire2();
TIM6->ARR = (55 - 1);  // ARR value 1 us per tick , for 10 us ARR = (10 - 1)
TIM6->CNT = 0;
TIM6->CR1 |= TIM_CR1_CEN; // Enable the Counter
STATE_DS18B20_ReadBit = stateEnd ;
return result ; 
break;

case stateEnd: 
/* do nothing */
break;

}
}




void WriteByteFSM(uint8_t byte){

if(WriteBitFSM(byte & 0x01)){
    /* shift the byte for the next bit */
    byte >>= 1;
    idx++ ;
    if(idx >= 8) idx = 0;
    }
  }