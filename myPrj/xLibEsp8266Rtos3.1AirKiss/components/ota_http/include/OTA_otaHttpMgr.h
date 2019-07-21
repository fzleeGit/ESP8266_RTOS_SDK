#ifndef OTA_OTAHTTPMGR_H
#define OTA_OTAHTTPMGR_H

//extern void LED_ledMgrInit(void);
//extern void LED_setLedStatus(bool bOnOff);
//extern const char *pNVSTagOta;
extern void OTA_otaInit(void);
extern void OTA_setOtaEnableFlag(void);
extern void OTA_clearOtaEnableFlag(void);
extern int OTA_getOtaEnableFlag(void);
extern void OTA_setConnectBIT(void);

#endif /* OTA_OTAHTTPMGR_H */
