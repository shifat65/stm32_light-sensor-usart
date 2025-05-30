#include<stdint.h>
#include<stm32f10x.h>

void En_clock(void);
void gpio_setup(void);
void Uart_config(void);
void delay_ms(void);
void delay(uint32_t count);
void systick_config(void);
uint8_t debounce(uint8_t last);
void ADC_config(void);

int main(void){
	En_clock();
	gpio_setup();
	Uart_config();
	systick_config();
	ADC_config();
	
	uint16_t analog_data = 0;
	
	uint16_t max_val = 0x0C00;
	uint16_t min_val = 0x0700;
	
	char data = 'b';
	char old_status = data;
	char rdata = 'b';
	while(1){

	//If conversion is done, read the data
			if(ADC1->SR & ADC_SR_EOC){ 
				analog_data = ADC1->DR;
				}

// Turn on 2 led if its dark, 1 if natural, off if bright
				if( analog_data>max_val){
					data = 'a';
				}
				else if ( analog_data<min_val){
					data = 'c';				
				}
				else {
					data = 'b';				
				}
	//sending data to another stm32 using usart if state changes
			if(old_status!=data){
			while(!(USART1->SR & USART_SR_TXE)){}
			if((USART1->SR & USART_SR_TXE))
			{	
				GPIOA->ODR |=  GPIO_ODR_ODR2;
				delay(10);
				USART1->DR = data;
				old_status = data;
				GPIOA->ODR &= ~GPIO_ODR_ODR2;
				delay(10);
				
			}}
	//Receiving data from another stm32.
			//while(!(USART1->SR & USART_SR_RXNE)){}
			
			if((USART1->SR & USART_SR_RXNE))
			{	
				GPIOA->ODR |=  GPIO_ODR_ODR5;
				delay(10);
				rdata = USART1->DR;
				GPIOA->ODR &= ~GPIO_ODR_ODR5;
				delay(10);
				
			}
			

	// turning on led depending on received data

				if( rdata== 'a' ){
					//turn on B6 and B7, B5
					GPIOB->ODR |= GPIO_ODR_ODR6;
					GPIOB->ODR |= GPIO_ODR_ODR7;
					GPIOB->ODR |= GPIO_ODR_ODR5;
				}
				else if ( rdata=='c'){
					//Turn on B6. but B7, and B5 turn off
					GPIOB->ODR |=  GPIO_ODR_ODR6;
					GPIOB->ODR &= ~GPIO_ODR_ODR7;
					GPIOB->ODR &= ~GPIO_ODR_ODR5;		
				
				}
				else if(rdata=='b') {
					
					GPIOB->ODR |= GPIO_ODR_ODR6;
					GPIOB->ODR |= GPIO_ODR_ODR7;
					GPIOB->ODR &= ~GPIO_ODR_ODR5;	
				
				}	



	//checking loop
			GPIOA->ODR |=  GPIO_ODR_ODR3;
			delay(25);
			GPIOA->ODR &= ~GPIO_ODR_ODR3;
			delay(25);
	
	}
	
	return 0;
}


void En_clock(void){
RCC->APB2ENR |= RCC_APB2ENR_AFIOEN | RCC_APB2ENR_IOPAEN | RCC_APB2ENR_IOPBEN;
}

void gpio_setup(void){
	
// PA1 as analog input
	GPIOA->CRL &= ~(GPIO_CRL_CNF1 | GPIO_CRL_MODE1);
	GPIOA->CRL |= 0UL;
	
//PA5 as RXNE status
	GPIOA->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
	GPIOA->CRL |= GPIO_CRL_MODE5;
	
//PA2 as TXE status
	GPIOA->CRL &= ~(GPIO_CRL_CNF2 | GPIO_CRL_MODE2);
	GPIOA->CRL |= GPIO_CRL_MODE2;

//PA3 as main loop detection
	GPIOA->CRL &= ~(GPIO_CRL_CNF3 | GPIO_CRL_MODE3);
	GPIOA->CRL |= GPIO_CRL_MODE3;
	
//PA6 indicates receiving a right message. toggle on receving right message. 
	GPIOA->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
	GPIOA->CRL |= GPIO_CRL_MODE6;
	
//PB5 as led in response of ADC
	GPIOB->CRL &= ~(GPIO_CRL_CNF5 | GPIO_CRL_MODE5);
	GPIOB->CRL |= GPIO_CRL_MODE5;
	
//PB7 as in response of ADC
	GPIOB->CRL &= ~(GPIO_CRL_CNF7 | GPIO_CRL_MODE7);
	GPIOB->CRL |= GPIO_CRL_MODE7;

//PB6 as in response of ADC
	GPIOB->CRL &= ~(GPIO_CRL_CNF6 | GPIO_CRL_MODE6);
	GPIOB->CRL |= GPIO_CRL_MODE6;
}

void Uart_config(void){
RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

// TX, P9 as alternet function push-pull output
GPIOA->CRH &= ~(GPIO_CRH_CNF9 | GPIO_CRH_MODE9);
GPIOA->CRH |= (GPIO_CRH_CNF9_1 | GPIO_CRH_MODE9);	
	
// TX, P10 as floting input
GPIOA->CRH &= ~(GPIO_CRH_CNF10 | GPIO_CRH_MODE10);
GPIOA->CRH |= (GPIO_CRH_CNF10_0);

//Enable UART1 , tx, rx

USART1->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
	
//Baud rate 0xEA6
USART1->BRR = 0x2BF1;

}

void systick_config(void){
	SysTick->LOAD = 72000-1;
	SysTick->VAL = 0;
	SysTick->CTRL = SysTick_CTRL_CLKSOURCE | SysTick_CTRL_ENABLE;
}

void delay_ms(void){
	while(! ( SysTick->CTRL & SysTick_CTRL_COUNTFLAG));
}

void delay(uint32_t count){
while(count--){
	delay_ms();
}}

uint8_t debounce(uint8_t last){
	uint8_t current = (GPIOA->IDR & GPIO_IDR_IDR0)? 1 : 0;
	
	if ( last!= current){
		delay(5);
		current = (GPIOA->IDR & GPIO_IDR_IDR0)? 1 : 0;
	}
return current;
}

void ADC_config(void){
	// Using ADC1 and pin PA1. which is at channel 1;
	
	RCC->APB2ENR |= RCC_APB2ENR_ADC1EN | RCC_APB2ENR_AFIOEN;

	ADC1->CR2 = 0;
	ADC1->SQR3 = 1;
	ADC1->CR2 |= ADC_CR2_ADON | ADC_CR2_CONT;//ADC power on 
	delay(500);
	ADC1->CR2 |= ADC_CR2_ADON;//enable adc and start continuous conversion 
}

