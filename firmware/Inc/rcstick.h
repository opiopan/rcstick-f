/*
 * rcstick.h
 *  Author: opiopan@gmail.com
 */

#ifndef RCSTICK_H_
#define RCSTICK_H_

#include "project.h"

typedef struct {
    TIM_HandleTypeDef*  hrtimer;
    SPI_HandleTypeDef*  spi;
    GPIO_TypeDef*       rf_cs_ch;
    uint16_t            rf_cs_pin;
    GPIO_TypeDef*       rf_test_ch;
    uint16_t            rf_test_pin;
    GPIO_TypeDef*       rf_int_ch;
    uint16_t            rf_int_pin;
    TIM_HandleTypeDef*  led_pwm_timer;
    uint16_t            led_pwm_ch;
    GPIO_TypeDef*       button_ch;
    uint16_t            button_pin;
} RcstickConf;

void run_rcstick(const RcstickConf* conf);

#endif