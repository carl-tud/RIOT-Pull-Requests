/*
 * Copyright (C) 2016 TriaGnoSys GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "kernel_defines.h"
#include "ztimer.h"
#include "mutex.h"
#include "pn53x.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "periph/uart.h"
#include "msg.h"
#include "byteorder.h"
#include "architecture.h"

#include "log.h"

#include "net/nfc/nfc_error.h"

#include "pn53x.h"

#define ENABLE_DEBUG CONFIG_PN53_DEBUG
#include "debug.h"


static inline void __debug_hex(const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i += 1) {
        printf("%02X ", buffer[i]);
    }
}

#define PN53_DEBUG_HEX(bytes, size) \
    do {                                \
      if (ENABLE_DEBUG) {             \
          __debug_hex(bytes, size);   \
      }                               \
    } while (0)

#define PN53_HCI_DEBUG(...) DEBUG("pn53x.hci: " __VA_ARGS__)
#define PN53_DEBUG(...) DEBUG("pn53x: " __VA_ARGS__)



#define PN53_BUS_I2C_ADDRESS           (0x24)

/* SPI bus parameters */
#define SPI_MODE                    (SPI_MODE_0)
#define SPI_CLK                     (SPI_CLK_1MHZ)

#define UART_BAUDRATE               (115200U)


ssize_t pn53_hci_transceive_command(pn53_connection_t* connection, iolist_t* command,
                            uint8_t** response, uint32_t timeout_ms) {
    assert(command);
    uint8_t code = *(uint8_t*)command->iol_base;
    ssize_t res = 0;
    if ((res = pn53_hci_transceive(connection, command, response, timeout_ms)) < 0) {
        return res;
    }

    if (res == 0) {
        return -PN53_ERROR_CONNECTION_RESPONSE_MISSING;
    }

    if (response && *response && **response != (code + 1)) {
        return -PN53_ERROR_CONNECTION_RESPONSE_MISMATCH;
    }
    *response += 1;
    return res - 1;
}

int pn53_check_communication(pn53_dev_t* dev) {
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_DIAGNOSE,
        0x00, /* Echo test */
        0x42, 'R', 'I', 'O', 'T', 0x42,
    };
    uint8_t* response;
    ssize_t res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command),
                                &response, CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS);
    if (res < 0) {
        return (int)res;
    }

    PN53_DEBUG("echo: ");
    PN53_DEBUG_HEX(response, (size_t)res);
    DEBUG("\n");

    if (res != sizeof(command) - 1) {
        PN53_DEBUG("comms check failed (echo)\n");
        return -1;
    }
    if (memcmp(&command[1], response, sizeof(command) - 1) != 0) {
        PN53_DEBUG("comms check failed (echo)\n");
        return -1;
    }

    PN53_DEBUG("comms check successful (echo)\n");
    return 0;
}

static pn53_model_t _model(pn53_firmware_version_t* fw) {
    switch (fw->ic) {
        case 0x31: return PN53_MODEL_PN531;
        case 0x32: return PN53_MODEL_PN532;
        case 0x33: return (fw->version == 1) ? PN53_MODEL_RCS956 : PN53_MODEL_PN533;
        default: return 0;
    }
}

int pn53_get_firmware_version(pn53_dev_t* dev, pn53_firmware_version_t** version) {
    uint8_t command = (uint8_t)PN53_COMMAND_GET_FIRMWARE_VERSION;
    int res = 0;

    pn53_firmware_version_t* fw;
    if ((res = pn53_hci_transceive_command2(&dev->connection, &command, sizeof(command), (uint8_t**)&fw, CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS)) < 0) {
        PN53_DEBUG("unable to get firmware version\n");
        return res;
    }

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        PN53_DEBUG("connected to <pn53x ic=0x%02X version=%i revision=%i nfc={a=%i b=%i dep=%i}>\n",
                   fw->ic, (int)fw->version, (int)fw->revision,
                   PN53_FIRMWARE_SUPPORTS_NFC_A(fw->support),
                   PN53_FIRMWARE_SUPPORTS_NFC_B(fw->support),
                   PN53_FIRMWARE_SUPPORTS_NFC_DEP(fw->support)
                  );
    }

    dev->model = _model(fw);

    if (version) {
        *version = fw;
    }
    return 0;
}

int pn53_init(pn53_dev_t* dev, const pn53_connection_config_t* config) {
    assert(dev);
    assert(config);
    int res = 0;

    dev->connection.config = config;
    dev->command_timeout = CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS;
    if ((res = pn53_hci_init(&dev->connection)) < 0) {
        return res;
    }
    PN53_DEBUG("HCI initialized\n");

    pn53_hci_reset(&dev->connection);
    PN53_DEBUG("reset done\n");

    /* Start with conservative minimum before knowing model */
    dev->connection.max_packet_length = 200;
    if (IS_ACTIVE(CONFIG_PN53_SELFCHECK)) {
        if ((res = pn53_check_communication(dev)) < 0) {
            PN53_DEBUG("Diagnose(0x06): failed wih %i\n", res);
            return res;
        }
    }

    if ((res = pn53_get_firmware_version(dev, NULL)) < 0) {
        return res;
    }
    return 0;
}

//int pn532_init(nfcdev_t *nfcdev, const void *dev_config) {
//    pn532_t *dev = (pn532_t *)nfcdev->dev;
//    const pn532_config_t *config = (const pn532_config_t *)dev_config;
//    nfcdev->ops = &pn532_ops;
//    int ret = _pn532_init(dev, &config->params, config->mode);
//    if (ret != 0) {
//        return -1;
//    }
//    nfcdev->state = NFCDEV_STATE_DISABLED;
//    return 0;
//}

#define __IOLIST(buffer, size, next) ((iolist_t) { \
    .iol_base = (void*)buffer, \
    .iol_len = (size_t)size, \
    .iol_next = next, \
})

int pn53_set_parameters(pn53_dev_t* dev, uint8_t parameters) {
    assert(dev);

    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_SET_PARAMETERS,
        parameters
    };

    ssize_t res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), NULL, CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS);
    if (res < 0) {
        return (int)res;
    }
    dev->nfc_parameters = parameters;
    return 0;
}

ssize_t pn53_read_registers(pn53_dev_t *dev, pn53_register_address_t* registers, uint8_t** values, size_t count) {
    int res = -1;
    uint8_t code = (uint8_t)PN53_COMMAND_READ_REGISTERS;
    iolist_t addrs = __IOLIST(registers, count * sizeof(pn53_register_address_t), NULL);
    iolist_t command = __IOLIST(&code, 1, &addrs);
    if ((res = pn53_hci_transceive_command(&dev->connection, &command, values, dev->command_timeout)) < 0) {
        return res;
    }

    if (dev->model == PN53_MODEL_PN533) {
        if (res == 0) {
            PN53_DEBUG("ReadRegisters: missing status code\n");
            return -EIO;
        }
        pn53_status_code_t code = pn53_status_code((*values)[0]);
        if (code != 0) {
            return -PN53_ERRNO_FROM_STATUS_CODE(code);
        }
        // Skip over status code
        *values += 1;
        res -= 1;
    }

    if ((size_t)res != count) {
        PN53_DEBUG("ReadRegisters: expected %" PRIuSIZE " values, got %" PRIuSIZE "\n", count, (size_t)res);
        return -EIO;
    }
    return res;
}

int pn53_write_registers(pn53_dev_t *dev, pn53_register_t* registers, size_t count) {
    int res = -1;
    uint8_t code = (uint8_t)PN53_COMMAND_WRITE_REGISTERS;
    iolist_t regs = __IOLIST(registers, count * sizeof(pn53_register_t), NULL);
    iolist_t command = __IOLIST(&code, 1, &regs);
    uint8_t* response;
    if ((res = pn53_hci_transceive_command(&dev->connection, &command, &response, dev->command_timeout)) < 0) {
        return res;
    }
    if (dev->model == PN53_MODEL_PN533) {
        if (res == 0) {
            PN53_DEBUG("WriteRegisters: missing status code\n");
            return -EIO;
        }
        pn53_status_code_t code = pn53_status_code(response[0]);
        if (code != 0) {
            return -PN53_ERRNO_FROM_STATUS_CODE(code);
        }
    }
    return 0;
}

