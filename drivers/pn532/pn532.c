/*
 * Copyright (C) 2026 Carl Seifert
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#define ENABLE_DEBUG CONFIG_PN53_DEBUG
#include "debug.h"

#include "pn532.h"

#define PN532_DEBUG(command, ...) DEBUG("pn532." command ": " __VA_ARGS__)

int pn532_init(pn532_dev_t* dev, const pn532_connection_config_t* config) {
    int res = 0;
    if ((res = pn53_init(dev, config)) < 0) {
        return res;
    }
    PN532_DEBUG("init", "max packet length %u\n", PN532_PACKET_LENGTH_MAX);
    dev->connection.max_packet_length = PN532_PACKET_LENGTH_MAX;
    if ((res = (int)pn532_sam_configuration(dev, PN53_SAM_NORMAL, 0xA0, true)) < 0) {
        PN532_DEBUG("init", "SAM failed with %i\n", res);
    }
    return 0;
}

ssize_t pn532_sam_configuration(pn532_dev_t* dev, pn532_sam_mode_t mode, uint8_t timeout, bool use_irq) {
    PN532_DEBUG("SAMConfiguration", "mode=%u\n", mode);
    uint8_t command[] = {
        (uint8_t)PN532_COMMAND_SAM_CONFIGURATION,
        (uint8_t)mode,
        timeout,
        (uint8_t)use_irq
    };
    return pn53_hci_transceive_command2(dev, command, sizeof(command), NULL, dev->command_timeout);
}

ssize_t pn532_set_uart_speed(pn532_dev_t* dev, pn53_uart_speed_t speed) {
    PN532_DEBUG("UARTSpeed", "speed=0x%02x\n", speed);
    uint8_t command[] = {
        (uint8_t)PN532_COMMAND_SET_SERIAL_BAUDRATE,
        (uint8_t)speed
    };
    return pn53_hci_transceive_command2(dev, command, sizeof(command), NULL, dev->command_timeout);
}

int pn532_power_down(pn532_dev_t* dev, pn532_wakeup_sources_t wakeup_sources, bool generate_irq) {
    assert((wakeup_sources & 0b100) == 0);
    if (!(wakeup_sources & PN532_WAKEUP_SOURCE_RF)) {
        assert(generate_irq == false);
    }
    uint8_t command[] = {
        (uint8_t)PN532_COMMAND_POWER_DOWN,
        (uint8_t)wakeup_sources,
        (uint8_t)generate_irq
    };
    uint8_t* status;
    ssize_t res = pn53_hci_transceive_command2(
        dev,
        command,
        (wakeup_sources & PN532_WAKEUP_SOURCE_RF) ? sizeof(command) : (sizeof(command) - 1),
        &status, dev->command_timeout
    );
    if (res < 0) {
        return (int)res;
    }
    pn53_status_code_t error = pn53_status_code(*status);
    if (error != 0) {
        return -(PN53_ERRNO_STATUS_BASE + error);
    }
    return 0;
}

#define PN532_IN_AUTO_POLL_POLLING_INTERVAL_UNIT_MS (150)

ssize_t pn532_in_auto_poll(pn53_dev_t* dev, uint8_t attempts, uint8_t interval_units, const uint8_t* types, size_t type_count, uint8_t** response) {
    assert(attempts > 0);
    assert(interval_units > 0);
    assert(type_count > 0);
    uint8_t header[] = { (uint8_t)PN53_COMMAND_IN_AUTO_POLL, attempts, interval_units };
    iolist_t _types = { .iol_base = (void*)types, .iol_len = type_count };
    iolist_t _command = { .iol_base = header, .iol_len = sizeof(header), .iol_next = &_types };
    return pn53_hci_transceive_command(dev, &_command, response, dev->command_timeout);
}
