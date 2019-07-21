#include <stdio.h>
#include "esp_system.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "nvs.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_err.h"
#include "internal/esp_wifi_internal.h"
#include "nvs_flash.h"
#include "rom/ets_sys.h"
#include "driver/uart.h"
#include "tcpip_adapter.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"
#include "airkiss.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>
#include "AIR_airkissImplement.h"
#include "WIFI_wifiMgr.h"
#include "OTA_otaHttpMgr.h"


/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
//#define EXAMPLE_ESP_WIFI_MODE_AP CONFIG_ESP_WIFI_MODE_AP //TRUE:AP FALSE:STA
#define EXAMPLE_ESP_WIFI_SSID "OP"
#define EXAMPLE_ESP_WIFI_PASS "888888888"
#define EXAMPLE_MAX_STA_CONN CONFIG_MAX_STA_CONN

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
//static const int WIFI_CONNECTED_BIT = BIT0;
static const char *TAG = "Wifi_Mgr";
static const char *pNVSTagWifiInfo = "wifi_info";

static const int CONNECTED_BIT = BIT0;
static const int ESPTOUCH_DONE_BIT = BIT1;
static const int AIRKISS_DONE_BIT = BIT2;

static xTaskHandle handleLlocalFind = NULL;

static bool bGetSSID = false;
//static const char *TAG = "xAirKiss";

//airkiss
#define COUNTS_BOACAST 30            //发包次数，微信建议20次以上
#define ACCOUNT_ID "gh_083fe269017c" //微信公众号
#define LOCAL_UDP_PORT 12476         //固定端口号
uint8_t deviceInfo[60] = {"5351722"};  //设备ID，也可以任意设置。

WIFI_Smartconfig_Status tSmartconfigStatus = WIFI_NOT_SC;

int sock_fd;

const airkiss_config_t akconf = {
    (airkiss_memset_fn)&memset,
    (airkiss_memcpy_fn)&memcpy,
    (airkiss_memcmp_fn)&memcmp,
    0,
};

static void taskUdpWithWechat(void *pvParameters)
{
    char rx_buffer[128];
    //char addr_str[128];
    uint8_t lan_buf[300];
    uint16_t lan_buf_len;
    struct sockaddr_in server_addr;
    //struct sockaddr_in client_addr;
    int sock_server; /* server socked */
    int err;
    //int counts = 0;

    sock_server = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_server == -1)
    {
        printf("failed to create sock_fd!\n");
        vTaskDelete(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr("255.255.255.255");
    server_addr.sin_port = htons(LOCAL_UDP_PORT);

    err = bind(sock_server, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (err == -1)
    {
        vTaskDelete(NULL);
    }

    struct sockaddr_in sourceAddr;
    socklen_t socklen = sizeof(sourceAddr);
    while (1)
    {
        //ESP_LOGI(TAG, "start wechat udp");
        memset(rx_buffer, 0, sizeof(rx_buffer));
        //stop to wait UDP pack from wechat UDP server
        int len = recvfrom(sock_server, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&sourceAddr, &socklen);

        ESP_LOGI(TAG, "IP:%s:%d", (char *)inet_ntoa(sourceAddr.sin_addr), htons(sourceAddr.sin_port));
        //ESP_LOGI(TAG, "Received %s ", rx_buffer);

        // Error occured during receiving
        if (len < 0)
        {
            ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
            break;
        }
        // Data received
        else
        {
            rx_buffer[len] = 0;                                                // Null-terminate whatever we received and treat like a string
            airkiss_lan_ret_t ret = airkiss_lan_recv(rx_buffer, len, &akconf); //检测是否为微信发的数据包
            airkiss_lan_ret_t packret;
            switch (ret)
            {
            case AIRKISS_LAN_SSDP_REQ:

                lan_buf_len = sizeof(lan_buf);
                //开始组装打包
                packret = airkiss_lan_pack(AIRKISS_LAN_SSDP_NOTIFY_CMD, ACCOUNT_ID, deviceInfo, 0, 0, lan_buf, &lan_buf_len, &akconf);
                if (packret != AIRKISS_LAN_PAKE_READY)
                {
                    ESP_LOGE(TAG, "Pack lan packet error!");
                    continue;
                }
                ESP_LOGI(TAG, "Pack lan packet ok !");
                //发送至微信客户端
                int err = sendto(sock_server, (char *)lan_buf, lan_buf_len, 0, (struct sockaddr *)&sourceAddr, sizeof(sourceAddr));
                if (err < 0)
                {
                    ESP_LOGE(TAG, "Error occured during sending: errno %d", errno);
                }
                break;
            default:
                break;
            }
        }
    }
}
/**
 * @description: 关闭进程
 * @param {type} 
 * @return: 
 */
void shutDownAirkissTask(void)
{
    shutdown(sock_fd, 0);
    close(sock_fd);
    vTaskDelete(&handleLlocalFind);
}

bool startAirkissUdpTask(void)
{
    int ret = pdFAIL;
    if (handleLlocalFind == NULL) {
        if(xTaskCreate(taskUdpWithWechat, "taskUdpWithWechat", configMINIMAL_STACK_SIZE * 3,
            NULL, configMAX_PRIORITIES - 10, &handleLlocalFind) != pdPASS) {
            printf("create airkiss thread failed.\n");
            return false;              
        }
    }
    return true;
}

void smartconfig_example_task(void *parm);

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    ESP_LOGI(TAG, "enter event_handler id=%d \n", event->event_id);
    /* For accessing reason codes in case of disconnection */
    system_event_info_t *info = &event->event_info;

    switch (event->event_id)
    {
    case SYSTEM_EVENT_STA_START:
        ESP_LOGI(TAG, "bGetSSID:%d", bGetSSID);
        if(bGetSSID == false) {
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
        } else {
            esp_wifi_connect();
        }
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip:%s",
                ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
                xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        OTA_setConnectBIT();
        break;
    case SYSTEM_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d",
                MAC2STR(event->event_info.sta_connected.mac),
                event->event_info.sta_connected.aid);
        break;
    case SYSTEM_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",
                MAC2STR(event->event_info.sta_disconnected.mac),
                event->event_info.sta_disconnected.aid);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGE(TAG, "Disconnect reason : %d", info->disconnected.reason);
        if (info->disconnected.reason == WIFI_REASON_BASIC_RATE_NOT_SUPPORT) {
            /*Switch to 802.11 bgn mode */
            esp_wifi_set_protocol(ESP_IF_WIFI_STA, WIFI_PROTOCAL_11B | WIFI_PROTOCAL_11G | WIFI_PROTOCAL_11N);
        }
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());
}

