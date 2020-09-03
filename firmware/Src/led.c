/*
 * led.c
 *
 *  Created on: 2019/05/03
 *  Author: opiopan@gmail.com
 */


#include "led.h"
#include "olog.h"

#define ON_VALUE 80

typedef enum {
    PK_END = 0,
    PK_LOOP,
    PK_ON,
    PK_OFF,
} PHASEKIND;

typedef struct {
    PHASEKIND   kind;
    int32_t     duration;
} PHASE;

#define SEQ_ON(d) {PK_ON, (d)}
#define SEQ_OFF(d) {PK_OFF, (d)}
#define SEQ_END {PK_END, 0}
#define SEQ_LOOP {PK_LOOP, 0}

const PHASE seq_error[] = {SEQ_ON(100000), SEQ_OFF(100000), SEQ_LOOP};
const PHASE seq_finding[] = {SEQ_ON(100000), SEQ_OFF(100000), SEQ_ON(100000), SEQ_OFF(1700000), SEQ_LOOP};
const PHASE seq_binding[] = {SEQ_ON(250000), SEQ_OFF(250000), SEQ_LOOP};
const PHASE seq_connecting[] = {SEQ_ON(200000), SEQ_OFF(1800000), SEQ_LOOP};
const PHASE seq_connected[] = {SEQ_ON(0), SEQ_END};
const PHASE seq_testmode[] = {SEQ_ON(100000), SEQ_OFF(100000), SEQ_ON(100000), SEQ_OFF(100000), SEQ_ON(100000), 
                              SEQ_OFF(500000), SEQ_LOOP};

const PHASE *seqs[] = {
    seq_error,
    seq_finding,
    seq_binding,
    seq_connecting,
    seq_connected,
    seq_testmode,
};

static void apply_stage(LEDCTX* ctx, int32_t now)
{
    const PHASE* seq = seqs[ctx->mode];
    if (seq[ctx->stage].kind == PK_END){
        return;
    }
    if (seq[ctx->stage].kind == PK_LOOP){
        ctx->stage = 0;
    }

    ctx->changetime = now + seq[ctx->stage].duration;
    __HAL_TIM_SET_COMPARE(
            ctx->pwm_timer, ctx->pwm_ch, 
            seq[ctx->stage].kind == PK_ON ? ON_VALUE : 0);
}

void led_init(LEDCTX *ctx, TIM_HandleTypeDef *pwm_timer, uint16_t pwm_ch)
{
   *ctx = (LEDCTX){
       .pwm_timer = pwm_timer,
       .pwm_ch = pwm_ch,
   };
   HAL_TIM_PWM_Start(ctx->pwm_timer, ctx->pwm_ch);
}

void led_set_mode(LEDCTX *ctx, LEDMODE mode, int32_t now)
{
    if (ctx->mode == mode){
        return;
    }
    ctx->mode = mode;
    ctx->stage = 0;
    apply_stage(ctx, now);
}

void led_schedule(LEDCTX *ctx, int32_t now)
{
    const PHASE *seq = seqs[ctx->mode];

    if (seq[ctx->stage].kind == PK_END){
        return;
    }

    if (now - ctx->changetime >= 0){
        ctx->stage++;
        apply_stage(ctx, now);
    }
}
