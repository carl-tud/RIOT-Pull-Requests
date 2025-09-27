#pragma once

#include <stddef.h>
#include <stdint.h>

#include <net/nfc/nfc.h>
#include <net/nfc/nfc_a.h>
#include <net/nfc/nfc_b.h>
#include <net/nfc/nfc_f.h>
#include <net/nfc/nfc_v.h>

typedef enum {
    NFCDEV_STATE_DISABLED = 0,
    NFCDEV_STATE_IDLE,
    NFCDEV_STATE_POLLING,
    NFCDEV_STATE_LISTENING,
    NFCDEV_STATE_DEP_INITIATOR,
    NFCDEV_STATE_DEP_TARGET,
} nfcdev_state_t;

struct nfcdev;

typedef struct {
    int (*init)(struct nfcdev *nfcdev, const void *dev_config);

    int (*poll_a)(struct nfcdev *nfcdev);
    int (*listen_a)(struct nfcdev *nfcdev, const nfc_a_listener_config_t *config);

    int (*poll_b)(struct nfcdev *nfcdev);
    int (*listen_b)(struct nfcdev *nfcdev, const nfc_b_listener_config_t *config);

    int (*poll_f)(struct nfcdev *nfcdev);
    int (*listen_f)(struct nfcdev *nfcdev, const nfc_f_listener_config_t *config);

    int (*poll_v)(struct nfcdev *nfcdev);
    int (*listen_v)(struct nfcdev *nfcdev, const nfc_v_listener_config_t *config);

    int (*dep_initiator_init)(struct nfcdev *nfcdev);
    int (*dep_target_init)(struct nfcdev *nfcdev);

    int (*dep_exchange_data)(struct nfcdev *nfcdev, const uint8_t *send, size_t send_len, 
        uint8_t *recv, size_t *recv_len);

    /* int (*target_exchange_data)(struct nfcdev *nfcdev, const uint8_t *send, size_t send_len, 
        uint8_t *recv, size_t *recv_len); */

    int (*target_send_data)(struct nfcdev *nfcdev, const uint8_t *send, size_t send_len);

    int (*target_receive_data)(struct nfcdev *nfcdev, uint8_t *recv, size_t *recv_len);

    int (*initiator_exchange_data)(struct nfcdev *nfcdev, const uint8_t *send, size_t send_len, 
        uint8_t *recv, size_t *recv_len);
} nfcdev_ops_t;

typedef struct nfcdev {
    void *dev; /**< Pointer to the device specific structure */
    void *config; /**< Pointer to the device specific configuration structure */
    const nfcdev_ops_t *ops; /**< Pointer to the device operations */
    nfcdev_state_t state; /**< Current state of the NFC device */
} nfcdev_t;


