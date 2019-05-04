/*
 * button.h
 *  Author: opiopan@gmail.com
 */

#pragma once

#include "project.h"

typedef enum{
    BUTTONMODE_NORMAL,
    BUTTONMODE_LONGPRESSED,
    BUTTONMODE_ULTRALONGPRESSED,
} BUTTON_MODE;

typedef enum {
    BUTTONEV_NONE = 0,
    BUTTONEV_PRESS,
    BUTTONEV_UP,
    BUTTONEV_LONG_PRESS,
    BUTTONEV_ULTRA_LONG_PRESS
}BUTTON_EVENT;

typedef struct {
    GPIO_TypeDef*   button_ch;
    uint16_t        button_pin;
    BOOL            is_changed;
    int32_t         update_time;
    int32_t         after_lpf_time;
    BOOL            is_on;
    BUTTON_MODE     mode;
}BUTTONCTX;

#define BUTTON_UPDATE_STATUS(ctx, now) ((ctx)->is_changed = TRUE, (ctx)->update_time = (now))

void button_init(BUTTONCTX* ctx, GPIO_TypeDef* button_ch, uint16_t button_pin);
BUTTON_EVENT button_schedule(BUTTONCTX* ctx, int32_t now);
