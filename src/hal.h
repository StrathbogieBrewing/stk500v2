#ifndef HAL_H
#define HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "tusb_config.h"

#define HAL_UART_PORT_COUNT CFG_TUD_CDC

bool hal_uart_init(void);
bool hal_uart_update(void);
bool hal_uart_open(uint8_t port, uint32_t baud);
bool hal_uart_put(uint8_t port, uint8_t data);
bool hal_uart_get(uint8_t port, uint8_t *data);
bool hal_uart_close(uint8_t port);

void hal_led_init(void);
void hal_led_set(bool state);

void hal_spi_init(void);
uint8_t hal_spi_set_sck_duration(uint8_t dur);
uint8_t hal_spi_get_sck_duration(void);
uint8_t hal_spi_tx_8(uint8_t data);
uint8_t hal_spi_tx_16(uint16_t data);
uint8_t hal_spi_tx_32(uint32_t data);
void hal_spi_disable(void);
void hal_spi_reset_pulse(void);

void hal_delay_us(uint64_t us);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* HAL_H */
