#pragma once

#include "net/nfcdev.h"

nfc_dep_initator_init(nfcdev_t *dev, nfc_bitrate_t bitrate);

nfc_dep_target_init(nfcdev_t *dev, nfc_bitrate_t bitrate, void (*callback) (const void *arg));

nfc_dep_exchange(nfcdev_t *dev, const void *send, size_t send_len, void *recv, size_t *recv_len);
