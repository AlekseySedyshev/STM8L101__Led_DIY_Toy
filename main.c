#include "stm8l10x.h"

#define ARR_P					16

#define P0    0x01
#define P1    0x02
#define P2    0x04
#define P3    0x08
#define P4    0x10
#define P5    0x20
#define P6    0x40
#define P7    0x80

#define LED0					TIM2->CCR2L
#define LED3					TIM3->CCR2L
#define LED6					TIM2->CCR1L
#define LED9					TIM3->CCR1L

#define BUT1 		(uint8_t)(GPIOC->IDR & P2)
#define BUT2 		(uint8_t)(GPIOC->IDR & P3)

#define PINS_INIT()		GPIOB->DDR |=0xFF;GPIOB->CR1 |=0xFF;GPIOB->CR2 |=0xFF;GPIOD->DDR |= 1;GPIOD->CR1 |= 1;GPIOD->CR2 |= 1;GPIOC->DDR |=0x10;GPIOC->CR1 |=0x10;GPIOC->CR2 |=0x10;GPIOA->DDR |=0x0C;GPIOA->CR1 &=~0xC;GPIOA->CR2 |=0xC; GPIOB->ODR |=0xF8;GPIOC->ODR |=0x10;GPIOA->ODR |=0xC;

#define GPIO_OFF() 	GPIOA->DDR =0;GPIOA->CR1 =0xFF; GPIOA->CR2 = 0;GPIOB->DDR =0; GPIOB->CR1 =0xFF; GPIOB->CR2 = 0; GPIOC->DDR =0; GPIOC->CR1 =0xFF; GPIOC->CR2 = 0;GPIOD->DDR =0; GPIOD->CR1 =0xff; GPIOD->CR2 = 0;

#define BUTTONS_INT()  GPIOC->DDR &=(~0x0C);GPIOC->CR1 |=0x0C;GPIOC->CR2 |=0x0C;

volatile bool mode=1;				//0 - Sleep, 1 - Active

