#pragma once

#include <stdint.h>
#include "assert.h"
#include "net/nfc/constants.h"

#define ISO_DEP_PCB_I_BLOCK_FIXED_VALUE       (0x02u)

#define ISO_DEP_PCB_BLOCK_NUMBER_MASK (0x01u)
#define ISO_DEP_PCB_NAD_MASK          (0x04u)
#define ISO_DEP_PCB_DID_MASK          (0x08u)
#define ISO_DEP_PCB_CHAINING_MASK     (0x10u)

#define ISO_DEP_PCB_BLOCK_TYPE_MASK         (0xC0u)
#define ISO_DEP_PCB_BLOCK_TYPE_I_VALUE      (0x00u)
#define ISO_DEP_PCB_BLOCK_TYPE_R_VALUE      (0x80u)
#define ISO_DEP_PCB_BLOCK_TYPE_S_VALUE      (0xC0u)

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

typedef union __attribute__((packed)) {
    struct {
        uint8_t down : 3;
        bool _zero: 1;
        uint8_t up : 3;
        bool same_constraint : 1;
    } __attribute__((packed));

    uint8_t byte;
} iso_dep_bitrate_capabilities_t;
