/*
 * hrtimer.h
 *  Author: opiopan@gmail.com
 */
#ifndef HRTIMER_H_
#define HRTIMER_H_

#include "project.h"

extern TIM_HandleTypeDef* hrtimer_handler;

#define HRTIMER_GETTIME() ((int32_t)__HAL_TIM_GET_COUNTER(hrtimer_handler))

void hrtimer_init(TIM_HandleTypeDef* handler);

#endif
