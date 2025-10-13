#pragma once

#include "net/nfcdev.h"

#include "net/nfc/mifare/mifare_classic_rw.h"
#include "net/nfc/t4t/t4t_rw.h"
#include "net/nfc/t2t/t2t_rw.h"
#include "net/nfc/ndef/ndef.h"

typedef struct {
    nfcdev_t *dev; /* NFC device */
} nfc_generic_rw_t;

int nfc_generic_rw_read_ndef(nfc_generic_rw_t *rw, ndef_t *ndef, nfcdev_t *dev);
