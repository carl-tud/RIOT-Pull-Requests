#pragma once

#include <stdint.h>
#include <stddef.h>
#include "assert.h"

typedef enum __attribute__((packed)) {
    /// Initiator (reader/writer) generates field modulated by passive target (tag)
    ///
    /// The initiator generates a field both during TX and RX phases that it modulates to send data
    /// while the target passively modules said initiator-generated field.
    /// This mode is also known as _passive mode_, where _passive_ refers to the 'passive'
    /// non-field-generating nature of the target.
    NFC_FIELD_MODEL_READER_WRITER_TAG = 0,

    /// Peers generate field to send data
    ///
    /// Both the initiator and target generate a field to send data to the other and turn it off
    /// while receiving from its peer.
    /// This mode is also known as _active mode_, where _active_ refers to the 'active'
    /// field-generating nature of the target.
    ///
    /// Currently, this mode is only used by a specific transport protocol, NFC-DEP (specified in
    /// ISO/IEC 18092).
    NFC_FIELD_MODEL_PEERS,
} nfc_field_model_t;

typedef enum {
    NFC_ROLE_TARGET = 0,
    NFC_ROLE_INITIATOR,
} nfc_role_t;

typedef enum __attribute__((packed)) {
    NFC_TECHNOLOGY_A = 1 << 1,
    NFC_TECHNOLOGY_B = 1 << 2,
    NFC_TECHNOLOGY_F = 1 << 3,
    NFC_TECHNOLOGY_V = 1 << 4,
} nfc_technology_t;

#define NFC_TECHNOLOGY_UNSET (0)

static inline char nfc_string_from_technology(nfc_technology_t technology) {
    assert(__builtin_ctz(technology) < __builtin_ctz(NFC_TECHNOLOGY_V));
    return ((char[]){ '?', 'A', 'B', 'F', 'V' })[__builtin_ctz(technology)];
}

#define NFC_BITRATE_DIVISOR_FC_128 (1)
#define NFC_BITRATE_DIVISOR_FC_64  (2)
#define NFC_BITRATE_DIVISOR_FC_32  (4)
#define NFC_BITRATE_DIVISOR_FC_16  (8)
#define NFC_BITRATE_DIVISOR_FC_8   (16)
#define NFC_BITRATE_DIVISOR_FC_4   (32)
#define NFC_BITRATE_DIVISOR_FC_2   (64)
#define NFC_BITRATE_DIVISOR_FC3_4  (96)
#define NFC_BITRATE_DIVISOR_FC     (128)
#define NFC_BITRATE_DIVISOR_FC3_2  (192)
#define NFC_BITRATE_DIVISOR_FC2    (256)

/// @name Bit rate indices, follows ISO/IEC 14443-4 Figures 24 and 25
/// @{

/// @brief `fc/128`
#define NFC_BITRATE_FLAG_FC_128 (1)

/// @brief `fc/64`
#define NFC_BITRATE_FLAG_FC_64  (1 << 1)

/// @brief `fc/32`
#define NFC_BITRATE_FLAG_FC_32  (1 << 2)

/// @brief `fc/16`
#define NFC_BITRATE_FLAG_FC_16  (1 << 3)

/// @brief `fc/8`
#define NFC_BITRATE_FLAG_FC_8   (1 << 4)

/// @brief `fc/4`
#define NFC_BITRATE_FLAG_FC_4   (1 << 5)

/// @brief `fc/2`
#define NFC_BITRATE_FLAG_FC_2   (1 << 6)

/// @brief `3fc/4`
#define NFC_BITRATE_FLAG_FC3_4  (1 << 8)

/// @brief `fc`
#define NFC_BITRATE_FLAG_FC     (1 << 9)

/// @brief `3fc/2`
#define NFC_BITRATE_FLAG_FC3_2  (1 << 10)

/// @brief `2fc`
#define NFC_BITRATE_FLAG_FC2    (1 << 11)
/// @}

/// Bit Rate for Near Field Communication
///
/// The `divisor` associated with each enum case is the bit rate's corresponding divisor `D`. `fc` is the ``NFCCarrierFrequency``.
///
/// - `1 etu = 128/fc / D`
/// - `bitRate = D · fc/128`
///
/// - Note: You should use the ``BitRate(_:)`` macro instead of directly accessing this enum's cases.
typedef enum __attribute__((packed)) {
    NFC_BITRATE_106K   = NFC_BITRATE_FLAG_FC_128,
    NFC_BITRATE_212K   = NFC_BITRATE_FLAG_FC_64,
    NFC_BITRATE_424K   = NFC_BITRATE_FLAG_FC_32,
    NFC_BITRATE_848K   = NFC_BITRATE_FLAG_FC_16,
    NFC_BITRATE_1695K  = NFC_BITRATE_FLAG_FC_8,
    NFC_BITRATE_3390K  = NFC_BITRATE_FLAG_FC_4,
    NFC_BITRATE_6780K  = NFC_BITRATE_FLAG_FC_2,
    NFC_BITRATE_10170K = NFC_BITRATE_FLAG_FC3_4,
    NFC_BITRATE_13560K = NFC_BITRATE_FLAG_FC,
    NFC_BITRATE_20340K = NFC_BITRATE_FLAG_FC3_2,
    NFC_BITRATE_27120K = NFC_BITRATE_FLAG_FC2,
} nfc_bitrate_t;

