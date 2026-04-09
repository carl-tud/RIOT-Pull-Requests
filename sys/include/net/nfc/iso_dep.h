#pragma once

#include <stdint.h>
#include "assert.h"
#include "net/nfc/constants.h"

#define NFC_ISO_DEP_PCB_I_BLOCK_FIXED_VALUE       (0x02u)

#define NFC_ISO_DEP_PCB_BLOCK_NUMBER_MASK (0x01u)
#define NFC_ISO_DEP_PCB_NAD_MASK          (0x04u)
#define NFC_ISO_DEP_PCB_DID_MASK          (0x08u)
#define NFC_ISO_DEP_PCB_CHAINING_MASK     (0x10u)

#define NFC_ISO_DEP_PCB_BLOCK_TYPE_MASK         (0xC0u)
#define NFC_ISO_DEP_PCB_BLOCK_TYPE_I_VALUE      (0x00u)
#define NFC_ISO_DEP_PCB_BLOCK_TYPE_R_VALUE      (0x80u)
#define NFC_ISO_DEP_PCB_BLOCK_TYPE_S_VALUE      (0xC0u)

typedef enum {
    ISO_DEP_BLOCK_TYPE_UNKNOWN = 0,
    ISO_DEP_BLOCK_TYPE_I,
    ISO_DEP_BLOCK_TYPE_R,
    ISO_DEP_BLOCK_TYPE_S,
} iso_dep_block_type_t;

typedef enum __attribute__((packed)) {
    ISO_DEP_FRAME_SIZE_16 = 0,
    ISO_DEP_FRAME_SIZE_24,
    ISO_DEP_FRAME_SIZE_32,
    ISO_DEP_FRAME_SIZE_40,
    ISO_DEP_FRAME_SIZE_48,
    ISO_DEP_FRAME_SIZE_64,
    ISO_DEP_FRAME_SIZE_96,
    ISO_DEP_FRAME_SIZE_128,
    ISO_DEP_FRAME_SIZE_356,
    ISO_DEP_FRAME_SIZE_512,
    ISO_DEP_FRAME_SIZE_1024,
    ISO_DEP_FRAME_SIZE_2048,
    ISO_DEP_FRAME_SIZE_4096,
} iso_dep_frame_size_t;

static inline size_t iso_dep_frame_size(iso_dep_frame_size_t integer) {
    extern uint16_t iso_dep_frame_sizes[];
    return (size_t)iso_dep_frame_sizes[integer];
}

static inline iso_dep_frame_size_t iso_dep_byte_bound_to_frame_size(size_t upper_bound) {
    if (upper_bound < 16) {
        return ISO_DEP_FRAME_SIZE_16;
    }
    extern uint16_t iso_dep_frame_sizes[];
    for (size_t s = 0; s <= ISO_DEP_FRAME_SIZE_4096; s += 1) {
        if (iso_dep_frame_size(s) > upper_bound) {
            return s - 1;
        }
    }
    return ISO_DEP_FRAME_SIZE_4096;
}

#define ISO_DEP_RATS_PREFIX (0xE0)

typedef struct __attribute__((packed)) {
    uint8_t prefix;
    uint8_t cid : 4;
    iso_dep_frame_size_t max_frame_size : 4;
} iso_dep_rats_t;

typedef struct __attribute__((packed)) {
    uint8_t down : 3;
    bool _zero: 1;
    uint8_t up : 3;
    bool same_constraint : 1;
} iso_dep_bitrate_capabilities_t;

typedef struct __attribute__((packed)) {
    iso_dep_bitrate_capabilities_t bitrates;
} iso_dep_ats_t1_t;

typedef struct __attribute__((packed)) {
    /// SFGT defines a specific guard time needed by the PICC before it is ready to receive the next frame after it has sent the ATS.
    nfc_time_index_t startup_frame_guard_time : 4;
    nfc_time_index_t frame_waiting_time : 4;
} iso_dep_ats_t2_t;

typedef struct __attribute__((packed)) {
    bool nad_supported : 1;
    bool cid_supported : 1;
    uint8_t _rfu1 : 6;
} iso_dep_ats_t3_t;

typedef struct __attribute__((packed)) {
    uint8_t length;
    iso_dep_frame_size_t max_frame_size : 4;
    bool ta_present : 1;
    bool tb_present : 1;
    bool tc_present : 1;
    bool _rfu0 : 1;
    iso_dep_ats_t1_t t1;
    iso_dep_ats_t2_t t2;
    iso_dep_ats_t3_t t3;
    uint8_t historical[];
} iso_dep_ats_t;

#define ISO_DEP_PPS_PREFIX (0b1101)

typedef struct __attribute__((packed)) {
    uint8_t prefix : 4;
    uint8_t cid : 4;
} iso_dep_pps_request_t;

typedef struct __attribute__((packed)) {
    uint8_t down_bitrate_divisor_power : 2;
    uint8_t up_bitrate_divisor_power : 2;
    uint8_t _rfu : 4;
} iso_dep_pps1_t;

static inline nfc_bitrate_t iso_dep_bitrate_from_divisor_power(uint8_t power) {
    assert(power <= 3);
    return (nfc_bitrate_t)(1 << power);
}

static inline uint8_t iso_dep_bitrate_divisor_power(nfc_bitrate_t bitrate) {
    assert(bitrate >= NFC_BITRATE_106K);
    assert(bitrate <= NFC_BITRATE_848K);
    return (uint8_t)__builtin_ctz((uint8_t)bitrate);
}

typedef struct __attribute__((packed)) {
    uint8_t prefix : 4;
    uint8_t cid : 4;
    uint8_t _fixed0 : 4;
    bool pps1_present : 1;
    uint8_t _fixed1 : 3;
    iso_dep_pps1_t pps1;
} iso_dep_pps_response_t;