uint16_t TimingDelay,blink;
uint8_t j=0,j1=0;
void TimingDelayDec(void) 																													{
 if (TimingDelay			!=0x00) TimingDelay--;
 if (!blink) {blink=12;}
 blink--;
}
void delay_ms (uint16_t DelTime) 																										{
    TimingDelay=DelTime;
  while(TimingDelay!= 0x00);
}
void initial(void)	{
	
CLK->CKDIVR = 3; //16MHz / 2^3 = 2 MHz
CLK->PCKENR |= CLK_PCKENR_TIM4 | CLK_PCKENR_AWU;

TIM4->PSCR = 4;
TIM4->ARR = 124; 																	// 2^4 = 16, 16*125 = 2000 
TIM4->SR1 &=~TIM4_SR1_UIF; 												//сброс флага прерывани€ TIM4_ClearFlag(TIM4_FLAG_UPDATE); 	
TIM4->IER	|= TIM4_IER_UIE; 												//прерывание включено
TIM4->CR1 |= TIM4_CR1_CEN; 												// запустить таймер
_asm("rim");

GPIO_OFF();
PINS_INIT();
//------------------------------------------------
CLK->PCKENR |= CLK_PCKENR_TIM2;
TIM2->PSCR = 8;
TIM2->ARRH = 0;TIM2->ARRL = ARR_P;
TIM2->SMCR 	=0;  																	//Count both pins Ch1, Ch2
TIM2->CCMR1	|= 0b110<<4 | TIM_CCMR_OCxPE; 			//PWM1 mode
TIM2->CCMR2 |= 0b110<<4 | TIM_CCMR_OCxPE;				//PWM1 mode
TIM2->CCER1 |= TIM_CCER1_CC1P | TIM_CCER1_CC2P;	//Active pulse Low
TIM2->CCER1 |= TIM_CCER1_CC1E | TIM_CCER1_CC2E; //Enable
TIM2->CNTRH=0;TIM2->CNTRL=0;										//—брос счетчика
TIM2->CCR1H=0;	TIM2->CCR1L=0;									//—брос счетчика 		
TIM2->CCR2H=0;	TIM2->CCR2L=0;									//—брос счетчика 		
TIM2->CR1 |= TIM_CR1_CMS;												//CMS = 11 UP and Down 
//TIM2->CR1 |= TIM_CR1_ARPE;
TIM2->BKR |=TIM_BKR_MOE;
TIM2->CR1 |= TIM_CR1_CEN; 											// запустить таймер
TIM2->EGR |= TIM_EGR_UG;

//--------------------------------------------------------------------
CLK->PCKENR |= CLK_PCKENR_TIM3;
TIM2->PSCR = 8;
TIM3->ARRH = 0;TIM3->ARRL = ARR_P;
TIM3->SMCR 	=0;
TIM3->CCMR1	|= 0b110<<4 |TIM_CCMR_OCxPE;					//PWM1 mode
TIM3->CCMR2 |= 0b110<<4 |TIM_CCMR_OCxPE;					//PWM1 mode
TIM3->CCER1 |= TIM_CCER1_CC1P | TIM_CCER1_CC2P;	//Active pulse Low
TIM3->CCER1 |= TIM_CCER1_CC1E | TIM_CCER1_CC2E;   //Enable

TIM3->CNTRH=0;	TIM3->CNTRL=0;										// —брос счетчика
TIM3->CCR1H=0;	TIM3->CCR1L=0;
TIM3->CCR2H=0;	TIM3->CCR2L=0;
TIM3->CR1 |= TIM_CR1_CMS; 												//CMS = 11 UP and Down
//TIM3->CR1 |= TIM_CR1_ARPE;
TIM3->BKR |=TIM_BKR_MOE;
TIM3->CR1 |= TIM_CR1_CEN; 												// запустить таймер
TIM3->EGR |= TIM_EGR_UG;

_asm("sim");		
			EXTI->CR1 |=	0b01<<2; // Pc2 - 01-Rising, 10 - Falling Edge, 11 - Both
			EXTI->CR1 |=	0b01<<3; // Pc3 - 01-Rising, 10 - Falling Edge, 11 - Both
_asm("rim");
BUTTONS_INT();
}
void sleep_10ms(uint16_t sleep_time) 				{	// go to sleep mode
	 uint16_t slp;
	 
for(slp=1;slp<=sleep_time;slp++) 								{	 
		
			AWU->APR &=(~0x3F) | 0x30; 						//AWU->APR &=(~0x3F) | 0x30; 
			AWU->TBR |=0x4; 											//AWU->TBR |=0x4;  10 mcек
			AWU->CSR |=AWU_CSR_AWUEN ;   					//Enable interrupt.
			_asm("halt");  												//Sleep go on
		}
}
//----------main led functions---------------
void flash_on(uint8_t time, uint8_t led) 		{	//led 0bABCD  
uint8_t i=0;

for (i=0;i<ARR_P+1;i++)
{
 if (led&0b0001) {LED0 = i;}
 if (led&0b0010) {LED3 = i;}
 if (led&0b0100) {LED6 = i;}
 if (led&0b1000) {LED9 = i;}
 delay_ms(time);
}

}
void flash_off(uint8_t time, uint8_t led) 	{
uint8_t i=0;
for (i=0;i<ARR_P+1;i++)
{
 if (led&0b0001) {LED0 = ARR_P-i;}
 if (led&0b0010) {LED3 = ARR_P-i;}
 if (led&0b0100) {LED6 = ARR_P-i;}
 if (led&0b1000) {LED9 = ARR_P-i;}
 delay_ms(time);
}
}
void led_on(uint8_t led1)										{
switch (led1) 															{
case 0:		{LED0=ARR_P;}					break;
case 1:		{GPIOA->ODR &=(~P2);} break;
case 2:		{GPIOA->ODR &=(~P3);}	break;
case 3:		{LED3 = ARR_P;}		 		break;
case 4:		{GPIOB->ODR &=(~P3);}	break;
case 5:		{GPIOB->ODR &=(~P4);}	break;
case 6:		{LED6 = ARR_P;}		 		break;
case 7:		{GPIOB->ODR &=(~P5);} break;
case 8:		{GPIOB->ODR &=(~P6);}	break;
case 9:		{LED9 = ARR_P;}				break;
case 10: 	{GPIOB->ODR &=(~P7);}	break;
case 11: 	{GPIOC->ODR &=(~P4);}	break;
case 12: 	{LED0 = ARR_P;}				break;
case 13:	{LED0=ARR_P;LED3=ARR_P;LED6=ARR_P;LED9 = ARR_P;GPIOA->ODR &=~0x0C;GPIOB->ODR &=~0xF8;GPIOC->ODR &=~0x10;} break;
default: 													break;
}}
void led_off(uint8_t led1)									{
switch (led1) 															{
case 0:		{LED0=0;}						break;	
case 1:		{GPIOA->ODR |=P2;} 	break;
case 2:		{GPIOA->ODR |=P3;}	break;
case 3:		{LED3 = 0;} 				break;	
case 4:		{GPIOB->ODR |=P3;}	break;
case 5:		{GPIOB->ODR |=P4;}	break;
case 6:		{LED6 = 0;} 				break;
case 7:		{GPIOB->ODR |=P5;}	break;
case 8:		{GPIOB->ODR |=P6;}	break;
case 9:		{LED9 = 0;}					break;
case 10: 	{GPIOB->ODR |=P7;}	break;
case 11:	{GPIOC->ODR |=P4;}	break;
case 12: 	{LED0 = 0;}					break;
case 13:	{LED0=0;LED3=0;LED6=0;LED9=0;GPIOA->ODR |=0x0C;GPIOB->ODR |=0xF8;GPIOC->ODR |=0x10;} break;
default: 												break;
}}
//----------roll led functions---------------
void roll_right(uint8_t time,uint8_t qnt)		{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<=12;j++)
		{	led_on(j);sleep_10ms(time/10);	}
	for (j=0;j<=12;j++)
		{	led_off(j);sleep_10ms(time/10);	}
}
}
void roll_left(uint8_t time,uint8_t qnt)		{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<=12;j++)
		{	led_on(12-j);sleep_10ms(time/10);	}//delay_ms(time);
	for (j=0;j<=12;j++)
		{	led_off(12-j);sleep_10ms(time/10);} //delay_ms(time);
}
}
void octa_roll(uint8_t time,uint8_t qnt)		{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<2;j++)
		{	led_on(j);led_on(j+2);led_on(j+4);led_on(j+6);led_on(j+8);led_on(j+10);sleep_10ms(time/10);//delay_ms(time);	
			led_off(13);	
		}
}

}
void square_roll(uint8_t time,uint8_t qnt)	{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<3;j++)
		{	led_on(j);led_on(j+3);led_on(j+6);led_on(j+9);sleep_10ms(time/10); //delay_ms(time);	
			led_off(13);	
		}
}

}
void triangl_roll(uint8_t time,uint8_t qnt) {
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<4;j++)
		{	led_on(j);led_on(j+4);led_on(j+8);sleep_10ms(time/10);//delay_ms(time);	
			led_off(13);	
		}
}
}
void double_roll(uint8_t time,uint8_t qnt) 	{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<6;j++)
		{	led_on(j);led_on(j+6);sleep_10ms(time/10);//delay_ms(time);	
			led_off(13);	
		}
	}
}
void single_roll(uint8_t time,uint8_t qnt) 	{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<12;j++)
		{	led_on(j);delay_ms(time);	
			led_off(13);	
		}
	}
}
void zigzag_roll(uint8_t time,uint8_t qnt) 	{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<4;j++)
		{	led_on(j);sleep_10ms(time/10);//delay_ms(time);
			led_on(j+6);sleep_10ms(time/10);//delay_ms(time);
			led_on(j+9);sleep_10ms(time/10);//delay_ms(time);
			led_on(j+3);sleep_10ms(time/10);//delay_ms(time);
		}
	for (j=0;j<4;j++)
		{	led_off(j);sleep_10ms(time/10);//delay_ms(time);
			led_off(j+6);sleep_10ms(time/10);//delay_ms(time);
			led_off(j+3);sleep_10ms(time/10);//delay_ms(time);
			led_off(j+9);sleep_10ms(time/10);//delay_ms(time);
		}
}
}
void half_roll_dw(uint8_t time,uint8_t qnt)	{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<=6;j++)
		{	led_on(j);led_on(12-j);sleep_10ms(time/10);//delay_ms(time);	
		}
	for (j=0;j<=6;j++)
		{	led_off(j);led_off(12-j);sleep_10ms(time/10);//delay_ms(time);	
		}	
		
		
	}

}
void half_roll_up(uint8_t time,uint8_t qnt)	{
uint8_t i;
for (i=0;i<qnt;i++)
{
	for (j=0;j<=6;j++)
		{	led_on(6-j);led_on(6+j);sleep_10ms(time/10);//delay_ms(time);	
		}
	for (j=0;j<=6;j++)
		{	led_off(6-j);led_off(6+j);sleep_10ms(time/10);//delay_ms(time);	
		}	
		
		
	}

}
//----------RND led functions----------------
void smail (uint8_t time,uint8_t qnt)		{
	uint8_t i;

	led_off(13);
for (i=0;i<qnt;i++)
	{ j=0;
		for (j=0;j<12;j++)	{
			led_on(j);sleep_10ms(time/10);
				switch (j) {
					case 10:  break;
					case 2:  	break;
					case 5:  	break;
					case 6:  	break;
					case 7: 	break;
					default:  led_off(j); break;
				}
		}
	}
}

