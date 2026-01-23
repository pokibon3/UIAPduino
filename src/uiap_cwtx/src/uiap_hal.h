//
// Hardware abstraction layer for UIAPduino
//
#pragma once

#define SCALE 3

//#define micros() (SysTick->CNT / DELAY_US_TIME)
//#define millis() (SysTick->CNT / DELAY_MS_TIME)

// TFT selection (set via build flags)

#if defined(TFT_ST7735) && defined(TFT_ST7739)
#error "Define only one of TFT_ST7735 or TFT_ST7739."
#endif

#ifndef TFT_CONFIG_ONLY
#if defined(TFT_ST7735)
#include "st7735.h"
#elif defined(TFT_ST7739)
#include "st7789.h"
#else
#error "Define TFT_ST7735 or TFT_ST7739."
#endif

#define TFT_WIDTH  ST7735_WIDTH
#define TFT_HEIGHT ST7735_HEIGHT
#endif

/*    pin assign    */
#define PIN_KEYIN  PD6     // KEYER IN
#define PIN_LED    PC0     // LED (was PD2)

#define SW1_PIN PA1         // SW1_PIN
#define SW2_PIN PC4         // SW2_PIN
#define SW3_PIN PD2         // SW3_PIN


extern int  GPIO_setup(void);
