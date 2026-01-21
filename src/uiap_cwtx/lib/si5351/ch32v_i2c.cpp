/*
 * Single-File-Header for CH32V I2C interface
 * 05-07-2023 E. Brombaugh
 */

#include "ch32v_i2c.h"
#include "ch32fun.h"
#include <string.h>
#include <cstdio>

#ifdef CH32V_I2C_IRQ
// some stuff that IRQ mode needs
static volatile uint8_t ch32v_i2c_send_buffer[64], *ch32v_i2c_send_ptr, ch32v_i2c_send_sz, ch32v_i2c_irq_state;

// uncomment this to enable time diags in IRQ
//#define IRQ_DIAG
#endif

/*
 * init just I2C
 */
void ch32v_i2c_setup(void)
{
	uint16_t tempreg;
	
	// Reset I2C1 to init all regs
	RCC->APB1PRSTR |= RCC_APB1Periph_I2C1;
	RCC->APB1PRSTR &= ~RCC_APB1Periph_I2C1;
	
	// set freq
	tempreg = I2C1->CTLR2;
	tempreg &= ~I2C_CTLR2_FREQ;
	tempreg |= (FUNCONF_SYSTEM_CORE_CLOCK/CH32V_I2C_PRERATE)&I2C_CTLR2_FREQ;
	I2C1->CTLR2 = tempreg;
	
	// Set clock config
	tempreg = 0;
#if (CH32V_I2C_CLKRATE <= 100000)
	// standard mode good to 100kHz
	tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(2*CH32V_I2C_CLKRATE))&I2C_CKCFGR_CCR;
#else
	// fast mode over 100kHz
#ifndef CH32V_I2C_DUTY
	// 33% duty cycle
	tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(3*CH32V_I2C_CLKRATE))&I2C_CKCFGR_CCR;
#else
	// 36% duty cycle
	tempreg = (FUNCONF_SYSTEM_CORE_CLOCK/(25*CH32V_I2C_CLKRATE))&I2C_CKCFGR_CCR;
	tempreg |= I2C_CKCFGR_DUTY;
#endif
	tempreg |= I2C_CKCFGR_FS;
#endif
	I2C1->CKCFGR = tempreg;

#ifdef CH32V_I2C_IRQ
	// enable IRQ driven operation
	NVIC_EnableIRQ(I2C1_EV_IRQn);
	
	// initialize the state
	ch32v_i2c_irq_state = 0;
#endif
	
	// Enable I2C
	I2C1->CTLR1 |= I2C_CTLR1_PE;

	// set ACK mode
	I2C1->CTLR1 |= I2C_CTLR1_ACK;
}

/*
 * error descriptions
 */
static const char *const errstr[] =
{
	"not busy",
	"master mode",
	"transmit mode",
	"tx empty",
	"transmit complete",
};

/*
 * error handler
 */
uint8_t ch32v_i2c_error(uint8_t err)
{
	// report error
	printf("ch32v_i2c_error - timeout waiting for %s\n\r", errstr[err]);
	
	// reset & initialize I2C
	ch32v_i2c_setup();

	return 1;
}

/*
 * check for 32-bit event codes
 */
uint8_t ch32v_i2c_chk_evt(uint32_t event_mask)
{
	/* read order matters here! STAR1 before STAR2!! */
	uint32_t status = I2C1->STAR1 | (I2C1->STAR2<<16);
	return (status & event_mask) == event_mask;
}

#ifdef CH32V_I2C_IRQ
/*
 * packet send for IRQ-driven operation
 */
