#pragma once

#include "t2t.h"
#include "net/nfcdev.h"

typedef struct {
    nfcdev_t *dev;
    nfc_t2t_t *tag;
    uint8_t sector; /* current sector for sectored tags */
} nfc_t2t_rw_t;

int nfc_t2t_rw_read(nfc_t2t_rw_t *rw, nfc_t2t_t *tag, nfcdev_t *dev);

int nfc_t2t_rw_read_ndef(nfc_t2t_rw_t *rw, ndef_t *ndef, nfcdev_t *dev);

int nfc_t2t_rw_set_read_only(nfc_t2t_rw_t *rw, nfcdev_t *dev);

int nfc_t2t_rw_write_ndef(nfc_t2t_rw_t *rw, const ndef_t *ndef, nfcdev_t *dev);
