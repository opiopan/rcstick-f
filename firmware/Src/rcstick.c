/*
 * rcstick.c
 *  Author: opiopan@gmail.com
 */

#include "usbd_custom_hid_if.h"
#include "rcstick.h"
#include "cc2500.h"
#include "sfhss.h"
#include "led.h"
#include "button.h"
#include "olog.h"

#include "project.h"

const RcstickConf* app_config;
CC2500CTX cc2500_ctx;
SFHSSCTX  sfhss_ctx;
LEDCTX led_ctx;
BUTTONCTX button_ctx;

/*=====================================================================
    Interruption callback
======================================================================*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(app_config->hrtimer);

    if (GPIO_Pin == app_config->rf_int_pin){
        SFHSS_SET_RECEIVED(&sfhss_ctx, now);
    }else if (GPIO_Pin == app_config->button_pin){
        BUTTON_UPDATE_STATUS(&button_ctx, now);
    }
}

/*=====================================================================
    loop on error state
======================================================================*/
void fatal_error()
{
    OLOG_LOGF("rcstick: couldn't work any more");
    int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(app_config->hrtimer);
    led_set_mode(&led_ctx, LEDMODE_ERROR, now);
    while (TRUE){
        int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(app_config->hrtimer);
        led_schedule(&led_ctx, now);
    }
}

/*=====================================================================
    Main logic
======================================================================*/
void run_rcstick(const RcstickConf *conf)
{
    olog_init();
    olog_printf("\033[0m\n\n");
    olog_printf("----------------------------------------------------\n");
    olog_printf("rcstick-f\n");
    olog_printf("     copyright: opiopan@gmail.com\n");
    olog_printf("     repository: https://github.com/opiopan/rcstick-f\n");
    olog_printf("----------------------------------------------------\n");

    /*----------------------------------------------------------------------
    Initialize peripherals & functions
    ----------------------------------------------------------------------*/
    app_config = conf;
    HAL_Delay(100);
    HAL_TIM_Base_Start(conf->hrtimer);
    HAL_TIM_PWM_Start(conf->led_pwm_timer, conf->led_pwm_ch);
    led_init(&led_ctx, conf->led_pwm_timer, conf->led_pwm_ch);
    if (!cc2500_init(&cc2500_ctx, conf->rf_cs_ch, conf->rf_cs_pin, conf->spi)){
        fatal_error();
    }
    sfhss_init(&sfhss_ctx, &cc2500_ctx);
    sfhss_calibrate(&sfhss_ctx);
    button_init(&button_ctx, conf->button_ch, conf->button_pin);
    
    int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(conf->hrtimer);
    led_set_mode(&led_ctx, LEDMODE_FINDING, now);

    OLOG_LOGI("rcstick: initialization finish");

    /*----------------------------------------------------------------------
    main loop
    ----------------------------------------------------------------------*/
    while (TRUE){
        int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(conf->hrtimer);
        //SFHSS_EVENT ev = sfhss_schedule(&sfhss_ctx, now);
        led_schedule(&led_ctx, now);

        switch(button_schedule(&button_ctx, now)){
        case BUTTONEV_UP:
            led_set_mode(&led_ctx, LEDMODE_ERROR, now);
            break;
        case BUTTONEV_LONG_PRESS:
            led_set_mode(&led_ctx, LEDMODE_FINDING, now);
            break;
        case BUTTONEV_ULTRA_LONG_PRESS:
            OLOG_LOGW("rcstick: reboot in order to enter DFU mode");
            NVIC_SystemReset();
        default: 
            break;
        }
    }
}
