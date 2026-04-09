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
    NFC_A_UID_SIZE_SINGLE = 0,
    NFC_A_UID_SIZE_DOUBLE = 1,
    NFC_A_UID_SIZE_TRIPLE = 2,
} nfc_a_uid_size_t;

typedef struct __attribute__((packed)) {
    uint8_t bit_frame_anticollision : 5;
    bool _rfu0 : 1;
    nfc_a_uid_size_t : 2;
    uint8_t proprietary : 4;
    uint8_t _rfu1 : 4;
} nfc_a_polling_response_t;

typedef nfc_a_polling_response_t nfc_atqa_t;

#define NFC_A_SDD_COMMAND_CASCADE_LEVEL1 (0x93)
#define NFC_A_SDD_COMMAND_CASCADE_LEVEL2 (0x95)
#define NFC_A_SDD_COMMAND_CASCADE_LEVEL3 (0x97)

typedef uint8_t nfc_a_acknowledgement_t;

#define NFC_A_SAK_MASK_ISO_DEP (0x20)
#define NFC_A_SAK_MASK_NFC_DEP (0x40)

static inline bool nfc_a_supports_iso_dep(nfc_a_acknowledgement_t sak) {
    return (sak & NFC_A_SAK_MASK_ISO_DEP) != 0;
}

static inline bool nfc_a_supports_nfc_dep(nfc_a_acknowledgement_t sak) {
    return (sak & NFC_A_SAK_MASK_NFC_DEP) != 0;
}

#define NFC_A_SLEEP_COMMAND0 (0x50)
#define NFC_A_SLEEP_COMMAND1 (0x00)

typedef uint8_t nfc_a_sleep_command_t[2];

#define NFC_HLTA0 (0x50)
#define NFC_HLTA1 (0x00)

/**
 * @brief Sense Response (SENS_RES) command
 */
#define NFC_A_SENS_RES_ANTICOLLISION_NFCID1_SIZE_MASK (0xC0)

/**
 * @brief Select Response (SEL_RES) command
 */
#define NFC_A_SEL_RES_LEN  (1u)

#define NFC_A_SEL_RES_T2T_VALUE (0x00)
#define NFC_A_SEL_RES_T2T_MASK  (0x60)

#define NFC_A_SEL_RES_T4T_VALUE (0x20)
#define NFC_A_SEL_RES_T4T_MASK  (0x20)

#define NFC_A_SEL_RES_DEP_VALUE (0x40)
#define NFC_A_SEL_RES_DEP_MASK  (0x40)

#define NFC_A_SEL_RES_NFCID1_COMPLETE_VALUE (0x00)
#define NFC_A_SEL_RES_NFCID1_COMPLETE_MASK  (0x04)

/**
 * @brief Proprietary bits for MIFARE tags, SENS_RES and SEL_RES values
 * 
 */
#define NFC_A_MIFARE_MINI_SENS_RES_PLATFORM            0x00
#define NFC_A_MIFARE_MINI_SENS_RES_ANTICOLLISION       0x04
#define NFC_A_MIFARE_MINI_SEL_RES                      0x09

#define NFC_A_MIFARE_CLASSIC_1K_SENS_RES_PLATFORM      0x00
#define NFC_A_MIFARE_CLASSIC_1K_SENS_RES_ANTICOLLISION 0x04
#define NFC_A_MIFARE_CLASSIC_1K_SEL_RES                0x08

#define NFC_A_MIFARE_CLASSIC_4K_SENS_RES_PLATFORM      0x00
#define NFC_A_MIFARE_CLASSIC_4K_SENS_RES_ANTICOLLISION 0x02
#define NFC_A_MIFARE_CLASSIC_4K_SEL_RES                0x18

#define NFC_A_MIFARE_ULTRALIGHT_SENS_RES_PLATFORM      0x00
#define NFC_A_MIFARE_ULTRALIGHT_SENS_RES_ANTICOLLISION 0x44
#define NFC_A_MIFARE_ULTRALIGHT_SEL_RES                0x00

#define NFC_A_MIFARE_DESFIRE_SENS_RES_PLATFORM         0x03
#define NFC_A_MIFARE_DESFIRE_SENS_RES_ANTICOLLISION    0x44
#define NFC_A_MIFARE_DESFIRE_SEL_RES                   0x20

#define NFC_A_MIFARE_PLUS_2K_SL1_SENS_RES_PLATFORM      0x00
#define NFC_A_MIFARE_PLUS_2K_SL1_SENS_RES_ANTICOLLISION 0x04
#define NFC_A_MIFARE_PLUS_2K_SL1_SEL_RES                0x08

