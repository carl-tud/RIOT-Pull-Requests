/*
 * Copyright (C) 2026 Carl Seifert
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "pn532.h"

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
        &dev->connection,
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
