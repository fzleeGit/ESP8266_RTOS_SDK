#ifndef UTL_FILTER_H
#define UTL_FILTER_H

#include <stdbool.h>
///fisrt order IIR lowpass filter 
uint16_t UTL_unFirstOrderFilter(uint16_t *unLastData, uint16_t unNewInput, uint8_t unFactor);

#endif
