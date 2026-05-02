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
#include "unaligned.h"

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

#define PN53_DEBUG_HCI(...) DEBUG("pn53x.hci: " __VA_ARGS__)
#define PN53_DEBUG(command, ...) DEBUG("pn53x." command ": " __VA_ARGS__)
#define PN53_DEBUG_REGISTER(...) PN53_DEBUG("register", __VA_ARGS__)



#define PN53_BUS_I2C_ADDRESS           (0x24)

/* SPI bus parameters */
#define SPI_MODE                    (SPI_MODE_0)
#define SPI_CLK                     (SPI_CLK_1MHZ)

#define UART_BAUDRATE               (115200U)


ssize_t pn53_hci_transceive_command(pn53_dev_t* dev, iolist_t* command,
                            uint8_t** response, uint32_t timeout_ms) {
    assert(command);
    assert(command->iol_base);
    uint8_t code = *((uint8_t*)command->iol_base);

    ssize_t res = 0;
    // This may reference the internal buffer, so destroy that wisely before
    dev->nfc_target_first_rx = NULL;
    dev->nfc_target_first_rx_length = 0;
    if ((res = pn53_hci_transceive(&dev->connection, command, response, timeout_ms)) < 0) {
        return res;
    }

    if (res == 0) {
        return -PN53_ERROR_CONNECTION_RESPONSE_MISSING;
    }

    if (response && *response && **response != (code + 1)) {
        return -PN53_ERROR_CONNECTION_RESPONSE_MISMATCH;
    }
    if (response) {
        *response += 1;
    }
    return res - 1;
}

