/*
 * button.c
 *  Author: opiopan@gmail.com
 */

#include "button.h"
#include "olog.h"

#define MSEC 1000
#define SEC 1000000
#define LPF_THRESHOLD 50 * MSEC
#define LONG_PRESS    2 * SEC
#define ULTRA_LONG_PRESS 10 * SEC

void button_init(BUTTONCTX* ctx, GPIO_TypeDef* button_ch, uint16_t button_pin)
{
    *ctx = (BUTTONCTX){
        .button_ch = button_ch,
        .button_pin = button_pin,
        .is_changed = FALSE,
        .is_on = FALSE,
        .mode = BUTTONMODE_NORMAL,
    };
}

BUTTON_EVENT button_schedule(BUTTONCTX *ctx, int32_t now)
{
    BUTTON_EVENT event = BUTTONEV_NONE;
    BOOL isOn = ctx->is_on;
    if (ctx->is_changed && now - ctx->update_time >= LPF_THRESHOLD){
        ctx->is_changed = FALSE;
        isOn = (HAL_GPIO_ReadPin(ctx->button_ch, ctx->button_pin) == 1);
    }

    if (isOn != ctx->is_on){
        OLOG_LOGI("button: %s a button", isOn ? "pressed" : "released");
        ctx->is_on = isOn;
        ctx->after_lpf_time = now;
        if (ctx->mode == BUTTONMODE_NORMAL){
            event = isOn ? BUTTONEV_PRESS : BUTTONEV_UP;
        }else{
            ctx->mode = BUTTONMODE_NORMAL;
        }
    }else if (isOn){
        int32_t elapse = now - ctx->after_lpf_time;
        if (elapse >= LONG_PRESS && ctx->mode == BUTTONMODE_NORMAL){
            ctx->mode = BUTTONMODE_LONGPRESSED;
            event = BUTTONEV_LONG_PRESS;
            OLOG_LOGI("button: detected long-pressed event");
        }else if (elapse >= ULTRA_LONG_PRESS && ctx->mode == BUTTONMODE_LONGPRESSED){
            ctx->mode = BUTTONMODE_ULTRALONGPRESSED;
            event = BUTTONEV_ULTRA_LONG_PRESS;
            OLOG_LOGI("button: detected ultra-long-pressed event");
        }
    }

    return event;
}
