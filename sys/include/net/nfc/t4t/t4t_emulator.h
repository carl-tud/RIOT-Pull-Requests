#pragma once

#include "net/nfcdev.h"
#include "t4t.h"

typedef struct {
    nfcdev_t *dev;
    nfc_t4t_t *tag;
} nfc_t4t_emulator_t;

void t4t_emulator_start(nfc_t4t_emulator_t *emulator, nfcdev_t *dev, 
    nfc_t4t_t *tag, nfc_a_nfcid1_t *nfcid1);