/**smart config callback */
static void sc_callback(smartconfig_status_t status, void *pdata)
{
    switch (status) {
        case SC_STATUS_LINK:
        {
            wifi_config_t *wifi_config = pdata;
            tSmartconfigStatus = WIFI_IN_SC;
            ESP_LOGI(TAG, "SSID:%s", wifi_config->sta.ssid);
            ESP_LOGI(TAG, "PASSWORD:%s", wifi_config->sta.password);
            WIFI_saveWifiInfoToFlash((unsigned char *)wifi_config->sta.ssid, (unsigned char *)wifi_config->sta.password);
            ESP_ERROR_CHECK(esp_wifi_disconnect());
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, wifi_config));
            ESP_ERROR_CHECK(esp_wifi_connect());
        }
        break;
        case SC_STATUS_LINK_OVER:
            ESP_LOGI(TAG, "SC_STATUS_LINK_OVER");
            //这里乐鑫回调目前在master分支已区分是否为微信配网还是esptouch配网，当airkiss配网才近场回调！
            if (pdata != NULL)
            {
                sc_callback_data_t *sc_callback_data = (sc_callback_data_t *)pdata;
                switch (sc_callback_data->type)
                {
                case SC_ACK_TYPE_ESPTOUCH:
                    ESP_LOGI(TAG, "Phone ip: %d.%d.%d.%d", sc_callback_data->ip[0], sc_callback_data->ip[1], sc_callback_data->ip[2], sc_callback_data->ip[3]);
                    ESP_LOGI(TAG, "TYPE: ESPTOUCH");
                    xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
                    break;
                case SC_ACK_TYPE_AIRKISS:
                    ESP_LOGI(TAG, "TYPE: AIRKISS");

                    xEventGroupSetBits(wifi_event_group, AIRKISS_DONE_BIT);
                    break;
                default:
                    ESP_LOGE(TAG, "TYPE: ERROR");
                    xEventGroupSetBits(wifi_event_group, ESPTOUCH_DONE_BIT);
                    break;
                }
            }

            break;
        default:
            break;
    }
}

void smartconfig_example_task(void *parm)
{
    EventBits_t uxBits;
    ESP_ERROR_CHECK(esp_smartconfig_set_type(SC_TYPE_ESPTOUCH_AIRKISS));
    ESP_ERROR_CHECK(esp_smartconfig_start(sc_callback));
    vTaskDelay(1000 / portTICK_RATE_MS);
    tSmartconfigStatus = WIFI_WAIT_SC;
    ESP_LOGI(TAG, "smartconfig start , start find device");
    while (1)
    {
        uxBits = xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT | ESPTOUCH_DONE_BIT | AIRKISS_DONE_BIT, true, false, portMAX_DELAY);
        if (uxBits & CONNECTED_BIT)
        {
            ESP_LOGI(TAG, "WiFi Connected to ap");
        }

        if (uxBits & AIRKISS_DONE_BIT)
        {
            tSmartconfigStatus = WIFI_NOT_SC;
            ESP_LOGI(TAG, "smartconfig over , start find device");
            esp_smartconfig_stop();
            startAirkissUdpTask();
            ESP_LOGI(TAG, "getAirkissVersion %s", airkiss_version());
            vTaskDelete(NULL);
        }

        if (uxBits & ESPTOUCH_DONE_BIT)
        {
            ESP_LOGI(TAG, "smartconfig over , but don't find device by airkiss...");
            esp_smartconfig_stop();
            vTaskDelete(NULL);
        }
    }
}
/* 
static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {
        case SYSTEM_EVENT_STA_START:
            xTaskCreate(smartconfig_example_task, "smartconfig_example_task", 4096, NULL, 3, NULL);
            //esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            ESP_LOGI(TAG, "got ip:%s",\
                    ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));\
                    xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_AP_STACONNECTED:
            ESP_LOGI(TAG, "station:" MACSTR " join, AID=%d",\
                    MAC2STR(event->event_info.sta_connected.mac),\
                    event->event_info.sta_connected.aid);
            break;
        case SYSTEM_EVENT_AP_STADISCONNECTED:
            ESP_LOGI(TAG, "station:" MACSTR "leave, AID=%d",\
                    MAC2STR(event->event_info.sta_disconnected.mac),\
                    event->event_info.sta_disconnected.aid);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGI(TAG,"wifi disconnect");
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, WIFI_CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}
*/
void WIFI_clearWifiInfoinFlash(void)
{
    nvs_handle out_handle;
    if (nvs_open(pNVSTagWifiInfo, NVS_READWRITE, &out_handle) == ESP_OK)
    {
        nvs_erase_all(out_handle);
        nvs_close(out_handle);
    }
}


