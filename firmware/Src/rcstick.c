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
static void fatal_error()
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
    communicate with host
======================================================================*/
#define CONVDATA(val) ((((uint32_t)(val)-970) * 255) / 1100)
static void send_report()
{
    struct{
        uint8_t id;
        uint8_t axis[8];
    } report;

    report.id = 1;
    report.axis[0] = CONVDATA(sfhss_ctx.data[0]);
    report.axis[1] = CONVDATA(sfhss_ctx.data[1]);
    report.axis[2] = CONVDATA(sfhss_ctx.data[2]);
    report.axis[3] = CONVDATA(sfhss_ctx.data[3]);
    report.axis[4] = CONVDATA(sfhss_ctx.data[4]);
    report.axis[5] = CONVDATA(sfhss_ctx.data[5]);
    report.axis[6] = CONVDATA(sfhss_ctx.data[6]);
    report.axis[7] = CONVDATA(sfhss_ctx.data[7]);
    USBD_CUSTOM_HID_SendReport_FS((uint8_t *)&report, sizeof(report));
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
    int32_t logtime = 0;
    BOOL needLog = FALSE;
    while (TRUE){
        int32_t now = (int32_t)__HAL_TIM_GET_COUNTER(conf->hrtimer);
        switch (sfhss_schedule(&sfhss_ctx, now)){
        case SFHSSEV_START_FINDING:
            led_set_mode(&led_ctx, LEDMODE_FINDING, now);
            break;
        case SFHSSEV_START_BINDING:
            led_set_mode(&led_ctx, LEDMODE_BINDING, now);
            break;
        case SFHSSEV_START_CONNECTING:
            led_set_mode(&led_ctx, LEDMODE_CONNECTING, now);
            break;
        case SFHSSEV_CONNECTED:
            led_set_mode(&led_ctx, LEDMODE_CONNECTED, now);
            break;
        default:
            break;
        }
        if (SFHSS_ISDIRTY(&sfhss_ctx)){
            send_report();
            SFHSS_RESET_DIRTY(&sfhss_ctx);
            if (needLog && now - logtime >= 1000000){
                logtime = now;
                olog_printf(
                    "%9d: ch1[%.dx] ch2[%.dx] ch3[%.dx] ch4[%.dx]\n", now,
                    sfhss_ctx.data[0], sfhss_ctx.data[1], sfhss_ctx.data[2], sfhss_ctx.data[3]);
                olog_printf(
                    "           ch5[%.dx] ch6[%.4d] ch7[%.4d] ch8[%.4d]\n",
                    sfhss_ctx.data[4], sfhss_ctx.data[5], sfhss_ctx.data[6], sfhss_ctx.data[7]);
            }
        }

        led_schedule(&led_ctx, now);

        switch(button_schedule(&button_ctx, now)){
        case BUTTONEV_UP:
            needLog = !needLog;
            logtime = now - 2000000;
            break;
        case BUTTONEV_LONG_PRESS:
            OLOG_LOGI("rcstick: restart binding");
            SFHSS_START_BINDING(&sfhss_ctx);
            break;
        case BUTTONEV_ULTRA_LONG_PRESS:
            OLOG_LOGW("rcstick: reboot in order to enter DFU mode");
            NVIC_SystemReset();
        default: 
            break;
        }
    }
}
