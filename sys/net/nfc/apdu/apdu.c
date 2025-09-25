#pragma once

#include "net/nfc/apdu/apdu.h"

inline uint8_t apdu_get_ins(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 2) {
        return 0xFF;  // Invalid length
    }
    return apdu[APDU_INS_POS];
}

inline uint8_t apdu_get_p1(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 4) {
        return 0xFF;  // Invalid length
    }
    return apdu[APDU_P1_POS];
}

inline uint8_t apdu_get_p2(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 4) {
        return 0xFF;  // Invalid length
    }
    return apdu[APDU_P2_POS];
}

inline uint8_t apdu_get_lc(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 5) {
        return 0xFF;  // Invalid length
    }
    return apdu[APDU_LC_POS];
}

inline const uint8_t *apdu_get_data(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 6) {
        return NULL;  // Invalid length
    }
    return &apdu[APDU_DATA_POS];
}
