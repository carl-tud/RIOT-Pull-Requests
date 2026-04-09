#pragma once

#include <stdint.h>
#include "assert.h"

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

typedef enum {
    NFC_TECHNOLOGY_A = 0,
    NFC_TECHNOLOGY_B,
    NFC_TECHNOLOGY_F,
    NFC_TECHNOLOGY_V,
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

/// Bit Rate for Near Field Communication
///
/// The `divisor` associated with each enum case is the bit rate's corresponding divisor `D`. `fc` is the ``NFCCarrierFrequency``.
///
/// - `1 etu = 128/fc / D`
/// - `bitRate = D · fc/128`
///
/// - Note: You should use the ``BitRate(_:)`` macro instead of directly accessing this enum's cases.
typedef enum {
    NFC_BITRATE_106K   = NFC_BITRATE_DIVISOR_FC_128,
    NFC_BITRATE_212K   = NFC_BITRATE_DIVISOR_FC_64,
    NFC_BITRATE_424K   = NFC_BITRATE_DIVISOR_FC_32,
    NFC_BITRATE_848K   = NFC_BITRATE_DIVISOR_FC_16,
    NFC_BITRATE_1695K  = NFC_BITRATE_DIVISOR_FC_8,
    NFC_BITRATE_3390K  = NFC_BITRATE_DIVISOR_FC_4,
    NFC_BITRATE_6780K  = NFC_BITRATE_DIVISOR_FC_2,
    NFC_BITRATE_10170K = NFC_BITRATE_DIVISOR_FC3_4,
    NFC_BITRATE_13560K = NFC_BITRATE_DIVISOR_FC,
    NFC_BITRATE_20340K = NFC_BITRATE_DIVISOR_FC3_2,
    NFC_BITRATE_27120K = NFC_BITRATE_DIVISOR_FC2,
} nfc_bitrate_t;

typedef uint8_t nfc_time_index_t;

#define NFC_CARRIER_FREQUENCY_HZ (13560000)

static inline uint16_t nfc_time_index_ms(nfc_time_index_t index) {
    assert(index <= 0xf);
    return 256 * 16/NFC_CARRIER_FREQUENCY_HZ * 1000 * (1 << index);
}

