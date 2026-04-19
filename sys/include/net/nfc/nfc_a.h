#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "net/nfc/constants.h"
#include "net/nfc/iso_dep.h"
#include "net/nfc/nfc_dep.h"

/// @brief Appends CRC-A
/// @param full_length Length of packet in bytes including 2-byte CRC-A
/// @param[in,out] packet Packet to write CRC-A into
void nfc_a_crc_append(uint8_t* packet, size_t full_length);

// MARK: - UID
/// @name UID
/// @{

typedef enum __attribute__((packed)) {
    /// 4
    NFC_A_UID_LENGTH_SINGLE_4 = 4,
    /// 7
    NFC_A_UID_LENGTH_DOUBLE_7 = 7,
    /// 10
    NFC_A_UID_LENGTH_TRIPLE_10 = 10,
} nfc_a_id_length_t;

#define NFC_A_ID_LENGTH_MAX  (NFC_A_UID_LENGTH_TRIPLE_10)

typedef struct __attribute__((packed)) {
    nfc_a_id_length_t length;
    uint8_t uid[10];
} nfc_a_id_t;

#define NFC_A_ID_LENGTH_INDICATOR(size) (((uint8_t)(size) - 4) / 3)

static inline uint8_t nfc_a_id_length_indictor(nfc_a_id_length_t size) {
    return NFC_A_ID_LENGTH_INDICATOR(size);
}

#define NFC_A_ID_LENGTH(indicator) (3 * (indicator) + 4)

static inline nfc_a_id_length_t nfc_a_id_length(uint8_t indicator) {
    return NFC_A_ID_LENGTH(indicator);
}

#define NFC_A_UID_CASCADE_TAG (0x88)

/// @}

// MARK: - Polling command
/// @name Polling command
/// @{

/// Only awake `REQ`/`SENS_REQ`
#define NFC_A_FRAME_CODE_POLLING_ONLY_AWAKE (0x26)
#define NFC_A_FRAME_CODE_REQA NFC_A_FRAME_CODE_POLLING_ONLY_AWAKE

/// Sleeping and awake `WUPA`/`ALL_REQ`
#define NFC_A_FRAME_CODE_POLLING_ALL (0x52)
#define NFC_A_FRAME_CODE_WUPA NFC_A_FRAME_CODE_POLLING_ALL

/// Participating in timeslot method
#define NFC_A_FRAME_CODE_POLLING_TIMESLOT (0x35)

typedef uint8_t nfc_a_polling_command_t;

typedef struct {
    const uint8_t* frame;
    nfc_frame_length_t length;
} nfc_a_polling_frame_t;

extern const nfc_a_polling_frame_t nfc_a_polling_frame_all;
extern const nfc_a_polling_frame_t nfc_a_polling_frame_only_awake;

/// @}

// MARK: - Polling response
/// @name Polling response
/// @{

typedef union __attribute__((packed)) {
    struct {
        uint8_t bit_frame_anticollision : 5;
        bool _rfu0 : 1;
        uint8_t uid_size_indicator : 2;
        uint8_t proprietary : 4;
        uint8_t _rfu1 : 4;
    } __attribute__((packed));

    struct {
        uint8_t anticollision_info;
        uint8_t platform_info;
    } __attribute__((packed));
    
    uint8_t raw[2];
} nfc_a_polling_response_t;

/// @}

// MARK: - Select command
/// @name Select command
/// @{

#define NFC_A_FRAME_CODE_SELECT_CL1 (0x93)
#define NFC_A_FRAME_CODE_SELECT_CL2 (0x95)
#define NFC_A_FRAME_CODE_SELECT_CL3 (0x97)

///
/// for byte counts 6 and 7, only a bit count of 0 is allowed
typedef struct __attribute__((packed)) {
    uint8_t code;

    union {
        struct {
            uint8_t trailing_bit_count : 4;
            uint8_t full_byte_count : 4;
        };
        uint8_t nvb;
    } __attribute__((packed));

    uint8_t uid_fragment[];
} nfc_a_select_command_t;

#define NFC_A_ANTICOLLISION_LOOPS_MAX (32)

/// @}

// MARK: - Select response
/// @name Select response
/// @{

#define NFC_A_SELECT_RESPONSE_MASK_UID_COMPLETE (0x04)
#define NFC_A_SELECT_RESPONSE_MASK_ISO_DEP (0x20)
#define NFC_A_SELECT_RESPONSE_MASK_NFC_DEP (0x40)
#define NFC_A_SELECT_RESPONSE_MASK_T2T (0x60)

typedef uint8_t nfc_a_select_response_t;

static inline bool nfc_a_uid_complete(nfc_a_select_response_t sak) {
    return (sak & NFC_A_SELECT_RESPONSE_MASK_UID_COMPLETE) != 0;
}

static inline bool nfc_a_supports_iso_dep(nfc_a_select_response_t sak) {
    return (sak & NFC_A_SELECT_RESPONSE_MASK_ISO_DEP) != 0;
}

