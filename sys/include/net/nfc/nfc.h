#pragma once

typedef struct {
} nfc_target_t;


typedef enum {
    NFC_APPLICATION_TYPE_UNKNOWN = 0,
    NFC_APPLICATION_TYPE_T2T,
    NFC_APPLICATION_TYPE_T3T,
    NFC_APPLICATION_TYPE_T4T,
    NFC_APPLICATION_TYPE_T5T,
    NFC_APPLICATION_MIFARE_CLASSIC,
    NFC_APPLICATION_MIFARE_ULTRALIGHT,
    NFC_APPLICATION_MIFARE_DESFIRE,
} nfc_application_type_t;

#include "nfc_a.h"
#include "nfc_b.h"
#include "nfc_f.h"