static ssize_t pn53_hci_transceive_command_status_response(pn53_dev_t* dev, iolist_t* command,
                                                uint8_t** response, size_t expected_response_length, uint32_t timeout_ms) {
    uint8_t* _response;
    ssize_t res = pn53_hci_transceive_command(dev, command, &_response, timeout_ms);
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
    ssize_t res = pn53_hci_transceive_command2(dev, command, sizeof(command),
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
    if ((res = pn53_hci_transceive_command2(dev, &command, sizeof(command), (uint8_t**)&fw, CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS)) < 0) {
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
    PN53_DEBUG_HCI("HCI initialized\n");

    pn53_hci_reset(&dev->connection);
    PN53_DEBUG_HCI("reset done\n");

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
    PN53_DEBUG("init", "resetting parameters\n");
    if ((res = pn53_set_parameters(dev, PN53_NFC_PARAMETER_TARGET_NFC_DEP_AUTO_HANDSHAKE)) < 0) {
        return res;
    }
    return 0;
}

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

    ssize_t res = pn53_hci_transceive_command2(dev, command, sizeof(command), NULL, CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS);
    if (res < 0) {
        return (int)res;
    }
    dev->nfc_parameters = parameters;
    return 0;
}

static void _format_bits_with_mask(uint8_t mask, uint8_t value, char* out_str) {
    for (int i = 7; i >= 0; i--) {
        int idx = 7 - i;
        if (i < 4) idx += 1; // Add space between nibbles

        if (mask & (1 << i)) {
            out_str[idx] = (value & (1 << i)) ? '1' : '0';
        } else {
            out_str[idx] = '.';
        }
    }
}

#define PN53_DEBUG_REGISTER_VALUE(addr, name, value) \
    PN53_DEBUG_REGISTER("0x%04X (%s): 0x%02X\n", be16toh(addr), (name), (value))

static void _debug_mask(char* pattern, const char* label, uint8_t mask, uint8_t value) {
    _format_bits_with_mask(mask, value, pattern);
    PN53_DEBUG_REGISTER("[=] %s - %u (%s)\n",
                        pattern, pn53_bitfield_get(value, mask), label);
}

#define PN53_DEBUG_REGISTER_SYMBOL(mask, label, value) \
    _debug_mask(mask_str, (label), (mask), (value))

static void _debug_register(pn53_register_address_t addr, uint8_t value);

ssize_t pn53_read_registers_(pn53_dev_t *dev, void* registers, uint8_t** values, size_t count) {
#if IS_ACTIVE(CONFIG_PN53_DEBUG_REGISTERS)
    static pn53_register_address_t _debug_addrs[32];
    memcpy(_debug_addrs, registers, MIN(count, ARRAY_SIZE(_debug_addrs)) * sizeof(pn53_register_address_t));
#endif
    PN53_DEBUG("ReadRegister", "%" PRIuSIZE "\n", count);
    uint8_t code = (uint8_t)PN53_COMMAND_READ_REGISTERS;
    iolist_t addrs = __IOLIST(registers, count * sizeof(pn53_register_address_t), NULL);
    iolist_t command = __IOLIST(&code, 1, &addrs);
    uint8_t* _values = NULL;
    ssize_t res = pn53_hci_transceive_command_status_response(dev, &command, &_values, count, dev->command_timeout);
    if (values) {
        *values = _values;
    }
    if (res < 0) {
        return res;
    }
#if IS_ACTIVE(CONFIG_PN53_DEBUG_REGISTERS)
    for (size_t i = 0; i < MIN(count, ARRAY_SIZE(_debug_addrs)); i += 1) {
        _debug_register(_debug_addrs[i], _values[i]);
    }
#endif
    return res;
}

int pn53_write_registers_(pn53_dev_t *dev, void* registers, size_t count) {
    PN53_DEBUG("WriteRegister", "%" PRIuSIZE "\n", count);
    if (IS_ACTIVE(CONFIG_PN53_DEBUG_REGISTERS)) {
        uint8_t* cursor = registers;
        for (size_t i = 0; i < count; i += 1) {
            pn53_register_address_t addr = unaligned_get_u16(cursor);
            cursor += sizeof(pn53_register_address_t);
            _debug_register(addr, *cursor++);
        }
    }

    int res = -1;
    uint8_t code = (uint8_t)PN53_COMMAND_WRITE_REGISTERS;
    iolist_t regs = __IOLIST(registers, count * sizeof(pn53_register_t), NULL);
    iolist_t command = __IOLIST(&code, 1, &regs);
    uint8_t* response;
    if ((res = pn53_hci_transceive_command(dev, &command, &response, dev->command_timeout)) < 0) {
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
    return pn53_hci_transceive_command2(dev, &command, sizeof(command), (uint8_t**)response, dev->command_timeout);
}

int pn53_write_gpio(pn53_dev_t* dev, pn53_write_gpio_payload_t payload) {
    uint8_t command[] = { (uint8_t)PN53_COMMAND_WRITE_GPIO, payload.p3.raw, payload.p7.raw };
    return (int)pn53_hci_transceive_command2(dev, command, sizeof(command), NULL, dev->command_timeout);
}

int pn53_get_general_status(pn53_dev_t* dev, pn53_general_status_t* status) {
    uint8_t command = (uint8_t)PN53_COMMAND_GET_GENERAL_STATUS;
    uint8_t* response;
    ssize_t res = 0;
    if ((res = pn53_hci_transceive_command2(dev, &command, sizeof(command), &response, dev->command_timeout)) < 0) {
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

            status->targets[i].mode = NFC_FIELD_MODE_READER_WRITER_TAG;
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
                    status->targets[i].mode = NFC_FIELD_MODE_PEERS;
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
        case PN53_RF_CONFIGURATION_ITEM_MAXIMUM_RETRIES: return sizeof(rf_config->max_retries);
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
    int res = (int)pn53_hci_transceive_command(dev, &command, NULL, dev->command_timeout);
    return res < 0 ? res : 0;
}

int pn53_set_field_enablement(pn53_dev_t* dev, bool intent_to_enable, bool collision_avoidance) {
    PN53_DEBUG("RFConfiguration", "field=%u rfca=%u\n", intent_to_enable, collision_avoidance);
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_RF_CONFIGURATION,
        (uint8_t)PN53_RF_CONFIGURATION_ITEM_RF_FIELD,
        (intent_to_enable & 1) | ((collision_avoidance & 1) << 1)
    };
    int res = (int)pn53_hci_transceive_command2(dev, command, sizeof(command), NULL, dev->command_timeout);
    return res < 0 ? res : 0;
}

ssize_t pn53_in_communicate_thru(pn53_dev_t* dev, const iolist_t* tx, uint8_t** rx, uint32_t timeout_ms) {
    assert(dev->nfc_role == NFC_ROLE_TARGET);
    assert(!tx || iolist_size(tx) <= pn53_max_command_payload_length(dev)); /* 262 */
    uint8_t code = (uint8_t)PN53_COMMAND_IN_COMMUNICATE_THRU;
    iolist_t _command = __IOLIST(&code, 1, (iolist_t*)tx);
    uint8_t* _response = NULL;
    ssize_t res = pn53_hci_transceive_command(dev, &_command, &_response, timeout_ms);
    if (res > 0) {
        assert(_response);
        pn53_status_code_t status = pn53_status_code(*_response++);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
        res -= 1;
    }
    if (rx) {
        *rx = _response;
    }
    return res;
}

ssize_t pn53_in_data_exchange(pn53_dev_t* dev, nfcdev_connection_id_t id, uint8_t* status_byte, const iolist_t* tx, uint8_t** rx, uint32_t timeout_ms) {
    assert(dev->nfc_role == NFC_ROLE_TARGET);
    assert(!tx || iolist_size(tx) <= pn53_max_exchange_payload_length(dev)); /* 262 */
    if (!pn53_target(dev, id)) {
        return -ENOTCONN;
    }
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_IN_COMMUNICATE_THRU,
        (uint8_t)id
    };
    if (status_byte) {
        command[1] |= ((*status_byte & PN53_STATUS_BYTE_FLAG_MORE) != 0) & 1;
    }
    iolist_t _command = __IOLIST(command, sizeof(command), (iolist_t*)tx);
    uint8_t* _response = NULL;
    ssize_t res = pn53_hci_transceive_command(dev, &_command, &_response, timeout_ms);
    if (res > 0) {
        assert(_response);
        pn53_status_code_t status = pn53_status_code(*_response);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
        if (status_byte) {
            *status_byte = *_response;
        }
        res -= 1;
        _response += 1;
    }
    if (rx) {
        *rx = _response;
    }
    return res;
}

int pn53_deselect_reselect_release(pn53_dev_t *dev, uint8_t tg, pn53_command_code_t code) {
    assert(tg <= 2);
    uint8_t command[] = { (uint8_t)code, tg };
    ssize_t res = pn53_hci_transceive_command2_status_response(dev, command, sizeof(command), NULL, 0, dev->command_timeout);
    return (res < 0) ? (int)res : 0;
}

int nfcdev_connect_pn53(nfcdev_t* nfcdev, nfcdev_connection_id_t tg) {
    pn53_dev_t* dev = nfcdev->dev;
    if (dev->nfc_role != NFC_ROLE_INITIATOR) {
        return 0;
    }
    if (tg == NFCDEV_CONNECTION_ID_CURRENT) {
        if (!pn53_current_target_id(dev)) {
            return -ENOTCONN;
        }
        return 0;
    }
    if (!tg || !pn53_target(dev, tg)) {
        PN53_DEBUG("connect", "Tg=%u unknown\n", tg);
        return -ENOENT;
    }
    if (pn53_current_target_id(dev) == tg) {
        PN53_DEBUG("connect", "Tg=%u is current\n", tg);
        return 0;
    }
    PN53_DEBUG("connect", "Tg=%u\n", tg);
    return pn53_reselect(dev, tg);
}

int nfcdev_disconnect_pn53(nfcdev_t* nfcdev, nfcdev_connection_id_t tg) {
    pn53_dev_t* dev = nfcdev->dev;
    if (dev->nfc_role != NFC_ROLE_INITIATOR) {
        return 0;
    }
    if (tg == NFCDEV_CONNECTION_ID_CURRENT) {
        if (!pn53_current_target_id(dev)) {
            return -ENOTCONN;
        }
        tg = pn53_current_target_id(dev);
    }
    else if (tg == _NFCDEV_DISCONNECT_ALL) {
        PN53_DEBUG("disconnect", "all Tg\n");
        return pn53_deselect_all(dev);
    }
    if (!tg || !pn53_target(dev, tg)) {
        PN53_DEBUG("disconnect", "Tg=%u unknown\n", tg);
        return -ENOENT;
    }
    PN53_DEBUG("disconnect", "Tg=%u\n", tg);
    return pn53_deselect(dev, tg);
}

int pn53_tg_set_general_bytes(pn53_dev_t* dev, const iolist_t* general_bytes) {
    assert(dev->nfc_role == NFC_ROLE_TARGET);
    assert(pn53_emulated_target(dev)->managed_transport == PN53_MANAGED_TRANSPORT_NFC_DEP);
    assert(!general_bytes || iolist_size(general_bytes) <= 47);

    uint8_t code = (uint8_t)PN53_COMMAND_TG_SET_GENERAL_BYTES;
    iolist_t _command = { .iol_base = (void*)&code, .iol_len = 1, .iol_next = (iolist_t*)general_bytes };
    uint8_t* response;
    ssize_t res = pn53_hci_transceive_command(dev, &_command, &response, dev->command_timeout);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*response);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
    }
    return res < 0 ? (int)res : 0;
}

