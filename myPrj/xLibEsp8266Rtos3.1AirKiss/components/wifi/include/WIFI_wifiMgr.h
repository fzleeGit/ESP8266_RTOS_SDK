#ifndef WIFI_WIFIMGR_H
#define WIFI_WIFIMGR_H

extern bool WIFI_creatWifiInitTask(void);
extern void WIFI_clearWifiInfoinFlash(void);
extern void WIFI_saveWifiInfoToFlash(uint8_t *ssid, uint8_t *password);

#endif /* WIFI_WIFIMGR_H */
