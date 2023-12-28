#include "program.h"
#include "command.h"
#include "hal.h"

#define CONFIG_PARAM_HW_VER 2

#define CONFIG_PARAM_BUILD_NUMBER_LOW 0
#define CONFIG_PARAM_BUILD_NUMBER_HIGH 1

#define CONFIG_PARAM_SW_MAJOR 2
#define CONFIG_PARAM_SW_MINOR 4

#define CONFIG_PARAM_VTARGET 50
#define CONFIG_PARAM_VADJUST 25
#define CONFIG_PARAM_OSC_PSCALE 2
#define CONFIG_PARAM_OSC_CMATCH 1

void program_request(uint8_t message_data[], uint16_t *message_size) {
    static uint8_t parameter_controller_init = 0;
    static uint8_t parameter_reset_polarity = 0;
    static uint32_t address = 0;
    static uint16_t start_address = 0;
    static uint16_t poll_address = 0;
    static uint8_t extended_address = 0;
    static bool is_flash_address = true;
    static bool is_big_flash = false;
    static bool is_new_address = true;

    unsigned char tmp, tmp2, ci, cj, cstatus;
    unsigned int i, nbytes;

    if (message_data[0] == CMD_SIGN_ON) {
        message_data[1] = STATUS_CMD_OK;
        message_data[2] = 8;
        message_data[3] = 'A';
        message_data[4] = 'V';
        message_data[5] = 'R';
        message_data[6] = 'I';
        message_data[7] = 'S';
        message_data[8] = 'P';
        message_data[9] = '_';
        message_data[10] = '2';
        *message_size = 11;
    } else if (message_data[0] == CMD_SET_PARAMETER) {
        if (message_data[1] == PARAM_SCK_DURATION) {
            hal_spi_set_sck_duration(message_data[2]);
        } else if (message_data[1] == PARAM_RESET_POLARITY) {
            parameter_reset_polarity = message_data[2];
        } else if (message_data[1] == PARAM_CONTROLLER_INIT) {
            parameter_controller_init = message_data[2];
        }
        message_data[1] = STATUS_CMD_OK;
        *message_size = 2;
    } else if (message_data[0] == CMD_GET_PARAMETER) {
        uint8_t parameter = message_data[1];
        message_data[1] = STATUS_CMD_OK;
        *message_size = 3;
        if (parameter == PARAM_BUILD_NUMBER_LOW) {
            message_data[2] = CONFIG_PARAM_BUILD_NUMBER_LOW;
        } else if (parameter == PARAM_BUILD_NUMBER_HIGH) {
            message_data[2] = CONFIG_PARAM_BUILD_NUMBER_HIGH;
        } else if (parameter == PARAM_HW_VER) {
            message_data[2] = CONFIG_PARAM_HW_VER;
        } else if (parameter == PARAM_SW_MAJOR) {
            message_data[2] = CONFIG_PARAM_SW_MAJOR;
        } else if (parameter == PARAM_SW_MINOR) {
            message_data[2] = CONFIG_PARAM_SW_MINOR;
        } else if (parameter == PARAM_VTARGET) {
            message_data[2] = CONFIG_PARAM_VTARGET;
        } else if (parameter == PARAM_VADJUST) {
            message_data[2] = CONFIG_PARAM_VADJUST;
        } else if (parameter == PARAM_SCK_DURATION) {
            message_data[2] = hal_spi_get_sck_duration();
        } else if (parameter == PARAM_RESET_POLARITY) {
            message_data[2] = parameter_reset_polarity;
        } else if (parameter == PARAM_CONTROLLER_INIT) {
            message_data[2] = parameter_controller_init;
        } else if (parameter == PARAM_OSC_PSCALE) {
            message_data[2] = CONFIG_PARAM_OSC_PSCALE;
        } else if (parameter == PARAM_OSC_CMATCH) {
            message_data[2] = CONFIG_PARAM_OSC_CMATCH;
        } else if (parameter == PARAM_TOPCARD_DETECT) {
            message_data[2] = 0x8c;
        } else if (parameter == PARAM_DATA) {
            message_data[2] = 0;
        } else {
            message_data[1] = STATUS_CMD_UNKNOWN;
            *message_size = 2;
        }
    } else if (message_data[0] == CMD_LOAD_ADDRESS) {
        address = ((uint32_t)message_data[1]) << 24;
        address |= ((uint32_t)message_data[2]) << 16;
        address |= ((uint32_t)message_data[3]) << 8;
        address |= ((uint32_t)message_data[4]);
        if (message_data[1] >= 0x80) {
            is_big_flash = true;
            extended_address = message_data[2];
        } else {
            is_big_flash = false;
            extended_address = 0;
        }
        is_new_address = true;
        message_data[1] = STATUS_CMD_OK;
        *message_size = 2;
    } else if (message_data[0] == CMD_FIRMWARE_UPGRADE) {
        message_data[1] = STATUS_CMD_FAILED;
        *message_size = 2;
    } else if (message_data[0] == CMD_ENTER_PROGMODE_ISP) {
        // The syntax of this command is as follows:
        // 0: Command ID 1 byte, CMD_ENTER_ PROGMODE_ISP
        // 1: timeout 1 byte, Command time-out (in ms)
        // 2: stabDelay 1 byte, Delay (in ms) used for pin stabilization
        // 3: cmdexeDelay 1 byte, Delay (in ms) in connection with the EnterProgMode command execution
        // 4: synchLoops 1 byte, Number of synchronization loops
        // 5: byteDelay 1 byte, Delay (in ms) between each byte in the EnterProgMode command.
        // 6: pollValue 1 byte, Poll value: 0x53 for AVR, 0x69 for AT89xx
        // 7: pollIndex 1 byte, Start address, received byte: 0 = no polling, 3 = AVR, 4 = AT89xx
        // cmd1 1 byte
        // cmd2 1 byte
        // cmd3 1 byte
        // cmd4 1 byte
        hal_led_set(true);
        hal_spi_init();
        hal_delay_us(1000UL * message_data[2]); // stabDelay

        // message_data[0] = CMD_ENTER_PROGMODE_ISP;
        // set default to failed:
        message_data[1] = STATUS_CMD_FAILED;
        *message_size = 2;

        // connect to the target
        i = 0;
        // limit the loops:
        if (message_data[4] > 48) {
            message_data[4] = 48;
        }
        if (message_data[5] < 1) {
            // minimum byteDelay
            message_data[5] = 1;
        }
        while (i < message_data[4]) { // synchLoops
            // watchdog_reset();
            hal_led_set(true);                      // blink during init
            hal_delay_us(1000UL * message_data[3]); // cmdexeDelay
            i++;
            hal_spi_tx_8(message_data[8]);          // cmd1
            hal_delay_us(1000UL * message_data[5]); // byteDelay
            hal_spi_tx_8(message_data[9]);          // cmd2
            hal_delay_us(1000UL * message_data[5]); // byteDelay
            hal_led_set(false);                     // blink during init
            tmp = hal_spi_tx_8(message_data[10]);   // cmd3
            hal_delay_us(1000UL * message_data[5]); // byteDelay
            tmp2 = hal_spi_tx_8(message_data[11]);  // cmd4

            if ((message_data[7] == 3) && (tmp == message_data[6])) {
                message_data[1] = STATUS_CMD_OK;
            }

            if ((message_data[7] != 3) && (tmp2 == message_data[6])) {
                message_data[1] = STATUS_CMD_OK;
            }
            if (message_data[7] == 0) { // pollIndex
                message_data[1] = STATUS_CMD_OK;
            }
            if (message_data[1] == STATUS_CMD_OK) {
                hal_led_set(true);
                i = message_data[4]; // end loop
            } else {
                hal_spi_reset_pulse();
                hal_delay_us(20000);
            }
        }
    } else if (message_data[0] == CMD_LEAVE_PROGMODE_ISP) {
        hal_spi_disable();
        hal_led_set(false);
        message_data[1] = STATUS_CMD_OK;
        *message_size = 2;
    } else if (message_data[0] == CMD_CHIP_ERASE_ISP) {
        hal_spi_tx_8(message_data[3]);
        hal_spi_tx_8(message_data[4]);
        hal_spi_tx_8(message_data[5]);
        hal_spi_tx_8(message_data[6]);
        if (message_data[2] == 0) {
            // pollMethod use delay
            hal_delay_us(1000UL * message_data[1]); // eraseDelay
        } else {
            // pollMethod RDY/BSY cmd
            ci = 150; // timeout
            while ((hal_spi_tx_32(0xF0000000) & 1) && ci) {
                ci--;
            }
        }
        message_data[1] = STATUS_CMD_OK;
        *message_size = 2;
    } else if ((message_data[0] == CMD_PROGRAM_EEPROM_ISP) || (message_data[0] == CMD_PROGRAM_FLASH_ISP)) {
        if (message_data[0] == CMD_PROGRAM_EEPROM_ISP) {
            is_flash_address = false;
        } else {
            is_flash_address = true;
        }

        // message_data[0] CMD_PROGRAM_FLASH_ISP = 0x13
        // message_data[1] NumBytes H
        // message_data[2] NumBytes L
        // message_data[3] mode
        // message_data[4] delay
        // message_data[5] cmd1 (Load Page, Write Program Memory)
        // message_data[6] cmd2 (Write Program Memory Page)
        // message_data[7] cmd3 (Read Program Memory)
        // message_data[8] poll1 (value to poll)
        // message_data[9] poll2
        // message_data[n+10] Data
        poll_address = 0;
        ci = 150;
        // set a minimum timed delay
        if (message_data[4] < 4) {
            message_data[4] = 4;
        }
        // set a max delay
        if (message_data[4] > 32) {
            message_data[4] = 32;
        }
        start_address = address & 0xFFFF;
        nbytes = ((unsigned int)message_data[1]) << 8;
        nbytes |= message_data[2];
        // if (nbytes > 280) {
        //     // corrupted message
        //     *message_size = 2;
        //     message_data[1] = STATUS_CMD_FAILED;
        //     break;
        // }
        // watchdog_reset();
        // store the original mode:
        tmp2 = message_data[3];
        // result code
        cstatus = STATUS_CMD_OK;
        // message_data[3] test Word/Page Mode bit:
        if ((message_data[3] & 1) == 0) {
            // word mode
            for (i = 0; i < nbytes; i++) {
                // The Low/High byte selection bit is
                // bit number 3. Set high byte for uneven bytes
                if (is_flash_address && i & 1) {
                    // high byte
                    hal_spi_tx_8(message_data[5] | (1 << 3));
                } else {
                    // low byte
                    hal_spi_tx_8(message_data[5]);
                }
                hal_spi_tx_16(address & 0xFFFF);
                hal_spi_tx_8(message_data[i + 10]);
                // if the data byte is not same as poll value
                // in that case we can do polling:
                if (message_data[8] != message_data[i + 10]) {
                    poll_address = address & 0xFFFF;
                    // restore the possibly modifed mode:
                    message_data[3] = tmp2;
                } else {
                    // switch the mode to timed delay (waiting), for this word
                    message_data[3] = 0x02;
                }
                //
                // watchdog_reset();
                if (!is_flash_address) {
                    // eeprom writing, eeprom needs more time
                    hal_delay_us(1000UL * 2);
                }
                // check the different polling mode methods
                if (message_data[3] & 0x04) {
                    // data value polling
                    tmp = message_data[8];
                    ci = 150; // timeout
                    while (tmp == message_data[8] && ci) {
                        // The Low/High byte selection bit is
                        // bit number 3. Set high byte for uneven bytes
                        // Read data:
                        if (is_flash_address && i & 1) {
                            hal_spi_tx_8(message_data[7] | (1 << 3));
                        } else {
                            hal_spi_tx_8(message_data[7]);
                        }
                        hal_spi_tx_16(poll_address);
                        tmp = hal_spi_tx_8(0x00);
                        ci--;
                    }
                } else if (message_data[3] & 0x08) {
                    // RDY/BSY polling
                    ci = 150; // timeout
                    while ((hal_spi_tx_32(0xF0000000) & 1) && ci) {
                        ci--;
                    }
                } else {
                    // timed delay (waiting)
                    hal_delay_us(1000UL * message_data[4]);
                }
                if (is_flash_address) {
                    // increment word address only when we have an uneven byte
                    if (i & 1)
                        address++;
                } else {
                    // increment address
                    address++;
                }
                if (ci == 0) {
                    cstatus = STATUS_CMD_TOUT;
                }
            }
        } else {
            // page mode, all modern chips
            for (i = 0; i < nbytes; i++) {
                // watchdog_reset();
                // In commands PROGRAM_FLASH and READ_FLASH "Load Extended Address"
                // command is executed before every operation if we are programming
                // processor with Flash memory bigger than 64k words and 64k words boundary
                // is just crossed or new address was just loaded.
                if (is_big_flash && ((address & 0xFFFF) == 0 || is_new_address)) {
                    // load extended addr byte 0x4d
                    hal_spi_tx_8(0x4d);
                    hal_spi_tx_8(0x00);
                    hal_spi_tx_8(extended_address);
                    hal_spi_tx_8(0x00);
                    is_new_address = 0;
                }
                // The Low/High byte selection bit is
                // bit number 3. Set high byte for uneven bytes
                if (is_flash_address && i & 1) {
                    hal_spi_tx_8(message_data[5] | (1 << 3));
                } else {
                    hal_spi_tx_8(message_data[5]);
                }
                hal_spi_tx_16(address & 0xFFFF);
                hal_spi_tx_8(message_data[i + 10]);

                // that the data byte is not same as poll value
                // in that case we can do polling:
                if (message_data[8] != message_data[i + 10]) {
                    poll_address = address & 0xFFFF;
                } else {
                    // switch the mode to timed delay (waiting)
                    // we must preserve bit 0x80
                    message_data[3] = (message_data[3] & 0x80) | 0x10;
                }
                if (is_flash_address) {
                    // increment word address only when we have an uneven byte
                    if (i & 1) {
                        address++;
                        if ((address & 0xFFFF) == 0xFFFF) {
                            extended_address++;
                        }
                    }
                } else {
                    address++;
                }
            }
            if (message_data[3] & 0x80) {
                hal_spi_tx_8(message_data[6]);
                hal_spi_tx_16(start_address);
                hal_spi_tx_8(0);
                //
                if (!is_flash_address) {
                    // eeprom writing, eeprom needs more time
                    hal_delay_us(1000UL * 1);
                }
                // check the different polling mode methods
                ci = 150; // timeout
                if (message_data[3] & 0x20 && poll_address) {
                    // Data value polling
                    tmp = message_data[8];
                    while (tmp == message_data[8] && ci) {
                        // The Low/High byte selection bit is
                        // bit number 3. Set high byte for uneven bytes
                        // Read data:
                        if (poll_address & 1) {
                            hal_spi_tx_8(message_data[7] | (1 << 3));
                        } else {
                            hal_spi_tx_8(message_data[7]);
                        }
                        hal_spi_tx_16(poll_address);
                        tmp = hal_spi_tx_8(0x00);
                        ci--;
                    }
                    if (ci == 0) {
                        cstatus = STATUS_CMD_TOUT;
                    }
                } else if (message_data[3] & 0x40) {
                    // RDY/BSY polling
                    while ((hal_spi_tx_32(0xF0000000) & 1) && ci) {
                        ci--;
                    }
                    if (ci == 0) {
                        cstatus = STATUS_RDY_BSY_TOUT;
                    }
                } else {
                    // simple waiting
                    hal_delay_us(1000UL * message_data[4]);
                }
            }
        }
        message_data[1] = cstatus;
        *message_size = 2;

    } else if ((message_data[0] == CMD_READ_EEPROM_ISP) || (message_data[0] == CMD_READ_FLASH_ISP)) {
        if (message_data[0] == CMD_READ_EEPROM_ISP) {
            is_flash_address = false;
        } else {
            is_flash_address = true;
        }

        // message_data[1] and message_data[2] NumBytes
        // message_data[3] cmd
        nbytes = ((unsigned int)message_data[1]) << 8;
        nbytes |= message_data[2];
        tmp = message_data[3];
        // limit answer len, prevent overflow:
        if (nbytes > 280) {
            nbytes = 280;
        }
        //
        for (i = 0; i < nbytes; i++) {
            // watchdog_reset();
            // In commands PROGRAM_FLASH and READ_FLASH "Load Extended Address"
            // command is executed before every operation if we are programming
            // processor with Flash memory bigger than 64k words and 64k words boundary
            // is just crossed or new address was just loaded.
            if (is_big_flash && ((address & 0xFFFF) == 0 || is_new_address)) {
                // load extended addr byte 0x4d
                hal_spi_tx_8(0x4d);
                hal_spi_tx_8(0x00);
                hal_spi_tx_8(extended_address);
                hal_spi_tx_8(0x00);
                is_new_address = 0;
            }
            // Select Low or High-Byte
            if (is_flash_address && i & 1) {
                hal_spi_tx_8(tmp | (1 << 3));
            } else {
                hal_spi_tx_8(tmp);
            }

            hal_spi_tx_16(address & 0xFFFF);
            message_data[i + 2] = hal_spi_tx_8(0);

            if (is_flash_address) {
                // increment word address only when we have an uneven byte
                if (i & 1) {
                    address++;
                    if ((address & 0xFFFF) == 0xFFFF) {
                        extended_address++;
                    }
                }
            } else {
                address++;
            }
        }
        *message_size = nbytes + 3;
        // message_data[0] = CMD_READ_FLASH_ISP; or CMD_READ_EEPROM_ISP
        message_data[1] = STATUS_CMD_OK;
        message_data[nbytes + 2] = STATUS_CMD_OK;
    }

    else if ((message_data[0] == CMD_PROGRAM_LOCK_ISP) || (message_data[0] == CMD_PROGRAM_FUSE_ISP)) {
        hal_spi_tx_8(message_data[1]);
        hal_spi_tx_8(message_data[2]);
        hal_spi_tx_8(message_data[3]);
        hal_spi_tx_8(message_data[4]);

        // message_data[0] = CMD_PROGRAM_FUSE_ISP; or CMD_PROGRAM_LOCK_ISP
        message_data[1] = STATUS_CMD_OK;
        message_data[2] = STATUS_CMD_OK;
        *message_size = 3;
    } else if ((message_data[0] == CMD_READ_OSCCAL_ISP) || (message_data[0] == CMD_READ_SIGNATURE_ISP) ||
               (message_data[0] == CMD_READ_LOCK_ISP) || (message_data[0] == CMD_READ_FUSE_ISP)) {
        for (ci = 0; ci < 4; ci++) {
            tmp = hal_spi_tx_8(message_data[ci + 2]);
            if (message_data[1] == (ci + 1)) {
                message_data[2] = tmp;
            }
            hal_delay_us(1000UL * 5);
        }
        *message_size = 4;
        // message_data[0] = CMD_READ_FUSE_ISP; or CMD_READ_LOCK_ISP or ...
        message_data[1] = STATUS_CMD_OK;
        // message_data[2] is the data (fuse byte)
        message_data[3] = STATUS_CMD_OK;
    }

    else if (message_data[0] == CMD_SPI_MULTI) {
        // 0: CMD_SPI_MULTI
        // 1: NumTx
        // 2: NumRx
        // 3: RxStartAddr counting from zero
        // 4+: TxData (len in NumTx)
        // example: 0x1d 0x04 0x04 0x00   0x30 0x00 0x00 0x00
        tmp = message_data[2];
        tmp2 = message_data[3];
        cj = 0;
        ci = 0;
        for (cj = 0; cj < message_data[1]; cj++) {
            hal_delay_us(1000UL * 5);
            if (cj >= tmp2 && ci < tmp) {
                // store answer starting from message_data[2]
                message_data[ci + 2] = hal_spi_tx_8(message_data[cj + 4]);
                ci++;
            } else {
                hal_spi_tx_8(message_data[cj + 4]);
            }
        }
        // padd with zero:
        while (ci < tmp) {
            message_data[ci + 2] = 0;
            ci++;
        }
        *message_size = ci + 3;
        // message_data[0] = CMD_SPI_MULTI
        message_data[1] = STATUS_CMD_OK;
        // message_data[2...ci+1] is the data
        message_data[ci + 2] = STATUS_CMD_OK;
    } else {
        message_data[1] = STATUS_CMD_UNKNOWN;
        *message_size = 2;
    }
}
