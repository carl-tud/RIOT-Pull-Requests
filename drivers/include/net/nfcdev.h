#pragma once

#include <stddef.h>
#include <stdint.h>

#include <nfc.h>

typedef struct {
    int (poll_a) (void *dev, nfc_bitrate_t bitrate);  /**< Poll NFC-A devices */
    int (listen_a)(void *dev, nfc_bitrate_t bitrate); /**< Listen for NFC-A devices */

    int (poll_b) (void *dev, nfc_bitrate_t bitrate);  /**< Poll NFC-B devices */
    int (listen_b)(void *dev, nfc_bitrate_t bitrate); /**< Listen for NFC-B devices */

    int (poll_f) (void *dev, nfc_bitrate_t bitrate);  /**< Poll NFC-F devices */
    int (listen_f)(void *dev, nfc_bitrate_t bitrate); /**< Listen for NFC-F devices */
    
    int (poll_v) (void *dev, nfc_bitrate_t bitrate);  /**< Poll NFC-V devices */
    int (listen_v)(void *dev, nfc_bitrate_t bitrate); /**< Listen for NFC-V devices */

    int (dep_initiator_init)(void *dev, nfc_bitrate_t bitrate); /**< Initialize DEP initiator */
    int (dep_target_init) (void *dev, nfc_bitrate_t bitrate);       /**< Initialize DEP target */

    int (exchange_data) (void *dev, const void *send, size_t send_len, void *recv,
        size_t *recv_len); /**< Exchange data with NFC device */

} nfcdev_t;