int pn53_register_symbol_set(pn53_register_symbols_t* symbols, pn53_register_symbol_t symbol, uint8_t value) {
    assert(symbols);
    assert(PN53_REGISTER_SYMBOL_MASK(symbol) != 0);
    if (PN53_REGISTER_SYMBOL_REGISTER(symbol) < PN53_SYMBOLS_START_REGISTER) {
        return -EINVAL;
    }
    size_t ix = PN53_REGISTER_SYMBOL_REGISTER(symbol) - PN53_SYMBOLS_START_REGISTER;
    if (ix >= PN53_REGISTER_SYMBOL_REGISTER_CAPACITY) {
        return -ENOBUFS;
    }

    uint8_t mask = PN53_REGISTER_SYMBOL_MASK(symbol);
    // Clear value where mask applies...
    symbols->register_values[ix] &= ~mask;

    size_t bit_offset = __builtin_ctz(mask);
    symbols->register_values[ix] |= (value << bit_offset) & mask;

    // Remember what bits we want to set...
    symbols->change_masks[ix] |= mask;

    return 0;
}

int16_t pn53_register_symbol_get(pn53_register_symbols_t* symbols, pn53_register_symbol_t symbol) {
    assert(symbols);
    assert(PN53_REGISTER_SYMBOL_MASK(symbol) != 0);
    if (PN53_REGISTER_SYMBOL_REGISTER(symbol) < PN53_SYMBOLS_START_REGISTER) {
        return -EINVAL;
    }
    size_t ix = PN53_REGISTER_SYMBOL_REGISTER(symbol) - PN53_SYMBOLS_START_REGISTER;
    if (ix >= PN53_REGISTER_SYMBOL_REGISTER_CAPACITY) {
        return -ENOBUFS;
    }

    uint8_t mask = PN53_REGISTER_SYMBOL_MASK(symbol);

    // Let's see if we have that value...
    if ((symbols->change_masks[ix] & mask) != mask) {
        return -EBUSY;
    }

    size_t bit_offset = __builtin_ctz(mask);
    return (symbols->register_values[ix] & mask) >> bit_offset;
}

int pn53_register_symbols_write(pn53_dev_t* dev, pn53_register_symbols_t* symbols) {
    static_assert(ARRAY_SIZE(symbols->change_masks) == ARRAY_SIZE(symbols->register_values),
                  "pn53_register_symbols_t has mismatching internal buffer sizes, please file "
                  "a bug report.");

    pn53_register_address_t addrs[sizeof(symbols->register_values)];
    size_t register_ix = 0;

    for (pn53_register_address_t i = 0; i < sizeof(symbols->change_masks); i += 1) {
        if (symbols->change_masks[i] != 0) {
            uint16_t addr = PN53_SYMBOLS_START_REGISTER + i;
            addrs[register_ix] = htole16(addr);
            register_ix += 1;
        }
    }

    size_t changed_registers = register_ix;

    uint8_t* values;
    ssize_t res = 0;
    if ((res = pn53_read_registers(dev, addrs, &values, changed_registers)) < 0) {
        return (int)res;
    }

    register_ix = 0;
    pn53_register_t regs[sizeof(symbols->register_values)];

    for (pn53_register_address_t i = 0; i < sizeof(symbols->change_masks); i += 1) {
        if (symbols->change_masks[i] != 0) {
            uint16_t addr = PN53_SYMBOLS_START_REGISTER + i;

            regs[register_ix] = (pn53_register_t) {
                .address = htole16(addr),
                .value = (values[register_ix] & ~symbols->change_masks[i]) | (symbols->register_values[i] & symbols->change_masks[i])
            };
            register_ix += 1;
        }
    }

    if ((res = pn53_write_registers(dev, regs, changed_registers)) < 0) {
        return (int)res;
    }
    return 0;
}

ssize_t pn53_read_gpio(pn53_dev_t* dev, pn53_read_gpio_payload_t** response) {
    uint8_t command = (uint8_t)PN53_COMMAND_READ_GPIO;
    return pn53_hci_transceive_command2(&dev->connection, &command, sizeof(command), (uint8_t**)response, dev->command_timeout);
}

int pn53_write_gpio(pn53_dev_t* dev, pn53_write_gpio_payload_t payload) {
    uint8_t command[] = { (uint8_t)PN53_COMMAND_WRITE_GPIO, payload.p3.raw, payload.p7.raw };
    return (int)pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), NULL, dev->command_timeout);
}

int pn53_get_general_status(pn53_dev_t* dev, pn53_general_status_t* status) {
    uint8_t command = (uint8_t)PN53_COMMAND_GET_GENERAL_STATUS;
    uint8_t* response;
    ssize_t res = 0;
    if ((res = pn53_hci_transceive_command2(&dev->connection, &command, sizeof(command), &response, dev->command_timeout)) < 0) {
        return (int)res;
    }

    if (res < 4) {
        PN53_DEBUG("GetGeneralStatus: expected at least 4, got %" PRIuSIZE " bytes\n", (size_t)res);
        return -EBADMSG;
    }
    size_t expected = 4 /* Err, Field, NbTg, SAM */ + response[2] /* NbTg */ * 4 /* Tg, BrRx, BrTx, Type */;
    if ((size_t)res < expected) {
        PN53_DEBUG("GetGeneralStatus: expected at least %" PRIuSIZE ", got %" PRIuSIZE " bytes\n", expected, (size_t)res);
        return -EBADMSG;
    }

    if (status) {
        assert(response);
        status->last_command_status = *response++;
        status->field_detected = *response++;
        status->target_count = *response++;

        for (uint8_t i = 0; i < status->target_count; i += 1) {
            /* logical = */ response++;
            status->targets[i].rx.bitrate = (nfc_bitrate_t)(1 << (*response++));
            status->targets[i].tx.bitrate = (nfc_bitrate_t)(1 << (*response++));

            status->targets[i].mode = NFC_COMMUNICATION_MODE_PASSIVE;
            status->targets[i].nfc_a_gemstone = false;

            uint8_t type = *response++;
            switch (type) {
                case 0x00:
                    status->targets[i].rx.technology = NFC_TECHNOLOGY_A | NFC_TECHNOLOGY_B;
                    status->targets[i].tx.technology = NFC_TECHNOLOGY_A | NFC_TECHNOLOGY_B;
                    break;
                case 0x10:
                    status->targets[i].tx.technology = NFC_TECHNOLOGY_F;
                    status->targets[i].rx.technology = NFC_TECHNOLOGY_F;
                    break;
                case 0x01:
                    status->targets[i].mode = NFC_COMMUNICATION_MODE_ACTIVE;
                    status->targets[i].rx.technology =
                        (status->targets[i].rx.bitrate == NFC_BITRATE_106K) ? NFC_TECHNOLOGY_A : NFC_TECHNOLOGY_F;
                    status->targets[i].tx.technology =
                        (status->targets[i].tx.bitrate == NFC_BITRATE_106K) ? NFC_TECHNOLOGY_A : NFC_TECHNOLOGY_F;
                    break;
                case 0x02:
                    status->targets[i].rx.technology = NFC_TECHNOLOGY_A;
                    status->targets[i].tx.technology = NFC_TECHNOLOGY_A;
                    status->targets[i].nfc_a_gemstone = true;
                    break;
                default:
                    PN53_DEBUG("GetGeneralStatus: illegal target type 0x%02x\n", type);
                    return -EBADMSG;
            }
        }
    }
    status->sam_status = (pn53_sam_status_t)*response;
    return 0;
}

static inline size_t _rf_configuration_payload_length(const pn53_rf_configuration_payload_t* rf_config) {
    switch (rf_config->item) {
        case PN53_RF_CONFIGURATION_ITEM_RF_FIELD: return sizeof(rf_config->rf_field);
        case PN53_RF_CONFIGURATION_ITEM_MAXIMUM_RETRIES_FRAME: return sizeof(rf_config->max_retries_frame);
        case PN53_RF_CONFIGURATION_ITEM_MAXIMUM_RETRIES: return sizeof(rf_config->max_retries_higher_layer);
        case PN53_RF_CONFIGURATION_ITEM_VARIOUS_TIMINGS: return sizeof(rf_config->various_timings);
        case PN53_RF_CONFIGURATION_ITEM_ANALOG_NFC_A: return sizeof(rf_config->registers_when_nfc_a);
        case PN53_RF_CONFIGURATION_ITEM_ANALOG_NFC_B: return sizeof(rf_config->registers_when_nfc_b);
        case PN53_RF_CONFIGURATION_ITEM_ANALOG_NFC_F: return sizeof(rf_config->registers_when_nfc_f);
        case PN53_RF_CONFIGURATION_ITEM_ANALOG_FAST_ISO_DEP: return sizeof(rf_config->registers_when_fast_iso_dep);
        default:
            assert(false);
            return 0;
    }
}

