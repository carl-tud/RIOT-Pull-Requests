#pragma once

#include <stdint.h>
#include "net/nfc/constants.h"

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
    NFC_A_UID_SIZE_SINGLE = 0,
    /// 7
    NFC_A_UID_SIZE_DOUBLE = 1,
    /// 10
    NFC_A_UID_SIZE_TRIPLE = 2,
} nfc_a_uid_size_t;

typedef union __attribute__((packed)) {
    struct {
        uint8_t bit_frame_anticollision : 5;
        bool _rfu0 : 1;
        nfc_a_uid_size_t uid_size : 2;
        uint8_t proprietary : 4;
        uint8_t _rfu1 : 4;
    } __attribute__((packed));

    struct {
        uint8_t anticollision_info;
        uint8_t platform_info;
    } __attribute__((packed));
    
    uint8_t bytes[2];
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



typedef struct {
    nfc_a_nfcid1_len_t len;
    uint8_t nfcid[10];
} nfc_a_nfcid1_t;

typedef struct {
    nfc_a_nfcid1_t nfcid1;
    nfc_a_bitrate_t bitrate;
} nfc_a_target_t;

typedef struct {
    nfc_a_sens_res_t sens_res;
    nfc_a_nfcid1_t nfcid1;
    uint8_t sel_res;
} nfc_a_listener_config_t;

nfc_application_type_t nfc_a_get_application_type(nfc_a_sens_res_t sens_res, uint8_t sel_res);


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
