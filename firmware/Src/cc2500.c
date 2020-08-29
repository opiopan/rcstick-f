/*
 * cc2500.c
 *
 *  Created on: 2019/01/17
 *      Author: opiopan@gmail.com
 */

#include "cc2500.h"
#include "olog.h"

#define CS_UP(ctx)      HAL_GPIO_WritePin((ctx)->selector.ch, (ctx)->selector.pin, GPIO_PIN_SET)
#define CS_DOWN(ctx)    HAL_GPIO_WritePin((ctx)->selector.ch, (ctx)->selector.pin, GPIO_PIN_RESET)
#define SPITIMEOUT 500
#define CC2500_PARTNUM 0x80

/*---------------------------------------------------------------
 * initialize context
 *-------------------------------------------------------------*/
BOOL cc2500_init(CC2500CTX* ctx, GPIO_TypeDef* sel_ch, uint16_t sel_pin, SPI_HandleTypeDef* spi)
{
    *ctx = (CC2500CTX){
        .selector = {
            .ch = sel_ch,
            .pin = sel_pin
        },
        .spi = spi
    };

    CS_UP(ctx);

    cc2500_reset(ctx);

    uint8_t partnum, version;
    cc2500_readStatusRegister(ctx, CC2500_30_PARTNUM, &partnum);
    cc2500_readStatusRegister(ctx, CC2500_31_VERSION, &version);
    OLOG_LOGI("CC2500: PARTNUM [%.2x], VERSION [%.2x]", partnum, version);
    if (partnum != CC2500_PARTNUM){
        OLOG_LOGE("CC2500: PARTNUM is invalid, That means CC2500 migth not be working");
    }
    cc2500_strobe(ctx, CC2500_SIDLE);

    return partnum == CC2500_PARTNUM;
}

/*---------------------------------------------------------------
 * basic communication
 *-------------------------------------------------------------*/
