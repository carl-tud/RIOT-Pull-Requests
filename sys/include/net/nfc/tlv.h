#pragma once

#include <stdint.h>

#define NFC_TLV_MINIMUM_SIZE 2

typedef struct {
    uint8_t type;
    uint16_t length;
    uint8_t *value;
} nfc_tlv_t;

int tlv_parse(uint8_t const *buffer, uint8_t *type, uint16_t *length, uint8_t **value);

int tlv_add(uint8_t *buffer, uint8_t type, uint16_t length, uint8_t const *value);