static inline bool nfc_a_supports_nfc_dep(nfc_a_select_response_t sak) {
    return (sak & NFC_A_SELECT_RESPONSE_MASK_NFC_DEP) != 0;
}

static inline bool nfc_a_supports_t2t(nfc_a_select_response_t sak) {
    return (sak & NFC_A_SELECT_RESPONSE_MASK_T2T) != 0;
}

/// @}

// MARK: - Halt command
/// @name Halt command
/// @{

#define NFC_A_FRAME_CODE_HALT (0x50)
#define NFC_A_FRAME_PAYLOAD_HALT (0x00)

typedef union __attribute__((packed)) {
    struct {
        uint8_t code;
        uint8_t zero;
    } __attribute__((packed));

    uint8_t raw[2];
} nfc_a_halt_command_t;

/// @}

// MARK: - RATS
/// @name Halt command
/// @{

typedef union __attribute__((packed)) {
    struct {
        uint8_t cid : 4;
        iso_dep_frame_size_t max_frame_size : 4;
    } __attribute__((packed));

    uint8_t raw;
} nfc_a_rats_payload_t;

#define NFC_A_FRAME_CODE_RATS (0xE0)

typedef union __attribute__((packed)) {
    struct {
        uint8_t code;
        nfc_a_rats_payload_t payload;
    } __attribute__((packed));

    uint8_t raw[sizeof(nfc_a_rats_payload_t) + 1];
} nfc_a_rats_t;

/// @}

// MARK: - ATS
/// @name ATS
/// @{

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

/// @}

// MARK: - PPS command
/// @name PPS command
/// @{

#define NFC_A_FRAME_CODE_UPPER_PPS (0b1101)

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

    uint8_t raw[3];
} nfc_a_pps_request_t;

/// @}

// MARK: - PPS response
/// @name PPS response
/// @{

typedef union __attribute__((packed)) {
    struct {
        uint8_t code : 4;
        uint8_t cid : 4;
    } __attribute__((packed));

    uint8_t raw;
} nfc_a_pps_response_t;

/// @}

nfc_application_type_t nfc_a_determine_application_type(nfc_a_polling_response_t* polling_response, uint8_t acknowledgement);

/// @name Proprietary polling responses and acknowledgements
/// @{
#define NFC_A_POLLING_RESPONSE0_MIFARE_MINI             0x00
#define NFC_A_POLLING_RESPONSE1_MIFARE_MINI             0x04
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_MINI               0x09

#define NFC_A_POLLING_RESPONSE0_MIFARE_CLASSIC_1K       0x00
#define NFC_A_POLLING_RESPONSE1_MIFARE_CLASSIC_1K       0x04
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_CLASSIC_1K         0x08

#define NFC_A_POLLING_RESPONSE0_MIFARE_CLASSIC_4K       0x00
#define NFC_A_POLLING_RESPONSE1_MIFARE_CLASSIC_4K       0x02
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_CLASSIC_4K         0x18

#define NFC_A_POLLING_RESPONSE0_MIFARE_ULTRALIGHT       0x00
#define NFC_A_POLLING_RESPONSE1_MIFARE_ULTRALIGHT       0x44
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_ULTRALIGHT         0x00

#define NFC_A_POLLING_RESPONSE0_MIFARE_DESFIRE          0x03
#define NFC_A_POLLING_RESPONSE1_MIFARE_DESFIRE          0x44
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_DESFIRE            0x20

#define NFC_A_POLLING_RESPONSE0_MIFARE_PLUS_2K_SL1      0x00
#define NFC_A_POLLING_RESPONSE1_MIFARE_PLUS_2K_SL1      0x04
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_PLUS_2K_SL1        0x08

#define NFC_A_POLLING_RESPONSE0_MIFARE_PLUS_4K_SL1      0x00
#define NFC_A_POLLING_RESPONSE1_MIFARE_PLUS_4K_SL1      0x02
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_PLUS_4K_SL1        0x18

#define NFC_A_POLLING_RESPONSE0_MIFARE_PLUS_SL3         0x03
#define NFC_A_POLLING_RESPONSE1_MIFARE_PLUS_SL3         0x44
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_PLUS_SL3           0x20

#define NFC_A_POLLING_RESPONSE0_MIFARE_SMARTMX          0x00
#define NFC_A_POLLING_RESPONSE1_MIFARE_SMARTMX          0x04
#define NFC_A_ACKNOWLEDGEMENT_MIFARE_SMARTMX            0x08
/// @}

typedef struct {
    nfc_a_id_t* id;
    nfc_a_polling_response_t polling_response_mask;
    nfc_a_select_response_t select_response_mask;
} nfc_a_polling_filter_t;

typedef struct {
    nfc_a_polling_frame_t* frames;
    size_t frame_count;

    nfc_a_polling_filter_t* filter;

    nfc_a_rats_t* rats;
} nfc_a_tag_polling_config_t;

typedef struct {
    nfc_a_id_t* id;
    nfc_a_polling_response_t polling_response;
    nfc_a_select_response_t select_response;
    nfc_a_ats_t* ats;
} nfc_a_tag_t;