#define NFC_BITRATE_UNSET (0)

static inline size_t nfc_bitrate_to_index(nfc_bitrate_t bitrate) {
    return __builtin_ctz(bitrate);
}

static inline nfc_bitrate_t nfc_bitrate_from_index(size_t index) {
    return 1 << index;
}

typedef uint8_t nfc_time_index_t;

#define NFC_CARRIER_FREQUENCY_HZ (13560000)

static inline unsigned int nfc_bitrate_kbps(nfc_bitrate_t bitrate) {
    return (NFC_CARRIER_FREQUENCY_HZ / 1000 + 64/bitrate)/128*bitrate;
}


static inline uint16_t nfc_time_index_ms(nfc_time_index_t index) {
    assert(index <= 0xf);
    return 256 * 16/NFC_CARRIER_FREQUENCY_HZ * 1000 * (1 << index);
}

/// Set of bitrates
///
/// ## Example
/// ```c
/// nfc_bitrate_set my_bitrates = NFC_BITRATE_106K | NFC_BITRATE_212K | NFC_BITRATE_424K;
/// ```
typedef uint16_t nfc_bitrate_set_t;

typedef enum {
    /// Stick with current base bitrate (default)
    NFC_BITRATE_CHOOSE_CURRENT = 0,

    /// Select fastest bitrate
    NFC_BITRATE_CHOOSE_FASTEST = 1,

    /// Select slowest available bitrate
    ///
    /// In most cases (NFC-A or NFC-B with ISO-DEP), this will the the same as
    /// @ref NFC_BITRATE_CHOOSE_CURRENT.
    NFC_BITRATE_CHOOSE_SLOWEST = 2,
} nfc_bitrate_selection_strategy_t;

typedef struct {
    nfc_bitrate_set_t set;
    nfc_bitrate_selection_strategy_t strategy;
} nfc_bitrate_selector_t;

static inline nfc_bitrate_t nfc_bitrate_select(
    nfc_bitrate_set_t set1,
    nfc_bitrate_set_t set2,
    nfc_bitrate_t current,
    nfc_bitrate_selection_strategy_t strategy
) {
    if (strategy == NFC_BITRATE_CHOOSE_CURRENT) {
        return current;
    }
    nfc_bitrate_set_t gcd = set1 & set2;
    if (gcd == 0) {
        return 0;
    }
    if (strategy == NFC_BITRATE_CHOOSE_FASTEST) {
        // Count leading zeroes, get fastest bit rate
        return (nfc_bitrate_t)(1 << ((sizeof(nfc_bitrate_set_t) * 8) - 1 - __builtin_clz(gcd)));
    } else {
        // Count trailing zeroes, get slowest bit rate
        return (nfc_bitrate_t)(1 << __builtin_ctz(gcd));
    }
}

#define NFC_TRAILING_BITS_ALL (0)

typedef union {
    struct {
        size_t bytes : (sizeof(size_t) * 8 - 8);
        uint8_t trailing_bits : 7;

        // Let's make casting this to ssize_t possible, i.e., not use the sign bit.
        uint8_t zero : 1;
    };
    size_t encoded;
} nfc_frame_length_t;

static inline uint8_t nfc_frame_length_trailing_bits(size_t length) {
    nfc_frame_length_t* frame_length = (nfc_frame_length_t*)&length;
    return frame_length->trailing_bits;
}

static inline uint8_t nfc_frame_length_bytes(size_t length) {
    nfc_frame_length_t* frame_length = (nfc_frame_length_t*)&length;
    return frame_length->bytes;
}

static inline size_t nfc_frame_length(nfc_frame_length_t frame_length) {
    return frame_length.encoded;
}

static_assert(sizeof(nfc_frame_length_t) == sizeof(size_t));

typedef enum {
    NFC_APPLICATION_TYPE_UNKNOWN = 0,
    NFC_APPLICATION_TYPE_T1T,
    NFC_APPLICATION_TYPE_T2T,
    NFC_APPLICATION_TYPE_T3T,
    NFC_APPLICATION_TYPE_T4T,
    NFC_APPLICATION_TYPE_T5T,
    NFC_APPLICATION_NFC_DEP,
    NFC_APPLICATION_MIFARE_ULTRALIGHT,  /* fully T2T compliant */
    NFC_APPLICATION_MIFARE_CLASSIC,     /* partially T2T compliant */
    NFC_APPLICATION_MIFARE_DESFIRE,     /* based on T4T */
    NFC_APPLICATION_MIFARE_PLUS,        /* based on T4T */
} nfc_application_type_t;

