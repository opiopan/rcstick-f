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

#define CC2500_BULKOPSMAX 64

typedef enum {
    CC2500_DMA_IDLE = 0,
    CC2500_DMA_WORKING,
    CC2500_DMA_COMPLETE,
    CC2500_DMA_ERROR,
} CC2500_DMASTAT;

typedef struct {
    struct {
        GPIO_TypeDef* ch;
        uint16_t pin;
    } selector;
    SPI_HandleTypeDef* spi;
    volatile CC2500_DMASTAT dmastat;
    int bufpos;
    uint8_t sbuf[CC2500_BULKOPSMAX];
    uint8_t rbuf[CC2500_BULKOPSMAX];
} CC2500CTX;

#define CC2500_DMA_IS_WORKING(c) ((c)->dmastat == CC2500_DMA_WORKING)
#define CC2500_MARK_DMA_SUCCESSFUL(c) ((c)->dmastat = CC2500_DMA_COMPLETE)
#define CC2500_MARK_DMA_ERROR(c) ((c)->dmastat = CC2500_DMA_ERROR)

BOOL cc2500_init(CC2500CTX* ctx, GPIO_TypeDef* sel_ch, uint16_t sel_pin, SPI_HandleTypeDef* spi);

int cc2500_strobe(CC2500CTX *ctx, uint8_t state);
int cc2500_writeRegister(CC2500CTX *ctx, uint8_t addr, uint8_t value);
int cc2500_readRegister(CC2500CTX* ctx, uint8_t addr, uint8_t* value);
int cc2500_writeRegisterBurst(CC2500CTX* ctx, uint8_t addr, const uint8_t* values, int len);
int cc2500_readRegisterBurst(CC2500CTX* ctx, uint8_t addr, uint8_t* values, int len);

int cc2500_reset(CC2500CTX* ctx);
int cc2500_strobeR(CC2500CTX* ctx, uint8_t state);
int cc2500_readFIFO(CC2500CTX* ctx, uint8_t* buf, int length);
int cc2500_waitForState(CC2500CTX *ctx, uint8_t state);

BOOL cc2500_beginMulitipleOps(CC2500CTX* ctx);
BOOL cc2500_issueDMAforMultipleOps(CC2500CTX* ctx);
BOOL cc2500_commitMultipleOps(CC2500CTX *ctx);
uint8_t* cc2500_addStrobeOps(CC2500CTX *ctx, uint8_t state);
uint8_t* cc2500_addWriteRegisterOps(CC2500CTX *ctx, uint8_t addr, uint8_t value);
uint8_t* cc2500_addReadRegisterOps(CC2500CTX *ctx, uint8_t addr, uint8_t value);
uint8_t* cc2500_addReadFIFOOps(CC2500CTX *ctx, int length);

#define cc2500_readStatusRegister(ctx, addr, value) cc2500_readRegisterBurst(ctx, addr, value, 1)

#endif /* CC2500_H_ */
