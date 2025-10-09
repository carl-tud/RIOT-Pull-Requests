/*
 * Copyright (C) 2024 Technische Universität Dresden
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     drivers_nrfx_nfct_dev
 * @{
 * @file
 * @brief       NFCT Driver for the NRFX boards
 *
 * @author      Nico Behrens <nifrabe@outlook.de>
 *
 * @}
 */

#include "nrfx_nfct_dev.h"

#include "nrfx_nfct.h"

#include "log.h"
#include "event.h"




/**
 * @brief driver state
 */
static volatile bool enabled = false;
static volatile bool initialized = false;

static volatile bool tx = false;
static volatile bool rx = false;

static mutex_t lock;

/**
 * @brief driver state 
 * 
 * @note Is set to true after a succesful autocollision resolution
 */
static bool selected = false;

#define BUFFER_SIZE 64

static uint8_t data_buffer_rx[BUFFER_SIZE];
static volatile uint32_t data_length_rx = 0;
static uint8_t data_buffer_tx[BUFFER_SIZE];

static nrfx_nfct_data_desc_t rx_desc = {
    .p_data = data_buffer_rx,
    .data_size = BUFFER_SIZE
};

static nrfx_nfct_data_desc_t tx_desc = {
    .p_data = data_buffer_tx,
    .data_size = 0
};

int block_with_timeout(uint32_t timeout_sec) {
    if (timeout_sec == 0) {
        mutex_lock(&lock);
        return 0;
    } else {
        ztimer_t timer = {0};
        ztimer_mutex_unlock(ZTIMER_SEC, &timer, timeout_sec, &lock);
        mutex_lock(&lock);
        bool triggered = !ztimer_remove(ZTIMER_SEC, &timer);
        if (triggered) {
            return NFC_ERR_TIMEOUT;
        } else {
            return 0;
        }
    }
}

/**
 * @brief Enables autocollision resolution in the NFCT peripheral
 *
 * @note Without autocollision resolution, the user would have to implement
 * their own autocollision procedure
 *
 * @param[in] nfctid1       Pointer to the NFCID1 to use
 * @param[in] nfctid1_size  Size of the NFCID1
 */
static void configure_autocolres(const uint8_t *nfctid1, uint8_t nfctid1_size, uint8_t sel_res)
{
    nrfx_nfct_nfcid1_t nfcid1 = {
        .p_id = nfctid1,
        .id_size = nfctid1_size,
    };

    nrfx_nfct_param_t param_nfcid = {
        .id = NRFX_NFCT_PARAM_ID_NFCID1,
        .data.nfcid1 = nfcid1
    };
    nrfx_nfct_parameter_set(&param_nfcid);

    LOG_DEBUG("Sel res is %02X\n", sel_res);
    nrfx_nfct_param_t param_sel_res = {
        .id = NRFX_NFCT_PARAM_ID_SEL_RES,
        .data = {sel_res}
    };
    nrfx_nfct_parameter_set(&param_sel_res);
    nrfx_nfct_autocolres_enable();
}

static void configure_fdt(uint32_t fdt, uint32_t fdt_min)
{
    nrfx_nfct_param_t param_fdt = {
        .id = NRFX_NFCT_PARAM_ID_FDT,
        .data = {fdt}
    };
    nrfx_nfct_parameter_set(&param_fdt);

    nrfx_nfct_param_t param_fdt_min = {
        .id = NRFX_NFCT_PARAM_ID_FDT_MIN,
        .data = {fdt_min}
    };
    nrfx_nfct_parameter_set(&param_fdt_min);
}

/**
 * @brief IRQ event handler called by the NRFX NFCT driver
 *
 * @param[in] event event to process
 */
static void irq_event_handler(const nrfx_nfct_evt_t *event)
{
    LOG_DEBUG("[IRQ] ");
    if (event->evt_id == NRFX_NFCT_EVT_RX_FRAMEEND) {
        if (rx) {
            nrfx_nfct_autocolres_disable();
            LOG_DEBUG("Received frame of size %lu\n", event->params.rx_frameend.rx_data.data_size);
            data_length_rx = event->params.rx_frameend.rx_data.data_size;
            if (data_length_rx > BUFFER_SIZE) {
                data_length_rx = BUFFER_SIZE;
            }


            if (event->params.rx_frameend.rx_status &    (NRF_NFCT_RX_FRAME_STATUS_CRC_MASK    |
                                                        NRF_NFCT_RX_FRAME_STATUS_PARITY_MASK |
                                                        NRF_NFCT_RX_FRAME_STATUS_OVERRUN_MASK)) {
                LOG_ERROR("RX frame error: CRC=%d, Parity=%d, Overrun=%d\n",
                            !!(event->params.rx_frameend.rx_status & NRF_NFCT_RX_FRAME_STATUS_CRC_MASK),
                            !!(event->params.rx_frameend.rx_status & NRF_NFCT_RX_FRAME_STATUS_PARITY_MASK),
                            !!(event->params.rx_frameend.rx_status & NRF_NFCT_RX_FRAME_STATUS_OVERRUN_MASK));
                return;
            }
            mutex_unlock(&lock);
            rx = false;
            tx = true;
        }
    }
    else if (event->evt_id == NRFX_NFCT_EVT_TX_FRAMEEND) {
        if (tx) {
            LOG_DEBUG("End of transmission\n");
            mutex_unlock(&lock);
            nrfx_nfct_rx(&rx_desc);
            tx = false;
            rx = true;
        }
    }
    else if (event->evt_id == NRFX_NFCT_EVT_SELECTED) {
        LOG_DEBUG("Field has been selected\n");
        mutex_unlock(&lock);
        selected = true;
    }
    else if (event->evt_id == NRFX_NFCT_EVT_TX_FRAMESTART) {
        LOG_DEBUG("Transmitting data\n");
    }
    else if (event->evt_id == NRFX_NFCT_EVT_RX_FRAMESTART) {
        LOG_DEBUG("Receiving data\n");
    }
    else if (event->evt_id == NRFX_NFCT_EVT_ERROR) {
        nrfx_nfct_error_t error = event->params.error.reason;
        LOG_DEBUG("ERROR: %X\n", event->params.error.reason);
    }
    else if (event->evt_id == NRFX_NFCT_EVT_FIELD_DETECTED) {
        LOG_DEBUG("Field detected\n");
    }
    else if (event->evt_id == NRFX_NFCT_EVT_FIELD_LOST) {
        LOG_DEBUG("Field lost\n");
        selected = false;
    }
    else {
        LOG_WARNING("Unknown event received\n");
    }
}

