#pragma once

#include "t2t.h"
#include "net/nfcdev.h"

#define NFC_T2T_EMULATOR_RX_TX_BUFFER_SIZE 64

typedef struct {
    nfcdev_t *dev;
    nfc_t2t_t *tag;
} nfc_t2t_emulator_t;

void start_emulation(nfc_t2t_emulator_t *emulator, nfcdev_t *dev, nfc_t2t_t *tag);
