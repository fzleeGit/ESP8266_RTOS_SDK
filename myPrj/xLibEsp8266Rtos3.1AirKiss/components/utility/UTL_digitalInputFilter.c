#include <stdint.h>
#include "UTL_digitalInputFilter.h"

/*
usage note:
1,bool bGetInputState(void)        {return (IN_BTN_PRESSED() == true);}          //return true -> in button active
2,define a  UTL_DigitalInputFilter_t tInButton = {0, 0, bGetInputState};
3,bDigitalInputFilter(&tInButton, 100);
4,notice the filter time, it depend on the calling frequency of the bDigitalInputFilter() function
*/
bool UTL_bDigitalInputFilter(UTL_DigitalInputFilter_t *pDIPin, uint16_t unFilterTime)
{
	//both activate and inactivate need to be filtered. so "if (--pDIPin->unFilterTimeCnt == 0)" is needed.
	if (pDIPin->pPinState()) {
		if (pDIPin->unFilterTimeCnt < unFilterTime) {
			if (++pDIPin->unFilterTimeCnt == unFilterTime) {
				if (pDIPin->bPinState == false) {
					pDIPin->bPinState = true;
				}
			}
		}
	} else if (pDIPin->unFilterTimeCnt != 0) {	
		if (--pDIPin->unFilterTimeCnt == 0) {
			if (pDIPin->bPinState) {
				pDIPin->bPinState = false;
			}
		}
	}
	return (pDIPin->bPinState);
}