void deep_sleep (void) 											{			// Deep_sleep
	led_on(13);	roll_left(50,1);
	GPIO_OFF();
	TIM2->CR1 &=~TIM_CR1_CEN;		TIM3->CR1 &=~TIM_CR1_CEN;
	TIM4->CR1 &=~TIM4_CR1_CEN; 	AWU->CSR &=~AWU_CSR_AWUEN; 
	CLK->PCKENR &=~(CLK_PCKENR_TIM2 | CLK_PCKENR_TIM3 | CLK_PCKENR_TIM4 | CLK_PCKENR_AWU);
	BUTTONS_INT();
	_asm("halt");  						//Go to sleep
	PINS_INIT();
	BUTTONS_INT();
	CLK->PCKENR |= CLK_PCKENR_TIM2 | CLK_PCKENR_TIM3 | CLK_PCKENR_TIM4 | CLK_PCKENR_AWU;
	AWU->CSR |=AWU_CSR_AWUEN;		TIM4->CR1 |=TIM4_CR1_CEN;
	TIM3->CR1 |=TIM_CR1_CEN;		TIM2->CR1 |=TIM_CR1_CEN;
	roll_right(50,1);
	led_on(13);sleep_10ms(50);led_on(13);
		
}
//------------------------------------------Start-----------
int main(void) {
initial();
mode=0; deep_sleep();
	
//----------------Main Cycle
	while (1)		 {
smail(100,5);		
half_roll_dw(100,1);
half_roll_up(100,1);
sleep_10ms(100);		
octa_roll(100,10);
led_on(13);flash_off(50,0b1111);led_off(13);
square_roll(100,10);
flash_off(50,0b1111);led_off(13);
triangl_roll(100,10);
led_on(13);flash_off(50,0b1111);led_off(13);
double_roll(100,10);
flash_off(50,0b0101);
single_roll(50,10);
flash_off(50,0b0001);
sleep_10ms(100);

zigzag_roll(50,10);
sleep_10ms(100);
roll_right(50,1);	
roll_left(50,1);
roll_right(50,1);	
roll_left(50,1);
roll_right(50,1);	
roll_left(50,1);

led_on(13);

sleep_10ms(100);
for (j=0;j<=12;j++)
{
led_off(12-j);delay_ms(100);
}

for (j=0;j<3;j++)
{ 
flash_on(50,0b1111);flash_off(50,0b1111);
roll_right(100,1);
flash_on(50,0b1010);flash_off(50,0b1010);
flash_on(50,0b0101);flash_off(50,0b0101);
roll_right(100,1);
flash_on(50,0b1000);flash_off(50,0b1000);
flash_on(50,0b0100);flash_off(50,0b0100);
flash_on(50,0b0010);flash_off(50,0b0010);
flash_on(50,0b0001);flash_off(50,0b0001);
}
flash_off(100,0b1111);
sleep_10ms(100);

} // END while
} // End Main

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param file: pointer to the source file name
  * @param line: assert_param error line source number
  * @retval : None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

    /* Infinite loop */
    while (1)
    {
    }
}
#endif

  
/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/