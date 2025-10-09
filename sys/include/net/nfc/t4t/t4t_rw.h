#pragma once

#include "net/nfc/t4t/t4t.h"
#include "net/nfcdev.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    nfcdev_t *dev;                 /* NFC device */
    nfc_t4t_t *tag;                /* Information about the tag */
} nfc_t4t_rw_t;
