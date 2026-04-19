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
#include "macros/utils.h"

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
#define PN53_DEBUG(command, ...) DEBUG("pn53x." command ": " __VA_ARGS__)



#define PN53_BUS_I2C_ADDRESS           (0x24)

/* SPI bus parameters */
#define SPI_MODE                    (SPI_MODE_0)
#define SPI_CLK                     (SPI_CLK_1MHZ)

#define UART_BAUDRATE               (115200U)


ssize_t pn53_hci_transceive_command(pn53_connection_t* connection, iolist_t* command,
                            uint8_t** response, uint32_t timeout_ms) {
    assert(command);
    assert(command->iol_base);
    uint8_t code = *((uint8_t*)command->iol_base);

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

static ssize_t pn53_hci_transceive_command_status_response(pn53_dev_t* dev, iolist_t* command,
                                                uint8_t** response, size_t expected_response_length, uint32_t timeout_ms) {
    uint8_t* _response;
    ssize_t res = pn53_hci_transceive_command(&dev->connection, command, &_response, timeout_ms);
    if (response) {
        *response = _response;
    }
    if (res < 0) {
        return (int)res;
    }

    if (dev->model == PN53_MODEL_PN533) {
        if (res == 0) {
            PN53_DEBUG("internal", "missing status code\n");
            return -EBADMSG;
        }
        pn53_status_code_t code = pn53_status_code(*_response);
        if (code != 0) {
            return -PN53_ERRNO_FROM_STATUS_CODE(code);
        }
        res -= 1;

        if (response) {
            *response += 1;
        }
    }

    if ((size_t)res != expected_response_length) {
        PN53_DEBUG("internal", "excess %" PRIdSIZE " bytes\n", res);
        return -EBADMSG;
    }
    return res;
}

static inline ssize_t pn53_hci_transceive_command2_status_response(pn53_dev_t* dev, uint8_t* command, size_t length,
                                                uint8_t** response, size_t expected_response_length, uint32_t timeout_ms) {
    iolist_t iolist = { .iol_base = (void*)command, .iol_len = length };
    return pn53_hci_transceive_command_status_response(dev, &iolist, response, expected_response_length, timeout_ms);
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

    PN53_DEBUG("Diagnose.echo", "");
    PN53_DEBUG_HEX(response, (size_t)res);
    DEBUG("\n");

    if (res != sizeof(command) - 1) {
        PN53_DEBUG("Diagnose.echo", "comms check failed\n");
        return -1;
    }
    if (memcmp(&command[1], response, sizeof(command) - 1) != 0) {
        PN53_DEBUG("Diagnose.echo", "comms check failed\n");
        return -1;
    }

    PN53_DEBUG("Diagnose.echo", "comms check successful\n");
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
        PN53_DEBUG("GetFwVersion", "unable to get firmware version\n");
        return res;
    }

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        PN53_DEBUG("GetFwVersion", "connected to <pn53x ic=0x%02X version=%i revision=%i nfc={a=%i b=%i dep=%i}>\n",
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
    PN53_HCI_DEBUG("HCI initialized\n");

    pn53_hci_reset(&dev->connection);
    PN53_HCI_DEBUG("reset done\n");

    /* Start with conservative minimum before knowing model */
    dev->connection.max_packet_length = 200;
    if (IS_ACTIVE(CONFIG_PN53_SELFCHECK)) {
        if ((res = pn53_check_communication(dev)) < 0) {
            PN53_DEBUG("init", "failed wih %i\n", res);
            return res;
        }
    }

    if ((res = pn53_get_firmware_version(dev, NULL)) < 0) {
        return res;
    }

    PN53_DEBUG("init", "resetting RF comms timeouts\n");
    // We rely on host timers, so let disable controller comms timeouts
    pn53_rf_configuration_payload_t timings = {
        .item = PN53_RF_CONFIGURATION_ITEM_VARIOUS_TIMINGS,
        .various_timings = { 0 }
    };
    if ((res = pn53_rf_configuration(dev, &timings)) < 0) {
        PN53_DEBUG("init", "failed to reset RF comms timeouts: %i\n", (int)res);
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

ssize_t pn53_read_registers_(pn53_dev_t *dev, void* registers, uint8_t** values, size_t count) {
    uint8_t code = (uint8_t)PN53_COMMAND_READ_REGISTERS;
    iolist_t addrs = __IOLIST(registers, count * sizeof(pn53_register_address_t), NULL);
    iolist_t command = __IOLIST(&code, 1, &addrs);
    return (int)pn53_hci_transceive_command_status_response(dev, &command, values, count, dev->command_timeout);
}

int pn53_write_registers_(pn53_dev_t *dev, void* registers, size_t count) {
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
            PN53_DEBUG("WriteRegisters", "missing status code\n");
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

    pn53_bitfield_set(&symbols->register_values[ix], mask, value);

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
    return pn53_bitfield_get(symbols->register_values[ix], mask);
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
            addrs[register_ix] = htobe16(addr);
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
                .address = htobe16(addr),
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
        PN53_DEBUG("GetGeneralStatus", "expected at least 4, got %" PRIuSIZE " bytes\n", (size_t)res);
        return -EBADMSG;
    }
    size_t expected = 4 /* Err, Field, NbTg, SAM */ + response[2] /* NbTg */ * 4 /* Tg, BrRx, BrTx, Type */;
    if ((size_t)res < expected) {
        PN53_DEBUG("GetGeneralStatus", "expected at least %" PRIuSIZE ", got %" PRIuSIZE " bytes\n", expected, (size_t)res);
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

            status->targets[i].mode = NFC_FIELD_MODEL_READER_WRITER_TAG;
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
                    status->targets[i].mode = NFC_FIELD_MODEL_PEERS;
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
                    PN53_DEBUG("GetGeneralStatus", "illegal target type 0x%02x\n", type);
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

int pn53_rf_configuration(pn53_dev_t* dev, const pn53_rf_configuration_payload_t* rf_config) {
    uint8_t code = PN53_COMMAND_RF_CONFIGURATION;
    iolist_t config = __IOLIST((const uint8_t*)rf_config, 1 + _rf_configuration_payload_length(rf_config), NULL);
    iolist_t command = __IOLIST(&code, 1, &config);
    int res = (int)pn53_hci_transceive_command(&dev->connection, &command, NULL, dev->command_timeout);
    return res < 0 ? res : 0;
}

int pn53_set_field_enablement(pn53_dev_t* dev, bool intent_to_enable, bool avoid_external_field) {
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_RF_CONFIGURATION,
        (uint8_t)PN53_RF_CONFIGURATION_ITEM_RF_FIELD,
        (intent_to_enable & 1) | ((avoid_external_field & 1) << 1)
    };
    int res = (int)pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), NULL, dev->command_timeout);
    return res < 0 ? res : 0;
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

ssize_t pn53_in_communicate_thru_data_exchange(pn53_dev_t* dev, const uint8_t* command, size_t length, uint8_t** response, uint32_t timeout_ms, pn53_command_code_t code) {
    iolist_t _data = __IOLIST(command, length, NULL);
    iolist_t _command = __IOLIST((uint8_t*)&code, 1, &_data);
    uint8_t* _response = NULL;
    ssize_t res = pn53_hci_transceive_command(&dev->connection, &_command, &_response, timeout_ms);
    if (res > 0) {
        assert(_response);
        pn53_status_code_t status = pn53_status_code(*_response);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
        res -= 1;
        _response += 1;
    }
    if (response) {
        *response = _response;
    }
    return res;
}

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

static uint8_t _tx_rx_framing(nfc_technology_t tech) {
    switch (tech) {
        case NFC_TECHNOLOGY_A: return 0;
        case NFC_TECHNOLOGY_B: return 3;
        case NFC_TECHNOLOGY_F: return 2;
        default:
            assert(false);
            UNREACHABLE();
            return 0;
    }
}

static uint8_t _tx_rx_mode(uint8_t framing, nfc_bitrate_t bitrate) {
    uint8_t mode = 0;
    pn53_bitfield_set(&mode, PN53_REGISTER_TXRX_MODE_BITRATE_INDEX, nfc_bitrate_to_index(bitrate));
    pn53_bitfield_set(&mode, PN53_REGISTER_TXRX_MODE_FRAMING, framing);
    return mode;
}

ssize_t pn53_in_list_passive_targets(pn53_dev_t* dev, uint8_t max_targets, pn53_technology_baudrate_t brty, uint8_t* data, size_t length, uint8_t** response, uint32_t timeout) {
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)brty,
    };
    iolist_t _data = __IOLIST(data, length, NULL);
    iolist_t _command = __IOLIST(command, sizeof(command), &_data);
    return pn53_hci_transceive_command(&dev->connection, &_command, response, timeout);
}

