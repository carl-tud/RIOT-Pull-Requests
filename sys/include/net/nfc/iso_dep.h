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

#define NFC_A_RATS_PREFIX (0xE0)

typedef union __attribute__((packed)) {
    struct {
        uint8_t prefix;
        uint8_t cid : 4;
        iso_dep_frame_size_t max_frame_size : 4;
    } __attribute__((packed));

    uint8_t bytes[2];
} nfc_a_rats_t;

#define NFC_A_RATS_MASK_FSDI   (0xF0)
#define NFC_A_RATS_MASK_CID    (0x0F)

typedef union __attribute__((packed)) {
    struct {
        uint8_t down : 3;
        bool _zero: 1;
        uint8_t up : 3;
        bool same_constraint : 1;
    } __attribute__((packed));

    uint8_t byte;
} iso_dep_bitrate_capabilities_t;

typedef struct __attribute__((packed)) {
    uint8_t length;

    union {
        struct {
            iso_dep_frame_size_t max_frame_size : 4;
            bool ta_present : 1;
            bool tb_present : 1;
            bool tc_present : 1;
            bool _rfu0 : 1;
        } __attribute__((packed));

        uint8_t t0;
    };

    union {
        struct {
            iso_dep_bitrate_capabilities_t bitrates;
        } __attribute__((packed)) ta_info;

        uint8_t ta;
    } __attribute__((packed));

    union {
        struct {
            /// SFGT defines a specific guard time needed by the PICC before it is ready to receive the next frame after it has sent the ATS.
            nfc_time_index_t startup_frame_guard_time : 4;
            nfc_time_index_t frame_waiting_time : 4;
        } __attribute__((packed)) tb_info;

        uint8_t tb;
    } __attribute__((packed));

    union {
        struct {
            bool nad_supported : 1;
            bool cid_supported : 1;
            uint8_t _rfu1 : 6;
        } __attribute__((packed)) tc_info;

        uint8_t tc;
    } __attribute__((packed));

    uint8_t historical[];
} nfc_a_ats_t;

#define NFC_A_ATS_MASK_T0_TA1_PRESENT   (0x10)
#define NFC_A_ATS_MASK_T0_TB1_PRESENT   (0x20)
#define NFC_A_ATS_MASK_T0_TC1_PRESENT   (0x40)
#define NFC_A_ATS_MASK_T0_TD1_PRESENT   (0x80)
#define NFC_A_ATS_MASK_T0_FSCI          (0x0F)
#define NFC_A_ATS_MASK_TA1_FWI          (0xF0)
#define NFC_A_ATS_MASK_TA1_SFGI         (0x0F)
#define NFC_A_ATS_MASK_TB1_SFGI         (0xF0)
#define NFC_A_ATS_MASK_TB1_CRC          (0x01)
#define NFC_A_ATS_MASK_TC1_FD           (0x0F)
#define NFC_A_ATS_MASK_TC1_SDC          (0x70)
#define NFC_A_ATS_MASK_TC1_DRC          (0x0E)
#define NFC_A_ATS_MASK_TC1_DSI          (0x01)

#define NFC_A_PPS_PREFIX (0b1101)

typedef union __attribute__((packed)) {
    struct {
        uint8_t prefix : 4;
        uint8_t cid : 4;
    } __attribute__((packed));

    uint8_t byte;
} nfc_a_pps_request_t;

static inline nfc_bitrate_t iso_dep_bitrate_from_divisor_power(uint8_t power) {
    assert(power <= 3);
    return (nfc_bitrate_t)(1 << power);
}

static inline uint8_t iso_dep_bitrate_divisor_power(nfc_bitrate_t bitrate) {
    assert(bitrate >= NFC_BITRATE_106K);
    assert(bitrate <= NFC_BITRATE_848K);
    return (uint8_t)__builtin_ctz((uint8_t)bitrate);
}

typedef union __attribute__((packed)) {
    struct {
        union {
            struct {
                uint8_t prefix : 4;
                uint8_t cid : 4;
            } __attribute__((packed));
            uint8_t ppss;
        } __attribute__((packed));

        union {
            struct {
                uint8_t _fixed0 : 4;
                bool pps1_present : 1;
                uint8_t _fixed1 : 3;
            } __attribute__((packed));
            uint8_t pps0;
        } __attribute__((packed));

        union {
            struct {
                uint8_t down_bitrate_divisor_power : 2;
                uint8_t up_bitrate_divisor_power : 2;
                uint8_t _rfu : 4;
            } __attribute__((packed)) pps1_info;
            uint8_t pps1;
        } __attribute__((packed));
    } __attribute__((packed));

    uint8_t bytes[3];
} nfc_a_pps_response_t;
