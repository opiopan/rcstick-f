/*
 * sfhss.h
 *
 *  Created on: 2019/01/20
 *      Author: opiopan
 */

#ifndef SFHSS_H_
#define SFHSS_H_

#include "project.h"
#include "cc2500.h"

#define SFHSS_COARSE                0
#define SFHSS_FREQ0_VAL             0xC4
#define SFHSS_CHNUM                 30
#define SFHSS_CH(ctx)               ((ctx)->ch * 6 + 16)
#define SFHSS_CALDATA(ctx)          ((ctx)->caldata[(ctx)->ch])
#define SFHSS_SET_RECEIVED(ctx, now)     ((ctx)->received = 1, (ctx)->rtime = (now))
#define SFHSS_RESET_RECEIVED(ctx)   ((ctx)->received = 0)
#define SFHSS_ISDIRTY(ctx)          ((ctx)->packetPos == 0 && (ctx)->isDirty)
#define SFHSS_RESET_DIRTY(ctx)      ((ctx)->isDirty = FALSE)
#define SFHSS_START_BINDING(ctx)    ((ctx)->phase = SFHSS_CALIBRATED)

#define SFHSS_CENTERPOS_WIDTH       1520  // PWM pulse width of cetner position (usec)

typedef enum {
    SFHSS_INIT = 0,
    SFHSS_CALIBRATED,
    SFHSS_START_BINDING,
    SFHSS_FINDING_RADIO,
    SFHSS_BINDING,
    SFHSS_BINDED,
    SFHSS_CONNECTING1,
    SFHSS_CONNECTING2,
    SFHSS_CONNECTED,
    SFHSS_PAKCET_RECEIVING,
    SFHSS_HOPPING,
    SFHSS_ABNORMAL_HOPPING,
} SFHSS_PHASE;

typedef enum {
    SFHSSEV_NONE = 0,
    SFHSSEV_START_FINDING,
    SFHSSEV_START_BINDING,
    SFHSSEV_START_CONNECTING,
    SFHSSEV_CONNECTED
}SFHSS_EVENT;

typedef struct {
    CC2500CTX*          cc2500;
    int                 ch;
    volatile int        received;
    int                 rtime;
    int                 ptime[2];
    int                 interval[2];
    int                 intervalSum[2];
    int                 measureCount[2];
    int                 skipCount;
    int                 stat_rcv;
    int                 stat_lost;
    int                 stat_failsafe;
    BOOL                isDirty;
    uint8_t *           dmabuf;
    int32_t             txaddr;
    SFHSS_PHASE         phase;
    uint8_t             packetPos;
    uint16_t            data[8];
    uint8_t             hopcode;
    uint8_t             caldata[SFHSS_CHNUM];
} SFHSSCTX;

void sfhss_init(SFHSSCTX* ctx, CC2500CTX* cc2500);
void sfhss_calibrate(SFHSSCTX* ctx);

SFHSS_EVENT sfhss_schedule(SFHSSCTX* ctx, int32_t now);
void sfhss_test(SFHSSCTX* ctx);

#endif /* SFHSS_H_ */
