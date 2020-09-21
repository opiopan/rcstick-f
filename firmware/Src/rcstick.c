/*
 * rcstick.c
 *  Author: opiopan@gmail.com
 */

#include "usbd_custom_hid_if.h"
#include "rcstick.h"
#include "hrtimer.h"
#include "cc2500.h"
#include "sfhss.h"
#include "led.h"
#include "button.h"
#include "olog.h"
#include "version.h"

#include "project.h"

const RcstickConf* app_config;
CC2500CTX cc2500_ctx;
SFHSSCTX  sfhss_ctx;
LEDCTX led_ctx;
BUTTONCTX button_ctx;

/*=====================================================================
    Interruption callbacks
======================================================================*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    int32_t now = HRTIMER_GETTIME();

    if (GPIO_Pin == app_config->rf_int_pin){
        SFHSS_SET_RECEIVED(&sfhss_ctx, now);
    }else if (GPIO_Pin == app_config->button_pin){
        BUTTON_UPDATE_STATUS(&button_ctx, now);
    }
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    CC2500_MARK_DMA_SUCCESSFUL(&cc2500_ctx);
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)
{
    CC2500_MARK_DMA_ERROR(&cc2500_ctx);
}

/*=====================================================================
    loop on error state
======================================================================*/
static void fatal_error()
{
    OLOG_LOGF("rcstick: couldn't work any more");
    int32_t now = HRTIMER_GETTIME();
    led_set_mode(&led_ctx, LEDMODE_ERROR, now);
    while (TRUE){
        int32_t now = HRTIMER_GETTIME();
        led_schedule(&led_ctx, now);
    }
}

/*=====================================================================
    communicate with host
======================================================================*/
typedef struct{
    uint8_t l;
    uint8_t h;
} AXISDATA;

