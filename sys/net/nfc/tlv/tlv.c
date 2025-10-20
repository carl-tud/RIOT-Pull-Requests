#include <string.h>
#include "log.h"

/* fills a TLV structure into a buffer */
int tlv_add(uint8_t *buffer, uint8_t type, uint16_t length, uint8_t const *value) {
    uint8_t *ptr = buffer;
    ptr[0] = type;
    ptr++;
    if (length < 0xFF) {
        *ptr = (uint8_t)length;
        ptr++;
    } else {
        *ptr = 0xFF;
        ptr++;
        *ptr = (uint8_t)(length >> 8);
        ptr++;
        *ptr = (uint8_t)(length & 0xFF);
        ptr++;
    }

    if (value != NULL && length > 0) {
        memcpy(ptr, value, length);
        ptr += length;
    }
    return (uint32_t)(ptr - buffer);
}

int tlv_parse(uint8_t const *buffer, uint8_t *type, uint16_t *length, uint8_t **value) {
    const uint8_t *ptr = buffer;
    *type = ptr[0];
    ptr++;
    if (ptr[0] == 0xFF) {
        ptr++;
        *length = ((uint16_t)ptr[0] << 8) | (uint16_t)ptr[1];
        ptr += 2;
    } else {
        *length = ptr[0];
        ptr++;
    }

    if (*length > 0) {
        *value = (uint8_t *)ptr;
        ptr += *length;
    } else {
        *value = NULL;
    }

    return (uint32_t)(ptr - buffer);
}
