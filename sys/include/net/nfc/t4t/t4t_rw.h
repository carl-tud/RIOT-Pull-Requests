#pragma once

#include "net/nfc/t4t/t4t.h"
#include "net/nfc/ndef/ndef.h"
#include "net/nfcdev.h"
#include <stdint.h>
#include <stdbool.h>

typedef struct {
    nfcdev_t *dev;                 /* NFC device */
    nfc_t4t_t *tag;                /* Information about the tag */
} nfc_t4t_rw_t;

int nfc_t4t_rw_read(nfc_t4t_rw_t *rw, nfc_t4t_t *tag, nfcdev_t *dev);

int nfc_t4t_rw_read_ndef(nfc_t4t_rw_t *rw, ndef_t *ndef, nfcdev_t *dev, bool is_selected);

int nfc_t4t_rw_write_ndef(nfc_t4t_rw_t *rw, const ndef_t *ndef, nfcdev_t *dev, bool is_selected);
