#include "UTL_ledMgr.h"

typedef enum {
	ST_LED_OFF,
	ST_LED_ON,
	ST_LED_BLINK
} LedState;

#define ST_BLINK_INIT	    0
#define ST_BLINK_NORMAL	    1

void UTL_manageLed(UTL_LEDHANDLE *pLed)
{
    //short unRequest;
	uint16_t unQuantityToShow;
	uint16_t unBlinkPeriod;
	uint16_t unTPause;
	LedState eLedState;

    if (pLed->unPreRequest != pLed->unRequest) {
        pLed->unPreRequest = pLed->unRequest;
        pLed->ucBlinkState = ST_BLINK_INIT;
        pLed->unBlinkCnt = 0;
        pLed->unPauseTimeCnt = 0;
        pLed->unTimeCnt = 0;
    }
	if (pLed->unRequest & BLINK_NUMBER_REQUEST) {
		// led used to show a quantity
		unQuantityToShow = 2 * (pLed->unRequest & ~BLINK_NUMBER_REQUEST);   //ON and OFF as one time, so x2
		unBlinkPeriod = TLED;       // ON/OFF driven period,
		unTPause = TPAUSE;          //blink pause time
		eLedState = ST_LED_BLINK;
	} else {
		// led used fixed (on or off)
		// or blinking at a particular frequency
		unQuantityToShow = 2;
		unTPause = 0;               //blink at a particular frequency, so pause time equal to 0
		unBlinkPeriod = pLed->unRequest & ~BLINK_NUMBER_REQUEST;
		if (unBlinkPeriod == 0) {
		    eLedState = ST_LED_OFF;
		} else if (unBlinkPeriod == 1) {
            eLedState = ST_LED_ON;
		} else {
		    eLedState = ST_LED_BLINK;
		}
	}

    switch (eLedState) {
        case ST_LED_ON:
            (*pLed->pLedSet)();
            break;
        case ST_LED_OFF:
            (*pLed->pLedClr)();
            break;
        case ST_LED_BLINK:
            switch (pLed->ucBlinkState) {
                case ST_BLINK_INIT:
                    (*pLed->pLedSet)();                     //blink start from led ON
                    pLed->ucBlinkState = ST_BLINK_NORMAL;
                    pLed->unBlinkCnt++;                     //if not add one, it will toggle one more in first time
                case ST_BLINK_NORMAL:
                    if (pLed->unTimeCnt < unBlinkPeriod) {  //in pause time, unTimeCnt also counted. As pause time come, Led ON at once.
                       pLed->unTimeCnt++;
                    }

                    if ((pLed->unTimeCnt >= unBlinkPeriod) && (pLed->unPauseTimeCnt == 0)) {
                        (*pLed->pLedTog)();
                        if (++pLed->unBlinkCnt >= unQuantityToShow) {
                            pLed->unPauseTimeCnt = unTPause;
                            pLed->unBlinkCnt = 0;
                        }
                        pLed->unTimeCnt = 0;
                    } else if (pLed->unPauseTimeCnt > 0) {
                        pLed->unPauseTimeCnt--;
                        if (pLed->unTimeCnt >= unBlinkPeriod) { //Reset Led after the last tick. only in quantity blink mode.
                            (*pLed->pLedClr)();
                        }
                    }
                    break;

                default:
                    break;
            }
            break;

        default:
            break;
    }

}