ssize_t pn53_tg_get_data(pn53_dev_t* dev, uint8_t** rx, uint32_t timeout_ms) {
    assert(dev->nfc_role == NFC_ROLE_TARGET);
    assert(pn53_emulated_target(dev)->managed_transport != PN53_MANAGED_TRANSPORT_NONE);

    uint8_t code = (uint8_t)PN53_COMMAND_TG_GET_DATA;
    uint8_t* response = NULL;
    ssize_t res = pn53_hci_transceive_command2(dev, &code, 1, &response, timeout_ms);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*response++);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
        res -= 1;
    }
    if (rx) {
        *rx = response;
    }
    return res;
}

int pn53_tg_set_data(pn53_dev_t* dev, const iolist_t* tx) {
    assert(dev->nfc_role == NFC_ROLE_TARGET);
    assert(pn53_emulated_target(dev)->managed_transport != PN53_MANAGED_TRANSPORT_NONE);
    assert(!tx || iolist_size(tx) <= pn53_max_exchange_payload_length(dev)); /* 262 */

    uint8_t code = (uint8_t)PN53_COMMAND_TG_SET_DATA;
    iolist_t _command = { .iol_base = &code, .iol_len = 1, .iol_next = (iolist_t*)tx };
    uint8_t* response = NULL;
    ssize_t res = pn53_hci_transceive_command(dev, &_command, &response, dev->command_timeout);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*response);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
    }
    return res < 0 ? (int)res : 0;
}

