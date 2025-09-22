/*
 * Copyright (C) 2024 Technische Universität Dresden
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

#pragma once

/**
 * @defgroup    drivers_nrfx_nfct_dev NRFX NFCT Driver
 * @ingroup     drivers_netdev
 * @{
 *
 * @file
 * @brief       Typedefs and function definitions for the NRFX NFCT Driver.
 *
 * @author      Nico Behrens <nifrabe@outlook.de>
 */

#include "kernel_defines.h"
#include "event.h"
#include "thread.h"

#if !IS_USED(MODULE_NRFX_NFCT)
#  error Please use the nrfx_nfct module to enable \
    the functionality on this device
#endif

#include "nrfx_nfct.h"
#include "net/nfcdev.h"
#include "net/nfc/t2t/t2t.h"

#define NRFX_NFCT_NFCDEV_OPS (nfcdev_ops_t) { \
    .init = init, \
    .listen_a = listen_a, \
    .target_exchange_data = target_exchange_data, \
}

/* As the NFCT driver depends solely on the NRFX's hardware we can only have one 
NRFX NFCT device driver instance */
#define NRFX_NFCT_DEV_DEFAULT (nfcdev_t) { \
    .dev = NULL, \
    .ops = &NRFX_NFCT_NFCDEV_OPS, \
    .state = NFCDEV_STATE_DISABLED, \
}

int init(nfcdev_t *nfcdev, const void *dev_config);

int listen_a(nfcdev_t *nfcdev, const nfc_a_listener_config_t *config);

int target_exchange_data(nfcdev_t *nfcdev, const uint8_t *tx, size_t tx_len,
                         uint8_t *rx, size_t *rx_len);


/** @} */
