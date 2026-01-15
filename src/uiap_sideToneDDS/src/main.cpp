//==================================================================//
// UIAP_sidetone 700Hz Sin wave DDS generator
// Written by Kimio Ohe JA9OIR/JA1AOQ
//==================================================================
#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include "uiap_hal.h"
#include "sin_table.h"
#include <stdio.h>
#include <stdint.h>

static volatile uint8_t s_sin_index = 0;
static volatile uint8_t s_pwm_active = 0;
static volatile uint8_t s_stop_pending = 0;

//
// TIM1 Update Interrupt Handler for PWM audio output
//
void TIM1_UP_IRQHandler(void)
{
	const uint8_t last_index = (uint8_t)(sizeof(sin_table) - 1);

	TIM1->INTFR &= (uint16_t)~TIM_IT_Update;
	if (!s_pwm_active)
	{
		TIM1->CH1CVR = 128;
		return;
	}

	TIM1->CH1CVR = sin_table[s_sin_index];
	if (s_sin_index >= last_index)
	{
		s_sin_index = 0;
		if (s_stop_pending)
		{
			s_stop_pending = 0;
			s_pwm_active = 0;
			TIM1->CH1CVR = 128;
		}
	} else{
		++s_sin_index;
	}
}

//
// main function
//
int main(void)
{
    SystemInit();
	GPIO_setup();					// gpio Setup;    
	tim1_pwm_init();             	// TIM1 PWM Setup

	uint8_t last_key = GPIO_digitalRead(PIN_KEYIN);		// keyer in
    while (1)
    {
		uint8_t key_now = GPIO_digitalRead(PIN_KEYIN);
//        printf("Key: %d\r\n", key_now);

		if (last_key && !key_now)		// detect rise edge
		{
			__disable_irq();
			s_sin_index = 0;
			s_stop_pending = 0;
			s_pwm_active = 1;
			__enable_irq();
			GPIO_digitalWrite(PIN_LED, high);
		}
		else if (!last_key && key_now)	// detect fall edge
		{
			GPIO_digitalWrite(PIN_LED, low);
			s_stop_pending = 1;
		}

		last_key = key_now;
    }
}