ssize_t pn53_parse_passive_targets(
    uint8_t* response, size_t length, pn53_logical_target_t* targets, uint8_t max_targets, void* arg,
    ssize_t (*parse)(uint8_t* response, size_t length, pn53_logical_target_t* target, void* arg)
) {
    memset(targets, 0, sizeof(pn53_logical_target_t) * max_targets);

    if (length < 1) {
        PN53_DEBUG("InList", "missing NbTg\n");
        goto malformed;
    }

    uint8_t target_count = *response++;
    PN53_DEBUG("InList", "%u targets\n", (unsigned int)target_count);
    if (target_count > max_targets) {
        PN53_DEBUG("InList", "... but max=%u\n", (unsigned int)max_targets);
        goto malformed;
    }
    length -= 1;

    for (uint8_t i = 0; i < target_count; i += 1) {
        if (length < 1) {
            PN53_DEBUG("InList", "missing Tg\n");
            goto malformed;
        }

        uint8_t tg = *response++;
        assert(tg == i + 1);
        length -= 1;

        ssize_t remaining = parse(response, length, &targets[i], arg);
        if (remaining < 0) {
            goto malformed;
        }
        response += length - (size_t)remaining;
        length = (size_t)remaining;
    }
    if (length > 0) {
        PN53_DEBUG("InList", "excess data %" PRIdSIZE " bytes\n", length);
    }
    return (ssize_t)target_count;

malformed:
    memset(targets, 0, sizeof(pn53_logical_target_t) * max_targets);
    return -EBADMSG;
}

