#include "net/nfc/apdu/apdu.h"

inline uint8_t capdu_get_ins(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 2) {
        return 0xFF;  // Invalid length
    }
    return apdu[APDU_INS_POS];
}

inline uint8_t capdu_get_p1(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 4) {
        return 0xFF;  // Invalid length
    }
    return apdu[APDU_P1_POS];
}

inline uint8_t capdu_get_p2(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 4) {
        return 0xFF;  // Invalid length
    }
    return apdu[APDU_P2_POS];
}

inline uint8_t capdu_get_lc(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 5) {
        return 0xFF;  // Invalid length
    }
    return apdu[APDU_LC_POS];
}

inline const uint8_t *capdu_get_data(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 6) {
        return NULL;  // Invalid length
    }
    return &apdu[APDU_DATA_POS];
}

bool capdu_is_valid(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 4) {
        return false;  // Minimum length for CAPDU is 4 bytes
    }

    uint8_t lc = capdu_get_lc(apdu, apdu_len);
    size_t expected_len = 4 + 1 + lc; // CLA, INS, P1, P2 + LC + Data

    if (apdu_len != expected_len) {
        return false;  // Not enough data
    }

    /* check the status code */
    if (apdu[apdu_len - 2] != APDU_SW1_NO_ERROR || apdu[apdu_len - 1] != APDU_SW2_NO_ERROR) {
        return false;
    }

    return true;
}


bool rapdu_is_valid(const uint8_t *apdu, size_t apdu_len) {
    if (apdu_len < 2) {
        return false;  /* Minimum length for RAPDU is 2 bytes */
    }

    /* check the status code */
    if (apdu[apdu_len - 2] != APDU_SW1_NO_ERROR || apdu[apdu_len - 1] != APDU_SW2_NO_ERROR) {
        return false;
    }

    return true;
}
