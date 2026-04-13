#pragma once

#include <stdint.h>
#include "assert.h"



typedef enum __attribute__((packed)) {
    NFC_COMMUNICATION_MODE_PASSIVE = 0,
    NFC_COMMUNICATION_MODE_ACTIVE,
} nfc_communication_mode_t;

typedef enum __attribute__((packed)) {
    NFC_TECHNOLOGY_A = 1,
    NFC_TECHNOLOGY_B = 1 << 1,
    NFC_TECHNOLOGY_F = 1 << 2,
    NFC_TECHNOLOGY_V = 1 << 3,
} nfc_technology_t;

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
#define NFC_BITRATE_INDEX_FC_128 (1)

/// @brief `fc/64`
#define NFC_BITRATE_INDEX_FC_64  (1 << 1)

/// @brief `fc/32`
#define NFC_BITRATE_INDEX_FC_32  (1 << 2)

/// @brief `fc/16`
#define NFC_BITRATE_INDEX_FC_16  (1 << 3)

/// @brief `fc/8`
#define NFC_BITRATE_INDEX_FC_8   (1 << 4)

/// @brief `fc/4`
#define NFC_BITRATE_INDEX_FC_4   (1 << 5)

/// @brief `fc/2`
#define NFC_BITRATE_INDEX_FC_2   (1 << 6)

/// @brief `3fc/4`
#define NFC_BITRATE_INDEX_FC3_4  (1 << 8)

/// @brief `fc`
#define NFC_BITRATE_INDEX_FC     (1 << 9)

/// @brief `3fc/2`
#define NFC_BITRATE_INDEX_FC3_2  (1 << 10)

/// @brief `2fc`
#define NFC_BITRATE_INDEX_FC2    (1 << 11)
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
    NFC_BITRATE_106K   = NFC_BITRATE_INDEX_FC_128,
    NFC_BITRATE_212K   = NFC_BITRATE_INDEX_FC_64,
    NFC_BITRATE_424K   = NFC_BITRATE_INDEX_FC_32,
    NFC_BITRATE_848K   = NFC_BITRATE_INDEX_FC_16,
    NFC_BITRATE_1695K  = NFC_BITRATE_INDEX_FC_8,
    NFC_BITRATE_3390K  = NFC_BITRATE_INDEX_FC_4,
    NFC_BITRATE_6780K  = NFC_BITRATE_INDEX_FC_2,
    NFC_BITRATE_10170K = NFC_BITRATE_INDEX_FC3_4,
    NFC_BITRATE_13560K = NFC_BITRATE_INDEX_FC,
    NFC_BITRATE_20340K = NFC_BITRATE_INDEX_FC3_2,
    NFC_BITRATE_27120K = NFC_BITRATE_INDEX_FC2,
} nfc_bitrate_t;

#define NFC_BITRATE_UNSET (0)

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

typedef struct {
    size_t bytes;
    uint8_t trailing_bits;
} nfc_frame_length_t;

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

typedef enum __attribute__((packed)) {
    /// Without length, CRC, prefixes, suffixes etc, any protocol unmanaged
    NFC_INTERFACE_FRAME = 1,

    /// With length, CRC, etc..., any protocol unmanaged
    NFC_INTERFACE_PACKET = 1 << 1,

    /// ISO-DEP payloads, protocol managedd
    NFC_INTERFACE_ISO_DEP = 1 << 2,

    /// NFC-DEP payloads, protocol managed
    NFC_INTERFACE_NFC_DEP = 1 << 3,
} nfc_framing_interface_t;

#define NFC_INTERFACE_ANY (0)