static inline void SETDATA(AXISDATA * usbdata, uint16_t rxdata, int *clipped)
{
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

static int send_report(const uint16_t* data)
{
    static struct{
        uint8_t id;
        AXISDATA axis[8];
    } report[4];
    static int current = 0;

    int clipped = 0;

    report[current].id = 1;
    SETDATA(report[current].axis + 0, data[0], &clipped);
    SETDATA(report[current].axis + 1, data[1], &clipped);
    SETDATA(report[current].axis + 2, data[2], &clipped);
    SETDATA(report[current].axis + 3, data[3], &clipped);
    SETDATA(report[current].axis + 4, data[4], &clipped);
    SETDATA(report[current].axis + 5, data[5], &clipped);
    SETDATA(report[current].axis + 6, data[6], &clipped);
    SETDATA(report[current].axis + 7, data[7], &clipped);
    USBD_CUSTOM_HID_SendReport_FS((uint8_t *)(report + current), sizeof(report[0]));
    current = (current + 1) & 3;

    return clipped;
}

/*=====================================================================
    logging
======================================================================*/
static void print_log(int now, SFHSS_STAT* stat, BOOL rawmode)
{
    if (rawmode){
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
    SFHSS_STAT* cstat = &sfhss_ctx.stat;
    olog_printf(
        "           RCV: \033[32m%d\033[0m, LOST: %s%d\033[0m FAILSAFE: %s%d\033[0m\n",
        cstat->rcv - stat->rcv,
        cstat->lost - stat->lost == 0 ? "" : ATTR_LOST,
        cstat->lost - stat->lost,
        cstat->failsafe - stat->failsafe == 0 ? "" : ATTR_SKIP,
        cstat->failsafe - stat->failsafe);
    olog_printf(
        "           RCV: %dus, endRCV: %dus, HOP: %dus, AHOP: %dus, endHOP: %dus\n",
        cstat->proc_rcv_cnt - stat->proc_rcv_cnt ? 
            (cstat->proc_rcv_time - stat->proc_rcv_time) / 
            (cstat->proc_rcv_cnt - stat->proc_rcv_cnt) : 0,
        cstat->proc_endrcv_cnt - stat->proc_endrcv_cnt ? 
            (cstat->proc_endrcv_time - stat->proc_endrcv_time) / 
            (cstat->proc_endrcv_cnt - stat->proc_endrcv_cnt) : 0,
        cstat->proc_hopping_cnt - stat->proc_hopping_cnt ? 
            (cstat->proc_hopping_time - stat->proc_hopping_time) / 
            (cstat->proc_hopping_cnt - stat->proc_hopping_cnt) : 0,
        cstat->proc_ahopping_cnt - stat->proc_ahopping_cnt ? 
            (cstat->proc_ahopping_time - stat->proc_ahopping_time) / 
            (cstat->proc_ahopping_cnt - stat->proc_ahopping_cnt) : 0,
        cstat->proc_endhopping_cnt - stat->proc_endhopping_cnt ? 
            (cstat->proc_endhopping_time - stat->proc_endhopping_time) / 
            (cstat->proc_endhopping_cnt - stat->proc_endhopping_cnt) : 0
    );
}

/*=====================================================================
    SPI performance measurement (experimental code)
======================================================================*/
static void measureSpiPerf()
{
    #define MESURENUM 10
    struct {
        int first_half;
        int latter_half;
    }data[MESURENUM];
    int begin;

    /*------------------------------------------------------------------
        Polling
    ------------------------------------------------------------------*/
    for (int i = 0; i < MESURENUM; i++){
        begin = HRTIMER_GETTIME();
        cc2500_beginMulitipleOps(&cc2500_ctx);
        cc2500_addReadRegisterBurstOps(&cc2500_ctx, 0, (i * 2 + 1));
        cc2500_commitMultipleOps(&cc2500_ctx);
        data[i].first_half = HRTIMER_GETTIME() - begin;
    }
    olog_printf("\n    SPI/POLLING performance (us)\n");
    olog_printf("           2B|  4B|  6B|  8B| 10B| 12B| 14B| 16B| 18B| 20B\n");
    olog_printf("    ----+----+----+----+----+----+----+----+----+----+----\n");
    olog_printf("    all |%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d\n\n",
                data[0].first_half, data[1].first_half, data[2].first_half,
                data[3].first_half, data[4].first_half,
                data[5].first_half, data[6].first_half, data[7].first_half,
                data[8].first_half, data[9].first_half);

    /*------------------------------------------------------------------
        DMA
    ------------------------------------------------------------------*/
    for (int i = 0; i < MESURENUM; i++)
    {
        begin = HRTIMER_GETTIME();
        cc2500_beginMulitipleOps(&cc2500_ctx);
        cc2500_addReadRegisterBurstOps(&cc2500_ctx, 0, (i * 2 + 1));
        cc2500_issueDMAforMultipleOps(&cc2500_ctx);
        data[i].first_half = HRTIMER_GETTIME() - begin;
        while(CC2500_DMA_IS_WORKING(&cc2500_ctx));
        begin = HRTIMER_GETTIME();
        cc2500_commitMultipleOps(&cc2500_ctx);
        data[i].latter_half = HRTIMER_GETTIME() - begin;
    }
    olog_printf("    SPI/DMA performance (us)\n");
    olog_printf("           2B|  4B|  6B|  8B| 10B| 12B| 14B| 16B| 18B| 20B\n");
    olog_printf("    ----+----+----+----+----+----+----+----+----+----+----\n");
    olog_printf("    pre |%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d\n",
                data[0].first_half, data[1].first_half, data[2].first_half,
                data[3].first_half, data[4].first_half,
                data[5].first_half, data[6].first_half, data[7].first_half,
                data[8].first_half, data[9].first_half);
    olog_printf("    post|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d|%4d\n\n",
                data[0].latter_half, data[1].latter_half, data[2].latter_half,
                data[3].latter_half, data[4].latter_half,
                data[5].latter_half, data[6].latter_half, data[7].latter_half,
                data[8].latter_half, data[9].latter_half);
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
    olog_printf("     firmware version: %s\n", version_string);
    olog_printf("     copyright: opiopan@gmail.com\n");
    olog_printf("     repository: https://github.com/opiopan/rcstick-f\n");
    olog_printf("----------------------------------------------------\n");

    HAL_Delay(200);

    /*----------------------------------------------------------------------
        Initialize peripherals & functions
    ----------------------------------------------------------------------*/
    app_config = conf;
    hrtimer_init(conf->hrtimer);
    led_init(&led_ctx, conf->led_pwm_timer, conf->led_pwm_ch);
    if (!cc2500_init(&cc2500_ctx, conf->rf_cs_ch, conf->rf_cs_pin, conf->spi)){
        fatal_error();
    }
    sfhss_init(&sfhss_ctx, &cc2500_ctx);
    sfhss_calibrate(&sfhss_ctx);
    button_init(&button_ctx, conf->button_ch, conf->button_pin);

    int32_t now = HRTIMER_GETTIME();
    led_set_mode(&led_ctx, LEDMODE_FINDING, now);

    OLOG_LOGI("rcstick: initialization finish");

    measureSpiPerf();

    /*----------------------------------------------------------------------
        main loop
    ----------------------------------------------------------------------*/
    uint16_t initial_data[8];
    memcpy(initial_data, sfhss_ctx.data, sizeof(initial_data));

    #define LOG_ON 1
    #define LOG_RAW 2
    int initialstate = TRUE;
    int8_t logmode = LOG_RAW;
    int32_t logtime = 0;
    SFHSS_STAT stat = sfhss_ctx.stat;
    int clipped = 0;
    BOOL testmode = FALSE;

    while (TRUE){
        int32_t now = HRTIMER_GETTIME();
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
            break;
        default:
            break;
        }
        if ((sfhss_ctx.phase == SFHSS_CONNECTED && SFHSS_ISDIRTY(&sfhss_ctx))){
            clipped = send_report(sfhss_ctx.data);
            SFHSS_RESET_DIRTY(&sfhss_ctx);
            if (testmode){
                led_set_mode(&led_ctx, clipped ? LEDMODE_CONNECTED : LEDMODE_TESTMODE, now);
            }
            if ((logmode & LOG_ON) && now - logtime >= 1000000){
                logtime = now;
                print_log(now, &stat, logmode & LOG_RAW);
                stat = sfhss_ctx.stat;
            }
        }else if (initialstate){
            send_report(initial_data);
        }

        led_schedule(&led_ctx, now);

        switch (button_schedule(&button_ctx, now)){
        case BUTTONEV_UP:
            logmode = (logmode % 3) + 1;
            OLOG_LOGI("rcstick: log mode cahnged [%s, %s]",
                      logmode & LOG_ON ? "ON" : "OFF",
                      logmode & LOG_RAW ? "RAW" : "USB");
            logtime = now - 2000000;
            testmode = !testmode;
            if (sfhss_ctx.phase == SFHSS_CONNECTED){
                led_set_mode(&led_ctx,
                             testmode && !clipped ? LEDMODE_TESTMODE : LEDMODE_CONNECTED,
                             now);
            }
            OLOG_LOGI("rcstick: test mode cahnged [%s]",
                      testmode ? "ON" : "OFF");
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
