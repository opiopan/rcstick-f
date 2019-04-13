/*
 * cc2500.c
 *
 *  Created on: 2019/01/17
 *      Author: opiopan@gmail.com
 */

#include "cc2500.h"

#define CS_UP(ctx)      ((ctx)->cs ? 0 : \
                        (HAL_GPIO_WritePin((ctx)->selector.ch, (ctx)->selector.pin, GPIO_PIN_SET), (ctx)->cs = TRUE))
#define CS_DOWN(ctx)    ((ctx)->inTransaction ? 0 : \
                        (HAL_GPIO_WritePin((ctx)->selector.ch, (ctx)->selector.pin, GPIO_PIN_RESET), (ctx)->cs = FALSE))
#define CS_UP_FORCE(ctx) (HAL_GPIO_WritePin((ctx)->selector.ch, (ctx)->selector.pin, GPIO_PIN_SET), (ctx)->cs = TRUE)
#define CS_DOWN_FORCE(ctx) (HAL_GPIO_WritePin((ctx)->selector.ch, (ctx)->selector.pin, GPIO_PIN_RESET), (ctx)->cs = FALSE)
#define SPITIMEOUT 500

/*---------------------------------------------------------------
 * initialize context
 *-------------------------------------------------------------*/
void cc2500_init(CC2500CTX* ctx, GPIO_TypeDef* sel_ch, uint16_t sel_pin, SPI_HandleTypeDef* spi)
{
    *ctx = (CC2500CTX){
        .selector = {
            .ch = sel_ch,
            .pin = sel_pin
        },
        .spi = spi
    };

    CS_UP(ctx);

    cc2500_begin(ctx);
    cc2500_reset(ctx);
    cc2500_strobe(ctx, CC2500_SIDLE);
    cc2500_commit(ctx);
}

/*---------------------------------------------------------------
 * initialize context
 *-------------------------------------------------------------*/
void cc2500_begin(CC2500CTX *ctx)
{
    //ctx->inTransaction++;
}

void cc2500_commit(CC2500CTX *ctx)
{
    /*
    ctx->inTransaction--;
    if (ctx->inTransaction == 0){
        CS_UP_FORCE(ctx);
    }
    */
}

/*---------------------------------------------------------------
 * basic communication
 *-------------------------------------------------------------*/
int cc2500_writeRegister(CC2500CTX* ctx, uint8_t addr, uint8_t value)
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
    do {
        if (HAL_SPI_TransmitReceive(ctx->spi, &sdata, &rdata, 1, SPITIMEOUT) != HAL_OK){
            CS_UP(ctx);
            return 0x80;
        }
    }while (rdata == 0xff);
    CS_UP(ctx);
    return rc;
}

int cc2500_strobe(CC2500CTX* ctx, uint8_t state)
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
    uint8_t rc = 0x80;
    uint8_t cmd = CC2500_3F_RXFIFO | CC2500_READ_BURST;
    CS_DOWN(ctx);
    if (HAL_SPI_TransmitReceive(ctx->spi, &cmd, &rc, 1, SPITIMEOUT) != HAL_OK){
        CS_UP(ctx);
        return 0x80;
    }
    if ((rc & CC2500_STATUS_CHIP_RDYn_BM) || CC2500_STATUS_FIFO_BYTES_AVAILABLE_BM < length){
        CS_UP(ctx);
        return rc | CC2500_STATUS_CHIP_RDYn_BM;
    }

    cmd = 0;
    for (int i = 0; i < length; i++){
        if (HAL_SPI_TransmitReceive(ctx->spi, &cmd, buf + i, 1, SPITIMEOUT) != HAL_OK){
            CS_UP(ctx);
            return 0x80;
        }
    }

    CS_UP(ctx);
    return rc;
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