ssize_t pn53_rf_configuration(pn53_dev_t* dev, const pn53_rf_configuration_payload_t* rf_config) {
    uint8_t code = PN53_COMMAND_RF_CONFIGURATION;
    iolist_t config = __IOLIST((const uint8_t*)rf_config, 1 + _rf_configuration_payload_length(rf_config), NULL);
    iolist_t command = __IOLIST(&code, 1, &config);
    return pn53_hci_transceive_command(&dev->connection, &command, NULL, dev->command_timeout);
}

//ssize_t pn53_in_jump_for_dep(pn53_dev_t* dev, uint8_t act_pass, uint8_t baud_rate, uint8_t next, const uint8_t* payload, size_t payload_length, uint8_t** response) {
//    uint8_t header[] = { (uint8_t)PN53_COMMAND_IN_JUMP_FOR_DEP, act_pass, baud_rate, next };
//    iolist_t payload_node = { .iol_base = (void*)payload, .iol_len = payload_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = header, .iol_len = sizeof(header), .iol_next = (payload_length > 0) ? &payload_node : NULL };
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*response)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}
//
//ssize_t pn53_in_jump_for_psl(pn53_dev_t* dev, uint8_t act_pass, uint8_t baud_rate, uint8_t next, const uint8_t* payload, size_t payload_length, uint8_t** response) {
//    uint8_t header[] = { (uint8_t)PN53_COMMAND_IN_JUMP_FOR_PSL, act_pass, baud_rate, next };
//    iolist_t payload_node = { .iol_base = (void*)payload, .iol_len = payload_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = header, .iol_len = sizeof(header), .iol_next = (payload_length > 0) ? &payload_node : NULL };
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*response)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}
//
//ssize_t pn53_in_atr(pn53_dev_t* dev, uint8_t target_number, uint8_t next, const uint8_t* payload, size_t payload_length, uint8_t** response) {
//    uint8_t header[] = { (uint8_t)PN53_COMMAND_IN_ATR, target_number, next };
//    iolist_t payload_node = { .iol_base = (void*)payload, .iol_len = payload_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = header, .iol_len = sizeof(header), .iol_next = (payload_length > 0) ? &payload_node : NULL };
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*response)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}
//
//int pn53_in_psl(pn53_dev_t* dev, uint8_t target_number, uint8_t baud_rate_initiator_to_target, uint8_t baud_rate_target_to_initiator) {
//    uint8_t command[] = { (uint8_t)PN53_COMMAND_IN_PSL, target_number, baud_rate_initiator_to_target, baud_rate_target_to_initiator };
//    uint8_t* response;
//    ssize_t res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), &response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code(response[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return (int)res;
//}
//
//ssize_t pn53_in_data_exchange(pn53_dev_t* dev, uint8_t target_number, const uint8_t* data_out, size_t data_out_length, uint8_t** data_in) {
//    uint8_t header[] = { (uint8_t)PN53_COMMAND_IN_DATA_EXCHANGE, target_number };
//    iolist_t data_node = { .iol_base = (void*)data_out, .iol_len = data_out_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = header, .iol_len = sizeof(header), .iol_next = (data_out_length > 0) ? &data_node : NULL };
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, data_in, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*data_in)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}
//
//ssize_t pn53_in_communicate_thru(pn53_dev_t* dev, const uint8_t* data_out, size_t data_out_length, uint8_t** data_in) {
//    uint8_t header = (uint8_t)PN53_COMMAND_IN_COMMUNICATE_THRU;
//    iolist_t data_node = { .iol_base = (void*)data_out, .iol_len = data_out_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = &header, .iol_len = sizeof(header), .iol_next = (data_out_length > 0) ? &data_node : NULL };
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, data_in, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*data_in)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}
//
//int pn53_in_deselect(pn53_dev_t* dev, uint8_t target_number) {
//    uint8_t command[] = { (uint8_t)PN53_COMMAND_IN_DESELECT, target_number };
//    uint8_t* response;
//    ssize_t res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), &response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code(response[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return (int)res;
//}
//
//int pn53_in_release(pn53_dev_t* dev, uint8_t target_number) {
//    uint8_t command[] = { (uint8_t)PN53_COMMAND_IN_RELEASE, target_number };
//    uint8_t* response;
//    ssize_t res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), &response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code(response[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return (int)res;
//}
//
//ssize_t pn53_in_select(pn53_dev_t* dev, uint8_t target_number, uint8_t** response) {
//    uint8_t command[] = { (uint8_t)PN53_COMMAND_IN_SELECT, target_number };
//    ssize_t res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*response)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}
//
//ssize_t pn53_in_auto_poll(pn53_dev_t* dev, uint8_t poll_number, uint8_t period, const uint8_t* types, size_t types_length, uint8_t** response) {
//    uint8_t header[] = { (uint8_t)PN53_COMMAND_IN_AUTO_POLL, poll_number, period };
//    iolist_t types_node = { .iol_base = (void*)types, .iol_len = types_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = header, .iol_len = sizeof(header), .iol_next = (types_length > 0) ? &types_node : NULL };
//    return pn53_hci_transceive_command(&dev->connection, &command_node, response, dev->command_timeout);
//}
//
//ssize_t pn53_tg_init_as_target(pn53_dev_t* dev, uint8_t mode, const uint8_t* mifare_params, const uint8_t* felica_params, const uint8_t* nfcid3t, uint8_t len_gt, const uint8_t* gt, uint8_t len_tk, const uint8_t* tk, uint8_t** response) {
//    uint8_t header_part1[] = { (uint8_t)PN53_COMMAND_TG_INIT_AS_TARGET, mode };
//    iolist_t tk_node = { .iol_base = (void*)tk, .iol_len = len_tk, .iol_next = NULL };
//    iolist_t len_tk_node = { .iol_base = &len_tk, .iol_len = 1, .iol_next = (len_tk > 0) ? &tk_node : NULL };
//    iolist_t gt_node = { .iol_base = (void*)gt, .iol_len = len_gt, .iol_next = &len_tk_node };
//    iolist_t len_gt_node = { .iol_base = &len_gt, .iol_len = 1, .iol_next = (len_gt > 0) ? &gt_node : &len_tk_node };
//    iolist_t nfcid3t_node = { .iol_base = (void*)nfcid3t, .iol_len = 10, .iol_next = &len_gt_node };
//    iolist_t felica_node = { .iol_base = (void*)felica_params, .iol_len = 18, .iol_next = &nfcid3t_node };
//    iolist_t mifare_node = { .iol_base = (void*)mifare_params, .iol_len = 6, .iol_next = &felica_node };
//    iolist_t command_node = { .iol_base = header_part1, .iol_len = sizeof(header_part1), .iol_next = &mifare_node };
//
//    return pn53_hci_transceive_command(&dev->connection, &command_node, response, dev->command_timeout);
//}
//
//int pn53_tg_set_general_bytes(pn53_dev_t* dev, const uint8_t* general_bytes, size_t length) {
//    uint8_t header = (uint8_t)PN53_COMMAND_TG_SET_GENERAL_BYTES;
//    iolist_t data_node = { .iol_base = (void*)general_bytes, .iol_len = length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = &header, .iol_len = sizeof(header), .iol_next = (length > 0) ? &data_node : NULL };
//    uint8_t* response;
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, &response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code(response[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return (int)res;
//}
//
//ssize_t pn53_tg_get_data(pn53_dev_t* dev, uint8_t** data_in) {
//    uint8_t command = (uint8_t)PN53_COMMAND_TG_GET_DATA;
//    ssize_t res = pn53_hci_transceive_command2(&dev->connection, &command, sizeof(command), data_in, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*data_in)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}
//
//int pn53_tg_set_data(pn53_dev_t* dev, const uint8_t* data_out, size_t data_out_length) {
//    uint8_t header = (uint8_t)PN53_COMMAND_TG_SET_DATA;
//    iolist_t data_node = { .iol_base = (void*)data_out, .iol_len = data_out_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = &header, .iol_len = sizeof(header), .iol_next = (data_out_length > 0) ? &data_node : NULL };
//    uint8_t* response;
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, &response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code(response[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return (int)res;
//}
//
//int pn53_tg_set_meta_data(pn53_dev_t* dev, const uint8_t* data_out, size_t data_out_length) {
//    uint8_t header = (uint8_t)PN53_COMMAND_TG_SET_META_DATA;
//    iolist_t data_node = { .iol_base = (void*)data_out, .iol_len = data_out_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = &header, .iol_len = sizeof(header), .iol_next = (data_out_length > 0) ? &data_node : NULL };
//    uint8_t* response;
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, &response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code(response[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return (int)res;
//}
//
//ssize_t pn53_tg_get_initiator_command(pn53_dev_t* dev, uint8_t** command_in) {
//    uint8_t command = (uint8_t)PN53_COMMAND_TG_GET_INITIATOR_COMMAND;
//    ssize_t res = pn53_hci_transceive_command2(&dev->connection, &command, sizeof(command), command_in, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*command_in)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}
//
//int pn53_tg_response_to_initiator(pn53_dev_t* dev, const uint8_t* response_out, size_t response_out_length) {
//    uint8_t header = (uint8_t)PN53_COMMAND_TG_RESPONSE_TO_INITIATOR;
//    iolist_t data_node = { .iol_base = (void*)response_out, .iol_len = response_out_length, .iol_next = NULL };
//    iolist_t command_node = { .iol_base = &header, .iol_len = sizeof(header), .iol_next = (response_out_length > 0) ? &data_node : NULL };
//    uint8_t* response;
//    ssize_t res = pn53_hci_transceive_command(&dev->connection, &command_node, &response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code(response[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return (int)res;
//}
//
//ssize_t pn53_tg_get_target_status(pn53_dev_t* dev, uint8_t** response) {
//    uint8_t command = (uint8_t)PN53_COMMAND_TG_GET_TARGET_STATUS;
//    ssize_t res = pn53_hci_transceive_command2(&dev->connection, &command, sizeof(command), response, dev->command_timeout);
//    if (res > 0) {
//        pn53_status_code_t status = pn53_status_code((*response)[0]);
//        if (status != PN53_STATUS_SUCCESS) {
//            return -PN53_ERRNO_FROM_STATUS_CODE(status);
//        }
//    }
//    return res;
//}

//static int _rf_configure(pn532_t *dev, unsigned cfg_item, const uint8_t *config,
//                         unsigned cfg_len)
//{
//    LOG_DEBUG("pn532: rf config %02x\n", cfg_item);
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START ] = CMD_RF_CONFIG;
//    buff[BUFF_DATA_START] = cfg_item;
//    for (unsigned i = 1; i <= cfg_len; i++) {
//        buff[BUFF_DATA_START + i] = *config++;
//    }
//
//    return send_rcv(dev, buff, cfg_len + 1, 0, STANDARD_TIMEOUT_SEC);
//}
//
//int _set_act_retries(pn532_t *dev, unsigned max_retries)
//{
//    uint8_t rtrcfg[] = { 0xff, 0x01, max_retries & 0xff };
//
//    return _rf_configure(dev, RF_CONFIG_MAX_RETRIES, rtrcfg, sizeof(rtrcfg));
//}



ssize_t pn53_list_passive_targets(pn53_dev_t* dev, uint8_t max_targets, pn53_technology_baudrate_t brty, uint8_t* data, size_t length, uint8_t** response, uint32_t timeout) {
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)brty,
    };
    iolist_t _data = __IOLIST(data, length, NULL);
    iolist_t _command = __IOLIST(command, sizeof(command), &_data);
    return pn53_hci_transceive_command(&dev->connection, &_command, response, timeout);
}

int pn53_list_passive_targets_a(pn53_dev_t* dev, uint8_t max_targets, nfc_a_id_t* id, nfc_a_polling_result_t* results, uint32_t timeout) {
    ssize_t res = 0;

    uint8_t command[3 + 10 + 2] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)PN53_TECHNOLOGY_A_106K,
        NFC_A_UID_CASCADE_TAG, 0, 0, 0, NFC_A_UID_CASCADE_TAG
    };

    size_t length = 3;

    if (id && (id->length > 0)) {
        PN53_DEBUG("List(a): anticol with given id of length %u\n", id->length);
        length += id->length;
        switch (id->length) {
            case NFC_A_UID_LENGTH_SINGLE_4:
                memcpy(&command[3], id->uid, NFC_A_UID_LENGTH_SINGLE_4);
                break;
            case NFC_A_UID_LENGTH_DOUBLE_7:
                length += 1;
                memcpy(&command[4], id->uid, NFC_A_UID_LENGTH_DOUBLE_7);
                break;
            case NFC_A_UID_LENGTH_TRIPLE_10:
                length += 2;
                memcpy(&command[4], id->uid, 3);
                memcpy(&command[8], &id->uid[3], 7);
                break;
            default:
                break;
        }
    }

    uint8_t* response;
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, length, &response, timeout)) < 0) {
        return (int)res;
    }
    if (res < 1) {
        PN53_DEBUG("List(a): missing NbTg\n");
        goto malformed;
    }
    memset(results, 0, sizeof(nfc_a_polling_result_t) * max_targets);
    uint8_t target_count = *response++;
    PN53_DEBUG("List(a): %u targets\n", (unsigned int)target_count);
    res -= 1;

    for (uint8_t i = 0; i < target_count; i += 1) {
        if ((size_t)res < 5) {
            PN53_DEBUG("List(a): target header too short\n");
            goto malformed;;
        }

        /* logical = */ response++;
        results[i].polling_response.raw[0] = *response++;
        results[i].polling_response.raw[1] = *response++;
        results[i].select_response = *response++;
        PN53_DEBUG("List(a): atqa=%02x%02x sak=%02x\n",
                   results[i].polling_response.raw[0],
                   results[i].polling_response.raw[1],
                   results[i].select_response);

        results[i].id = (nfc_a_id_t*)response++;
        res -= 5;

        if ((size_t)res < (size_t)results[i].id->length) {
            PN53_DEBUG("List(a): id cut off, "
                       "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                       (size_t)results[i].id->length, (size_t)res);
            goto malformed;
        }
        response += results[i].id->length;
        res -= results[i].id->length;

        if ((dev->nfc_parameters & (1 << 4)) &&
            nfc_a_supports_iso_dep(results[i].select_response) &&
            res > 0
        ) {
            nfc_a_ats_t* ats = (nfc_a_ats_t*)response;
            if ((size_t)res < (size_t)ats->length) {
                PN53_DEBUG("List(a): ATS cut off, "
                           "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                           (size_t)ats->length, (size_t)res);
                goto malformed;
            }
            PN53_DEBUG("List(a): ATS length=%" PRIuSIZE "\n",
                       (size_t)ats->length);

            results[i].higher_layer.iso_dep.ats = ats;
            response += ats->length;
            res -= ats->length;
        }
    }
    if (res > 0) {
        PN53_DEBUG("List(a): excess data %" PRIdSIZE " bytes\n", res);
    }
    return (ssize_t)target_count;

malformed:
    memset(results, 0, sizeof(nfc_a_polling_result_t) * max_targets);
    return -EBADMSG;
}

int pn53_list_passive_targets_b(pn53_dev_t* dev, uint8_t max_targets, nfc_bitrate_t bitrate,
                                uint8_t application_family, nfc_b_polling_method_t method,
                                nfc_b_polling_result_t* results, uint32_t timeout
) {
    ssize_t res = 0;

    pn53_technology_baudrate_t brty;
    switch (bitrate) {
        case NFC_BITRATE_106K: brty = PN53_TECHNOLOGY_B_106K; break;
        case NFC_BITRATE_212K: brty = PN53_TECHNOLOGY_B_212K; break;
        case NFC_BITRATE_424K:
            if (dev->model == PN53_MODEL_PN533) {
                brty = PN53_TECHNOLOGY_B_424K; break;
            }
            // fallthrough
        case NFC_BITRATE_848K:
            if (dev->model == PN53_MODEL_PN533) {
                brty = PN53_TECHNOLOGY_B_848K; break;
            }
            // fallthrough
        default:
            PN53_DEBUG("List(b): unsupported bitrate %u kbit/s\n", nfc_bitrate_kbps(bitrate));
            return -ENOTSUP;
    }

    uint8_t* response;
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)brty,
        application_family,
        (uint8_t)method
    };
    // Only append method if method argument != 0
    size_t length = sizeof(command) - ((method != 0) ? 0 : 1);
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, length, &response, timeout)) < 0) {
        return (int)res;
    }
    if (res < 1) {
        PN53_DEBUG("List(b): missing NbTg\n");
        goto malformed;
    }
    memset(results, 0, sizeof(nfc_b_polling_result_t) * max_targets);

    uint8_t target_count = *response++;
    PN53_DEBUG("List(b): %u targets\n", (unsigned int)target_count);
    res -= 1;

    for (uint8_t i = 0; i < target_count; i += 1) {
        if ((size_t)res < (sizeof(nfc_b_polling_response_payload_t) + 2 /* #Tg, 0x50, ATTRIB length */)) {
            PN53_DEBUG("List(b): target header too short\n");
            goto malformed;;
        }

        /* logical = */ response++;
        res -= 1;

        /* 0x50, the ATQB frame code / prefix is missing from the PN532/PN533 manuals.
         * Carl's PN532 on the Adafruit PN532 shield sends an addition 0x01 byte before 0x50. */

        uint8_t next = *response++;
        res -= 1;
        if (next != NFC_B_FRAME_CODE_POLLING_RESPONSE) {
            PN53_DEBUG("List(b): 0x%02x in place of 0x50 ATQB prefix?\n", next);
            next = *response++;
            res -= 1;
            PN53_DEBUG("List(b): ... skipping, next is 0x%02x\n", next);
            if (next != NFC_B_FRAME_CODE_POLLING_RESPONSE) {
                PN53_DEBUG("List(b): ... != missing ATQB prefix\n");
                PN53_DEBUG("List(b): malformed, missing 0x50 ATQB prefix\n");
                goto malformed;
            }
            PN53_DEBUG("List(b): ... == missing ATQB prefix\n");
        }

        results[i].polling_response = (nfc_b_polling_response_payload_t*)response;

        response += sizeof(nfc_b_polling_response_payload_t);
        res -= sizeof(nfc_b_polling_response_payload_t);

        if (res > 0) {
            uint8_t attrib_length = *response++;
            res -= 1;

            if ((size_t)res < (size_t)attrib_length) {
                PN53_DEBUG("List(b): ATTRIB cut off, "
                           "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                           (size_t)attrib_length, (size_t)res);
                goto malformed;
            }

            if (attrib_length > 0) {
                results[i].higher_layer.attrib_response_length = (size_t)attrib_length;
                results[i].higher_layer.attrib = (nfc_b_attrib_response_t*)response;

                response += (size_t)attrib_length;
                res -= attrib_length;
            }
        }
    }
    if (res > 0) {
        PN53_DEBUG("List(b): excess data %" PRIdSIZE " bytes\n", res);
    }
    return (ssize_t)target_count;

malformed:
    memset(results, 0, sizeof(nfc_b_polling_result_t) * max_targets);
    return -EBADMSG;
}


