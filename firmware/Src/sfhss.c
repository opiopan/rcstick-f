/*
 * sfhss.c
 *
 *  Created on: 2019/01/20
 *      Author: opiopan@gmail.com
 */

#include "sfhss.h"

#define BINDING_MEASURE_COUNT 15
#define SHORT_INTERVAL_MAX (6660 + 600)
#define SHORT_INTERVAL_MIN (6660 - 600)
#define LONG_INTERVAL_MAX (6660 * 30 + 600)
#define LONG_INTERVAL_MIN (6660 * 30 - 600)
#define HOPPING_TIMEOUT 1000
#define FALLBACK_COUNT 30

// Some important initialization parameters, all others are either default,
// IOCFG2   2F - GDO2_INV=0 GDO2_CFG=2F - HW0
// IOCFG1   2E - GDO1_INV=0 GDO1_CFG=2E - High Impedance
// IOCFG0   07 - GDO0_INV=0 GDO0_CFG=07 - assert when packet is received
// FIFOTHR  07 - 33 decimal RX FIFO threshold
// SYNC1    D3
// SYNC0    91
// PKTLEN   0D - Packet length, 0D bytes
// PKTCTRL1 04 - APPEND_STATUS on, all other are receive parameters - irrelevant
// PKTCTRL0 0C - No whitening, use FIFO, CC2400 compatibility on, use CRC, fixed packet length
// ADDR     29
// CHANNR   10
// FSCTRL1  06 - IF 152343.75Hz, see page 65
// FSCTRL0  00 - zero freq offset
// FREQ2    5C - synthesizer frequency 2399999633Hz for 26MHz crystal, ibid
// FREQ1    4E
// FREQ0    C4
// MDMCFG4  7C - CHANBW_E - 01, CHANBW_M - 03, DRATE_E - 0C. Filter bandwidth = 232142Hz
// MDMCFG3  43 - DRATE_M - 43. Data rate = 128143bps
// MDMCFG2  83 - disable DC blocking, 2-FSK, no Manchester code, 15/16 sync bits detected
// MDMCFG1  23 - no FEC, 4 preamble bytes, CHANSPC_E - 03
// MDMCFG0  3B - CHANSPC_M - 3B. Channel spacing = 249938Hz (each 6th channel used, resulting in spacing of 1499628Hz)
// DEVIATN  44 - DEVIATION_E - 04, DEVIATION_M - 04. Deviation = 38085.9Hz
// MCSM2    07 - receive parameters, default, irrelevant
// MCSM1    3C - default CCA, when packet received stay RX mode, when sent go to IDLE
// MCSM0    08 - no autocalibration, PO_TIMEOUT - 64, no pin radio control, no forcing XTAL to stay in SLEEP
// FOCCFG   1D - not interesting, Frequency Offset Compensation
// BSCFG    1C
// AGCCTRL2 43
// AGCCTRL1 40
// AGCCTRL1 91
// WOREVT1  57
// WOREVT0  6B
// WORCTRL  F8
// FREND1   B6
// FREND0   10 - PA_POWER = 0
// FSCAL3   EA
// FSCAL2   0A
// FSCAL1   11
// FSCAL0   11
const uint8_t SFHSS_init_values[] = {
  /* 00 */ 0x2F, 0x2E, 0x07, 0x07, 0xD3, 0x91, 0x0D, 0x04,
  /* 08 */ 0x0C, 0x29, 0x10, 0x06, 0x00, 0x5C, 0x4E, SFHSS_FREQ0_VAL + SFHSS_COARSE,
  /* 10 */ 0x7C, 0x43, 0x83, 0x23, 0x3B, 0x44, 0x07, 0x3c,
  /* 18 */ 0x08, 0x1D, 0x1C, 0x43, 0x40, 0x91, 0x57, 0x6B,
  /* 20 */ 0xF8, 0xB6, 0x10, 0xEA, 0x0A, 0x11, 0x11
};

static void chuneChannel(SFHSSCTX* ctx)
{
    cc2500_begin(ctx->cc2500);
    cc2500_strobe(ctx->cc2500, CC2500_SIDLE);
    cc2500_writeRegister(ctx->cc2500, CC2500_0A_CHANNR, SFHSS_CH(ctx));
    cc2500_strobe(ctx->cc2500, CC2500_SCAL);
    cc2500_commit(ctx->cc2500);
}

