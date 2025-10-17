#pragma once

#include "net/nfc/mifare/mifare_classic.h"
#include "net/nfcdev.h"
#include "net/nfc/ndef/ndef.h"

typedef struct {
    nfc_mifare_classic_tag_t *tag;  /* Information about the tag */
    nfcdev_t *dev;                 /* NFC device */
} nfc_mifare_classic_rw_t;

int nfc_mifare_classic_rw_read_ndef(nfc_mifare_classic_rw_t *rw, ndef_t *ndef, nfcdev_t *dev);

int nfc_mifare_classic_rw_read_tag(nfc_mifare_classic_rw_t *rw, nfc_mifare_classic_tag_t *tag, 
    nfcdev_t *dev);