ssize_t pn53_list_passive_targets_f(pn53_dev_t* dev, uint8_t max_targets, nfc_bitrate_t bitrate,
    nfc_f_system_code_t system_code, nfc_f_polling_additional_request_t additional_request,
    uint8_t timeslots, nfc_f_polling_result_t* results, uint32_t timeout
) {
    ssize_t res = 0;
    pn53_technology_baudrate_t brty;
    switch (bitrate) {
        case NFC_BITRATE_212K: brty = PN53_TECHNOLOGY_F_212K; break;
        case NFC_BITRATE_424K: brty = PN53_TECHNOLOGY_F_424K; break;
        default:
            PN53_DEBUG("List(f): unsupported bitrate %u kbit/s\n", nfc_bitrate_kbps(bitrate));
            return -ENOTSUP;
    }

    uint8_t* response;
    uint8_t command[3 + sizeof(nfc_f_polling_command_t) - 1 /* without length */] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)brty
    };
    nfc_f_polling_command_t* pol = (nfc_f_polling_command_t*)(&command[2]);
    pol->header.code = NFC_F_PACKET_CODE_POLLING_COMMAND;
    pol->payload = (nfc_f_polling_command_payload_t) {
        .system_code = htole16(system_code),
        .additional_request = additional_request,
        .timeslots = timeslots,
    };
    // Only append method if method argument != 0
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), &response, timeout)) < 0) {
        return (int)res;
    }
    if (res < 1) {
        PN53_DEBUG("List(f): missing NbTg\n");
        goto malformed;
    }
    memset(results, 0, sizeof(nfc_f_polling_result_t) * max_targets);

    uint8_t target_count = *response++;
    PN53_DEBUG("List(f): %u targets\n", (unsigned int)target_count);
    res -= 1;

    for (uint8_t i = 0; i < target_count; i += 1) {
        if ((size_t)res < (1 + sizeof(nfc_f_polling_response_t) - sizeof(nfc_f_polling_response_payload_t))) {
            PN53_DEBUG("List(f): target header too short\n");
            goto malformed;;
        }

        /* logical = */ response++;
        res -= 1;

        nfc_f_polling_response_t* pol = (nfc_f_polling_response_t*)response;
        if ((size_t)res < (size_t)pol->header.length) {
            PN53_DEBUG("List(f): POL_RES cut off, "
                       "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                       (size_t)pol->header.length, (size_t)res);
            goto malformed;
        }
        if (pol->header.code != NFC_F_PACKET_CODE_POLLING_RESPONSE) {
            PN53_DEBUG("List(f): invalid response code %02x\n", (uint8_t)pol->header.code);
            goto malformed;
        }

        size_t expected = sizeof(nfc_f_polling_response_t);
        if (additional_request == NFC_F_POLLING_REQUEST_NOTHING) {
            expected -= sizeof(nfc_f_polling_response_payload_t);
        }

        if (pol->header.length != expected) {
            PN53_DEBUG("List(f): POL_RES had invalid length, "
                       "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                       expected, (size_t)pol->header.length);
            goto malformed;
        }

        switch (additional_request) {
            case NFC_F_POLLING_REQUEST_SYSTEM_CODE:
                results[i].system_code = byteorder_lebuftohs(pol->payload);
                break;
            case NFC_F_POLLING_REQUEST_BITRATES:
                results[i].bitrates = (nfc_bitrate_t)pol->payload[1] << 1;
                break;
            default: break;
        }

        results[i].id = &pol->id;
        results[i].pmm = &pol->pmm;
    }
    if (res > 0) {
        PN53_DEBUG("List(f): excess data %" PRIdSIZE " bytes\n", res);
    }
    return (ssize_t)target_count;

malformed:
    memset(results, 0, sizeof(nfc_f_polling_result_t) * max_targets);
    return -EBADMSG;
}



//int pn53_poll_a(pn53_dev_t* dev, const nfc_a_poll_config_t* config, nfc_a_target_t* targets, size_t capacity) {
//    
//}
//
// int pn532_get_passive_iso14443a(pn532_t *dev, nfc_iso14443a_t *out,
//                                 unsigned max_retries)
// {
//     int ret = -1;
//     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//     if (_set_act_retries(dev, max_retries) == 0) {
//         ret = _list_passive_targets(dev, buff, PN532_BR_106_ISO_14443_A, 1,
//                                     LIST_PASSIVE_LEN_14443(1));
//     }
//
//     if (ret > 0 && buff[0] > 0) {
//         out->target = buff[1];
//         out->sns_res = (buff[2] << 8) | buff[3];
//         out->acknowledgement = buff[4];
//         out->id_len  = buff[5];
//         out->type = ISO14443A_UNKNOWN;
//
//         for (int i = 0; i < out->id_len; i++) {
//             out->id[i] = buff[6 + i];
//         }
//
//         /* try to find out the type */
//         if (out->id_len == 4) {
//             out->type = ISO14443A_MIFARE;
//         }
//         else if (out->id_len == 7) {
//             /* In the case of type 4, the first byte of RATS is the length
//              * of RATS including the length itself (6+7) */
//             if (buff[13] == ret - 13) {
//                 out->type = ISO14443A_TYPE4;
//             }
//         }
//         ret = 0;
//     }
//     else {
//         ret = -1;
//     }
//
//     return ret;
// }
//
//void pn532_deselect_passive(pn532_t *dev)
//{
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START ] = CMD_DESELECT;
//    buff[BUFF_DATA_START] = 0x00; /* all targets */
//
//    send_rcv(dev, buff, 1, 1, STANDARD_TIMEOUT_SEC);
//}
//
//void pn532_release_passive(pn532_t *dev)
//{
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START ] = CMD_RELEASE;
//    buff[BUFF_DATA_START] = 0x00;
//
//    send_rcv(dev, buff, 1, 1, STANDARD_TIMEOUT_SEC);
//}
//
//// int pn532_mifareclassic_authenticate(pn532_t *dev, nfc_iso14443a_t *card,
////                                      pn532_mifare_key_t keyid, uint8_t *key, unsigned block)
//// {
////     int ret = -1;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////     buff[BUFF_DATA_START    ] = card->target;
////     buff[BUFF_DATA_START + 1] = keyid;
////     buff[BUFF_DATA_START + 2] = block; /* current block */
//
////     /*
////      * The card ID directly follows the key in the buffer
////      * The key consists of 6 bytes and starts at offset 3
////      */
////     for (int i = 0; i < 6; i++) {
////         buff[BUFF_DATA_START + 3 + i] = key[i];
////     }
//
////     for (int i = 0; i < card->id_len; i++) {
////         buff[BUFF_DATA_START + 9 + i] = card->id[i];
////     }
//
////     ret = send_rcv(dev, buff, 9 + card->id_len, 1, STANDARD_TIMEOUT_SEC);
////     if (ret == 1) {
////         ret = buff[0];
////         card->auth = 1;
////     }
//
////     return ret;
//// }
//
//// int pn532_mifareclassic_write(pn532_t *dev, uint8_t *idata, nfc_iso14443a_t *card,
////                               unsigned block)
//// {
////     int ret = -1;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     if (card->auth) {
//
////         buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////         buff[BUFF_DATA_START    ] = card->target;
////         buff[BUFF_DATA_START + 1] = MIFARE_CMD_WRITE;
////         buff[BUFF_DATA_START + 2] = block; /* current block */
////         memcpy(&buff[BUFF_DATA_START + 3], idata, MIFARE_CLASSIC_BLOCK_SIZE);
//
////         if (send_rcv(dev, buff, 19, 1, STANDARD_TIMEOUT_SEC) == 1) {
////             ret = buff[0];
////         }
//
////     }
////     return ret;
//// }
//
//// static int pn532_mifare_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
////                              unsigned block, unsigned len)
//// {
////     int ret = -1;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////     buff[BUFF_DATA_START    ] = card->target;
////     buff[BUFF_DATA_START + 1] = MIFARE_CMD_READ;
////     buff[BUFF_DATA_START + 2] = block; /* current block */
//
////     if (send_rcv(dev, buff, 3, len + 1, STANDARD_TIMEOUT_SEC) == (int)(len + 1)) {
////         memcpy(odata, &buff[1], len);
////         ret = 0;
////     }
//
////     return ret;
//// }
//
//// int pn532_mifareclassic_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
////                              unsigned block)
//// {
////     if (card->auth) {
////         return pn532_mifare_read(dev, odata, card, block, MIFARE_CLASSIC_BLOCK_SIZE);
////     }
////     else {
////         return -1;
////     }
//// }
//
//// int pn532_mifareulight_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
////                             unsigned page)
//// {
////     return pn532_mifare_read(dev, odata, card, page, 32);
//// }
//
//// static int send_rcv_apdu(pn532_t *dev, uint8_t *buff, unsigned slen, unsigned rlen)
//// {
////     int ret;
//
////     rlen += 3;
////     if ((rlen >= RAPDU_MAX_DATA_LEN) || (slen >= CAPDU_MAX_DATA_LEN)) {
////         return -1;
////     }
//
////     ret = send_rcv(dev, buff, slen, rlen);
////     if ((ret == (int)rlen) && (buff[0] == 0x00)) {
////         ret = (buff[rlen - 2] << 8) | buff[rlen - 1];
////         if (ret == (int)0x9000) {
////             ret = 0;
////         }
////     }
//
////     return ret;
//// }
//

