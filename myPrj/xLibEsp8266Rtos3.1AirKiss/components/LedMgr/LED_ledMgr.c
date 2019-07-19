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
#include "UTL_ledMgr.h"
#include "LED_ledMgr.h"
#include "WIFI_wifiMgr.h"


#define PLUG_RELAY_IO 	        GPIO_NUM_4
#define PLUG_RED_LED_IO 	    GPIO_NUM_0
#define PLUG_GREEN_LED_IO 	    GPIO_NUM_2
#define GPIO_OUTPUT_PIN_SEL     (1ULL<<PLUG_GREEN_LED_IO | 1ULL<<PLUG_RED_LED_IO  | 1ULL<<PLUG_RELAY_IO)    //((1ULL<<GPIO_OUTPUT_IO_0) | (1ULL<<GPIO_OUTPUT_IO_1))

/* blink codes (for frequencies) */
#define FREQ_START_SC 	EV_LED_BLINK_FREQ(6)        //led blink frequency when start smartconfig 
#define FREQ_WAIT_SC 	EV_LED_BLINK_FREQ(1)
#define FREQ_1 	EV_LED_BLINK_FREQ(1)

#define BLINK_3 	EV_LED_BLINK_NUMBER(3)

#define DO_LED_STATUS(x)		(tLedStatus.unRequest = (x))

static void toggleIO(gpio_num_t tIONum, bool* bIOState);

/*status LED control */
static bool bStatusLEDState = false;
static void clrLedStatus(void) 	{	gpio_set_level(PLUG_RED_LED_IO, 1); }
static void setLedStatus(void) 	{	gpio_set_level(PLUG_RED_LED_IO, 0); }
static void togLedStatus(void) 	{	toggleIO(PLUG_RED_LED_IO, &bStatusLEDState); }

/*power  LED control */
/*always on  */
static void setLedPower(void) 	{	gpio_set_level(PLUG_GREEN_LED_IO, 0); }

static UTL_LEDHANDLE tLedStatus = { 0, 0, 0, 0, 0, 0, clrLedStatus, setLedStatus, togLedStatus } ;
static const char *TAG = "led_Mgr";


static void toggleIO(gpio_num_t tIONum, bool* bIOState)
{
    if(*bIOState == true) {
        gpio_set_level(tIONum, 1);
        *bIOState = false;
    } else {
        gpio_set_level(tIONum, 0);
        *bIOState = true;
    }
}

static void ledTask(void *arg)
{
    WIFI_Smartconfig_Status tSmartConfigStatus;
    ESP_LOGI(TAG,"start LED task.\n");
    setLedPower();

    for (;;) {
        tSmartConfigStatus = WIFI_tGetSmartConfigStatus();
        if(tSmartConfigStatus != WIFI_NOT_SC) {
            if(tSmartConfigStatus == WIFI_WAIT_SC) {
                DO_LED_STATUS(FREQ_WAIT_SC);
            } else {
                DO_LED_STATUS(FREQ_START_SC);
            }
        } else {
            DO_LED_STATUS(EV_LED_OFF);
            //TODO LED ON/OFF by relay status
        }
        UTL_manageLed(&tLedStatus);
        vTaskDelay(1 / portTICK_RATE_MS);
    }
}
///#define DO_LED_ALARM(x)		(tLedAlarm.unRequest = (x))

//UTL_manageLed(&tLedPower);
void LED_ledMgrInit(void)
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

    if(xTaskCreate(ledTask, "ledTask", configMINIMAL_STACK_SIZE * 3,
            NULL, configMAX_PRIORITIES - 5, NULL) != pdPASS) {
        ESP_LOGI(TAG,"create airkiss thread failed.\n");
        return false;
    }
}