#define NFC_A_MIFARE_PLUS_4K_SL1_SENS_RES_PLATFORM      0x00
#define NFC_A_MIFARE_PLUS_4K_SL1_SENS_RES_ANTICOLLISION 0x02
#define NFC_A_MIFARE_PLUS_4K_SL1_SEL_RES              0x18

#define NFC_A_MIFARE_PLUS_SL3_SENS_RES_PLATFORM       0x03
#define NFC_A_MIFARE_PLUS_SL3_SENS_RES_ANTICOLLISION  0x44
#define NFC_A_MIFARE_PLUS_SL3_SEL_RES                 0x20

#define NFC_A_MIFARE_SMARTMX_SENS_RES_PLATFORM        0x00
#define NFC_A_MIFARE_SMARTMX_SENS_RES_ANTICOLLISION   0x04
#define NFC_A_MIFARE_SMARTMX_SEL_RES                  0x08

/**
 * @brief Request for Answer To Select (RATS) command
 */
#define NFC_A_RATS_LENGTH       (2u)
#define NFC_A_RATS_MAGIC_NUMBER (0xE0)

#define NFC_A_RATS_FSDI_MASK    (0xF0)
#define NFC_A_RATS_DID_MASK     (0x0F)

/**
 * @brief Answer To Select (ATS) response
 */
#define NFC_A_ATS_MIN_LENGTH            (1u)
#define NFC_A_ATS_MAX_LENGTH            (20u)
#define NFC_A_ATS_T0_TA1_PRESENT_MASK   (0x10)
#define NFC_A_ATS_T0_TB1_PRESENT_MASK   (0x20)
#define NFC_A_ATS_T0_TC1_PRESENT_MASK   (0x40)
#define NFC_A_ATS_T0_TD1_PRESENT_MASK   (0x80)
#define NFC_A_ATS_T0_FSCI_MASK          (0x0F)
#define NFC_A_ATS_TA1_FWI_MASK          (0xF0)
#define NFC_A_ATS_TA1_SFGI_MASK         (0x0F)
#define NFC_A_ATS_TB1_SFGI_MASK         (0xF0)
#define NFC_A_ATS_TB1_CRC_MASK          (0x01)
#define NFC_A_ATS_TC1_FD_MASK           (0x0F)
#define NFC_A_ATS_TC1_SDC_MASK          (0x70)
#define NFC_A_ATS_TC1_DRC_MASK          (0x0E)
#define NFC_A_ATS_TC1_DSI_MASK          (0x01)

/** 
 * @brief Cascade Tag
 *
 */
#define NFC_A_NFCID1_CT        (0x88)

/**
 * @brief Cascade Level of Select Command as part of SDD_REQ and SEL_REQ
 * 
 */
#define NFC_A_SEL_CMD_CL1      (0x93)
#define NFC_A_SEL_CMD_CL2      (0x95)
#define NFC_A_SEL_CMD_CL3      (0x97)

#define NFC_A_SEL_REQ_LEN  (7u)

#define NFC_A_CRC_LEN  (2u)

typedef enum {
    NFC_A_BITRATE_106 = 0,
    NFC_A_BITRATE_212,
    NFC_A_BITRATE_424,
} nfc_a_bitrate_t;

typedef enum {
    NFC_A_NFCID1_LEN4 = 4,
    NFC_A_NFCID1_LEN7 = 7,
    NFC_A_NFCID1_LEN10 = 10,
} nfc_a_nfcid1_len_t;

typedef struct {
    nfc_a_nfcid1_len_t len;
    uint8_t nfcid[10];
} nfc_a_nfcid1_t;

typedef struct {
    nfc_a_nfcid1_t nfcid1;
    nfc_a_bitrate_t bitrate;
} nfc_a_target_t;

typedef struct {
    uint8_t anticollision_information;
    uint8_t platform_information;
} nfc_a_sens_res_t;

typedef struct {
    nfc_a_sens_res_t sens_res;
    nfc_a_nfcid1_t nfcid1;
    uint8_t sel_res;
} nfc_a_listener_config_t;

typedef struct {
    uint8_t tl;
    uint8_t t0;
    uint8_t ta;
    uint8_t tb;
    uint8_t tc;
    uint8_t historical_bytes[14];
} nfc_a_rats_t;

/* also known as SAK */
typedef uint8_t nfc_a_sel_res_t; 

nfc_application_type_t nfc_a_get_application_type(nfc_a_sens_res_t sens_res, uint8_t sel_res);