uint8_t ch32v_i2c_send(uint8_t addr, uint8_t *data, uint8_t sz)
{
	int32_t timeout;
	
#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(3));
#endif
	
	// error out if buffer under/overflow
	if((sz > sizeof(ch32v_i2c_send_buffer)) || !sz)
		return 2;
	
	// wait for previous packet to finish
	while(ch32v_i2c_irq_state);
	
#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(16+3));
	GPIOC->BSHR = (1<<(4));
#endif
	
	// init buffer for sending
	ch32v_i2c_send_sz = sz;
	ch32v_i2c_send_ptr = ch32v_i2c_send_buffer;
	memcpy((uint8_t *)ch32v_i2c_send_buffer, data, sz);
	
	// wait for not busy
	timeout = TIMEOUT_MAX;
	while((I2C1->STAR2 & I2C_STAR2_BUSY) && (timeout--));
	if(timeout==-1)
		return ch32v_i2c_error(0);

	// Set START condition
	I2C1->CTLR1 |= I2C_CTLR1_START;

	// wait for master mode select
	timeout = TIMEOUT_MAX;
	while((!ch32v_i2c_chk_evt(CH32V_I2C_EVENT_MASTER_MODE_SELECT)) && (timeout--));
	if(timeout==-1)
		return ch32v_i2c_error(1);
	
	// send 7-bit address + write flag
	I2C1->DATAR = addr<<1;

	// wait for transmit condition
	timeout = TIMEOUT_MAX;
	while((!ch32v_i2c_chk_evt(CH32V_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) && (timeout--));
	if(timeout==-1)
		return ch32v_i2c_error(2);

	// Enable TXE interrupt
	I2C1->CTLR2 |= I2C_CTLR2_ITBUFEN | I2C_CTLR2_ITEVTEN;
	ch32v_i2c_irq_state = 1;

#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(16+4));
#endif
	
	// exit
	return 0;
}

/*
 * IRQ handler for I2C events
 */
void I2C1_EV_IRQHandler(void)
{
	uint16_t STAR1, STAR2 __attribute__((unused));
	
#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(4));
#endif

	// read status, clear any events
	STAR1 = I2C1->STAR1;
	STAR2 = I2C1->STAR2;
	
	/* check for TXE */
	if(STAR1 & I2C_STAR1_TXE)
	{
		/* check for remaining data */
		if(ch32v_i2c_send_sz--)
			I2C1->DATAR = *ch32v_i2c_send_ptr++;

		/* was that the last byte? */
		if(!ch32v_i2c_send_sz)
		{
			// disable TXE interrupt
			I2C1->CTLR2 &= ~(I2C_CTLR2_ITBUFEN | I2C_CTLR2_ITEVTEN);
			
			// reset IRQ state
			ch32v_i2c_irq_state = 0;
			
			// wait for tx complete
			while(!ch32v_i2c_chk_evt(CH32V_I2C_EVENT_MASTER_BYTE_TRANSMITTED));

			// set STOP condition
			I2C1->CTLR1 |= I2C_CTLR1_STOP;
		}
	}

#ifdef IRQ_DIAG
	GPIOC->BSHR = (1<<(16+4));
#endif
}
#else
/*
 * low-level packet send for blocking polled operation via i2c
 */
uint8_t ch32v_i2c_send(uint8_t addr, const uint8_t *data, int sz)
{
	int32_t timeout;
	
	// wait for not busy
	timeout = TIMEOUT_MAX;
	while((I2C1->STAR2 & I2C_STAR2_BUSY) && (timeout--));
	if(timeout==-1)
		return ch32v_i2c_error(0);

	// Set START condition
	I2C1->CTLR1 |= I2C_CTLR1_START;
	
	// wait for master mode select
	timeout = TIMEOUT_MAX;
	while((!ch32v_i2c_chk_evt(CH32V_I2C_EVENT_MASTER_MODE_SELECT)) && (timeout--));
	if(timeout==-1)
		return ch32v_i2c_error(1);
	
	// send 7-bit address + write flag
	I2C1->DATAR = addr<<1;

	// wait for transmit condition
	timeout = TIMEOUT_MAX;
	while((!ch32v_i2c_chk_evt(CH32V_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) && (timeout--));
	if(timeout==-1)
		return ch32v_i2c_error(2);

	// send data one byte at a time
	while(sz--)
	{
		// wait for TX Empty
		timeout = TIMEOUT_MAX;
		while(!(I2C1->STAR1 & I2C_STAR1_TXE) && (timeout--));
		if(timeout==-1)
			return ch32v_i2c_error(3);
		
		// send command
		I2C1->DATAR = *data++;
	}

	// wait for tx complete
	timeout = TIMEOUT_MAX;
	while((!ch32v_i2c_chk_evt(CH32V_I2C_EVENT_MASTER_BYTE_TRANSMITTED)) && (timeout--));
	if(timeout==-1)
		return ch32v_i2c_error(4);

	// set STOP condition
	I2C1->CTLR1 |= I2C_CTLR1_STOP;
	
	// we're happy
	return 0;
}
#endif

/*
 * high-level packet send for I2C
 */
uint8_t ch32v_pkt_send(const uint8_t *data, int sz, uint8_t cmd)
{
	uint8_t pkt[33];
	
	/* build command or data packets */
	if(cmd)
	{
		pkt[0] = 0;
		pkt[1] = *data;
	}
	else
	{
		pkt[0] = 0x40;
		memcpy(&pkt[1], data, sz);
	}
	return ch32v_i2c_send(CH32V_I2C_ADDR, pkt, sz+1);
}

/*
 * init I2C and GPIO
 */
uint8_t ch32v_i2c_init(void)
{
	// Enable GPIOC and I2C
	RCC->APB1PCENR |= RCC_APB1Periph_I2C1;

	RCC->APB2PCENR |= RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO;
	// PC1 is SDA, 10MHz Output, alt func, open-drain
	GPIOC->CFGLR &= ~(0xf<<(4*1));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF)<<(4*1);
	
	// PC2 is SCL, 10MHz Output, alt func, open-drain
	GPIOC->CFGLR &= ~(0xf<<(4*2));
	GPIOC->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_OD_AF)<<(4*2);

	// load I2C regs
	ch32v_i2c_setup();
	
	return 0;
}

/*
 * reset is not used for CH32V I2C interface
 */
void ch32v_rst(void)
{
}