static ssize_t pn53_parse_passive_target_a(uint8_t* response, size_t length, pn53_logical_target_t* target, bool* auto_rats_enabled) {
    assert(target);
    target->super.tag.technology = NFC_TECHNOLOGY_A;
    nfc_a_tag_t* tag = &target->super.tag.a;

    if (length < 4) {
        PN53_DEBUG("InList.a", "target header too short\n");
        return -EBADMSG;
    }

    tag->polling_response.raw[0] = *response++;
    tag->polling_response.raw[1] = *response++;
    tag->select_response = *response++;
    PN53_DEBUG("List.a", "atqa=%02x%02x sak=%02x\n",
               tag->polling_response.raw[0], tag->polling_response.raw[1], tag->select_response);

    tag->id = (nfc_a_id_t*)response++;
    length -= 5;

    if (length < (size_t)tag->id->length) {
        PN53_DEBUG("InList.a", "id cut off, "
                   "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                   (size_t)tag->id->length, length);
        return -EBADMSG;
    }
    response += tag->id->length;
    length -= tag->id->length;

    if (*auto_rats_enabled && nfc_a_supports_iso_dep(tag->select_response) && length > 0) {
        nfc_a_ats_t* ats = (nfc_a_ats_t*)response;
        if (length < (size_t)ats->length) {
            PN53_DEBUG("InList.a", "ATS cut off, expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                       (size_t)ats->length, length);
            return -EBADMSG;
        }
        PN53_DEBUG("InList.a", "ATS length=%" PRIuSIZE "\n", (size_t)ats->length);

        tag->ats = ats;
        target->managed_transport = PN53_MANAGED_TRANSPORT_ISO_DEP;

        response += ats->length;
        length -= ats->length;
    }
    return length;
}

static ssize_t pn53_parse_passive_target_b(uint8_t* response, size_t length, pn53_logical_target_t* target) {
    assert(target);
    target->super.tag.technology = NFC_TECHNOLOGY_B;
    nfc_b_tag_t* tag = &target->super.tag.b;

    if (length < (sizeof(nfc_b_polling_response_payload_t) + 2 /* 0x50, ATTRIB length */)) {
        PN53_DEBUG("InList.b", "target header too short\n");
        return -EBADMSG;
    }

    /* 0x50, the ATQB frame code / prefix is missing from the PN532/PN533 manuals.
     * Carl's PN532 on the Adafruit PN532 shield sends an addition 0x01 byte before 0x50. */

    uint8_t next = *response++;
    length -= 1;
    if (next == 1 || next == 2) {
        PN53_DEBUG("InList.b", "0x%02x in place of 0x50 ATQB prefix, skipping\n", next);
        next = *response++;
        length -= 1;
    }
    if (next != NFC_B_FRAME_CODE_POLLING_RESPONSE) {
        PN53_DEBUG("InList.b", "malformed, missing 0x50 ATQB prefix\n");
        return -EBADMSG;
    }

    tag->polling_response = (nfc_b_polling_response_payload_t*)response;

    response += sizeof(nfc_b_polling_response_payload_t);
    length -= sizeof(nfc_b_polling_response_payload_t);

    if (length > 0) {
        uint8_t attrib_length = *response++;
        length -= 1;

        if (length < (size_t)attrib_length) {
            PN53_DEBUG("InList.b", "ATTRIB cut off, "
                       "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                       (size_t)attrib_length, (size_t)length);
            return -EBADMSG;
        }

        if (attrib_length > 0) {
            tag->attrib_response_length = (size_t)attrib_length;
            tag->attrib = (nfc_b_attrib_response_t*)response;

            response += (size_t)attrib_length;
            length -= attrib_length;
        }
    }
    return length;
}

static ssize_t pn53_parse_passive_target_f(uint8_t* response, size_t length, pn53_logical_target_t* target, nfc_f_polling_additional_request_t* additional_request) {
    assert(target);
    target->super.tag.technology = NFC_TECHNOLOGY_F;
    nfc_f_tag_t* tag = &target->super.tag.f;

    if (length < (sizeof(nfc_f_polling_response_t) - sizeof(nfc_f_polling_response_payload_t))) {
        PN53_DEBUG("InList.f", "target header too short\n");
        return -EBADMSG;
    }

    nfc_f_polling_response_t* pol = (nfc_f_polling_response_t*)response;
    if (length < (size_t)pol->header.length) {
        PN53_DEBUG("InList.f", "POL_RES cut off, expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                   (size_t)pol->header.length, length);
        return -EBADMSG;
    }
    if (pol->header.code != NFC_F_PACKET_CODE_POLLING_RESPONSE) {
        PN53_DEBUG("InList.f", "invalid response code %02x\n", (uint8_t)pol->header.code);
        return -EBADMSG;
    }

    size_t expected = sizeof(nfc_f_polling_response_t);
    if (additional_request == NFC_F_POLLING_REQUEST_NOTHING) {
        expected -= sizeof(nfc_f_polling_response_payload_t);
    }

    if (pol->header.length != expected) {
        PN53_DEBUG("InList.f", "POL_RES had invalid length, "
                   "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                   expected, (size_t)pol->header.length);
        return -EBADMSG;
    }
    length -= expected;

    switch (*additional_request) {
        case NFC_F_POLLING_REQUEST_SYSTEM_CODE:
            tag->system_code = byteorder_lebuftohs(pol->payload);
            break;
        case NFC_F_POLLING_REQUEST_BITRATES:
            tag->bitrates = (nfc_bitrate_t)pol->payload[1] << 1;
            break;
        default: break;
    }

    tag->id = &pol->id;
    tag->pmm = &pol->pmm;
    return length;
}

ssize_t pn53_in_list_passive_targets_a(pn53_dev_t* dev, uint8_t max_targets, nfc_a_id_t* id, nfc_a_tag_t* tags, uint32_t timeout) {
    assert(max_targets > 0);
    assert(max_targets <= ARRAY_SIZE(dev->nfc_targets));
    assert(max_targets <= ((dev->model == PN53_MODEL_PN532) ? 2 : 1));

    ssize_t res = 0;
    uint8_t command[3 + 10 + 2] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)PN53_TECHNOLOGY_A_106K,
        NFC_A_UID_CASCADE_TAG, 0, 0, 0, NFC_A_UID_CASCADE_TAG
    };

    size_t length = 3;

    if (id && (id->length > 0)) {
        PN53_DEBUG("InList.a", "anticol with given id of length %u\n", id->length);
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

    dev->nfc_role = NFC_ROLE_INITIATOR;
    uint8_t* response;
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, length, &response, timeout)) < 0) {
        return res;
    }
    bool auto_rats_enabled = dev->nfc_parameters & (1 << 4);
    if ((res = pn53_parse_passive_targets(response, (size_t)res, dev->nfc_targets, max_targets, pn53_parse_passive_target_a, (void*)&auto_rats_enabled)) < 0) {
        return res;
    }
    for (uint8_t i = 0; i < max_targets; i += 1) {
        tags[i] = dev->nfc_targets[i].super.tag.a;
    }
    return res;
}

ssize_t pn53_in_list_passive_targets_b(pn53_dev_t* dev, uint8_t max_targets, nfc_bitrate_t bitrate,
                                uint8_t application_family, nfc_b_polling_method_t method,
                                nfc_b_tag_t* tags, uint32_t timeout
) {
    assert(max_targets > 0);
    assert(max_targets <= ARRAY_SIZE(dev->nfc_targets));
    assert(max_targets <= ((dev->model == PN53_MODEL_PN532) ? 2 : 1));

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
            PN53_DEBUG("InList.b", "unsupported bitrate %u kbit/s\n", nfc_bitrate_kbps(bitrate));
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

    dev->nfc_role = NFC_ROLE_INITIATOR;
    // Only append method if method argument != 0
    size_t length = sizeof(command) - ((method != 0) ? 0 : 1);
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, length, &response, timeout)) < 0) {
        return res;
    }
    if ((res = pn53_parse_passive_targets(response, (size_t)res, dev->nfc_targets, max_targets, pn53_parse_passive_target_b, NULL)) < 0) {
        return res;
    }
    for (uint8_t i = 0; i < max_targets; i += 1) {
        tags[i] = dev->nfc_targets[i].super.tag.b;
    }
    return res;
}