//
//// int pn532_iso14443a_4_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
////                            unsigned offset, uint8_t len)
//// {
////     int ret;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////     buff[BUFF_DATA_START    ] = card->target;
////     buff[BUFF_DATA_START + 1] = 0x00;
////     buff[BUFF_DATA_START + 2] = 0xb0;
////     buff[BUFF_DATA_START + 3] = (offset >> 8) & 0xff;
////     buff[BUFF_DATA_START + 4] = offset & 0xff;
////     buff[BUFF_DATA_START + 5] = len;
//
////     ret = send_rcv_apdu(dev, buff, 6, len);
////     if (ret == 0) {
////         memcpy(odata, &buff[RAPDU_DATA_BEGIN], len);
////     }
//
////     return ret;
//// }
//
//int change_rf_field(pn532_t *dev, bool on) {
//    uint8_t command[] = {RF_CONFIGURATION, 0x01, (on) ? 0x01 : 0x00};
//    return send_rcv(dev, command, 2, 0, STANDARD_TIMEOUT_SEC);
//}
//
//static void _load_fifo_data(pn532_t *dev, const uint8_t *data, unsigned len) {
//    /* clear FIFO data */
//    pn532_write_reg(dev, PN532_REGISTER_FIFOLevel, 0x80);
//
//    for (unsigned i = 0; i < len; i++) {
//        pn532_write_reg(dev, PN532_REGISTER_FIFOData, data[i]);
//    }
//}
//
//static void _read_fifo_data(pn532_t *dev, uint8_t *data, unsigned len) {
//    for (unsigned i = 0; i < len; i++) {
//        pn532_read_reg(dev, &(data[i]), PN532_REGISTER_FIFOData);
//    }
//}
//
//
//int _init_as_target_nfc_f(pn532_t *dev, uint8_t *nfc_f_params) {
//    pn532_write_reg(dev, PN532_REGISTER_Command, 0x00);
//
//    /* set NFC F rf config */
//    _rf_configure(dev, 0x0B, nfc_f_rf_config, 8);
//
//    /* load the NFC F params into the FIFO buffer */
//    uint8_t params_buffer[25] = {0};
//    memcpy(params_buffer + 2 + 3 + 1, nfc_f_params, 18);
//
//
//    pn532_write_reg(dev, PN532_REGISTER_Command, 0x00); /* idle command */
//    pn532_write_reg(dev, PN532_REGISTER_FIFOLevel, 0b10000000); /* clear fifo */
//    _load_fifo_data(dev, params_buffer, 25); /* write params into fifo */
//
//    pn532_write_reg(dev, PN532_REGISTER_Command, 0b00000001); /* configure command */
//
//    pn532_write_reg(dev, PN532_REGISTER_Control,   0b00000000);
//    pn532_write_reg(dev, PN532_REGISTER_Mode,      0b00111111);
//    pn532_write_reg(dev, PN532_REGISTER_FelNFC2,   0b10000000);
//    pn532_write_reg(dev, PN532_REGISTER_TxMode,    0b10010010); /* this must be changed based on the bitrate*/
//    pn532_write_reg(dev, PN532_REGISTER_RxMode,    0b10011010); /* this must be changed based on the bitrate */
//    pn532_write_reg(dev, PN532_REGISTER_TxControl, 0b10000000);
//    pn532_write_reg(dev, PN532_REGISTER_TxAuto,    0b00100000);
//    pn532_write_reg(dev, PN532_REGISTER_Demod,     0b01100001);
//    pn532_write_reg(dev, PN532_REGISTER_CommIrq,   0b01111111);
//    pn532_write_reg(dev, PN532_REGISTER_DivIrq,    0b01111111);
//    pn532_write_reg(dev, PN532_REGISTER_Command,   0b00001101);
//
//    uint8_t commirq, status_1, status_2, divirq;
//    while (true) {
//        ztimer_sleep(ZTIMER_MSEC, 10);
//        pn532_read_reg(dev, &commirq,  PN532_REGISTER_CommIrq);
//        pn532_read_reg(dev, &status_1, PN532_REGISTER_Status1);
//        pn532_read_reg(dev, &status_2, PN532_REGISTER_Status2);
//        pn532_read_reg(dev, &divirq,   PN532_REGISTER_DivIrq);
//        LOG_DEBUG("pn532: CIU comm irq %02x\n", commirq);
//        if ((commirq & 0b00110000) == 0b00110000) {
//            pn532_write_reg(dev, PN532_REGISTER_CommIrq, 0b00110000); /* clear IRQ */
//            uint8_t fifo_size;
//            pn532_read_reg(dev, &fifo_size, PN532_REGISTER_FIFOLevel);
//
//            uint8_t fifo_data[128] = {0};
//            _read_fifo_data(dev, fifo_data, fifo_size);
//
//
//            if (fifo_size == 0) {
//                return -1; /* no data in FIFO */
//            }
//
//            LOG_DEBUG("pn532: fifo size is %d\n", fifo_size);
//            if (fifo_size == fifo_data[0]) {
//                LOG_DEBUG("pn532: fifo data is ");
//                PRINTBUFF(fifo_data, fifo_size);
//
//                return 0; /* success */
//            
//            }
//            pn532_write_reg(dev, PN532_REGISTER_Command, 0b00001101); /* restart command */
//        }
//        
//    }
//}
//
//static int _tg_init_as_target(pn532_t *dev, uint8_t mode,
//    uint8_t *mifare_params, uint8_t *felica_params, uint8_t *nfcid3t, uint8_t *buff,
//    size_t timeout_sec) {
//    assert(dev != NULL);
//    assert(buff != NULL);
//
//    LOG_DEBUG("pn532: setting CIU Mode\n");
//    int ret = pn532_write_reg(dev, PN532_REGISTER_Mode, 0b00111111);
//    if (ret != 0) {
//        return ret;
//    }
//
//    buff[BUFF_CMD_START] = CMD_INIT_AS_TARGET;
//
//    /* target mode */
//    buff[BUFF_DATA_START] = mode;
//    
//    if (mifare_params != NULL) {
//        _rf_configure(dev, 0x0A, nfc_a_rf_config, sizeof(nfc_a_rf_config));
//        memcpy(&buff[BUFF_DATA_START + 1], mifare_params, 6);
//    }
//
//    if (felica_params != NULL) {
//        memcpy(&buff[BUFF_DATA_START + 1 + 6], felica_params, 18);
//    }
//
//    if (nfcid3t != NULL) {
//        memcpy(&buff[BUFF_DATA_START + 1 + 6 + 18], nfcid3t, 10);
//    }
//    /* AutoRFCA: on, RFCA: off
//    uint8_t enable_auto_rfca = 0b00000010;
//    _rf_configure(dev, 0x01, &enable_auto_rfca, 1);  */
//
//    LOG_DEBUG("pn532: init as target mode %i\n", mode);
//    /* recv len depends on the technology used */
//
//    return send_rcv(dev, buff, 1 + 6 + 18 + 10 + 2, 20, timeout_sec);
//}


