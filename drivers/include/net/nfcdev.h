#pragma once

#include <stddef.h>
#include <stdint.h>

#include <nfc.h>

typedef struct {
    nfc_a_nfcid1_t uid;
    nfc_a_sens_res_t sens_res;
    nfc_a_sel_res_t sel_res;
} nfc_a_config_t;

typedef struct {
    int (init) (void *dev);

    int (poll_a) (void *dev);  /**< Poll NFC-A devices */
    int (listen_a)(void *dev, nfc_a_config_t config); /**< Listen for NFC-A devices */

    int (poll_b) (void *dev);  /**< Poll NFC-B devices */
    int (listen_b)(void *dev, nfc_b_config_t config); /**< Listen for NFC-B devices */

    int (poll_f) (void *dev);  /**< Poll NFC-F devices */
    int (listen_f)(void *dev, nfc_f_config_t config); /**< Listen for NFC-F devices */
    
    int (poll_v) (void *dev);  /**< Poll NFC-V devices */
    int (listen_v)(void *dev, nfc_v_config_t config); /**< Listen for NFC-V devices */

    int (dep_initiator_init)(void *dev); /**< Initialize DEP initiator */
    int (dep_target_init) (void *dev);   /**< Initialize DEP target */

    int (dep_exchange_data) (void *dev, const void *send, size_t send_len, void *recv,
        size_t *recv_len); /**< Exchange data with NFC device */
    
    int (target_exchange_data) (void *dev, const void *send, size_t send_len, void *recv,
        size_t *recv_len); /**< Exchange data as NFC target */
    
    int (initiator_exchange_data) (void *dev, const void *send, size_t send_len, void *recv,
        size_t *recv_len); /**< Exchange data as NFC initiator */

    bool initialized = false;
} nfcdev_t;
