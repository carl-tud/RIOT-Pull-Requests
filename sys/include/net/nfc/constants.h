#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "assert.h"
#include "sys/types.h"

#if !defined(CONFIG_NFC_GENERAL_BYTES_CAPACITY) || defined(DOXYGEN)
#  define CONFIG_NFC_GENERAL_BYTES_CAPACITY (8)
#endif

#if !defined(CONFIG_NFC_HIGHER_LAYER_ACTIVATION_MESSAGE_CAPACITY) || defined(DOXYGEN)
#  define CONFIG_NFC_HIGHER_LAYER_ACTIVATION_MESSAGE_CAPACITY (8)
#endif

typedef enum __attribute__((packed)) {
    /// Initiator (reader/writer) generates field modulated by passive target (tag)
    ///
    /// The initiator generates a field both during TX and RX phases that it modulates to send data
    /// while the target passively modules said initiator-generated field.
    /// This mode is also known as _passive mode_, where _passive_ refers to the 'passive'
    /// non-field-generating nature of the target.
    NFC_FIELD_MODE_READER_WRITER_TAG = 0,

    /// Peers generate field to send data
    ///
    /// Both the initiator and target generate a field to send data to the other and turn it off
    /// while receiving from its peer.
    /// This mode is also known as _active mode_, where _active_ refers to the 'active'
    /// field-generating nature of the target.
    ///
    /// Currently, this mode is only used by a specific transport protocol, NFC-DEP (specified in
    /// ISO/IEC 18092).
    NFC_FIELD_MODE_PEERS,
} nfc_field_mode_t;

const char* nfc_string_from_field_mode(nfc_field_mode_t field_mode);

typedef enum {
    NFC_ROLE_TARGET = 0,
    NFC_ROLE_INITIATOR,
} nfc_role_t;

const char* nfc_string_from_role(nfc_role_t role);

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
typedef nfc_bitrate_t nfc_bitrate_set_t;

static inline nfc_bitrate_t nfc_bitrate_set_fastest(nfc_bitrate_set_t set) {
    return (nfc_bitrate_t)(1U << ((sizeof(unsigned int) * 8) - 1 - __builtin_clz((unsigned int)set)));
}

static inline nfc_bitrate_t nfc_bitrate_set_slowest(nfc_bitrate_set_t set) {
    return (nfc_bitrate_t)(1 << __builtin_ctz(set));
}

#define NFC_BITRATE_SET_UP_TO(bitrate) \
    ((nfc_bitrate_set_t)(bitrate) | (nfc_bitrate_set_t)(bitrate - 1))

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

    NFC_BITRATE_CHOOSE_FORCED = 3,
} nfc_bitrate_selection_strategy_t;

typedef struct {
    nfc_bitrate_set_t set;
    nfc_bitrate_selection_strategy_t strategy;
} nfc_bitrate_selector_t;

typedef struct {
    nfc_bitrate_selector_t downstream;
    nfc_bitrate_selector_t upstream;
} nfc_bidirectional_bitrate_selector_t;

#define NFC_SELECT_FASTEST_UP_TO(bitrate) (nfc_bidirectional_bitrate_selector_t) { \
    .downstream = { .set = NFC_BITRATE_SET_UP_TO(bitrate), .strategy = NFC_BITRATE_CHOOSE_FASTEST }, \
    .upstream = { .set = NFC_BITRATE_SET_UP_TO(bitrate), .strategy = NFC_BITRATE_CHOOSE_FASTEST }, \
}

nfc_bitrate_t nfc_bitrate_select(
    nfc_bitrate_set_t set1,
    nfc_bitrate_set_t set2,
    nfc_bitrate_t current,
    nfc_bitrate_selection_strategy_t strategy
);

void nfc_bitrate_select_bidirectional(
    const nfc_bidirectional_bitrate_selector_t* selector,
    nfc_bitrate_t* downstream, nfc_bitrate_t* upstream,
    nfc_bitrate_set_t downstream_supported, nfc_bitrate_set_t upstream_supported,
    bool require_symmetric
);

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

#define NFCDEV_TRAILING_BITS_ALL (0)

typedef union {
    struct {
        size_t bytes : (sizeof(size_t) * 8 - 8);
        uint8_t trailing_bits : 7;

        // Let's make casting this to ssize_t possible, i.e., not use the sign bit.
        uint8_t zero : 1;
    };
    size_t _encoded;
    ssize_t _signed;
} nfcdev_frame_length_t;

static_assert(sizeof(nfcdev_frame_length_t) == sizeof(size_t));
static_assert(sizeof(nfcdev_frame_length_t) == sizeof(ssize_t));

#define NFCDEV_FRAME_LENGTH_BYTES_MAX (((nfcdev_frame_length_t) { ._encoded = SIZE_MAX }).bytes)

typedef enum __attribute__((packed)) {
    /// Without NFC-A parity bits, otherwise same as @ref NFCDEV_INTERFACE_FRAME
    NFCDEV_INTERFACE_BITS = 1,

    /// Without length, CRC, headers, trailers etc, any protocol unmanaged
    NFCDEV_INTERFACE_FRAME = 2,

    /// With length, CRC, etc..., any protocol unmanaged
    NFCDEV_INTERFACE_PACKET = 3,

    /// ISO-DEP payloads, protocol managedd
    NFCDEV_INTERFACE_ISO_DEP = 4,

    /// NFC-DEP payloads, protocol managed
    NFCDEV_INTERFACE_NFC_DEP = 5,
} nfcdev_interface_t;

typedef union __attribute__((packed))  {
    struct {
        nfcdev_interface_t interface : 3;
        bool use_nad : 1;
        bool use_did : 1;
        bool slice : 1;
        bool reassemble : 1;
        bool _rfu0 : 1;
        uint8_t trailing_bits : 3;
        uint8_t _rfu : 5;
    };
    uint16_t _encoded;
} nfcdev_nfio_flags_t;
