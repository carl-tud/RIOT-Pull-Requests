#pragma once

#include "net/nfc/constants.h"
#include "net/nfc/nfc_a.h"
#include "net/nfc/nfc_b.h"
#include "net/nfc/nfc_f.h"
#include "net/nfc/nfc_v.h"
#include "net/nfc/iso_dep.h"
#include "net/nfc/nfc_dep.h"
#include "net/nfc/nfc_error.h"


typedef struct {
    nfc_technology_t technology;

    /// Ignored if @ref nfc_polling_loop_t::technology is @ref NFC_TECHNOLOGY_V
    nfc_bitrate_t bitrate;

    union {
        nfc_a_polling_config_t* a;
        nfc_b_polling_config_t* b;
        nfc_f_polling_config_t* f;
        nfc_v_polling_config_t* v;
    };
} nfc_polling_loop_t;

typedef struct {
    size_t loop_count;
    nfc_polling_loop_t* loops;
} nfc_polling_config_t;

// might need different strategies: should controller directly further connect if technology is is found,
// or callback-based

static inline bool nfc_polling_loop_is_valid(nfc_polling_loop_t* loop) {
    assert(loop);
    switch (loop->technology) {
        case NFC_TECHNOLOGY_A:
        case NFC_TECHNOLOGY_B:
            return loop->bitrate == NFC_BITRATE_106K;
            break;
        case NFC_TECHNOLOGY_F:
            return (loop->bitrate == NFC_BITRATE_212K) || (loop->bitrate == NFC_BITRATE_424K) == 0;
            break;
        case NFC_TECHNOLOGY_V:
            return loop->bitrate == NFC_BITRATE_UNSET;
        default:
            return false;
    }
}

// Should the driver need to group hce configs or do we just provide one?

typedef struct {
    nfc_technology_t technology;

    /// Ignored if @ref nfc_polling_result_t::technology is @ref NFC_TECHNOLOGY_V
    nfc_bitrate_t bitrate;

    union {
        nfc_a_polling_result_t a;
        nfc_b_polling_result_t b;
        nfc_f_polling_result_t f;
        nfc_v_polling_result_t v;
    };
} nfc_polling_result_t;


typedef struct {
    nfc_technology_t technology;

    /// Ignored if @ref nfc_hce_config_t::technology is @ref NFC_TECHNOLOGY_V
    nfc_bitrate_t bitrate;

    union {
        nfc_a_hce_config_t* a;
        nfc_b_hce_config_t* b;
        nfc_f_hce_config_t* f;
        nfc_v_hce_config_t* v;
    };
} nfc_hce_config_t;
