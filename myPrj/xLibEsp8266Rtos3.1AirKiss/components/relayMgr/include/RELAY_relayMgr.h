#ifndef RELAY_RELAYMGR_H
#define RELAY_RELAYMGR_H

extern void RELAY_relayMgrInit(void);
extern void RELAY_setRelay(void);
extern void RELAY_resetRelay(void);
extern bool RELAY_getRelayState(void);
extern void RELAY_relayCtlMgr(void);

#endif /* RELAY_RELAYMGR_H */
