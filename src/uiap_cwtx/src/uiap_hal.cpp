//
// Hardware abstraction layer for UIAPduino
//
#include "ch32fun.h"
#include "ch32v003_GPIO_branchless.h"
#include "uiap_hal.h"
#include <stdint.h>

//==================================================================
//	setup
//==================================================================
int GPIO_setup()
{
    // Enable GPIO Ports A, C, D
    GPIO_port_enable(GPIO_port_A);
    GPIO_port_enable(GPIO_port_C);
    GPIO_port_enable(GPIO_port_D);
    // Set Pin Modes
    GPIO_pinMode(PIN_KEYIN, GPIO_pinMode_I_pullUp, GPIO_Speed_10MHz);
	GPIO_pinMode(PIN_LED, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);
//	GPIO_pinMode(PIN_TONE, GPIO_pinMode_O_pushPull, GPIO_Speed_10MHz);

    GPIO_pinMode(SW1_PIN, GPIO_pinMode_I_pullUp, GPIO_Speed_10MHz);
    GPIO_pinMode(SW2_PIN, GPIO_pinMode_I_pullUp, GPIO_Speed_10MHz);
    GPIO_pinMode(SW3_PIN, GPIO_pinMode_I_pullUp, GPIO_Speed_10MHz);

    return 0;
}

