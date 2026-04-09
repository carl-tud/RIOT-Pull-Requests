#pragma once

#include <stdint.h>
#include "net/nfc/constants.h"
#include "net/nfc/iso_dep.h"

typedef enum __attribute__((packed)) {
    /// Only awake
    NFC_A_POLLING_COMMAND_GROUP_ONLY_AWAKE = 0x26,
    /// Sleeping and awake
    NFC_A_POLLING_COMMAND_GROUP_ALL = 0x52,
    /// Participating in timeslot method
    NFC_A_POLLING_COMMAND_GROUP_TIMESLOT = 0x35,
} nfc_a_polling_command_t;

#define NFC_REQA NFC_A_POLLING_COMMAND_GROUP_ONLY_AWAKE
#define NFC_WUPA NFC_A_POLLING_COMMAND_GROUP_ALL

typedef enum __attribute__((packed)) {
    /// 4
    NFC_A_UID_SIZE_SINGLE_4 = 4,
    /// 7
    NFC_A_UID_SIZE_DOUBLE_7 = 7,
    /// 10
    NFC_A_UID_SIZE_TRIPLE_10 = 10,
} nfc_a_uid_size_t;


#define _nfc_a_uid_size_indicator(size) (((uint8_t)(size) - 4) / 3)

static inline uint8_t nfc_a_uid_size_indictor(nfc_a_uid_size_t size) {
    return _nfc_a_uid_size_indicator(size);
}

#define _nfc_a_uid_size(indicator) (3 * (indicator) + 4)

static inline nfc_a_uid_size_t nfc_a_uid_size(uint8_t indicator) {
    return _nfc_a_uid_size(indicator);
}

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

#define NFC_A_SDD_CASCADE_TAG (0x88)

#define NFC_A_SELECT_COMMAND_CASCADE_LEVEL1 (0x93)
#define NFC_A_SELECT_COMMAND_CASCADE_LEVEL2 (0x95)
#define NFC_A_SELECT_COMMAND_CASCADE_LEVEL3 (0x97)

#define NFC_A_NFCID1_SIZE_MASK (0xC0)

typedef uint8_t nfc_a_acknowledgement_t;

#define NFC_A_ACKNOWLEDGEMENT_MASK_UID_COMPLETE (0x04)
#define NFC_A_ACKNOWLEDGEMENT_MASK_ISO_DEP (0x20)
#define NFC_A_ACKNOWLEDGEMENT_MASK_NFC_DEP (0x40)
#define NFC_A_ACKNOWLEDGEMENT_MASK_T2T (0x60)

static inline bool nfc_a_uid_complete(nfc_a_acknowledgement_t sak) {
    return (sak & NFC_A_ACKNOWLEDGEMENT_MASK_UID_COMPLETE) != 0;
}

static inline bool nfc_a_supports_iso_dep(nfc_a_acknowledgement_t sak) {
    return (sak & NFC_A_ACKNOWLEDGEMENT_MASK_ISO_DEP) != 0;
}

static inline bool nfc_a_supports_nfc_dep(nfc_a_acknowledgement_t sak) {
    return (sak & NFC_A_ACKNOWLEDGEMENT_MASK_NFC_DEP) != 0;
}

static inline bool nfc_a_supports_t2t(nfc_a_acknowledgement_t sak) {
    return (sak & NFC_A_ACKNOWLEDGEMENT_MASK_T2T) != 0;
}

#define NFC_A_SLEEP_COMMAND0 (0x50)
#define NFC_A_SLEEP_COMMAND1 (0x00)

typedef uint8_t nfc_a_sleep_command_t[2];

#define NFC_A_RATS_PREFIX (0xE0)

typedef union __attribute__((packed)) {
    struct {
        uint8_t prefix;
        uint8_t cid : 4;
        iso_dep_frame_size_t max_frame_size : 4;
    } __attribute__((packed));

    uint8_t raw[2];
} nfc_a_rats_t;

#define NFC_A_RATS_MASK_FSDI   (0xF0)
#define NFC_A_RATS_MASK_CID    (0x0F)

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

    uint8_t raw;
} nfc_a_pps_request_t;

#define _iso_dep_bitrate_from_divisor_power(power) ((nfc_bitrate_t)(1 << (power)))

static inline nfc_bitrate_t iso_dep_bitrate_from_divisor_power(uint8_t power) {
    assert(power <= 3);
    return _iso_dep_bitrate_from_divisor_power(power);
}

#define _iso_dep_bitrate_divisor_power(bitrate) ((uint8_t)__builtin_ctz((uint8_t)(bitrate)))

static inline uint8_t iso_dep_bitrate_divisor_power(nfc_bitrate_t bitrate) {
    assert(bitrate >= NFC_BITRATE_106K);
    assert(bitrate <= NFC_BITRATE_848K);
    return _iso_dep_bitrate_divisor_power(bitrate);
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

    uint8_t raw[3];
} nfc_a_pps_response_t;

typedef struct {
    nfc_a_uid_size_t len;
    uint8_t nfcid[10];
    nfc_a_rats_t rats;
} nfc_a_uid_t;

typedef struct {
    nfc_a_polling_command_t polling_command;
    nfc_a_uid_t uid;
    uint8_t acknowledgement;
} nfc_a_poll_config_t;

typedef struct {
    nfc_a_polling_response_t polling_response;
    nfc_a_uid_t uid;
    nfc_a_acknowledgement_t acknowledgement;
    nfc_a_rats_t rats;
} nfc_a_listen_parameters_t;

typedef struct {
    nfc_a_polling_response_t polling_response;
    nfc_a_uid_t uid;
    nfc_a_acknowledgement_t acknowledgement;
    
    struct {
        struct {
            nfc_bitrate_t upstream;
            nfc_bitrate_t downstream;
            bool allow_lower_bitrates : 1;
            bool same_bitrate_both_directions : 1;
            uint8_t historical[];
        } iso_dep;

        struct {

        } nfc_dep;
    } higher_layer;
} nfc_a_listen_config_t;

nfc_application_type_t nfc_a_get_application_type(nfc_a_polling_response_t polling_response, uint8_t acknowledgement);

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
