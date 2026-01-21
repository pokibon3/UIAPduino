/*
 * Single-File-Header for CH32V I2C interface
 * 05-07-2023 E. Brombaugh
 */

#ifndef _CH32V_I2C_H
#define _CH32V_I2C_H

#include <stdint.h>

// For the CH32V203, we support remapping, if so,
// #define CH32V_REMAP_I2C

// CH32V I2C address
#define CH32V_I2C_ADDR 0x3c

// I2C Bus clock rate - must be lower the Logic clock rate
#define CH32V_I2C_CLKRATE 100000

// I2C Logic clock rate - must be higher than Bus clock rate
#define CH32V_I2C_PRERATE 200000

// uncomment this for high-speed 36% duty cycle, otherwise 33%
#define CH32V_I2C_DUTY

// I2C Timeout count
#define TIMEOUT_MAX 100000

// uncomment this to enable IRQ-driven operation
//#define CH32V_I2C_IRQ

// event codes we use
#define  CH32V_I2C_EVENT_MASTER_MODE_SELECT ((uint32_t)0x00030001)  /* BUSY, MSL and SB flag */
#define  CH32V_I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED ((uint32_t)0x00070082)  /* BUSY, MSL, ADDR, TXE and TRA flags */
#define  CH32V_I2C_EVENT_MASTER_BYTE_TRANSMITTED ((uint32_t)0x00070084)  /* TRA, BUSY, MSL, TXE and BTF flags */

void ch32v_i2c_setup(void);
uint8_t ch32v_i2c_error(uint8_t err);
uint8_t ch32v_i2c_chk_evt(uint32_t event_mask);
#ifdef CH32V_I2C_IRQ
uint8_t ch32v_i2c_send(uint8_t addr, uint8_t *data, uint8_t sz);
void I2C1_EV_IRQHandler(void) __attribute__((interrupt));
#else
uint8_t ch32v_i2c_send(uint8_t addr, const uint8_t *data, int sz);
#endif
uint8_t ch32v_pkt_send(const uint8_t *data, int sz, uint8_t cmd);
uint8_t ch32v_i2c_init(void);
void ch32v_rst(void);

#endif
