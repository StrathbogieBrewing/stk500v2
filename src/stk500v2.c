#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "command.h"
#include "hal.h"
#include "stk500v2.h"
#include "program.h"

#define STK500V2_MAX_MESSAGE_SIZE (275)
#define STK500V2_PORT (0)

void stk500v2_parse_request(uint8_t rx_byte);

void stk500v2_init(void) {
    hal_led_init();
    hal_uart_init();
}

// echo to either Serial0 or Serial1
// with Serial0 as all lower case, Serial1 as all upper case
static void echo_serial_port(uint8_t port, uint8_t data) {
    if (port == 0) {
        // echo back 1st port as lower case
        if (isupper(data))
            data += 'a' - 'A';
    } else {
        // echo back 2nd port as upper case
        if (islower(data))
            data -= 'a' - 'A';
    }
    hal_uart_put(port, data);
}

void stk500v2_update(void) {
    static bool hal_led_state = false;
    hal_uart_update();

    for (uint8_t port = 0; port < HAL_UART_PORT_COUNT; port++) {
        uint8_t data;
        if (hal_uart_get(port, &data) == true) {
            // echo back to both serial ports
            echo_serial_port(0, data);
            echo_serial_port(1, data);

            stk500v2_parse_request(data);

            hal_led_set(hal_led_state);
            hal_led_state = !hal_led_state;
        }
    }
}

void stk500v2_send_response(uint8_t sequence_number, uint8_t message_data[], uint16_t message_size) {
    uint8_t checksum = 0;
    uint16_t message_index = 0;

    hal_uart_put(STK500V2_PORT, MESSAGE_START);
    checksum = MESSAGE_START ^ 0;
    hal_uart_put(STK500V2_PORT, sequence_number);
    checksum ^= sequence_number;
    uint8_t size_msb = (uint8_t)(message_size >> 8);
    hal_uart_put(STK500V2_PORT, size_msb);
    checksum ^= size_msb;
    uint8_t size_lsb = (uint8_t)message_size;
    hal_uart_put(STK500V2_PORT, size_lsb);
    checksum ^= size_lsb;
    hal_uart_put(STK500V2_PORT, TOKEN);
    checksum ^= TOKEN;
    for (message_index = 0; message_index < message_size; message_index++) {
        hal_uart_put(STK500V2_PORT, message_data[message_index]);
        checksum ^= message_data[message_index];
    }
    hal_uart_put(STK500V2_PORT, checksum);
}

void stk500v2_parse_request(uint8_t rx_byte) {

    typedef enum parser_state_t {
        PARSER_START = 0,
        PARSER_SEQUENCE_NUMBER,
        PARSER_SIZE_MSB,
        PARSER_SIZE_LSB,
        PARSER_TOKEN,
        PARSER_DATA,
        PARSER_CHECKSUM,
    } parser_state_t;

    static uint8_t checksum = 0;
    static uint8_t sequence_number = 0;
    static uint8_t message_data[STK500V2_MAX_MESSAGE_SIZE];
    static uint16_t message_size = 0;
    static uint16_t message_index = 0;
    static parser_state_t parser_state = PARSER_START;

    // parse as per avr068 table 3.1
    if (parser_state == PARSER_START) {
        if (rx_byte == MESSAGE_START) {
            parser_state = PARSER_SEQUENCE_NUMBER;
            checksum = rx_byte ^ 0;
        }
    } else if (parser_state == PARSER_SEQUENCE_NUMBER) {
        sequence_number = rx_byte;
        checksum ^= rx_byte;
        parser_state = PARSER_SIZE_MSB;
    } else if (parser_state == PARSER_SIZE_MSB) {
        checksum ^= rx_byte;
        message_size = rx_byte << 8;
        parser_state = PARSER_SIZE_LSB;
    } else if (parser_state == PARSER_SIZE_LSB) {
        checksum ^= rx_byte;
        message_size |= rx_byte;
        parser_state = PARSER_TOKEN;
    } else if (parser_state == PARSER_TOKEN) {
        checksum ^= rx_byte;
        if (rx_byte == TOKEN) {
            parser_state = PARSER_DATA;
            message_index = 0;
        } else {
            parser_state = PARSER_START;
        }
    } else if (parser_state == PARSER_DATA) {
        if ((message_index < message_size) && (message_index < STK500V2_MAX_MESSAGE_SIZE)) {
            checksum ^= rx_byte;
            message_data[message_index] = rx_byte;
            message_index++;
            if (message_index == message_size) {
                parser_state = PARSER_CHECKSUM;
            }
        }
    } else if (parser_state == PARSER_CHECKSUM) {
        if (rx_byte == checksum && message_size > 0) {
            program_request(message_data, &message_size);
            stk500v2_send_response(sequence_number, message_data, message_size);
        } else {
            message_data[0] = ANSWER_CKSUM_ERROR;
            message_data[1] = STATUS_CKSUM_ERROR;
            stk500v2_send_response(sequence_number, message_data, 2);
        }
        parser_state = PARSER_START;
        message_size = 0;
        message_index = 0;
        sequence_number = 0;
    }
}