ssize_t pn53_in_list_passive_targets_f(pn53_dev_t* dev, uint8_t max_targets, nfc_bitrate_t bitrate,
    nfc_f_system_code_t system_code, nfc_f_polling_additional_request_t additional_request,
    uint8_t timeslots, nfc_f_tag_t* tags, uint32_t timeout
) {
    assert(max_targets > 0);
    assert(max_targets <= ARRAY_SIZE(dev->nfc_targets));
    assert(max_targets <= ((dev->model == PN53_MODEL_PN532) ? 2 : 1));

    ssize_t res = 0;
    pn53_technology_baudrate_t brty;
    switch (bitrate) {
        case NFC_BITRATE_212K: brty = PN53_TECHNOLOGY_F_212K; break;
        case NFC_BITRATE_424K: brty = PN53_TECHNOLOGY_F_424K; break;
        default:
            PN53_DEBUG("InList.f", "unsupported bitrate %u kbit/s\n", nfc_bitrate_kbps(bitrate));
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
        .system_code = htobe16(system_code),
        .additional_request = additional_request,
        .timeslots = timeslots,
    };

    dev->nfc_role = NFC_ROLE_INITIATOR;
    // Only append method if method argument != 0
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), &response, timeout)) < 0) {
        return res;
    }
    if ((res = pn53_parse_passive_targets(response, (size_t)res, dev->nfc_targets, max_targets, pn53_parse_passive_target_f, NULL)) < 0) {
        return res;
    }
    for (uint8_t i = 0; i < max_targets; i += 1) {
        tags[i] = dev->nfc_targets[i].super.tag.f;
    }
    return res;
}

ssize_t pn53_poll(nfcdev_t* dev, nfcdev_polling_config_t* config) {
    (void)dev;
    (void)config;
    return 0;
}

int pn53_deselect_reselect_release(pn53_dev_t *dev, uint8_t tg, pn53_command_code_t code) {
    assert(tg <= 2);
    uint8_t command[] = { (uint8_t)code, tg };
    ssize_t res = pn53_hci_transceive_command2_status_response(dev, command, sizeof(command), NULL, 0, dev->command_timeout);
    return (res < 0) ? (int)res : 0;
}

int pn53_fifo_receive_start_(pn53_dev_t* dev, bool transceive) {
    ssize_t res = 0;
    pn53_register_t reg[] = {
        {
            .address = PN53_REGISTER_FIFO_LEVEL,
            .value = PN53_REGISTER_FIFO_LEVEL_FLAG_FLUSH
        },
        {
            .address = PN53_REGISTER_COMMAND,
            .value = (uint8_t)(transceive ? PN53_CIU_COMMAND_TRANSCEIVE : PN53_CIU_COMMAND_RECEIVE)
        }
    };

    PN53_DEBUG("fifo.rx", "flushing FIFO, CIU_Command=%s\n", transceive ? "Transceive" : "Receive");
    if ((res = pn53_write_registers(dev, reg, ARRAY_SIZE(reg))) < 0) {
        return (int)res;
    }

    return 0;
}

static inline void _copy_register_into(uint8_t** cursor, pn53_register_address_t addr, uint8_t value) {
    memcpy(*cursor, &addr, sizeof(addr));
    *cursor += sizeof(addr);
    **cursor = value;
    *cursor += 1;
}

