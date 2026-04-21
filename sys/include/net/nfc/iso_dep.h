#pragma once

#include <stdint.h>
#include "assert.h"
#include "net/nfc/constants.h"



#define ISO_DEP_PCB_MASK_BLOCK_TYPE    (0xC0)
#define ISO_DEP_PCB_MASK_BLOCK_TYPE_VALUE_I    (0x40)
#define ISO_DEP_PCB_MASK_BLOCK_TYPE_VALUE_R    (0x80)
#define ISO_DEP_PCB_MASK_BLOCK_TYPE_VALUE_S    (0xC0)

#define ISO_DEP_PCB_FLAG_CID_FOLLOWING (0x80)

#define ISO_DEP_PCB_I_BLOCK_MASK_BLOCK_NUMBER  (0x01)
#define ISO_DEP_PCB_I_BLOCK_FLAG_NAD_FOLLOWING (0x04)
#define ISO_DEP_PCB_I_BLOCK_FLAG_CID_FOLLOWING (0x08)
#define ISO_DEP_PCB_I_BLOCK_FLAG_CHAINING      (0x10)

#define ISO_DEP_PCB_R_BLOCK_MASK_BLOCK_NUMBER  (0x01)
#define ISO_DEP_PCB_R_BLOCK_FLAG_ALWAYS_ONE    (0x22)
#define ISO_DEP_PCB_R_BLOCK_FLAG_CID_FOLLOWING ISO_DEP_PCB_FLAG_CID_FOLLOWING
#define ISO_DEP_PCB_R_BLOCK_FLAG_NEGATIVE_ACK  (0x10)

#define ISO_DEP_PCB_S_BLOCK_FLAG_CID_FOLLOWING ISO_DEP_PCB_FLAG_CID_FOLLOWING
#define ISO_DEP_PCB_S_BLOCK_MASK_KIND          (0x32)
#define ISO_DEP_PCB_S_BLOCK_MASK_KIND_VALUE_PARAMETERS    (0x30)
#define ISO_DEP_PCB_S_BLOCK_MASK_KIND_VALUE_DESELECT      (0x02)
#define ISO_DEP_PCB_S_BLOCK_MASK_KIND_VALUE_WTX           (0x32)

typedef enum __attribute__((packed)) {
    ISO_DEP_BLOCK_TYPE_UNKNOWN = 0,
    ISO_DEP_BLOCK_TYPE_I = 1,
    ISO_DEP_BLOCK_TYPE_R = 2,
    ISO_DEP_BLOCK_TYPE_S = 3,
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

typedef union __attribute__((packed)) {
    struct {
        uint8_t down : 3;
        bool _zero : 1;
        uint8_t up : 3;
        bool same_constraint : 1;
    } __attribute__((packed));

    uint8_t byte;
} iso_dep_bitrate_capabilities_t;

#define ISO_DEP_BITRATE_FROM_DIVISOR_POWER(power) ((nfc_bitrate_t)(1 << (power)))

static inline nfc_bitrate_t iso_dep_bitrate_from_divisor_power(uint8_t power) {
    assert(power <= 3);
    return ISO_DEP_BITRATE_FROM_DIVISOR_POWER(power);
}

#define ISO_DEP_BITRATE_DIVISOR_POWER(bitrate) ((uint8_t)__builtin_ctz((uint8_t)(bitrate)))

static inline uint8_t iso_dep_bitrate_divisor_power(nfc_bitrate_t bitrate) {
    assert(bitrate >= NFC_BITRATE_106K);
    assert(bitrate <= NFC_BITRATE_848K);
    return ISO_DEP_BITRATE_DIVISOR_POWER(bitrate);
}

#define ISO_DEP_PARAMETERS_REQUEST_TAG                      (0xA0)
#define ISO_DEP_PARAMETERS_INDICATION_TAG                   (0xA1)
#define ISO_DEP_PARAMETERS_ACTIVATION_TAG                   (0xA2)
#define ISO_DEP_PARAMETERS_ACKNOWLEDGEMENT_TAG              (0xA3)

#define ISO_DEP_PARAMETERS_INDICATION_DOWNSTREAM_TAG        (0x80)
#define ISO_DEP_PARAMETERS_INDICATION_UPSTREAM_TAG          (0x81)
#define ISO_DEP_PARAMETERS_INDICATION_FRAMING_OPTIONS_TAG   (0x82)

#define ISO_DEP_PARAMETERS_ACTIVATION_DOWNSTREAM_TAG        (0x83)
#define ISO_DEP_PARAMETERS_ACTIVATION_UPSTREAM_TAG          (0x84)
#define ISO_DEP_PARAMETERS_ACTIVATION_FRAMING_OPTIONS_TAG   (0x85)

#define ISO_DEP_PARAMETERS_FRAMING_UPSTREAM_START_STOP_BIT_SUPPRESION (1)
#define ISO_DEP_PARAMETERS_FRAMING_UPSTREAM_SOF_EOF_SUPPRESION   (1 << 1)
