/*
 * @Author: Xman 
 * @Date: 2019-07-11 10:33:19 
 * @Last Modified by: Xman
 * @Last Modified time: 2019-07-11 12:49:35
 */

#include <stdio.h>
#include "esp_system.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "UTL_digitalInputFilter.h"
#include "BTN_buttonMgr.h"

#define PLUG_PWR_BTN_IO 	    GPIO_NUM_14
#define GPIO_INPUT_PIN_SEL      (1ULL<<PLUG_PWR_BTN_IO)    //((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))
#define PLUG_PWR_BTN_STATE()    (gpio_get_level(PLUG_PWR_BTN_IO))

#define PLUG_USR_LED_IO 	    GPIO_NUM_5
#define GPIO_OUTPUT_PIN_SEL     (1ULL<<PLUG_USR_LED_IO)    //((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

#define INPUT_PIN_FILTER_TIME   (50 / portTICK_RATE_MS)     //so very PIN_FILTER_TIME x PIN_FILTER_CALL_TIME pin state will update once
#define PWR_BTN_LONG_PRESS_CNT  (4000 / portTICK_RATE_MS)   // long press confirm time
#define BTN_TASK_CALL_TIME      (1 / portTICK_RATE_MS)                           //ms



static const char *TAG = "keyMgr";
static xQueueHandle gpio_evt_queue = NULL;
static bool bKeyPress = false;
static bool bKeyNotPress = false;
static int nKeyLastPressTime = 0;

BTN_ButtonStatus tBTNPowerStatus;




/*----------------------------------------------------------------------------*/
/*                          private functions section                         */
/*----------------------------------------------------------------------------*/
static bool bIsPowerBtnPressed(void)          {return (PLUG_PWR_BTN_STATE() == false);} 
static UTL_DigitalInputFilter_t tBTNPower    = {0, 0, bIsPowerBtnPressed};

bool BTN_bIsBTNGlitchHigh(BTN_ButtonStatus *tBTNStatus)
{
    bool bRet = tBTNStatus->BF.bIsBTNGlitchHigh;
    tBTNStatus->BF.bIsBTNGlitchHigh = false;
    
    return bRet;
}

bool BTN_bIsBTNLongPressed(BTN_ButtonStatus *tBTNStatus)
{
    static bool bIsBtnPressed = false;
    bool bRet = false;//tBTNStatus->BF.bIsBTNLongPressed;

    if(tBTNStatus->BF.bIsBTNLongPressed == true && bIsBtnPressed == false) {
        //if(BTN_bIsBTNGlitchHigh(tBTNStatus) == true) {
        bRet = true;
        bIsBtnPressed = tBTNStatus->BF.bIsBTNPressed;
        //}
    } else if(tBTNStatus->BF.bIsBTNPressed == false) {
        bIsBtnPressed = false;
    }

    if(tBTNStatus->BF.bIsBTNLongPressed == true) {
        tBTNStatus->BF.bIsBTNLongPressed = false;
        tBTNStatus->BF.unLongPressedCnt = 0;
    }
    
    return bRet;
}

static void buttonStatus(UTL_DigitalInputFilter_t *tBTNPinState, BTN_ButtonStatus *tBTNStatus, uint32_t unLongPressedConfirmCnt)
{
    if(tBTNStatus->BF.bIsBTNPressed != tBTNPinState->bPinState) {
        if(tBTNStatus->BF.bIsBTNPressed == true && tBTNPinState->bPinState == false) {
            tBTNStatus->BF.bIsBTNGlitchLow = true;
        } else {
            tBTNStatus->BF.bIsBTNGlitchHigh = true;
        }
    }
    tBTNStatus->BF.bIsBTNPressed = tBTNPinState->bPinState;
    if(tBTNStatus->BF.bIsBTNPressed) {
        if(tBTNStatus->BF.unLongPressedCnt < unLongPressedConfirmCnt) {
            tBTNStatus->BF.unLongPressedCnt++;
        } else {
            tBTNStatus->BF.bIsBTNLongPressed = true;
        }
    } else {
        tBTNStatus->BF.bIsBTNLongPressed = false;
        tBTNStatus->BF.unLongPressedCnt = 0;
    }
}

static void gpio_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