int pn53_fifo_transmit_write_(pn53_dev_t* dev, const uint8_t* tx, size_t length, uint8_t trailing_bit_count,
                               bool transceive, bool transceive_after_receiving
) {
    ssize_t res = 0;
    uint8_t* regs = (uint8_t*)&dev->connection.backing + PN35_FRAME_HEADER_NORMAL_COMMAND;

    uint8_t* values = NULL;
    pn53_register_address_t addrs[] = {
        PN53_REGISTER_FIFO_LEVEL,
        PN53_REGISTER_COMMON_IRQ,
    };

    uint8_t bit_framing = pn53_bitfield_create(PN53_REGISTER_BIT_FRAMING_MASK_TX_TRAILING_BIT_COUNT, trailing_bit_count);

    size_t max_fifo_bytes_at_once = (dev->connection.max_packet_length - PN53_FRAME_OVERHEAD_MIN_COMMAND) / sizeof(pn53_register_t) - 5;
    PN53_DEBUG("fifo.tx", "can max write %" PRIuSIZE " bytes into FIFO per HCI frame\n", max_fifo_bytes_at_once);

    size_t fifo_length = 0;
    bool transmitting = false;

    while (length > 0) {
        size_t can_write = MIN(max_fifo_bytes_at_once, MIN(length, CONFIG_PN53_FIFO_SIZE - fifo_length));
        assert(can_write > 0);

        PN53_DEBUG("fifo.tx", "writing %" PRIuSIZE " bytes into FIFO out of %" PRIuSIZE " remaining\n", can_write, length);
        uint8_t* cursor = regs;
        size_t reg_count = 0;

        if (!transmitting) {
            _copy_register_into(&cursor, PN53_REGISTER_COMMAND, PN53_CIU_COMMAND_IDLE);
            _copy_register_into(&cursor, PN53_REGISTER_FIFO_LEVEL, PN53_REGISTER_FIFO_LEVEL_FLAG_FLUSH);
            _copy_register_into(&cursor, PN53_REGISTER_BIT_FRAMING, bit_framing);
            reg_count += 3;
        }

        for (size_t i = 0; i < can_write; i += 1) {
            _copy_register_into(&cursor, PN53_REGISTER_FIFO_DATA, tx[i]);
        }
        reg_count += can_write;

        if (!transmitting) {
            if (!(transceive && transceive_after_receiving)) {
                _copy_register_into(&cursor, PN53_REGISTER_COMMAND,
                    transceive ? PN53_CIU_COMMAND_TRANSCEIVE : PN53_CIU_COMMAND_TRANSMIT);
                reg_count += 1;
            }
            if (transceive) {
                _copy_register_into( &cursor, PN53_REGISTER_BIT_FRAMING,
                    PN53_REGISTER_BIT_FRAMING_FLAG_START_SEND | bit_framing);
                reg_count += 1;
            }
            transmitting = true;
        }

        if ((res = pn53_write_registers_(dev, regs, reg_count)) < 0) {
            return res;
        }
        length -= can_write;
        tx += can_write;

        if (length > 0) {
            uint8_t refill_threshold = fifo_length + can_write - (can_write <= CONFIG_PN53_FIFO_TRANSMIT_REFILL_THRESHOLD ?
                1 : CONFIG_PN53_FIFO_TRANSMIT_REFILL_THRESHOLD);

            while (true) {
                if ((res = pn53_read_registers(dev, addrs, &values, ARRAY_SIZE(addrs))) < 0) {
                    return (int)res;
                }

                if ((values[1] & PN53_REGISTER_COMMON_IRQ_FLAG_TX_FINISHED)) {
                    PN53_DEBUG("fifo.tx", "TX finished, but still need to send %" PRIuSIZE " bytes"
                               " -- controller is transmitting faster than host can refill FIFO\n",
                               length);
                    return -EFBIG;
                }

                // If the controller has sent at least 5 bytes in the meantime, we refill it
                fifo_length = pn53_bitfield_get(values[0], PN53_REGISTER_FIFO_LEVEL_MASK_BYTE_COUNT);
                if (fifo_length <= refill_threshold) {
                    break;
                }
            }
        }
    }

    PN53_DEBUG("fifo.tx", "waiting for TX op to finish\n");
    while (true) {
        if ((res = pn53_read_registers(dev, addrs, &values, ARRAY_SIZE(addrs))) < 0) {
            return (int)res;
        }

        if ((values[1] & PN53_REGISTER_COMMON_IRQ_FLAG_TX_FINISHED) && pn53_bitfield_get(values[0], PN53_REGISTER_FIFO_LEVEL_MASK_BYTE_COUNT) == 0) {
            PN53_DEBUG("fifo.tx", "TX finished\n");
            break;
        }
    }
    return 0;
}

static void _fifo_receive_read_timeout(void* keep_polling) {
    *((bool*)keep_polling) = false;
}

ssize_t pn53_fifo_receive_read_(pn53_dev_t* dev, uint8_t* frame, size_t capacity, uint8_t* trailing_bit_count, uint32_t timeout_ms) {
    ssize_t res = 0;
    uint8_t* addrs = (uint8_t*)&dev->connection.backing + PN35_FRAME_HEADER_NORMAL_COMMAND;
    pn53_register_address_t fifo_data = PN53_REGISTER_FIFO_DATA;

    uint8_t* values = NULL;
    size_t received = 0;
    size_t fifo_byte_count = 0;

    pn53_register_address_t additional_regs[] = {
        PN53_REGISTER_FIFO_LEVEL,
        PN53_REGISTER_CONTROL,
        PN53_REGISTER_COMMON_IRQ,
    };

    size_t max_fifo_bytes_at_once = (dev->connection.max_packet_length - PN53_FRAME_OVERHEAD_MIN_COMMAND) / sizeof(pn53_register_address_t) - ARRAY_SIZE(additional_regs);
    PN53_DEBUG("fifo.rx", "can max read %" PRIuSIZE " bytes from FIFO per HCI frame\n", max_fifo_bytes_at_once);

    bool keep_polling = true;
    ztimer_t timeout = { .callback=_fifo_receive_read_timeout, .arg=&keep_polling };
    if (timeout_ms != PN53_FIFO_TIMEOUT_NEVER) {
        ztimer_set(ZTIMER_MSEC, &timeout, timeout_ms);
    }

    while (keep_polling) {
        size_t can_read = MIN(max_fifo_bytes_at_once, fifo_byte_count);
        PN53_DEBUG("fifo.rx", "reading %" PRIuSIZE " bytes out of %" PRIuSIZE " in FIFO\n", can_read, fifo_byte_count);
        for (size_t i = 0; i < can_read * sizeof(pn53_register_address_t); i += sizeof(pn53_register_address_t)) {
            memcpy(addrs + i, &fifo_data, sizeof(fifo_data));
        }
        memcpy(addrs + can_read * sizeof(pn53_register_address_t), additional_regs, sizeof(additional_regs));

        if ((res = pn53_read_registers_(dev, addrs, &values, can_read + ARRAY_SIZE(additional_regs))) < 0) {
            return res;
        }

        received += can_read;
        PN53_DEBUG("fifo.rx", "read %" PRIuSIZE ", totalling %" PRIuSIZE " bytes\n", can_read, received);

        if (capacity < can_read) {
            PN53_DEBUG("fifo.rx",
                       "fragment buffer has capacity of %" PRIuSIZE ", but need %" PRIuSIZE " bytes\n",
                       capacity, fifo_byte_count);
            res = -ENOBUFS;
            goto _return;
        } else {
            memcpy(frame, values, can_read);
            frame += can_read;
            capacity -= can_read;
        }

        uint8_t* additional_values = &values[can_read];
        fifo_byte_count = (size_t)pn53_bitfield_get(additional_values[0], PN53_REGISTER_FIFO_LEVEL_MASK_BYTE_COUNT);
        PN53_DEBUG("fifo.rx", "%" PRIuSIZE " bytes still in FIFO\n", fifo_byte_count);
        *trailing_bit_count = pn53_bitfield_get(additional_values[1], PN53_REGISTER_CONTROL_MASK_RX_TRAILING_BIT_COUNT);

        if ((additional_values[2] & PN53_REGISTER_COMMON_IRQ_FLAG_RX_FINISHED) && fifo_byte_count == 0) {
            PN53_DEBUG("fifo.rx", "RX finished\n");
            break;
        }
    }
    res = received;

_return:
    if (timeout_ms != PN53_FIFO_TIMEOUT_NEVER) {
        bool triggered = !ztimer_remove(ZTIMER_MSEC, &timeout);
        if (IS_ACTIVE(ENABLE_DEBUG) && triggered) {
            PN53_DEBUG("fifo.rx", "timed out after %" PRIu32 " ms\n", timeout_ms);
        }
    }
    return res;
}