//uint8_t pn532_get_target_status(pn532_t *dev) {
//    assert(dev != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    uint8_t status = 0xff;
//    buff[BUFF_CMD_START] = CMD_GET_TARGET_STATUS;
//    if (send_rcv(dev, buff, 0, 2, STANDARD_TIMEOUT_SEC) == 2) {
//        status = buff[0];
//        /* discard baud rate in byte 2 */
//    }
//
//    return status;
//}
//
///* int pn532_init_tag(pn532_t *dev, nfc_application_type_t app_type) {
//    assert(dev != NULL);
//
//    if (app_type == NFC_APPLICATION_TYPE_T2T) {
//        LOG_DEBUG("pn532: init target as T2T\n");
//        uint8_t mifare_params[] = {
//            0x00, 0x00, 0x58, 0xC8, 0xB5, 0x00
//        };
//        return _init_as_target(dev, 0b00000000, mifare_params, NULL, NULL);
//    } else if (app_type == NFC_APPLICATION_TYPE_T3T) {
//        LOG_DEBUG("pn532: init target as T3T\n");
//        uint8_t felica_params[] = {
//            0x02, 0xFE, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08,
//            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
//            0x12, 0xFC 
//        };
//        return _init_as_target_nfc_f(dev, felica_params);
//    }
//
//    return -1;
//} */
//
//int pn532_listen_a(nfcdev_t *nfcdev, const nfc_a_listen_config_t *config) {
//    LOG_DEBUG("pn532: init target as NFC-A\n");
//    assert(nfcdev != NULL);
//    assert(config != NULL);
//
//    assert(config->uid.len == 4); /* only 4 byte uid supported */
//    assert(config->uid.nfcid[0] == 0x08); /* the first byte must be 0x08 for the PN532*/
//
//    /* the NFC-A params are called Mifare params in the PN532 manual */
//    uint8_t mifare_params[6] = {0};  /* SENS_RES, NFCID1t, SEL_RES */
//
//    mifare_params[0] = config->polling_response.anticollision_information;
//    mifare_params[1] = config->polling_response.platform_information;
//
//    mifare_params[2] = config->uid.nfcid[1];
//    mifare_params[3] = config->uid.nfcid[2];
//    mifare_params[4] = config->uid.nfcid[3];
//
//    mifare_params[5] = config->acknowledgement;
//
//    uint8_t mode = 0x00;
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN] = {0};
//    if (config->acknowledgement && NFC_A_SEL_RES_T4T_VALUE) {
//        /* we enable automatic handeling of NFC-DEP for T4T */
//        pn532_set_parameters((pn532_t *) nfcdev->dev, 0b00100000); /* enable NFC-DEP */
//        mode &= 0b00000100;
//    } else {
//        pn532_set_parameters((pn532_t *) nfcdev->dev, 0b00000000); /* disable NFC-DEP */
//    }
//
//
//    int ret = _tg_init_as_target((pn532_t *) nfcdev->dev, mode, mifare_params, NULL, NULL, buff, 
//        LISTEN_TIMEOUT_SEC);
//    if (ret <= 1) {
//        return ret;
//    }
//
//    pn532_t *dev = (pn532_t *) nfcdev->dev;
//
//    /* the buff now contains the first command we received from the initiator */
//    if (0b00001000 & buff[0]) {
//        LOG_DEBUG("pn532: received ISO-DEP RATS\n");
//        /* The PN532 is initialized as ISO14443A-4 target (NFC-DEP), the buffer should
//         * contain the RATS sent by the initiator. We don't need to save it inside the
//         * initiator buffer. Therefore, we do nothing.
//         */
//        dev->iso_dep = true;
//    } else if ((0b00001000 & buff[0]) == 0x00) {
//        /* We should be a passive target. Potentially a T2T or a proprietary tag. */
//        dev->initiator_command_len = ret - 1;
//        LOG_DEBUG("pn532: received initiator command len %d\n", dev->initiator_command_len);
//        assert(dev->initiator_command_len <= CONFIG_PN532_INITIATOR_COMMAND_BUFFER_LEN);
//        memcpy(dev->initiator_command, &buff[1], dev->initiator_command_len);
//        dev->iso_dep = false;
//    } else {
//        LOG_ERROR("pn532: unknown initiator command received %02x\n", buff[0]);
//        return -1;
//    }
//
//    return 0;
//}