int pn53_tg_set_meta_data(pn53_dev_t* dev, const iolist_t* tx) {
    assert(dev->nfc_role == NFC_ROLE_TARGET);
    assert(pn53_emulated_target(dev)->managed_transport == PN53_MANAGED_TRANSPORT_NFC_DEP);
    assert(!tx || iolist_size(tx) <= pn53_max_exchange_payload_length(dev)); /* 262 */

    uint8_t code = (uint8_t)PN53_COMMAND_TG_SET_META_DATA;
    iolist_t _command = { .iol_base = &code, .iol_len =1, .iol_next = (iolist_t*)tx };
    uint8_t* response = NULL;
    ssize_t res = pn53_hci_transceive_command(dev, &_command, &response, dev->command_timeout);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*response);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
    }
    return res < 0 ? (int)res : 0;
}

ssize_t pn53_tg_get_initiator_command(pn53_dev_t* dev, uint8_t** rx, uint32_t timeout_ms) {
    assert(dev->nfc_role == NFC_ROLE_TARGET);

    uint8_t code = (uint8_t)PN53_COMMAND_TG_GET_INITIATOR_COMMAND;
    uint8_t* response = NULL;
    ssize_t res = pn53_hci_transceive_command2(dev, &code, 1, &response, timeout_ms);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*response++);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
        res -= 1;
    }
    if (rx) {
        *rx = response;
    }
    return res;
}

int pn53_tg_response_to_initiator(pn53_dev_t* dev, const iolist_t* tx) {
    assert(dev->nfc_role == NFC_ROLE_TARGET);
    assert(iolist_size(tx) <= pn53_max_exchange_payload_length(dev)); /* 262 */

    uint8_t code = (uint8_t)PN53_COMMAND_TG_RESPONSE_TO_INITIATOR;
    iolist_t _command = { .iol_base = &code, .iol_len = 1, .iol_next = (iolist_t*)tx };
    uint8_t* response = NULL;
    ssize_t res = pn53_hci_transceive_command(dev, &_command, &response, dev->command_timeout);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*response);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
    }
    return res < 0 ? (int)res : 0;
}

