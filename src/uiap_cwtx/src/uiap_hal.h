//
// Hardware abstraction layer for UIAPduino
//

/*    pin assign    */
#define PIN_KEYIN  PC3     // KEYER IN
#define PIN_LED    PC0     // LED (was PD2)
// ---- PWM出力ピン (TIM1 no remap: CH1=PD2) ----
#define PIN_OUT    PD2     // PWM AUDIO OUT (TIM1_CH1)

extern int  GPIO_setup(void);
extern void start_pwm(void);
extern void stop_pwm(void);
extern void tim1_pwm_init(void);

extern "C" void TIM1_UP_IRQHandler(void) __attribute__((interrupt));
