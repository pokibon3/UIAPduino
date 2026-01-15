//
// Hardware abstraction layer for UIAPduino
//
#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include "uiap_hal.h"
#include <stdio.h>
#include <stdint.h>

uint32_t count;

/*
 * initialize TIM1 for PWM
 */
void tim1_pwm_init( void )
{
	// Enable GPIOD and TIM1
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
	RCC->APB2PCENR |= RCC_APB2Periph_TIM1;

	// PD2 is T1CH1, 10MHz Output alt func, push-pull (no remap)
	GPIOD->CFGLR &= ~(0xf<<(4*2));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF)<<(4*2);

	// Reset TIM1 to init all regs
	RCC->APB2PRSTR |= RCC_APB2Periph_TIM1;
	RCC->APB2PRSTR &= ~RCC_APB2Periph_TIM1;

	// SMCFGR: default clk input is CK_INT
	// set TIM1 clock prescaler divider (48MHz / 268 = 179.1kHz)
	TIM1->PSC = 0;
	// set PWM total cycle width for ~700Hz DDS update (48MHz / 268 / 256)
	TIM1->ATRLR = 268 - 1;

	// for channel 1, let CCxS stay 00 (output), set OCxM to 110 (PWM I)
	// enabling preload causes the new pulse width in compare capture register only to come into effect when UG bit in SWEVGR is set (= initiate update) (auto-clears)
	TIM1->CHCTLR1 |= TIM_OC1M_2 | TIM_OC1M_1 | TIM_OC1PE;

	// CTLR1: default is up, events generated, edge align
	// enable auto-reload of preload
	TIM1->CTLR1 |= TIM_ARPE;

	// Enable Channel output, set default state
	TIM1->CCER |= TIM_CC1E;

	// initialize counter
	TIM1->SWEVGR |= TIM_UG;
	// enable update interrupt
	TIM1->INTFR = (uint16_t)~TIM_IT_Update;
	NVIC->IPRIOR[TIM1_UP_IRQn] = 0 << 7 | 1 << 6;
	NVIC->IENR[((uint32_t)(TIM1_UP_IRQn) >> 5)] |= (1 << ((uint32_t)(TIM1_UP_IRQn) & 0x1F));
	TIM1->DMAINTENR |= TIM_IT_Update;
	// set default duty cycle 50% for channel 1
	TIM1->CH1CVR = 128;
	// Enable TIM1 main output
	TIM1->BDTR |= TIM_MOE;
	// Enable TIM1
	TIM1->CTLR1 |= TIM_CEN;
}

// ===================== トーン制御 =====================
void start_pwm() 
{
	TIM1->CTLR1 |= TIM_CEN;
}

void stop_pwm() 
{
	TIM1->CTLR1 &= ~TIM_CEN;
}

//==================================================================
//	setup
//==================================================================
int GPIO_setup()
{
    // Enable GPIO Ports A, C, D
    //GPIO_port_enable(GPIO_port_A);
    GPIO_port_enable(GPIO_port_C);
    GPIO_port_enable(GPIO_port_D);
    // Set Pin Modes
    GPIO_pinMode(PIN_KEYIN, GPIO_pinMode_I_pullUp, GPIO_Speed_10MHz);
    // PWM pin is configured as AF in tim1_pwm_init()
	GPIO_pinMode(PIN_LED, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
//	GPIO_pinMode(PIN_TONE, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
    return 0;
}

