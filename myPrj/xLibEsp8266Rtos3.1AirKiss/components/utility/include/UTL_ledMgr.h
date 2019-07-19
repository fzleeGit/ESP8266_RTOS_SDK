#ifndef UTL_LED_MGR_H
#define UTL_LED_MGR_H


#include <stdint.h>
#include "freertos/FreeRTOS.h"
/********************************************************************************************************************************
static void clrLedPower(void) 	{	CLR_LED_POWER(); }
static void setLedPower(void) 	{	SET_LED_POWER(); }
static void togLedPower(void) 	{	TOG_LED_POWER(); }

static UTL_LEDHANDLE tLedPower = { 0, 0, 0, 0, 0, 0, clrLedPower, setLedPower, togLedPower } ;


#define DO_LED_ALARM(x)		(tLedAlarm.unRequest = (x))

UTL_manageLed(&tLedPower);
*********************************************************************************************************************************/

//User need to modify
#define TICKS_PER_SEC           portTICK_RATE_MS * 1000         //the thread calling time. user need to modify this value according to your systerm.     
#define TLED	                300			/* period (ms) in which led is on/off during a blink signaling */
#define TPAUSE	                1800	    /* time (ms) in which led is off before a code start */      
//end User need to modify 

#define EV_LED_OFF		0
#define EV_LED_ON		1

#define BLINK_NUMBER_REQUEST	0x8000	                                    /* msbit of a short value */
#define EV_LED_BLINK_FREQ(x)	(TICKS_PER_SEC/(2*(x))) 	                /* to obtain x Hz */
#define EV_LED_BLINK_NUMBER(x)	(BLINK_NUMBER_REQUEST | (x))				/* blink quantity  */

//typedef unsigned short uint16_t;
//typedef unsigned char uint8_t;

typedef struct {
	uint16_t unRequest;
	uint16_t unPreRequest;
    uint16_t unPauseTimeCnt;        // used to indicate remaining pause time
    uint16_t unTimeCnt;		        // used to derive led on time and led off time
    uint16_t unBlinkCnt;		    // used to indicate remaining quantity blink
    uint8_t  ucBlinkState;
	//LedState currentState;			// led state
	void (*pLedClr)(void);			// pointer to function used to clear led
	void (*pLedSet)(void);			// pointer to function used to set led
	void (*pLedTog)(void);			// pointer to function used to toggle led
} UTL_LEDHANDLE;

extern void UTL_manageLed(UTL_LEDHANDLE *pLed);

#endif
