#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "net/nfc.h"

// might need different strategies: should controller directly respond if technology is polled,
// or callback-based

typedef enum {
    NFCDEV_STATE_UNINITIALIZED = 0,
    NFCDEV_STATE_DISABLED,
    NFCDEV_STATE_IDLE,
    NFCDEV_STATE_POLLING,
    NFCDEV_STATE_LISTENING,
    NFCDEV_STATE_NFC_DEP_CONNECTED,
} nfcdev_state_t;

typedef enum {
    NFCDEV_ROLE_TARGET = 0,
    NFCDEV_ROLE_INITIATOR,
} nfcdev_role_t;

struct nfcdev_ops;

typedef struct nfcdev {
    void* dev; /**< Pointer to the device specific structure */
    void* config; /**< Pointer to the device specific configuration structure */
    const struct nfcdev_ops* ops; /**< Pointer to the device operations */
    nfcdev_state_t state; /**< Current state of the NFC device */
} nfcdev_t;

typedef struct {
    int (*init)(nfcdev_t* nfcdev, const void *dev_config);

    int (*poll) (nfcdev_t* nfcdev, const nfc_polling_config_t* config);
    int (*listen) (nfcdev_t* nfcdev, const nfc_hce_config_t* config);

    nfc_framing_interface_t (*active_interfaces)(nfcdev_t* nfcdev);

    ssize_t (*send)(nfcdev_t* nfcdev, const uint8_t* frame, size_t length, nfc_framing_interface_t interface);
    ssize_t (*recv)(nfcdev_t* nfcdev, const uint8_t* frame, size_t capacity, nfc_framing_interface_t interface);

    int (*mifare_classic_authenticate) (nfcdev_t* nfcdev, uint8_t block_number,
        const nfc_a_id_t *uid, bool use_key_a,
        const uint8_t *key);
} nfcdev_ops_t;


