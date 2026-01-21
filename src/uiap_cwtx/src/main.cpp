//==================================================================//
// UIAP 7MHz CW Transmitter & 700Hz Sin wave DDS generator
// Written by Kimio Ohe JA9OIR/JA1AOQ
//==================================================================
#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include "uiap_hal.h"
#include "sin_table.h"
#include <cstdio>
#include "si5351.h"


static volatile uint8_t s_sin_index = 0;
static volatile uint8_t s_pwm_active = 0;
static volatile uint8_t s_stop_pending = 0;

Si5351 si5351;

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

void setup()
{
    bool i2c_found;
	SystemInit();				// ch32v003 Setup
	GPIO_setup();					// gpio Setup;    
	tim1_pwm_init();             	// TIM1 PWM Setup

    // Start serial and initialize the Si5351
    i2c_found = si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    if(!i2c_found){
        printf("Device not found on I2C bus!\n");
    }

    // Set CLK0 to output 7 MHz
    si5351.set_freq(701500000ULL, SI5351_CLK0);
	si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_2MA);
    si5351.output_enable(SI5351_CLK0, false);
    // Query a status update and wait a bit to let the Si5351 populate the
    // status flags correctly.
    si5351.update_status();
    Delay_Ms(500);
}

int main()
{
    setup();

    uint8_t last_key = GPIO_digitalRead(PIN_KEYIN);		// keyer in
    while (1) {
		uint8_t key_now = GPIO_digitalRead(PIN_KEYIN);
//        printf("Key: %d\r\n", key_now);

		if (last_key && !key_now) {		// detect rise edge
			__disable_irq();
			s_sin_index = 0;
			s_stop_pending = 0;
			s_pwm_active = 1;
			__enable_irq();
			GPIO_digitalWrite(PIN_LED, high);
            si5351.output_enable(SI5351_CLK0, true);
        } else if (!last_key && key_now) {	// detect fall edge
			GPIO_digitalWrite(PIN_LED, low);
            si5351.output_enable(SI5351_CLK0, false);
			s_stop_pending = 1;
		}
		last_key = key_now;
    }

    return 0;
}
