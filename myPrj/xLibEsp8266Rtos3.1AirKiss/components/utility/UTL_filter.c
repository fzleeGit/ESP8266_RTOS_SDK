#include <stdint.h>
#include "UTL_filter.h"

//fisrt order IIR lowpass filter 
uint16_t UTL_unFirstOrderFilter(uint16_t *unLastData, uint16_t unNewInput, uint8_t unFactor)
{
    uint16_t unTempData;
    
    //unFactor = n  y = n-1/n old + 1/n new
    unTempData = *unLastData - (*unLastData >> unFactor) + unNewInput;
    *unLastData = unTempData;
    return (unTempData >> unFactor);
}