int cc2500_strobe(CC2500CTX *ctx, uint8_t state)
{
    uint8_t rc = 0x80;
    state |= CC2500_READ_SINGLE;
    CS_DOWN(ctx);
    if (HAL_SPI_TransmitReceive(ctx->spi, &state, &rc, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    CS_UP(ctx);
    return rc;
}

int cc2500_writeRegister(CC2500CTX *ctx, uint8_t addr, uint8_t value)
{
    uint8_t rc = 0x80;
    addr |= CC2500_WRITE_SINGLE;
    CS_DOWN(ctx);
    if (HAL_SPI_TransmitReceive(ctx->spi, &addr, &rc, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    if (rc & CC2500_STATUS_CHIP_RDYn_BM){
        CS_UP(ctx);
        return rc;
    }
    if (HAL_SPI_Transmit(ctx->spi, &value, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    CS_UP(ctx);
    return rc;
}

int cc2500_readRegister(CC2500CTX* ctx, uint8_t addr, uint8_t* value)
{
    uint8_t rc = 0x80;
    uint8_t maddr = CC2500_READ_SINGLE | addr;
    CS_DOWN(ctx);
    if (HAL_SPI_TransmitReceive(ctx->spi, &maddr, &rc, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    if (rc & CC2500_STATUS_CHIP_RDYn_BM){
        CS_UP(ctx);
        return rc;
    }
    uint8_t dummy = 0;
    if (HAL_SPI_TransmitReceive(ctx->spi, &dummy, value, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    CS_UP(ctx);
    return rc;
}

int cc2500_writeRegisterBurst(CC2500CTX* ctx, uint8_t addr, const uint8_t* values, int len)
{
    uint8_t rc = 0x80;
    addr |= CC2500_WRITE_BURST;
    CS_DOWN(ctx);
    if (HAL_SPI_TransmitReceive(ctx->spi, &addr, &rc, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    if (rc & CC2500_STATUS_CHIP_RDYn_BM){
        CS_UP(ctx);
        return rc;
    }
    for (int i = 0; i < len; i++){
        if (HAL_SPI_Transmit(ctx->spi, (uint8_t*)values + i, 1, SPITIMEOUT) != HAL_OK){
            CS_UP(ctx);
            return 0x80;
        }
    }
    CS_UP(ctx);
    return rc;
}

int cc2500_readRegisterBurst(CC2500CTX* ctx, uint8_t addr, uint8_t* values, int len)
{
    uint8_t rc = 0x80;
    addr |= CC2500_READ_BURST;
    CS_DOWN(ctx);
    if (HAL_SPI_TransmitReceive(ctx->spi, &addr, &rc, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    if (rc & CC2500_STATUS_CHIP_RDYn_BM){
        CS_UP(ctx);
        return rc;
    }
    uint8_t dummy = 0;
    for (int i = 0; i < len; i++){
        if (HAL_SPI_TransmitReceive(ctx->spi, &dummy, values + i, 1, SPITIMEOUT) != HAL_OK){
            CS_UP(ctx);
            return 0x80;
        }
    }
    CS_UP(ctx);
    return rc;
}

/*---------------------------------------------------------------
 * high level communication
 *-------------------------------------------------------------*/
int cc2500_reset(CC2500CTX* ctx)
{
    uint8_t rc = 0x80;
    uint8_t sdata, rdata;
    CS_DOWN(ctx);
    sdata = CC2500_SRES;
    if (HAL_SPI_TransmitReceive(ctx->spi, &sdata, &rc, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    sdata = CC2500_SNOP;
    int count=0;
    do {
        count++;
        if (HAL_SPI_TransmitReceive(ctx->spi, &sdata, &rdata, 1, SPITIMEOUT) != HAL_OK){
            CS_UP(ctx);
            return 0x80;
        }
    }while (rdata == 0xff);
    CS_UP(ctx);
    OLOG_LOGI("CC2500: waited for reset at %d times", count);
    return rc;
}

int cc2500_strobeR(CC2500CTX* ctx, uint8_t state)
{
    uint8_t rc = 0x80;
    state |= CC2500_READ_SINGLE;
    CS_DOWN(ctx);
    if (HAL_SPI_TransmitReceive(ctx->spi, &state, &rc, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    CS_UP(ctx);
    return rc;
}

int cc2500_readFIFO(CC2500CTX* ctx, uint8_t* buf, int length)
{
    CS_DOWN(ctx);
    static uint8_t sbuf[64 + 1] = {CC2500_3F_RXFIFO | CC2500_READ_BURST};
    if(HAL_SPI_TransmitReceive(ctx->spi, sbuf, buf, length, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    if (buf[0] & CC2500_STATE_RX_OVERFLOW){
        CS_UP(ctx);
        OLOG_LOGE("CC2500: rx fifo is in state of overflow");
        cc2500_strobe(ctx, CC2500_SFRX);
        return buf[0] | CC2500_STATUS_CHIP_RDYn_BM;
    }
    if (buf[0] & CC2500_STATUS_CHIP_RDYn_BM){
        CS_UP(ctx);
        return buf[0] | CC2500_STATUS_CHIP_RDYn_BM;
    }

    CS_UP(ctx);
    return buf[0];
}

int cc2500_waitForState(CC2500CTX* ctx, uint8_t state)
{
    CS_DOWN(ctx);
    uint8_t sdata, rdata;
    sdata = CC2500_SNOP;
    do {
        if (HAL_SPI_TransmitReceive(ctx->spi, &sdata, &rdata, 1, SPITIMEOUT) != HAL_OK){
            CS_UP(ctx);
            return 0x80;
        }
    }while ((rdata & CC2500_STATUS_STATE_BM) != state);
    CS_UP(ctx);
    return rdata;
}

/*---------------------------------------------------------------
 * multiple commands communications
 *-------------------------------------------------------------*/
BOOL cc2500_beginMulitipleOps(CC2500CTX *ctx)
{
    ctx->bufpos = 0;
    return TRUE;
}

BOOL cc2500_commitMultipleOps(CC2500CTX *ctx)
{
    int rc = HAL_OK;
    if (ctx->bufpos){
        CS_DOWN(ctx);
        rc = HAL_SPI_TransmitReceive(ctx->spi, ctx->sbuf, ctx->rbuf, ctx->bufpos, SPITIMEOUT);
        CS_UP(ctx);
        ctx->bufpos = 0;
    }
    return rc == HAL_OK;
}

uint8_t *cc2500_addStrobeOps(CC2500CTX *ctx, uint8_t state)
{
    if (ctx->bufpos + 1 >= CC2500_BULKOPSMAX){
        return NULL;
    }
    ctx->sbuf[ctx->bufpos] = state | CC2500_READ_SINGLE;
    uint8_t* rc = ctx->rbuf + ctx->bufpos;
    ctx->bufpos++;
    return rc;
}

uint8_t *cc2500_addWriteRegisterOps(CC2500CTX *ctx, uint8_t addr, uint8_t value)
{
    if (ctx->bufpos + 2 >= CC2500_BULKOPSMAX){
        return NULL;
    }
    ctx->sbuf[ctx->bufpos] = addr | CC2500_WRITE_SINGLE;
    ctx->sbuf[ctx->bufpos + 1] = value;
    uint8_t *rc = ctx->rbuf + ctx->bufpos;
    ctx->bufpos += 2;
    return rc;
}

uint8_t *cc2500_addReadRegisterOps(CC2500CTX *ctx, uint8_t addr, uint8_t value)
{
    if (ctx->bufpos + 2 >= CC2500_BULKOPSMAX){
        return NULL;
    }
    ctx->sbuf[ctx->bufpos] = addr | CC2500_READ_SINGLE;
    ctx->sbuf[ctx->bufpos + 1] = value;
    uint8_t *rc = ctx->rbuf + ctx->bufpos;
    ctx->bufpos += 2;
    return rc;
}
