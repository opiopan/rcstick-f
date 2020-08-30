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
typedef struct {
    uint8_t l;
    uint8_t h;
} AXISDATA;

static inline void SETDATA(AXISDATA* usbdata, uint16_t rxdata, int* clipped){
    int data = (int)rxdata - SFHSS_CENTERPOS_WIDTH;
    if (data > 511){
        data = 511;
        (*clipped)++;
    }else if (data < -511){
        data = -511;
        (*clipped)++;
    }
    data <<= 6;
    usbdata->h = (data & 0xff00) >> 8;
    usbdata->l = data & 0xff;
}

static int send_report()
{
    static struct {
        uint8_t id;
        AXISDATA axis[8];
    } report[4]; 
    static int current = 0;

    int clipped = 0;

    report[current].id = 1;
    SETDATA(report[current].axis + 0, sfhss_ctx.data[0], &clipped);
    SETDATA(report[current].axis + 1, sfhss_ctx.data[1], &clipped);
    SETDATA(report[current].axis + 2, sfhss_ctx.data[2], &clipped);
    SETDATA(report[current].axis + 3, sfhss_ctx.data[3], &clipped);
    SETDATA(report[current].axis + 4, sfhss_ctx.data[4], &clipped);
    SETDATA(report[current].axis + 5, sfhss_ctx.data[5], &clipped);
    SETDATA(report[current].axis + 6, sfhss_ctx.data[6], &clipped);
    SETDATA(report[current].axis + 7, sfhss_ctx.data[7], &clipped);
    USBD_CUSTOM_HID_SendReport_FS((uint8_t*)(report + current), sizeof(report[0]));
    current = (current + 1) & 3;

    return clipped;
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
    HAL_Delay(200);
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
    #define LOG_ON 1
    #define LOG_RAW 2
    int initialstate = TRUE;
    int8_t logmode = LOG_RAW;
    int32_t logtime = 0;
    int32_t recvnum = 0;
    int32_t lostnum = 0;
    int32_t failsafenum = 0;
    int clipped = 0;
    BOOL testmode = FALSE;

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
            led_set_mode(&led_ctx, testmode && !clipped ? LEDMODE_TESTMODE : LEDMODE_CONNECTED, now);
            initialstate = FALSE;
            recvnum = 0;
            lostnum = 0;
            break;
        default:
            break;
        }
        if (initialstate || (sfhss_ctx.phase == SFHSS_CONNECTED && SFHSS_ISDIRTY(&sfhss_ctx))){
            clipped = send_report();
            SFHSS_RESET_DIRTY(&sfhss_ctx);
            if (sfhss_ctx.phase == SFHSS_CONNECTED && testmode){
                led_set_mode(&led_ctx, clipped ? LEDMODE_CONNECTED : LEDMODE_TESTMODE, now);
            }
            if (!initialstate && (logmode & LOG_ON) && now - logtime >= 1000000){
                logtime = now;
                if (logmode & LOG_RAW){
                    olog_printf(
                        "%9d: ch1[%4d] ch2[%4d] ch3[%4d] ch4[%4d]\n", now,
                        sfhss_ctx.data[0], sfhss_ctx.data[1], 
                        sfhss_ctx.data[2], sfhss_ctx.data[3]);
                    olog_printf(
                        "           ch5[%4d] ch6[%4d] ch7[%4d] ch8[%4d]\n",
                        sfhss_ctx.data[4], sfhss_ctx.data[5], 
                        sfhss_ctx.data[6], sfhss_ctx.data[7]);
                }else{
                    olog_printf(
                        "%9d: ch1[%4d] ch2[%4d] ch3[%4d] ch4[%4d]\n", now,
                        sfhss_ctx.data[0] - SFHSS_CENTERPOS_WIDTH,
                        sfhss_ctx.data[1] - SFHSS_CENTERPOS_WIDTH,
                        sfhss_ctx.data[2] - SFHSS_CENTERPOS_WIDTH,
                        sfhss_ctx.data[3] - SFHSS_CENTERPOS_WIDTH);
                    olog_printf(
                        "           ch5[%4d] ch6[%4d] ch7[%4d] ch8[%4d]\n",
                        sfhss_ctx.data[4] - SFHSS_CENTERPOS_WIDTH,
                        sfhss_ctx.data[5] - SFHSS_CENTERPOS_WIDTH,
                        sfhss_ctx.data[6] - SFHSS_CENTERPOS_WIDTH,
                        sfhss_ctx.data[7] - SFHSS_CENTERPOS_WIDTH);
                }
                static const char ATTR_LOST[] = "\033[31m";
                static const char ATTR_SKIP[] = "\033[32m";
                olog_printf(
                    "           RCV: \033[32m%d\033[0m, LOST: %s%d\033[0m FAILSAFE: %s%d\033[0m\n",
                    sfhss_ctx.stat_rcv - recvnum,
                    sfhss_ctx.stat_lost - lostnum == 0 ? "" : ATTR_LOST,
                    sfhss_ctx.stat_lost - lostnum,
                    sfhss_ctx.stat_failsafe - failsafenum == 0 ? "" : ATTR_SKIP,
                    sfhss_ctx.stat_failsafe - failsafenum);
                recvnum = sfhss_ctx.stat_rcv;
                lostnum = sfhss_ctx.stat_lost;
                failsafenum = sfhss_ctx.stat_failsafe;
            }
        }

        led_schedule(&led_ctx, now);

        switch(button_schedule(&button_ctx, now)){
        case BUTTONEV_UP:
            logmode = (logmode % 3) + 1;
            OLOG_LOGI("rcstick: log mode cahnged [%s, %s]",
                logmode & LOG_ON ? "ON": "OFF",
                logmode & LOG_RAW ? "RAW": "USB");
            logtime = now - 2000000;
            testmode = !testmode;
            if (sfhss_ctx.phase == SFHSS_CONNECTED){
                led_set_mode(&led_ctx,
                             testmode && !clipped ? LEDMODE_TESTMODE : LEDMODE_CONNECTED,
                             now);
            }
            OLOG_LOGI("rcstick: test mode cahnged [%s]",
                testmode ? "ON": "OFF");
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
