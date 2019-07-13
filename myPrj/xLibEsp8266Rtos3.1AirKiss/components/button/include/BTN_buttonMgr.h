#ifndef BTN_BUTTONMGR_H
#define BTN_BUTTONMGR_H

//#define PLUG_USR_KEY_IO_MUX		PERIPHS_IO_MUX_MTDO_U
//#define PLUG_USR_KEY_IO_FUNC	FUNC_GPIO15
//#define PLUG_USR_KEY_IO_PIN  	GPIO_Pin_15
typedef union {
	uint16_t unPackedValue;
	struct {
        bool bIsBTNPressed                  :1;
        bool bIsBTNLongPressed              :1;
        bool bIsBTNGlitchHigh               :1;
        bool bIsBTNGlitchLow                :1;
        uint16_t unLongPressedCnt           :12;
	} BF;
} BTN_ButtonStatus;

extern bool BTN_buttonIOInit(void);

#endif /* BTN_BUTTONMGR_H */
