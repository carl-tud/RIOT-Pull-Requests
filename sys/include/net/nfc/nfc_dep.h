#pragma once

#include "net/nfcdev.h"

typedef struct {
    nfc_baudrate_t baudrate;
    uint8_t nfcid3t[3];
} nfc_dep_listener_config_t;