int pn53_tg_get_target_status(pn53_dev_t* dev, pn53_target_status_t* status, nfc_bitrate_t* down, nfc_bitrate_t* up) {
    uint8_t command = (uint8_t)PN53_COMMAND_TG_GET_TARGET_STATUS;
    uint8_t* response = NULL;
    ssize_t res = pn53_hci_transceive_command2(dev, &command, 1, &response, dev->command_timeout);
    if (res != 2) {
        return -EBADMSG;
    }
    status->raw = *response++;
    *down = nfc_bitrate_from_index(*response >> 4);
    *up = nfc_bitrate_from_index(*response & 0x0F);
    return 0;
}

extern ssize_t nfcdev_poll_pn53(nfcdev_t* nfcdev, const nfcdev_polling_config_t* config, nfc_target_t* targets, nfcdev_connection_id_t* connection_ids, size_t max_targets);

extern int nfcdev_listen_pn53(nfcdev_t* dev, const nfcdev_listening_config_t* config, nfc_target_t* target, uint32_t timeout_ms);

extern int nfcdev_send_pn53(nfcdev_t* nfcdev, const iolist_t* tx, nfcdev_nfio_flags_t flags);

extern ssize_t nfcdev_receive_pn53(nfcdev_t* nfcdev,
                                   uint8_t** rx, size_t capacity,
                                   uint32_t rx_timeout_ms, nfcdev_nfio_flags_t flags
);

extern ssize_t nfcdev_transceive_pn53(nfcdev_t* nfcdev,
                                      const iolist_t* tx,
                                      uint8_t** rx, size_t capacity,
                                      uint32_t rx_timeout_ms, nfcdev_nfio_flags_t flags
);

extern int nfcdev_configure_radio_pn53(nfcdev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role);

