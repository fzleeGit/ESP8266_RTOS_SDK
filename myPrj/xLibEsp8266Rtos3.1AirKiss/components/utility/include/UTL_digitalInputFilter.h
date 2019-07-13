#ifndef UTL_DIGITAL_INPUT_FILTER_H
#define UTL_DIGITAL_INPUT_FILTER_H

#include <stdbool.h>
//#include <stdint.h>

typedef struct {
    bool bPinState;
    uint16_t unFilterTimeCnt;
    bool (*pPinState)(void);			// pointer to function used to check pin state
} UTL_DigitalInputFilter_t;

bool UTL_bDigitalInputFilter(UTL_DigitalInputFilter_t *pDIPin, uint16_t unFilterTime);
//void UTL_resetPinGlitch(UTL_DigitalInputFilter_t *pDIPin);

#endif