static int _check_radio_config(pn53_dev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role) {
    assert(role == NFC_ROLE_INITIATOR || role == NFC_ROLE_TARGET);
    PN53_DEBUG("radio", "requested applying TX/RX radio config\n");
    PN53_DEBUG("radio", "tx=%c@%u rx=%c@%u\n",
               nfc_string_from_technology(tx->technology), nfc_bitrate_kbps(tx->bitrate),
               nfc_string_from_technology(rx->technology), nfc_bitrate_kbps(rx->bitrate));
    PN53_DEBUG("radio", "role=%s\n", role == NFC_ROLE_INITIATOR ? "initiator" : "target");
    nfc_bitrate_t max_bitrate = MAX(tx->bitrate, rx->bitrate);

    if (tx->generate_field && !rx->generate_field) {
        PN53_DEBUG("radio", "field_model=peers\n");

        switch (max_bitrate) {
            case NFC_BITRATE_106K:
            case NFC_BITRATE_212K:
            case NFC_BITRATE_424K:
                break;
            default:
                PN53_DEBUG("radio", "bitrate %u kbps unsupported in active mode\n", nfc_bitrate_kbps(max_bitrate));
                return -ENOTSUP;
        }
    } else {
        PN53_DEBUG("radio", "field_model=r/w+tag\n");

        switch (max_bitrate) {
            case NFC_BITRATE_106K:
            case NFC_BITRATE_212K:
            case NFC_BITRATE_424K:
            case NFC_BITRATE_848K:
            case NFC_BITRATE_1695K:
                break;
            case NFC_BITRATE_3390K:
                if (dev->model == PN53_MODEL_PN532) {
                    break;
                }
                __attribute__ ((fallthrough));
            default:
                PN53_DEBUG("radio", "bitrate %u kbps unsupported\n", nfc_bitrate_kbps(max_bitrate));
                return -ENOTSUP;
        }
        switch (role) {
            case NFC_ROLE_INITIATOR:
                if (tx->generate_field && rx->generate_field) {
                    break;
                } else {
                    PN53_DEBUG("radio", "gen_field={tx=%i rx=%i} not a standardised field model", tx->generate_field, rx->generate_field);
                    return -ENOTSUP;
                }
            case NFC_ROLE_TARGET:
                if (!tx->generate_field && !rx->generate_field) {
                    break;
                } else {
                    PN53_DEBUG("radio", "gen_field={tx=%i rx=%i} not a standardised field model", tx->generate_field, rx->generate_field);
                    return -ENOTSUP;
                }
            default:
                assert(false);
                UNREACHABLE();
                return -1;
        }
    }
    return 0;
}

int pn53_configure_radio_unchecked(pn53_dev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role) {
    uint8_t tx_mode = 0;
    uint8_t rx_mode = PN53_REGISTER_RX_MODE_IGNORE_INVALID;
    if (rx->options & NFCDEV_RX_MULTIPLE) {
        rx_mode |= PN53_REGISTER_RX_MODE_MULTIPLE_FRAMES;
    }

    uint8_t tx_framing = 0;
    uint8_t rx_framing = 0;
    if (tx->generate_field && !rx->generate_field) {
        tx_framing = 1;
        rx_framing = 1;
    } else {
        tx_framing = _tx_rx_framing(tx->technology);
        rx_framing = _tx_rx_framing(rx->technology);
    }

    tx_mode |= _tx_rx_mode(tx_framing, tx->bitrate);
    rx_mode |= _tx_rx_mode(rx_framing, rx->bitrate);

    uint8_t nfc_b_register = 0;
    if (tx->options & NFCDEV_NFC_B_WITHOUT_SOF) {
        nfc_b_register |= PN53_REGISTER_NFC_B_FLAG_TX_NO_SOF;
    }
    if (tx->options & NFCDEV_NFC_B_WITHOUT_EOF) {
        nfc_b_register |= PN53_REGISTER_NFC_B_FLAG_TX_NO_EOF;
    }

    pn53_register_t regs[] = {
        {
            .address = PN53_REGISTER_CONTROL,
            .value = role == NFC_ROLE_INITIATOR ? PN53_REGISTER_CONTROL_FLAG_INITIATOR : 0
        },
        {
            .address = PN53_REGISTER_TX_MODE,
            .value = tx_mode
        },
        {
            .address = PN53_REGISTER_RX_MODE,
            .value = rx_mode
        },
        {
            .address = PN53_REGISTER_NFC_B,
            .value = nfc_b_register
        }
    };

    int res = (int)pn53_write_registers(dev, regs, ARRAY_SIZE(regs));
    if (res < 0) {
        return res;
    }
    dev->nfc_role = role;
    dev->tx_mode = tx_mode;
    dev->rx_mode = rx_mode;
    return 0;
}

int nfcdev_configure_radio_pn53(nfcdev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role) {
    int res = 0;
    if ((res = _check_radio_config(dev->dev, tx, rx, role)) < 0) {
        return res;
    }
    return pn53_configure_radio_unchecked(dev->dev, tx, rx, role);
}

static int pn53_connect(pn53_dev_t* dev, nfcdev_connection_id_t connection_id) {
    int res = 0;
    if (connection_id > 1) {
        return -ENOENT;
    }

    if (!pn53_current_target(dev)) {
        return -ENOENT;
    }

    if (dev->nfc_targets[(connection_id + 1) % 2].super.parameters.polling.bitrate != NFC_BITRATE_UNSET) {
        // There is another target, need to select
        if ((res = pn53_reselect(dev, connection_id)) < 0) {
            return res;
        }
    }
    dev->nfc_current_connection = connection_id;
    return 0;
}