void WIFI_saveWifiInfoToFlash(uint8_t *ssid, uint8_t *password)
{
    nvs_handle out_handle;
    char data[65];
    
    if (nvs_open(pNVSTagWifiInfo, NVS_READWRITE, &out_handle) != ESP_OK)
    {
        return;
    }

    memset(data, 0x0, sizeof(data));
    strncpy(data, (char *)ssid, strlen((char *)ssid));
    if (nvs_set_str(out_handle, "ssid", data) != ESP_OK)
    {
        printf("--set ssid fail");
    }

    memset(data, 0x0, sizeof(data));
    strncpy(data, (char *)password, strlen((char *)password));
    if (nvs_set_str(out_handle, "password", data) != ESP_OK)
    {
        printf("--set password fail");
    }
    nvs_close(out_handle);
}

void wifiInit(void *pvParameters)
{
    wifi_config_t config;
    wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
    size_t size = 0;

    nvs_handle out_handle;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //ESP_ERROR_CHECK(esp_wifi_start());
 
    //从本地存储读取是否存在ssid和password, 若不存在，进入smartconfig 模式
    if (nvs_open(pNVSTagWifiInfo, NVS_READONLY, &out_handle) == ESP_OK) {
        memset(&config, 0x0, sizeof(config));
        size = sizeof(config.sta.ssid);
        if (nvs_get_str(out_handle, "ssid", (char *)config.sta.ssid, &size) == ESP_OK) {
            if (size > 0) {
                size = sizeof(config.sta.password);
                if (nvs_get_str(out_handle, "password", (char *)config.sta.password, &size) == ESP_OK) {
                    ESP_LOGI(TAG, "-- get ssid: %s", config.sta.ssid);
                    ESP_LOGI(TAG, "-- get password: %s", config.sta.password);
                    bGetSSID = true;
                    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &config));
                    ESP_ERROR_CHECK(esp_wifi_start());
                } else {
                    ESP_LOGI(TAG, "--get password fail");
                }
            }
        }
        nvs_close(out_handle);
    } else {
        ESP_LOGI(TAG, "no wifi info stored");
    }

    if (!bGetSSID) {
        ESP_LOGI(TAG, "--get ssid fail, bGetSSID = false");
        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
        ESP_ERROR_CHECK(esp_wifi_start());
        /*wifi_config_t wifi_config = {
            .sta = {
                .ssid = EXAMPLE_ESP_WIFI_SSID,
                .password = EXAMPLE_ESP_WIFI_PASS
            },
        };
        WIFI_saveWifiInfoToFlash((unsigned char *)EXAMPLE_ESP_WIFI_SSID, (unsigned char *)EXAMPLE_ESP_WIFI_PASS);

        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
        ESP_ERROR_CHECK(esp_wifi_start());*/
    }
/*
    ESP_LOGI(TAG, "wifi_init_sta finished.");
    ESP_LOGI(TAG, "connect to ap SSID:%s password:%s",\
                EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS); */
    vTaskDelete(NULL);
}

esp_err_t erroTest(void)
{
    return ESP_FAIL;
}

bool WIFI_creatWifiInitTask(void)
{
    //ESP_ERROR_CHECK(erroTest());
    ESP_LOGE(TAG, "rst reason : %d \n",esp_reset_reason());
    if (xTaskCreate(wifiInit, "wifiInit", configMINIMAL_STACK_SIZE * 6, NULL,\
         configMAX_PRIORITIES - 13, NULL) != pdPASS) {
        printf("wifi init thread failed.\n");
        return false;
    }
    return true;
}

WIFI_Smartconfig_Status WIFI_tGetSmartConfigStatus(void)
{
    return tSmartconfigStatus;
}
/* 
void app_main()
{

    printf("SDK version:%s\n", esp_get_idf_version());

    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    //xTaskCreate(wifi_init_sta, "wifi_init_sta", 1024 * 10, NULL, 2, NULL);
}
*/