static void keyTaskMgr(void *arg)
{
    uint32_t io_num;
    //int lastLevel = 0;

    for (;;) {
        /* 
        if(lastLevel != gpio_get_level(PLUG_PWR_BTN_IO)) {
            ESP_LOGI(TAG, "GPIO[%d], val: %d\n", PLUG_PWR_BTN_IO, gpio_get_level(PLUG_PWR_BTN_IO));
            lastLevel = gpio_get_level(PLUG_PWR_BTN_IO);
        }
        */
        UTL_bDigitalInputFilter(&tBTNPower, INPUT_PIN_FILTER_TIME);
        buttonStatus(&tBTNPower, & tBTNPowerStatus, PWR_BTN_LONG_PRESS_CNT);
        vTaskDelay(1 / portTICK_RATE_MS);

        if(BTN_bIsBTNLongPressed(&tBTNPowerStatus) == true) {
            ESP_LOGI(TAG, "Long press\n");
        } else if(BTN_bIsBTNGlitchHigh(&tBTNPowerStatus) == true) {
            ESP_LOGI(TAG, "key Glitch\n");
        }

    }

    //printf("Enter key task\n");
    for (;;) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            ESP_LOGI(TAG, "GPIO[%d] intr, val: %d\n", io_num, gpio_get_level(io_num));
            //记录下用户按下按键的时间点
        if (gpio_get_level(io_num) == 0) {
            bKeyPress = pdTRUE;
            nKeyLastPressTime = esp_log_timestamp(); 
            printf("presstime_first:%d \n", nKeyLastPressTime);
            //如果当前GPIO口的电平已经记录为按下，则开始减去上次按下按键的时间点
        } else if (bKeyPress) {
            //记录抬升时间点
            bKeyNotPress = pdTRUE;
            
            printf("presstime_2nd:%d \n", esp_log_timestamp());
            nKeyLastPressTime = esp_log_timestamp() - nKeyLastPressTime;
        }
            //printf("Enter key task222\n");
        }

        //仅仅当按下标志位和按键弹起标志位都为1时候，才执行回调
        if (bKeyPress & bKeyNotPress) {
            bKeyPress = pdFALSE;
            bKeyNotPress = pdFALSE;
            printf("presstime:%d \n", nKeyLastPressTime);

            //如果大于1s则回调长按，否则就短按回调
            if (nKeyLastPressTime > (5000 / portTICK_RATE_MS)) {
                printf("KEY_LONG_PRESS");
                //return KEY_LONG_PRESS;
            } else {
                printf("KEY_SHORT_PRESS");
                //return KEY_SHORT_PRESS;
            }
        }
        //vTaskDelay(1 / portTICK_RATE_MS);
    }
}

static void IOTestTask(void *arg)
{
    int cnt = 0;

    while (1) {
        //ESP_LOGI(TAG, "cnt: %d\n", cnt++);
        cnt++;
        vTaskDelay(1000 / portTICK_RATE_MS);
        gpio_set_level(PLUG_USR_LED_IO, cnt % 2);
        //gpio_set_level(GPIO_OUTPUT_IO_1, cnt % 2);
    }
}
/**
 * key IO init
 */
bool BTN_buttonIOInit(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO15/16
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    //interrupt of any edge(下降沿和上升沿触发中断)
    io_conf.intr_type = GPIO_INTR_ANYEDGE;//GPIO_INTR_ANYEDGE;//GPIO_INTR_NEGEDGE
    //bit mask of the pins
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    //set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    //enable pull-up mode
    io_conf.pull_up_en = 1;
    //io_conf.pull_down_en = 0;
    gpio_config(&io_conf);

    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(1, sizeof(uint32_t));
    //start gpio task
    if(xTaskCreate(keyTaskMgr, "keyTaskMgr", configMINIMAL_STACK_SIZE * 3,\
            NULL, configMAX_PRIORITIES - 5, NULL) != pdPASS) {
        ESP_LOGI(TAG,"create airkiss thread failed.\n");
        return false;
    }
    if(xTaskCreate(IOTestTask, "IOTestTask", configMINIMAL_STACK_SIZE * 3,\
            NULL, configMAX_PRIORITIES - 5, NULL) != pdPASS) {
        ESP_LOGI(TAG,"create airkiss thread failed.\n");
        return false;
    }

    //install gpio isr service
    gpio_install_isr_service(0);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(PLUG_PWR_BTN_IO, gpio_isr_handler, (void *) PLUG_PWR_BTN_IO);
    gpio_isr_handler_remove(PLUG_PWR_BTN_IO);
    
    return true;
}