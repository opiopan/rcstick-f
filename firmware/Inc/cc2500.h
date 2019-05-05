/*
 * cc2500.h
 *
 *  Created on: 2019/01/17
 *      Author: opiopan
 */

#ifndef CC2500_H_
#define CC2500_H_

#include "project.h"
#include "iface_cc2500.h"

typedef struct {
    struct {
        GPIO_TypeDef* ch;
        uint16_t pin;
    } selector;
    SPI_HandleTypeDef* spi;
    int inTransaction;
    BOOL cs:1;
} CC2500CTX;

BOOL cc2500_init(CC2500CTX* ctx, GPIO_TypeDef* sel_ch, uint16_t sel_pin, SPI_HandleTypeDef* spi);

void cc2500_begin(CC2500CTX* ctx);
void cc2500_commit(CC2500CTX* ctx);

int cc2500_writeRegister(CC2500CTX* ctx, uint8_t addr, uint8_t value);
int cc2500_readRegister(CC2500CTX* ctx, uint8_t addr, uint8_t* value);
int cc2500_writeRegisterBurst(CC2500CTX* ctx, uint8_t addr, const uint8_t* values, int len);
int cc2500_readRegisterBurst(CC2500CTX* ctx, uint8_t addr, uint8_t* values, int len);

int cc2500_reset(CC2500CTX* ctx);
int cc2500_strobe(CC2500CTX* ctx, uint8_t state);
int cc2500_strobeR(CC2500CTX* ctx, uint8_t state);
int cc2500_readFIFO(CC2500CTX* ctx, uint8_t* buf, int length);
int cc2500_waitForState(CC2500CTX *ctx, uint8_t state);

#define cc2500_readStatusRegister(ctx, addr, value) cc2500_readRegisterBurst(ctx, addr, value, 1)

#endif /* CC2500_H_ */