int nfcdev_connect_pn53(nfcdev_t* dev, nfcdev_connection_id_t connection_id) {
    return pn53_connect(dev->dev, connection_id);
}

#define PN53_INTERFACE_OP_TX (0b01)
#define PN53_INTERFACE_OP_RX (0b10)

static int _configure_rx_tx(pn53_dev_t* dev, uint8_t ops, uint8_t trailing_tx_bits, uint8_t tx_flags, uint8_t rx_flags, uint8_t manual_recv_flags) {
    assert((ops & PN53_INTERFACE_OP_TX) || (ops & PN53_INTERFACE_OP_RX));
    tx_flags &= PN53_REGISTER_TX_MODE_AUTO_CRC;
    rx_flags &= PN53_REGISTER_RX_MODE_AUTO_CRC;
    manual_recv_flags &= PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY;

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        PN53_DEBUG("radio", "requested crc={tx=%u rx=%u} trailing_bits=%u\n", tx_flags != 0, rx_flags != 0, trailing_tx_bits <= 7 ? trailing_tx_bits : 8);
    }

    pn53_register_t regs[4] = {};
    uint8_t* persisted[ARRAY_SIZE(regs)] = {};
    size_t reg_count = 0;

    if ((dev->manual_receiver & PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY) != manual_recv_flags
        && (pn53_bitfield_get(dev->tx_mode, PN53_REGISTER_TX_MODE_FRAMING) == _tx_rx_framing(NFC_TECHNOLOGY_A) ||
            pn53_bitfield_get(dev->rx_mode, PN53_REGISTER_RX_MODE_FRAMING) == _tx_rx_framing(NFC_TECHNOLOGY_A)
    )) {
        PN53_DEBUG("radio", "need to set ManualRCV (NFC-A parity)\n");
        persisted[reg_count] = &dev->manual_receiver;
        regs[reg_count++] = (pn53_register_t) {
            .address = PN53_REGISTER_MANUAL_RECEIVER,
            .value = (dev->manual_receiver &~ PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY) | manual_recv_flags
        };
    }

    // When TxMode is already as desired, we don't want to issue an additonal WriteRegister HCI
    // command when FIFO transmit is used as the transmit function write to BitFraming anyway
    // and thus can set the trailing bit count.
    if ((ops & PN53_INTERFACE_OP_TX) && (
            (dev->tx_mode & PN53_REGISTER_TX_MODE_AUTO_CRC) != tx_flags ||
            ((trailing_tx_bits <= 7)
                && pn53_bitfield_get(dev->bit_framing, PN53_REGISTER_BIT_FRAMING_MASK_TX_TRAILING_BIT_COUNT) != trailing_tx_bits))
    ) {
        PN53_DEBUG("radio", "need to set TxMode/BitFraming\n");

        if (trailing_tx_bits <= 7) {
            uint8_t bit_framing = dev->bit_framing;
            pn53_bitfield_set(&bit_framing, PN53_REGISTER_CONTROL_MASK_RX_TRAILING_BIT_COUNT, trailing_tx_bits);

            persisted[reg_count] = &dev->bit_framing;
            regs[reg_count++] = (pn53_register_t) {
                .address = PN53_REGISTER_BIT_FRAMING,
                .value = bit_framing
            };
        }

        persisted[reg_count] = &dev->tx_mode;
        regs[reg_count++] = (pn53_register_t) {
            .address = PN53_REGISTER_TX_MODE,
            .value = (dev->tx_mode &~ PN53_REGISTER_TX_MODE_AUTO_CRC) | tx_flags
        };
    }

    if ((ops & PN53_INTERFACE_OP_RX) && (dev->rx_mode & PN53_REGISTER_RX_MODE_AUTO_CRC) != rx_flags) {
        PN53_DEBUG("radio", "need to set RxMode\n");

        persisted[reg_count] = &dev->rx_mode;
        regs[reg_count++] = (pn53_register_t) {
            .address = PN53_REGISTER_RX_MODE,
            .value = (dev->rx_mode & ~PN53_REGISTER_RX_MODE_AUTO_CRC) | rx_flags
        };
    }

    assert(reg_count <= ARRAY_SIZE(regs));
    if (reg_count > 0) {
        PN53_DEBUG("radio", "configuring for interface\n");
        int res = 0;
        if ((res = (int)pn53_write_registers(dev, regs, reg_count)) < 0) {
            return res;
        }

        for (size_t i = 0; i < reg_count; i += 1) {
            assert(persisted[i]);
            *persisted[i] = regs[i].value;
        }
    }
    return 0;
}

static int pn53_send(pn53_dev_t* dev, const uint8_t* tx, nfc_frame_length_t length, nfcdev_interface_t interface) {
    int res = 0;
    if (!(interface == NFCDEV_INTERFACE_BITS || interface == NFCDEV_INTERFACE_FRAME)) {
        if (length.trailing_bits != NFC_TRAILING_BITS_ALL) {
            assert(false);
            PN53_DEBUG("txrx", "only full-byte boundaries support on interface\n");
        }
    }
    switch (interface) {
        case NFCDEV_INTERFACE_BITS:
            if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_TX,
                length.trailing_bits, 0, 0, PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY)) < 0) {
                PN53_DEBUG("txrx", "failed to configure radio\n");
                return res;
            }
            return pn53_fifo_transmit(dev, tx, length.bytes, length.trailing_bits);
        case NFCDEV_INTERFACE_FRAME:
            if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_TX, -1, 0, 0, 0)) < 0) {
                PN53_DEBUG("txrx", "failed to configure radio\n");
                return res;
            }
            return pn53_fifo_transmit(dev, tx, length.bytes, length.trailing_bits);
        case NFCDEV_INTERFACE_PACKET:
            if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_TX, -1,
                PN53_REGISTER_TX_MODE_AUTO_CRC, 0, 0)) < 0) {
                PN53_DEBUG("txrx", "failed to configure radio\n");
                return res;
            }
            return pn53_fifo_transmit(dev, tx, length.bytes, NFC_TRAILING_BITS_ALL);
        case NFCDEV_INTERFACE_ISO_DEP:
        case NFCDEV_INTERFACE_NFC_DEP:
            switch (dev->nfc_role) {
                case NFC_ROLE_INITIATOR:
                    PN53_DEBUG("txrx.packet", "use `transceive` instead as initiator for ISO-DEP/NFC-DEP\n");
                    return -ENOTSUP;
                case NFC_ROLE_TARGET:
                    pn53_logical_target_t* target = pn53_current_target(dev);
                    if (!target && target->managed_transport != (pn53_managed_target_transport_t)interface) {
                        PN53_DEBUG("txrx.dep", "controller did not activate interface, consider nfcdev_hostnfc\n");
                        return -ENOTCONN;
                    }
                    return -1;
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }
        default:
            PN53_DEBUG("tx", "interface not supported\n");
            return -ENOTSUP;
    }
}

