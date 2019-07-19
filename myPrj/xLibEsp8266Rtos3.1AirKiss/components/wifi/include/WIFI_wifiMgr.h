#ifndef WIFI_WIFIMGR_H
#define WIFI_WIFIMGR_H

typedef enum {
    WIFI_NOT_SC = 0,     /*!< Not in smartconfig */
    WIFI_WAIT_SC = 1,     /*!< in smartconfig, waitting APP config */
    WIFI_IN_SC = 2,     /*!< in smartconfig, receiving package from wifi */
    /** @endcond */
} WIFI_Smartconfig_Status;

extern bool WIFI_creatWifiInitTask(void);
extern void WIFI_clearWifiInfoinFlash(void);
extern void WIFI_saveWifiInfoToFlash(uint8_t *ssid, uint8_t *password);
extern WIFI_Smartconfig_Status WIFI_tGetSmartConfigStatus(void);

#endif /* WIFI_WIFIMGR_H */
