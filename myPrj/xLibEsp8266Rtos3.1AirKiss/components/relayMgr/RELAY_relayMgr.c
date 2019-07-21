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
#include "RELAY_relayMgr.h"


#define PLUG_RELAY_IO 	        GPIO_NUM_4
#define GPIO_OUTPUT_PIN_SEL     (1ULL<<PLUG_RELAY_IO)    //((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

static const char *TAG = "relayMgr";
static bool bRelayOn = false;

void RELAY_relayMgrInit(void)
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
    gpio_set_level(PLUG_RELAY_IO, 0);
    ESP_LOGI(TAG, "RELAY_relayMgrInit");
}

void RELAY_setRelay(void)
{
    gpio_set_level(PLUG_RELAY_IO, 1);
    bRelayOn = true;
}

void RELAY_resetRelay(void)
{
    gpio_set_level(PLUG_RELAY_IO, 0);
    bRelayOn = false;
}


bool RELAY_getRelayState(void)
{
    return bRelayOn;
}

void RELAY_relayCtlMgr(void)
{
    if(bRelayOn) {
        gpio_set_level(PLUG_RELAY_IO, 0);
        bRelayOn = false;
    } else {
        gpio_set_level(PLUG_RELAY_IO, 1);
        bRelayOn = true;
    }
}