static ssize_t pn53_transceive(pn53_dev_t* dev, const uint8_t* tx, nfc_frame_length_t length, uint8_t** rx, uint32_t timeout_ms, nfcdev_interface_t interface) {
    assert(tx);
    if (!(interface == NFCDEV_INTERFACE_BITS || interface == NFCDEV_INTERFACE_FRAME)) {
        if (length.trailing_bits != 0) {
            assert(false);
            PN53_DEBUG("txrx", "only full-byte boundaries support on interface\n");
        }
    }
    PN53_DEBUG("txrx", "transceiving, sending %" PRIuSIZE " bytes (%u trailing bits)\n",
               length.bytes, length.trailing_bits ? length.trailing_bits : 8);
    assert(length.bytes > 0);
    ssize_t res = 0;
    // TODO: use config variables to use FIFO transmit instead of incommunicatethru in each of the three BITS/FRAME/PACKET interfaces?
    // CONFIG_PN53_INITIATOR_TRANSCEIVE_USE_FIFO_INTERFACE_BITS
    // CONFIG_PN53_INITIATOR_TRANSCEIVE_USE_FIFO_INTERFACE_FRAME
    // CONFIG_PN53_INITIATOR_TRANSCEIVE_USE_FIFO_INTERFACE_PACKET
    // TODO: figure out target behaviour? TgSetData only after TgInitAsTarget or always possible?
    switch (interface) {
        case NFCDEV_INTERFACE_BITS:
            if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_TX | PN53_INTERFACE_OP_RX,
                length.trailing_bits, 0, 0, PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY)) < 0) {
                PN53_DEBUG("txrx", "failed to configure radio\n");
                return res;
            }
            return -ENOTSUP;
        case NFCDEV_INTERFACE_FRAME:
            if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_TX | PN53_INTERFACE_OP_RX,
                length.trailing_bits, 0, 0, 0)) < 0) {
                PN53_DEBUG("txrx", "failed to configure radio\n");
                return res;
            }
            switch (dev->nfc_role) {
                case NFC_ROLE_INITIATOR:
                    PN53_DEBUG("txrx.frame", "does this work without having selected a target?\n");
                    return pn53_in_communicate_thru(dev, tx, length.bytes, rx, timeout_ms);
                case NFC_ROLE_TARGET:
                    PN53_DEBUG("txrx.frame", "todo\n");
                    return -1;
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }
        case NFCDEV_INTERFACE_PACKET:
            if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_TX | PN53_INTERFACE_OP_RX, -1,
                PN53_REGISTER_TX_MODE_AUTO_CRC, PN53_REGISTER_RX_MODE_AUTO_CRC, 0)) < 0) {
                PN53_DEBUG("txrx", "failed to configure radio\n");
                return res;
            }
            switch (dev->nfc_role) {
                case NFC_ROLE_INITIATOR:
                    PN53_DEBUG("txrx.packet", "does this work without having selected a target?\n");
                    return pn53_in_communicate_thru(dev, tx, length.bytes, rx, timeout_ms);
                case NFC_ROLE_TARGET:
                    PN53_DEBUG("txrx.packet", "todo\n");
                    return -1;
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }
            PN53_DEBUG("txrx.packet", "todo\n");
            return -1;
        case NFCDEV_INTERFACE_ISO_DEP:
        case NFCDEV_INTERFACE_NFC_DEP:
            switch (dev->nfc_role) {
                case NFC_ROLE_INITIATOR: {
                    pn53_logical_target_t* target = pn53_current_target(dev);
                    if (!target && target->managed_transport != (pn53_managed_target_transport_t)interface) {
                        PN53_DEBUG("txrx.dep", "controller did not activate interface, consider nfcdev_hostnfc\n");
                        return -ENOTCONN;
                    }
                    return pn53_in_data_exchange(dev, tx, length.bytes, rx, timeout_ms);
                }
                case NFC_ROLE_TARGET:
                    PN53_DEBUG("txrx.dep", "todo\n");
                    return -1;
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }
        default:
            PN53_DEBUG("txrx", "interface not supported\n");
            return -ENOTSUP;
    }
}


int nfcdev_send_pn53(nfcdev_t* dev, const uint8_t* buffer, size_t length, nfcdev_interface_t interface) {
    return pn53_send(dev->dev, buffer, (nfc_frame_length_t) { .encoded = length }, interface);
}

ssize_t nfcdev_transceive_pn53(nfcdev_t* dev, const uint8_t* tx, size_t length, uint8_t** rx, uint32_t timeout_ms, nfcdev_interface_t interface) {
    return pn53_transceive(dev->dev, tx, (nfc_frame_length_t) { .encoded = length }, rx, timeout_ms, interface);
}

nfcdev_ops_t nfcdev_ops_pn53 = {
    .transceive = nfcdev_transceive_pn53,
    .send = nfcdev_send_pn53,
    .configure_radio = nfcdev_configure_radio_pn53,
};

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
