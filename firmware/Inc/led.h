/*
 * led.h
 *
 *  Created on: 2019/05/03
 *  Author: opiopan@gmail.com
 */

#ifndef LED_H_
#define LED_H_

#include "project.h"

typedef enum{
    LEDMODE_ERROR = 0,
    LEDMODE_FINDING,
    LEDMODE_BINDING,
    LEDMODE_CONNECTING,
    LEDMODE_CONNECTED,
    LEDMODE_TESTMODE,
} LEDMODE;

typedef struct {
    TIM_HandleTypeDef*  pwm_timer;
    uint16_t            pwm_ch;
    LEDMODE             mode;
    int32_t             stage;
    int32_t             changetime;
} LEDCTX;

void led_init(LEDCTX* ctx, TIM_HandleTypeDef* pwm_timer, uint16_t pwm_ch);
void led_set_mode(LEDCTX* ctx, LEDMODE mode, int32_t now);
void led_schedule(LEDCTX* ctx, int32_t now);

#endif
