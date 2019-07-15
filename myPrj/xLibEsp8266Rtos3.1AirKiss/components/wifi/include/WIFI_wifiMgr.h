#ifndef WIFI_WIFIMGR_H
#define WIFI_WIFIMGR_H

extern bool WIFI_creatWifiInitTask(void);
extern void funtion_wifi_clear_info(void);
extern void funtion_wifi_save_info(uint8_t *ssid, uint8_t *password);
extern void wifi_call_test(void);
#endif /* WIFI_WIFIMGR_H */
