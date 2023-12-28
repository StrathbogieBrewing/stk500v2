#ifndef STK500V2_H
#define STK500V2_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

void stk500v2_init(void);
void stk500v2_update(void);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // STK500V2_H