/* NFC Device Interface functions */
int nrfx_nfct_dev_init(nfcdev_t *nfcdev, const void *dev_config) {
    (void) dev_config;

    nrfx_nfct_config_t nrfx_nfct_config = {
        .rxtx_int_mask = NRFX_NFCT_EVT_FIELD_DETECTED |
                         NRFX_NFCT_EVT_ERROR |
                         NRFX_NFCT_EVT_SELECTED |
                         NRFX_NFCT_EVT_FIELD_LOST |
                         NRFX_NFCT_EVT_RX_FRAMEEND |
                         NRFX_NFCT_EVT_RX_FRAMESTART |
                         NRFX_NFCT_EVT_TX_FRAMEEND |
                         NRFX_NFCT_EVT_TX_FRAMESTART
                      /* NRF_NFCT_INT_RXERROR_MASK    |
                         NRF_NFCT_RX_FRAME_STATUS_CRC_MASK    |
                         NRF_NFCT_RX_FRAME_STATUS_PARITY_MASK |
                         NRF_NFCT_RX_FRAME_STATUS_OVERRUN_MASK |
                         NRF_NFCT_ERROR_FRAMEDELAYTIMEOUT_MASK */
        ,
        .cb = irq_event_handler,
        .irq_priority = 4
    };

    nrfx_err_t error = nrfx_nfct_init(&nrfx_nfct_config);
    if (error != NRFX_SUCCESS) {
        return -1;
    }

    nrf_nfct_shorts_enable(NRF_NFCT, NRF_NFCT_SHORT_FIELDDETECTED_ACTIVATE_MASK |
                                     NRF_NFCT_SHORT_FIELDLOST_SENSE_MASK);

    mutex_init(&lock);
    mutex_lock(&lock);
    nfcdev->state = NFCDEV_STATE_DISABLED;

    return 0;
}

int nrfx_nfct_dev_listen_a(nfcdev_t *nfcdev, const nfc_a_listener_config_t *config) {
    configure_autocolres(config->nfcid1.nfcid, config->nfcid1.len, config->sel_res);
    configure_fdt(0x01000, 0x0480);
    // configure_fdt(0x05000, 0x1300);

    nrfx_nfct_enable();

    nfcdev->state = NFCDEV_STATE_LISTENING;

    LOG_DEBUG("Listening for NFC-A initiators...\n");

    int ret = block_with_timeout(100);
    if (ret != 0) {
        return ret;
    }

    rx = true;
    tx = false;
    nrfx_nfct_rx(&rx_desc);

    return 0;
}

static int send_data(nfcdev_t *nfcdev, const uint8_t *data, size_t len) {
    (void) nfcdev;
    if (len > BUFFER_SIZE) {
        return -1;
    }

    memcpy(data_buffer_tx, data, len);
    tx_desc.data_size = len;

    nrfx_err_t error = nrfx_nfct_tx(&tx_desc, NRF_NFCT_FRAME_DELAY_MODE_WINDOWGRID);
    if (error != NRFX_SUCCESS) {
        return -1;
    }

    int ret = block_with_timeout(5);
    if (ret != 0) {
        return ret;
    }

    rx = true;
    tx = false;

    return 0;
}

static int receive_data(nfcdev_t *nfcdev, uint8_t *data, size_t *len) {
    (void) nfcdev;
    if (*len > BUFFER_SIZE) {
        return -1;
    }

    nrfx_nfct_rx(&rx_desc);

    int ret = block_with_timeout(5);
    if (ret != 0) {
        return ret;
    }

    *len = data_length_rx;
    memcpy(data, data_buffer_rx, *len);

    /* print bytes here */
    for (size_t i = 0; i < *len; i++) {
        LOG_DEBUG("Received byte %d: 0x%02X\n", i, data_buffer_rx[i]);
    }

    tx = false;
    rx = true;

    return 0;
}

int nrfx_nfct_dev_target_send_data(nfcdev_t *nfcdev, const uint8_t *tx, size_t tx_len) {
    return send_data(nfcdev, tx, tx_len);
}

int nrfx_nfct_dev_target_receive_data(nfcdev_t *nfcdev, uint8_t *rx, size_t *rx_len) {
    return receive_data(nfcdev, rx, rx_len);
}

int nrfx_nfct_dev_deinit(nfcdev_t *nfcdev) {
    (void) nfcdev;
    nrfx_nfct_disable();
    nrfx_nfct_uninit();
    return 0;
}