//int pn532_mifare_classic_authenticate(nfcdev_t *nfcdev, uint8_t block, 
//    const nfc_a_id_t *uid, bool is_key_a, const uint8_t *key) {
//    assert(nfcdev != NULL);
//    assert(key != NULL);
//    assert(uid != NULL);
//    assert(uid->len == NFC_A_NFCID1_LEN4);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
//    buff[BUFF_DATA_START    ] = 1; /* tg number must always be 1 */
//    buff[BUFF_DATA_START + 1] = is_key_a ? MIFARE_CLASSIC_AUTH_A : MIFARE_CLASSIC_AUTH_B;
//    buff[BUFF_DATA_START + 2] = block; /* current block */
//
//
//    memcpy(&buff[BUFF_DATA_START + 3], key, 6);
//
//    memcpy(&buff[BUFF_DATA_START + 9], uid->nfcid, uid->len);
//
//    int ret_len = send_rcv((pn532_t *) nfcdev->dev, buff, 9 + uid->len, 1, STANDARD_TIMEOUT_SEC);
//    if (ret_len > 0) {
//        if (buff[0] != 0x00) {
//            /* error */
//            LOG_DEBUG("pn532: error in mifare classic auth %02x\n", buff[0]);
//            return NFC_ERR_AUTH;
//        }
//        return 0;
//    } else {
//        return NFC_ERR_AUTH;
//    }
//}
//
//int pn532_initiator_exchange_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len,
//                                  uint8_t *rcv, size_t *receive_len) {
//    assert(nfcdev != NULL);
//    assert(nfcdev->dev != NULL);
//    assert(BUFF_DATA_START + 1 + send_len <= CONFIG_PN532_BUFFER_LEN);
//    assert(receive_len != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
//    buff[BUFF_DATA_START    ] = 1; /* tg number must always be 1 */
//
//    memcpy(&buff[BUFF_DATA_START + 1], send, send_len);
//    int ret_len = send_rcv(nfcdev->dev, buff, send_len + 1, CONFIG_PN532_BUFFER_LEN - 8, 
//        STANDARD_TIMEOUT_SEC);
//
//    /* receive_len is only the data received, not the status byte */
//    if (ret_len > 0) {
//        if (*receive_len < (size_t)(ret_len - 1)) {
//            /* the receive buffer is too small */
//            LOG_ERROR("pn532: receive buffer too small\n");
//            *receive_len = 0;
//            return -1;
//        }
//
//        *receive_len = ret_len - 1;
//        if (buff[0] != 0x00) {
//            /* error */
//            LOG_ERROR("pn532: error in data exchange %02x\n", buff[0]);
//            *receive_len = 0;
//            return -1;
//        }
//
//        LOG_DEBUG("pn532: received %u bytes\n", *receive_len);
//        /* copy the data into the receive buffer, excluding the status byte */
//        memcpy(rcv, buff + 1, *receive_len);
//    }
//    return 0;
//}
//
///* returns the length of bytes sent */
//static int _tg_response_to_initiator(pn532_t *dev, const uint8_t *send, size_t send_len) {
//    LOG_DEBUG("pn532: response to initiator\n");
//    assert (send != NULL);
//    assert (send_len > 0);
//    assert(BUFF_DATA_START + send_len <= CONFIG_PN532_BUFFER_LEN);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START] = CMD_RESPONSE_TO_INITIATOR;
//
//    memcpy(&buff[BUFF_DATA_START], send, send_len);
//
//    /* receive 1 status byte */
//    int ret_len = send_rcv(dev, buff, send_len, 1, STANDARD_TIMEOUT_SEC);
//    if (ret_len != 1) {
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    if (buff[0] != 0x00) {
//        LOG_ERROR("pn532: error in response to initiator %02x\n", buff[0]);
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    return ret_len;
//}
//
//// static int _tg_get_target_status(pn532_t *dev, uint8_t *status, uint8_t *baud_rate) {
////     LOG_DEBUG("pn532: get target status\n");
////     assert(status != NULL);
////     assert(baud_rate != NULL);
//
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
////     buff[BUFF_CMD_START] = CMD_GET_TARGET_STATUS;
//
////     int ret_len = send_rcv(dev, buff, 0, 1, STANDARD_TIMEOUT_SEC);
////     if (ret_len != 1) {
////         return NFC_ERR_COMMUNICATION;
////     }
//
////     *status = buff[0];
////     *baud_rate = buff[1];
////     return ret_len;
//// }
//
///* this is used to receive */
//static int _tg_get_initiator_command(pn532_t *dev, uint8_t *rcv, size_t *receive_len) {
//    LOG_DEBUG("pn532: get initiator command\n");
//    assert(rcv != NULL);
//    assert(receive_len != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START] = CMD_GET_INITIATOR_COMMAND;
//
//    int ret_len = send_rcv(dev, buff, 0, CONFIG_PN532_BUFFER_LEN - 10, STANDARD_TIMEOUT_SEC);
//
//    if (ret_len == 0) {
//        LOG_ERROR("pn532: no data received from initiator\n");
//        *receive_len = 0;
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    if (ret_len <= 1) {
//        *receive_len = 0;
//        return ret_len;
//    }
//
//    /* check if ret_len is bigger than receive_len */
//    if (ret_len - 1 > (int)*receive_len) {
//        LOG_ERROR("pn532: receive buffer too small\n");
//        *receive_len = 0;
//        return -1;
//    }
//
//    if (buff[0] != 0x00) {
//        LOG_ERROR("pn532: error in get initiator command %02x\n", buff[0]);
//        *receive_len = 0;
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    *receive_len = ret_len - 1;
//    memcpy(rcv, buff + 1, *receive_len);
//
//    return ret_len;
//}
//
///* for NFC-DEP and ISO-DEP */
//static int _tg_get_data(pn532_t *dev, uint8_t *rcv, size_t *receive_len) {
//    LOG_DEBUG("pn532: get data\n");
//    assert(dev != NULL);
//    assert(rcv != NULL);
//    assert(receive_len != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START] = CMD_GET_DATA;
//
//    int ret_len = send_rcv(dev, buff, 0, CONFIG_PN532_BUFFER_LEN - 8, STANDARD_TIMEOUT_SEC);
//
//    if (ret_len < 0) {
//        *receive_len = 0;
//        return NFC_ERR_COMMUNICATION;
//    }
//
//#define ERROR_CODE_DESELECTED 0x29
//    if (buff[0] == ERROR_CODE_DESELECTED) {
//        LOG_ERROR("pn532: target deselected by initiator\n");
//        *receive_len = 0;
//        return NFC_ERR_DESELECTED;
//    }
//
//    /* check if ret_len is bigger than receive_len */
//    if (ret_len - 1 > (int)*receive_len) {
//        LOG_ERROR("pn532: receive buffer too small\n");
//        *receive_len = 0;
//        return -1;
//    }
//
//    if (buff[0] != 0x00) {
//        LOG_ERROR("pn532: error in get data %02x\n", buff[0]);
//        *receive_len = 0;
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    *receive_len = ret_len - 1;
//    memcpy(rcv, buff + 1, *receive_len);
//
//    return ret_len;
//}
//
///* for NFC-DEP and ISO-DEP */
//static int _tg_set_data(pn532_t *dev, const uint8_t *send, size_t send_len) {
//    LOG_DEBUG("pn532: set data\n");
//    assert(dev != NULL);
//    assert(send != NULL);
//    assert(send_len > 0);
//    assert(BUFF_DATA_START + send_len <= CONFIG_PN532_BUFFER_LEN);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START] = CMD_SET_DATA;
//
//    memcpy(&buff[BUFF_DATA_START], send, send_len);
//
//    int ret_len = send_rcv(dev, buff, send_len, 0, STANDARD_TIMEOUT_SEC);
//    if (ret_len != 1) {
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    if (buff[0] != 0x00) {
//        LOG_ERROR("pn532: error in send data %02x\n", buff[0]);
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    return ret_len;
//}
//
//int pn532_target_send_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len) {
//    assert(nfcdev != NULL);
//    assert(nfcdev->dev != NULL);
//
//    if (send_len > CONFIG_PN532_BUFFER_LEN - 8) {
//        LOG_ERROR("pn532: send buffer too large (%u bytes, max %d)\n", send_len, CONFIG_PN532_BUFFER_LEN - 8);
//        return -1;
//    }
//
//    if (((pn532_t *) nfcdev->dev)->iso_dep) {
//        /* for communication with an ISO 14443-4 PICC (ISO-DEP) */
//        return _tg_set_data(nfcdev->dev, send, send_len);
//    } else {
//        /* for communication with a non-ISO-DEP target (T2T, MIFARE Classic, etc.) */
//        _tg_response_to_initiator(nfcdev->dev, send, send_len);
//        return 0;
//    }
//
//
//}
//
//int pn532_target_receive_data(nfcdev_t *nfcdev, uint8_t *rcv, size_t *receive_len) {
//    assert(nfcdev != NULL);
//    assert(nfcdev->dev != NULL);
//    assert(rcv != NULL);
//    assert(receive_len != NULL);
//
//    /* the PN532 init as target command returns after receiving the first command */
//    pn532_t *dev = (pn532_t *) nfcdev->dev;
//
//    if (dev->initiator_command_len > 0) {
//        assert(dev->initiator_command_len <= CONFIG_PN532_INITIATOR_COMMAND_BUFFER_LEN);
//        memcpy(rcv, dev->initiator_command, dev->initiator_command_len);
//        LOG_DEBUG("pn532: returning cached initiator command of length %zu\n", dev->initiator_command_len);
//        *receive_len = dev->initiator_command_len;
//        dev->initiator_command_len = 0; /* clear the buffer */
//        return *receive_len;
//    }
//
//    if (dev->iso_dep) {
//        return _tg_get_data(nfcdev->dev, rcv, receive_len);
//    } else {
//        return _tg_get_initiator_command(nfcdev->dev, rcv, receive_len);
//    }
//}
//
//int pn532_poll_dep(nfcdev_t *nfcdev, nfc_bitrate_t br) {
//    assert(nfcdev != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    uint8_t target_type;
//    switch (br) {
//        case NFC_BITRATE_106K:
//            target_type = 0x00;
//            break;
//        case NFC_BITRATE_212K:
//            target_type = 0x01;
//            break;
//        case NFC_BITRATE_424K:
//            target_type = 0x02;
//            break;
//        default:
//            return -1;
//    }
//
//    int ret = _list_passive_targets(nfcdev->dev, buff, target_type, 1, 30);
//    if (ret <= 0) {
//        return -1;
//    }
//
//    if (buff[0] != 1) {
//        LOG_ERROR("pn532: error during polling\n");
//        return NFC_ERR_POLL_NO_TARGET;
//    }
//
//    return 0;
//}
//
//int pn532_listen_dep(nfcdev_t *nfcdev, nfc_bitrate_t br, const uint8_t *nfcid3t) {
//    assert(nfcdev != NULL);
//    (void) br;
//
//    uint8_t mode = TG_INIT_AS_TARGET_DEP_ONLY;
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN] = {0};
//
//    _tg_init_as_target(nfcdev->dev, mode, NULL, NULL, (uint8_t *) nfcid3t, buff, 
//        LISTEN_TIMEOUT_SEC);
//
//    return 0;
//};