static void chuneChannelFast(SFHSSCTX* ctx)
{
    cc2500_begin(ctx->cc2500);
    cc2500_writeRegister(ctx->cc2500, CC2500_0A_CHANNR, SFHSS_CH(ctx));
    cc2500_writeRegister(ctx->cc2500, CC2500_25_FSCAL1, SFHSS_CALDATA(ctx));
    cc2500_commit(ctx->cc2500);
}

static void nextChannel(SFHSSCTX* ctx, uint8_t hopcode)
{
    uint8_t ch = ctx->ch;
    ch += hopcode + 2;
    if (ch > 29)
    {
        if (ch < 31)
            ch += hopcode + 2;
        ch -= 31;
    }
    ctx->ch = ch;
}

volatile int normalCount = 0;
volatile int failsafeCount = 0;
volatile int addrcount = 0;

static BOOL readPacket(SFHSSCTX* ctx, uint8_t* cmd)
{
    uint8_t buf[15];
    uint16_t txaddr = 0;
    uint8_t hopcode;

    cc2500_readFIFO(ctx->cc2500, buf, sizeof(buf));
    SFHSS_RESET_RECEIVED(ctx);

    uint8_t* pkt = buf + 0;
    txaddr = (uint16_t)pkt[1] << 8 | pkt[2];
    hopcode = ((pkt[11] & 0x7) << 2) | ((pkt[12]  & 0xc0) >> 6);
    *cmd = pkt[12] & 0x3f;

    if (ctx->txaddr < 0){
        ctx->txaddr = txaddr;
    }
    if (ctx->txaddr != txaddr){
        return FALSE;
    }
    ctx->hopcode = hopcode;

    uint16_t data[4];
    data[0] = (((uint16_t)pkt[5] & 0x07) << 9) | ((uint16_t)pkt[6] << 1) | (((uint16_t)pkt[7] & 0x80) >> 7);
    data[1] = (((uint16_t)pkt[7] & 0x7f) << 5) | (((uint16_t)pkt[8] & 0xf8) >> 3);
    data[2] = (((uint16_t)pkt[8] & 0x07) << 9) | ((uint16_t)pkt[9] << 1) | (((uint16_t)pkt[10] & 0x80) >> 7);
    data[3] = (((uint16_t)pkt[10] & 0x7f) << 5) | (((uint16_t)pkt[11] & 0xf8) >> 3);

    if (*cmd & 4){
        failsafeCount++;
    }else{
        normalCount++;
        int offset = (*cmd & 1) << 2;
        ctx->isDirty |=
                data[0] != ctx->data[offset + 0] ||  data[1] != ctx->data[offset + 1] ||
                data[2] != ctx->data[offset + 2] || data[3] != ctx->data[offset + 3];
        ctx->data[offset + 0] = data[0];
        ctx->data[offset + 1] = data[1];
        ctx->data[offset + 2] = data[2];
        ctx->data[offset + 3] = data[3];
    }

    if (ctx->phase == SFHSS_BINDING){
        if (ctx->measureCount[ctx->packetPos] > 0){
            ctx->interval[ctx->packetPos] = ctx->rtime - ctx->ptime[ctx->packetPos];
            ctx->intervalSum[ctx->packetPos] += ctx->interval[ctx->packetPos];
        }
        ctx->measureCount[ctx->packetPos] += 1;
    }
    ctx->ptime[ctx->packetPos] = ctx->rtime;
    ctx->packetPos = 1 - (*cmd & 1);

    return TRUE;
}

void sfhss_init(SFHSSCTX* ctx, CC2500CTX* cc2500, TIM_HandleTypeDef* timer)
{
    *ctx = (SFHSSCTX){
        .cc2500 = cc2500,
        .timer = timer,
        .phase = SFHSS_INIT,
    };
    cc2500_writeRegisterBurst(ctx->cc2500, 0, SFHSS_init_values, sizeof(SFHSS_init_values));
}

void sfhss_calibrate(SFHSSCTX* ctx)
{
    cc2500_begin(ctx->cc2500);
    for (int i = 0; i < SFHSS_CHNUM; i++){
        ctx->ch = i;
        chuneChannel(ctx);
        cc2500_waitForState(ctx->cc2500, CC2500_STATE_IDLE);
        cc2500_readRegister(ctx->cc2500, CC2500_25_FSCAL1, &SFHSS_CALDATA(ctx));
    }
    cc2500_commit(ctx->cc2500);
    ctx->phase = SFHSS_CALIBRATED;
}