nfcdev_ops_t nfcdev_ops_pn53 = {
    .poll             = nfcdev_poll_pn53,
    .connect          = nfcdev_connect_pn53,

    .listen           = nfcdev_listen_pn53,

    .transceive       = nfcdev_transceive_pn53,
    .send             = nfcdev_send_pn53,
    .receive          = nfcdev_receive_pn53,

    .configure_radio  = nfcdev_configure_radio_pn53,
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

static void _debug_register(pn53_register_address_t addr, uint8_t value) {
    char mask_str[] = ".... ....";
    switch (addr) {
        case PN53_REGISTER_CONTROL_SWITCH_RNG:
            PN53_DEBUG_REGISTER_VALUE(addr, "ControlSwitchRng", value);
            break;
        case PN53_REGISTER_MODE:
            PN53_DEBUG_REGISTER_VALUE(addr, "Mode", value);
            break;
        case PN53_REGISTER_TX_MODE:
            PN53_DEBUG_REGISTER_VALUE(addr, "TxMode", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_MODE_AUTO_CRC, "AUTO_CRC", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_MODE_BITRATE_INDEX, "BITRATE_INDEX", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_MODE_INVERTED, "INVERTED", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_MODE_MIX, "MIX", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_MODE_FRAMING, "FRAMING", value);
            break;
        case PN53_REGISTER_RX_MODE:
            PN53_DEBUG_REGISTER_VALUE(addr, "RxMode", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_RX_MODE_AUTO_CRC, "AUTO_CRC", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_RX_MODE_BITRATE_INDEX, "BITRATE_INDEX", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_RX_MODE_IGNORE_INVALID, "IGNORE_INVALID", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_RX_MODE_MULTIPLE_FRAMES, "MULTIPLE_FRAMES", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_RX_MODE_FRAMING, "FRAMING", value);
            break;
        case PN53_REGISTER_TX_CONTROL:
            PN53_DEBUG_REGISTER_VALUE(addr, "TxControl", value);
            break;
        case PN53_REGISTER_TX_AUTO:
            PN53_DEBUG_REGISTER_VALUE(addr, "TxAuto", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_AUTO_FLAG_TURN_OFF_FIELD_AFTER_TX, "TURN_OFF_FIELD_AFTER_TX", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_AUTO_FLAG_FORCE_100_PERCENT_ASK, "FORCE_100_PERCENT_ASK", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_AUTO_FLAG_WAKE_UP_BY_FIELD, "WAKE_UP_BY_FIELD", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_AUTO_FLAG_COLLISION_AVOIDANCE, "COLLISION_AVOIDANCE", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_AUTO_FLAG_INITIAL_FIELD_ON, "INITIAL_FIELD_ON", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_AUTO_FLAG_TX1_FIELD_ON, "TX1_FIELD_ON", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_TX_AUTO_FLAG_TX2_FIELD_ON, "TX2_FIELD_ON", value);
            break;
        case PN53_REGISTER_TX_SELECTOR:
            PN53_DEBUG_REGISTER_VALUE(addr, "TxSel", value);
            break;
        case PN53_REGISTER_RX_SELECTOR:
            PN53_DEBUG_REGISTER_VALUE(addr, "RxSel", value);
            break;
        case PN53_REGISTER_RX_THRESHOLD:
            PN53_DEBUG_REGISTER_VALUE(addr, "RxThreshold", value);
            break;
        case PN53_REGISTER_DEMODULATOR:
            PN53_DEBUG_REGISTER_VALUE(addr, "Demod(ulator)", value);
            break;
        case PN53_REGISTER_NFC_F_1:
            PN53_DEBUG_REGISTER_VALUE(addr, "FelNFC1 (NFC-F #1)", value);
            break;
        case PN53_REGISTER_NFC_F_2:
            PN53_DEBUG_REGISTER_VALUE(addr, "FelNFC2 (NFC-F #2)", value);
            break;
        case PN53_REGISTER_NFC_A:
            PN53_DEBUG_REGISTER_VALUE(addr, "MifNFC (NFC-A)", value);
            break;
        case PN53_REGISTER_MANUAL_RECEIVER:
            PN53_DEBUG_REGISTER_VALUE(addr, "ManualRCV", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY, "TX_RX_MANUAL_PARITY", value);
            break;
        case PN53_REGISTER_NFC_B:
            PN53_DEBUG_REGISTER_VALUE(addr, "TypeB (NFC-B)", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_NFC_B_FLAG_RX_REQUIRE_SOF, "RX_REQUIRE_SOF", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_NFC_B_FLAG_RX_REQUIRE_EOF, "RX_REQUIRE_EOF", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_NFC_B_FLAG_EOF_SOF_WIDTH_MAX, "EOF_SOF_WIDTH_MAX", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_NFC_B_FLAG_TX_NO_SOF, "TX_NO_SOF", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_NFC_B_FLAG_TX_NO_EOF, "TX_NO_EOF", value);
            break;
        case PN53_REGISTER_CRC_HIGH:
            PN53_DEBUG_REGISTER_VALUE(addr, "CRC (high)", value);
            break;
        case PN53_REGISTER_CRC_LOW:
            PN53_DEBUG_REGISTER_VALUE(addr, "CRC (low)", value);
            break;
        case PN53_REGISTER_MILLER_MODULATION_WIDTH:
            PN53_DEBUG_REGISTER_VALUE(addr, "ModWidth", value);
            break;
        case PN53_REGISTER_TX_BIT_PHASE:
            PN53_DEBUG_REGISTER_VALUE(addr, "TxBitPhase", value);
            break;
        case PN53_REGISTER_RF_CONFIG:
            PN53_DEBUG_REGISTER_VALUE(addr, "RF Config", value);
            break;
        case PN53_REGISTER_CONDUCTANCE_N_DRIVER_OFF:
            PN53_DEBUG_REGISTER_VALUE(addr, "GsNOff (Conductance N off)", value);
            break;
        case PN53_REGISTER_CONDUCTANCE_N_DRIVER_ON:
            PN53_DEBUG_REGISTER_VALUE(addr, "GsNOn (Conductance N on)", value);
            break;
        case PN53_REGISTER_CONDUCTANCE_P_DRIVER_NO_MODULATION:
            PN53_DEBUG_REGISTER_VALUE(addr, "CWGsP (Conductance P mod=0)", value);
            break;
        case PN53_REGISTER_CONDUCTANCE_P_DRIVER_MODULATION:
            PN53_DEBUG_REGISTER_VALUE(addr, "ModGsP (Conductance P mod=1)", value);
            break;
        case PN53_REGISTER_TIMER_MODE:
            PN53_DEBUG_REGISTER_VALUE(addr, "TMode", value);
            break;
        case PN53_REGISTER_TIMER_PRESCALER:
            PN53_DEBUG_REGISTER_VALUE(addr, "TPrescaler", value);
            break;
        case PN53_REGISTER_TIMER_RELOAD_VALUE_HIGH:
            PN53_DEBUG_REGISTER_VALUE(addr, "TReloadVal (high)", value);
            break;
        case PN53_REGISTER_TIMER_RELOAD_VALUE_LOW:
            PN53_DEBUG_REGISTER_VALUE(addr, "TReloadVal (low)", value);
            break;
        case PN53_REGISTER_TIMER_COUNTER_VALUE_HIGH:
            PN53_DEBUG_REGISTER_VALUE(addr, "TCounterVal (high)", value);
            break;
        case PN53_REGISTER_TIMER_COUNTER_VALUE_LOW:
            PN53_DEBUG_REGISTER_VALUE(addr, "TCounterVal (low)", value);
            break;
        case PN53_REGISTER_TEST_SELECTION1:
            PN53_DEBUG_REGISTER_VALUE(addr, "TestSel1", value);
            break;
        case PN53_REGISTER_TEST_SELECTION2:
            PN53_DEBUG_REGISTER_VALUE(addr, "TestSel2", value);
            break;
        case PN53_REGISTER_TEST_PIN_ENABLE:
            PN53_DEBUG_REGISTER_VALUE(addr, "TestPinEn", value);
            break;
        case PN53_REGISTER_TEST_PIN_VALUE:
            PN53_DEBUG_REGISTER_VALUE(addr, "TestPinValue", value);
            break;
        case PN53_REGISTER_TEST_BUS:
            PN53_DEBUG_REGISTER_VALUE(addr, "TestBus", value);
            break;
        case PN53_REGISTER_TEST_AUTO:
            PN53_DEBUG_REGISTER_VALUE(addr, "AutoTest", value);
            break;
        case PN53_REGISTER_VERSION:
            PN53_DEBUG_REGISTER_VALUE(addr, "Version", value);
            break;
        case PN53_REGISTER_TEST_ANALOG:
            PN53_DEBUG_REGISTER_VALUE(addr, "AnalogTest", value);
            break;
        case PN53_REGISTER_TEST_DAC1:
            PN53_DEBUG_REGISTER_VALUE(addr, "TestDAC1", value);
            break;
        case PN53_REGISTER_TEST_DAC2:
            PN53_DEBUG_REGISTER_VALUE(addr, "TestDAC2", value);
            break;
        case PN53_REGISTER_TEST_ADCV:
            PN53_DEBUG_REGISTER_VALUE(addr, "TestADC", value);
            break;
        case PN53_REGISTER_RF_LEVEL_DETECTOR:
            PN53_DEBUG_REGISTER_VALUE(addr, "RF Level Detector", value);
            break;
        case PN53_REGISTER_SECURE_IC_CLOCK:
            PN53_DEBUG_REGISTER_VALUE(addr, "Secure IC Clokc", value);
            break;
        case PN53_REGISTER_COMMAND:
            PN53_DEBUG_REGISTER_VALUE(addr, "CIU Command", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMAND_MASK_COMMAND, "COMMAND", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMAND_FLAG_POWER_DOWN, "POWER_DOWN", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMAND_FLAG_RECV_OFF, "RECV_OFF", value);
            break;
        case PN53_REGISTER_COMMON_INTERRUPT_ENABLE:
            PN53_DEBUG_REGISTER_VALUE(addr, "Comm(on)I(nterrupt)En(able)", value);
            break;
        case PN53_REGISTER_DIVERSE_INTERRUPT_ENABLE:
            PN53_DEBUG_REGISTER_VALUE(addr, "Div(erse)I(nterrupt)En(able)", value);
            break;
        case PN53_REGISTER_COMMON_IRQ:
            PN53_DEBUG_REGISTER_VALUE(addr, "Comm(on)Irq", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMON_IRQ_FLAG_TX_FINISHED, "TX_FINISHED", value);
            // Note: RX_FINISHED and IDLE share the 0x20 bit
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMON_IRQ_FLAG_RX_FINISHED, "RX_FINISHED_OR_IDLE", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMON_IRQ_FLAG_HI_ALERT, "HI_ALERT", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMON_IRQ_FLAG_LO_ALERT, "LO_ALERT", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMON_IRQ_FLAG_ERROR, "ERROR", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_COMMON_IRQ_FLAG_TIMER, "TIMER", value);
            break;
        case PN53_REGISTER_DIVERSE_IRQ:
            PN53_DEBUG_REGISTER_VALUE(addr, "Div(erse)Irq", value);
            break;
        case PN53_REGISTER_ERROR:
            PN53_DEBUG_REGISTER_VALUE(addr, "Error", value);
            break;
        case PN53_REGISTER_STATUS1:
            PN53_DEBUG_REGISTER_VALUE(addr, "Status1", value);
            break;
        case PN53_REGISTER_STATUS2:
            PN53_DEBUG_REGISTER_VALUE(addr, "Status2", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_STATUS2_FLAG_CRYPTO1_ENABLED, "CRYPTO1", value);
            break;
        case PN53_REGISTER_FIFO_DATA:
            PN53_DEBUG_REGISTER_VALUE(addr, "FIFOData", value);
            break;
        case PN53_REGISTER_FIFO_LEVEL:
            PN53_DEBUG_REGISTER_VALUE(addr, "FIFOLevel", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_FIFO_LEVEL_MASK_BYTE_COUNT, "BYTE_COUNT", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_FIFO_LEVEL_FLAG_FLUSH, "FLUSH", value);
            break;
        case PN53_REGISTER_FIFO_WATER_LEVEL:
            PN53_DEBUG_REGISTER_VALUE(addr, "WaterLevel", value);
            break;
        case PN53_REGISTER_CONTROL:
            PN53_DEBUG_REGISTER_VALUE(addr, "Control", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_CONTROL_FLAG_TIMER_STOP, "TIMER_STOP", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_CONTROL_FLAG_TIMER_START, "TIMER_START", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_CONTROL_FLAG_COPY_NFC_DEP_ID_TO_FIFO, "COPY_NFC_DEP_ID_TO_FIFO", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_CONTROL_FLAG_INITIATOR, "INITIATOR", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_CONTROL_MASK_RX_TRAILING_BIT_COUNT, "RX_TRAILING_BIT_COUNT", value);
            break;
        case PN53_REGISTER_BIT_FRAMING:
            PN53_DEBUG_REGISTER_VALUE(addr, "BitFraming", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_BIT_FRAMING_FLAG_START_SEND, "START_SEND", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_BIT_FRAMING_MASK_RX_BIT_OFFSET, "RX_BIT_OFFSET", value);
            PN53_DEBUG_REGISTER_SYMBOL(PN53_REGISTER_BIT_FRAMING_MASK_TX_TRAILING_BIT_COUNT, "TX_TRAILING_BIT_COUNT", value);
            break;
        case PN53_REGISTER_COLLISION:
            PN53_DEBUG_REGISTER_VALUE(addr, "Coll(ision)", value);
            break;
        case PN53_REGISTER_SFR_P3:
            PN53_DEBUG_REGISTER_VALUE(addr, "SFR_P3", value);
            break;
        case PN53_REGISTER_SFR_P3CFGA:
            PN53_DEBUG_REGISTER_VALUE(addr, "SFR_P3CFGA", value);
            break;
        case PN53_REGISTER_SFR_P3CFGB:
            PN53_DEBUG_REGISTER_VALUE(addr, "SFR_P3CFGB", value);
            break;
        case PN53_REGISTER_SFR_P7CFGA:
            PN53_DEBUG_REGISTER_VALUE(addr, "SFR_P7CFGA", value);
            break;
        case PN53_REGISTER_SFR_P7CFGB:
            PN53_DEBUG_REGISTER_VALUE(addr, "SFR_P7CFGB", value);
            break;
        case PN53_REGISTER_SFR_P7:
            PN53_DEBUG_REGISTER_VALUE(addr, "SFR_P7", value);
            break;
        default:
            PN53_DEBUG_REGISTER("[=] 0x%04X: 0x%02X\n", addr, value);
            break;
    }
}
