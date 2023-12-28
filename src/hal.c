#include "hardware/spi.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "pico/time.h"

#include "tusb.h"

#include "hal.h"

void hal_delay_us(uint64_t us) { sleep_us(us); }

void hal_led_init(void) { cyw43_arch_init(); }

void hal_led_set(bool state) { cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state ? 1 : 0); }

bool hal_uart_init(void) { tusb_init(); }

bool hal_uart_update(void) { tud_task(); }

bool hal_uart_open(uint8_t port, uint32_t baud) {
    uint8_t is_successful = false;
    if (port < HAL_UART_PORT_COUNT) {
        is_successful = true;
    }
    return is_successful;
}

bool hal_uart_put(uint8_t port, uint8_t data) {
    uint8_t is_successful = false;
    if (port < HAL_UART_PORT_COUNT) {
        tud_cdc_n_write_char(port, data);
        tud_cdc_n_write_flush(port);
        is_successful = true;
    }
    return is_successful;
}

bool hal_uart_get(uint8_t port, uint8_t *data) {
    uint8_t is_successful = false;
    if (port < HAL_UART_PORT_COUNT) {
        if (tud_cdc_n_available(port)) {
            tud_cdc_n_read(port, data, 1);
            is_successful = true;
        }
    }
    return is_successful;
}

bool hal_uart_close(uint8_t port) {
    uint8_t is_successful = false;
    if (port < HAL_UART_PORT_COUNT) {
        is_successful = true;
    }
    return is_successful;
}

#define HAL_SPI_RESET_GPIO (20)

#define HAL_SPI_DEFAULT_SCK_DURATION (1)
static uint8_t hal_spi_sck_duration = HAL_SPI_DEFAULT_SCK_DURATION;

spi_inst_t *hal_spi_inst = spi_default;

void hal_spi_init(void) {
    hal_spi_set_sck_duration(hal_spi_sck_duration);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI);
    sleep_us(20000);

    gpio_init(HAL_SPI_RESET_GPIO);
    gpio_set_dir(HAL_SPI_RESET_GPIO, GPIO_OUT);
    gpio_put(HAL_SPI_RESET_GPIO, 0);
    sleep_us(20000);
    gpio_put(HAL_SPI_RESET_GPIO, 1);

}

uint8_t hal_spi_set_sck_duration(uint8_t duration) {
    if (duration > 4) {
        duration = 4;
        spi_init(hal_spi_inst, 4000);
    } else if (duration == 3) {
        spi_init(hal_spi_inst, 28800);
    } else if (duration == 2) {
        spi_init(hal_spi_inst, 57600);
    } else if (duration == 1) {
        spi_init(hal_spi_inst, 230400);
    } else if (duration == 0) {
        spi_init(hal_spi_inst, 921600);
    }
    return duration;
}

uint8_t hal_spi_get_sck_duration(void) { return hal_spi_sck_duration; }

uint8_t hal_spi_tx_8(uint8_t data) {
    uint8_t rx_data;
    uint8_t tx_data = data;
    spi_write_read_blocking(hal_spi_inst, &tx_data, &rx_data, 1);
    return rx_data;
}

uint8_t hal_spi_tx_16(uint16_t data) {
    uint8_t rx_data;
    uint8_t tx_data = data >> 8;
    spi_write_read_blocking(hal_spi_inst, &tx_data, &rx_data, 1);
    tx_data = data;
    spi_write_read_blocking(hal_spi_inst, &tx_data, &rx_data, 1);
    return rx_data;
}

uint8_t hal_spi_tx_32(uint32_t data) {
    uint8_t rx_data;
    uint8_t tx_data = data >> 24;
    spi_write_read_blocking(hal_spi_inst, &tx_data, &rx_data, 1);
    tx_data = data >> 16;
    spi_write_read_blocking(hal_spi_inst, &tx_data, &rx_data, 1);
    tx_data = data >> 8;
    spi_write_read_blocking(hal_spi_inst, &tx_data, &rx_data, 1);
    tx_data = data;
    spi_write_read_blocking(hal_spi_inst, &tx_data, &rx_data, 1);
    return rx_data;
}

void hal_spi_disable(void) {
    spi_deinit(hal_spi_inst);
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_NULL);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_NULL);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_NULL);
    gpio_set_function(PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_NULL);
    gpio_put(HAL_SPI_RESET_GPIO, 1);
    sleep_us(20000);
    gpio_set_dir(HAL_SPI_RESET_GPIO, GPIO_IN);
}

void hal_spi_reset_pulse(void) {
    gpio_put(HAL_SPI_RESET_GPIO, 1);
    sleep_us(100);
    gpio_put(HAL_SPI_RESET_GPIO, 0);
    sleep_us(20000);
}