void sfhss_schedule(SFHSSCTX* ctx)
{
    int now = (int)__HAL_TIM_GET_COUNTER(ctx->timer);
    switch ((int)ctx->phase){

    case SFHSS_CALIBRATED:{
        cc2500_strobe(ctx->cc2500, CC2500_SIDLE);
        cc2500_strobe(ctx->cc2500, CC2500_SFRX);
        ctx->ch = 0;
        chuneChannelFast(ctx);
        SFHSS_RESET_RECEIVED(ctx);
        cc2500_strobe(ctx->cc2500, CC2500_SRX);
        ctx->phase = SFHSS_START_BINDING;
        break;
    }

    case SFHSS_START_BINDING:{
        ctx->txaddr = -1;
        ctx->measureCount[0] = 0;
        ctx->measureCount[1] = 0;
        ctx->interval[0] = 0;
        ctx->interval[1] = 0;
        ctx->intervalSum[0] = 0;
        ctx->intervalSum[1] = 0;
        ctx->phase = SFHSS_FINDING_RADIO;
        break;
    }

    case SFHSS_FINDING_RADIO:{
        if (ctx->received){
            ctx->phase = SFHSS_BINDING;
        }
        break;
    }

    case SFHSS_BINDING:{
        if (ctx->received){
            uint8_t cmd;
            if (!readPacket(ctx, &cmd)){
                break;
            }
            if (ctx->measureCount[cmd & 1] > 1){
                int interval = ctx->interval[cmd & 1];
                if (!(interval > LONG_INTERVAL_MIN && interval < LONG_INTERVAL_MAX)){
                    ctx->phase = SFHSS_START_BINDING;
                }
            }
            if (ctx->measureCount[0] > BINDING_MEASURE_COUNT &&
                ctx->measureCount[1] > BINDING_MEASURE_COUNT){
                ctx->interval[0] = ctx->intervalSum[0] / (ctx->measureCount[0] - 1) / 30;
                ctx->interval[1] = ctx->intervalSum[1] / (ctx->measureCount[1] - 1) / 30;
                ctx->phase = SFHSS_BINDED;
            }
        }else if (now - ctx->rtime > LONG_INTERVAL_MAX){
            ctx->phase = SFHSS_START_BINDING;
        }
        break;
    }

    case SFHSS_BINDED:{
        cc2500_strobe(ctx->cc2500, CC2500_SIDLE);
        cc2500_strobe(ctx->cc2500, CC2500_SFRX);
        ctx->ch = 0;
        chuneChannelFast(ctx);
        SFHSS_RESET_RECEIVED(ctx);
        cc2500_strobe(ctx->cc2500, CC2500_SRX);
        ctx->phase = SFHSS_CONNECTING1;
        break;
    }

    case SFHSS_CONNECTING1:{
        if (ctx->received){
            uint8_t cmd;
            readPacket(ctx, &cmd);
            if (!(cmd & 1)){
                ctx->phase = SFHSS_CONNECTING2;
            }
        }
        break;
    }

    case SFHSS_CONNECTING2:{
        if (ctx->received){
            uint8_t cmd;
            readPacket(ctx, &cmd);
            if (cmd & 1){
                if (ctx->ptime[1] - ctx->ptime[0] > ctx->interval[0] / 2){
                    ctx->phase = SFHSS_CONNECTING1;
                }else{
                    ctx->skipCount = 0;
                    ctx->phase = SFHSS_CONNECTED;
                    nextChannel(ctx, ctx->hopcode);
                    chuneChannelFast(ctx);
                }
            }
        }
        break;
    }

    case SFHSS_CONNECTED:{
        if (ctx->received){
            uint8_t cmd;
            readPacket(ctx, &cmd);
            if (cmd & 1){
                nextChannel(ctx, ctx->hopcode);
                chuneChannelFast(ctx);
            }
        }else{
            int elapse = now - ctx->ptime[ctx->packetPos];
            if (elapse > (ctx->interval[ctx->packetPos] * (ctx->skipCount + 1)) + HOPPING_TIMEOUT){
                ctx->skipCount++;
                if (ctx->skipCount < FALLBACK_COUNT){
                    ctx->packetPos = 0;
                    nextChannel(ctx, ctx->hopcode);
                    chuneChannelFast(ctx);
                }else{
                    ctx->phase = SFHSS_BINDED;
                }
            }
        }
        break;
    }
    }
}
