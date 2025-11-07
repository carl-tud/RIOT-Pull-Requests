#pragma once

#include "t2t.h"
#include "net/nfcdev.h"

#define NFC_T2T_EMULATOR_RX_TX_BUFFER_SIZE 64

typedef enum {
    NFC_T2T_STATE_IDLE = 0,
    NFC_T2T_STATE_ACTIVE,
    NFC_T2T_STATE_SLEEPING,
} nfc_t2t_state_t;

typedef struct {
    nfcdev_t *dev;
    nfc_t2t_t *tag;
    nfc_t2t_state_t state;
    uint8_t sector;
} nfc_t2t_emulator_t;


int t2t_emulator_start(nfc_t2t_emulator_t *emulator, nfcdev_t *dev, 
    nfc_t2t_t *tag, nfc_a_nfcid1_t *nfcid1);
