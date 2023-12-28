#ifndef PROGRAM_H
#define PROGRAM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

void program_request(uint8_t message_data[], uint16_t *message_size);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // PROGRAM_H