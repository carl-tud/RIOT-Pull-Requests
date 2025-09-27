#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "net/nfc/apdu/apdu.h"

#define T4T_MAX_NDEF_SIZE 2048

#define T4T_CC_FILE_CCLEN_POS 0
#define T4T_CC_FILE_CCLEN_LEN 2
#define T4T_CC_FILE_MAPPING_VERSION_POS 2
#define T4T_CC_FILE_MAPPING_VERSION_LEN 1
#define T4T_CC_FILE_MLE_POS 3
#define T4T_CC_FILE_MLE_LEN 2
#define T4T_CC_FILE_MLC_POS 5
#define T4T_CC_FILE_MLC_LEN 2
#define T4T_CC_FILE_NDEF_FILE_CONTROL_TLV_POS 7
#define T4T_CC_FILE_NDEF_FILE_CONTROL_TLV_LEN 8

#define T4T_NDEF_FILE_TLV_TYPE 0x03
#define T4T_NDEF_FILE_TLV_LEN  0x06

#define T4T_NDEF_FILE_TLV_FILE_ID_POS 1
#define T4T_NDEF_FILE_TLV_FILE_ID_LEN 2
#define T4T_NDEF_FILE_TLV_MAX_NDEF_SIZE_POS 3
#define T4T_NDEF_FILE_TLV_MAX_NDEF_SIZE_LEN 2
#define T4T_NDEF_FILE_TLV_READ_ACCESS_POS 5
#define T4T_NDEF_FILE_TLV_READ_ACCESS_LEN 1
#define T4T_NDEF_FILE_TLV_WRITE_ACCESS_POS 6
#define T4T_NDEF_FILE_TLV_WRITE_ACCESS_LEN 1

typedef struct {
    uint8_t type;
    uint8_t length;
    uint16_t file_id;
    uint16_t max_ndef_size;
    uint8_t read_access;
    uint8_t write_access;
} t4t_ndef_file_control_tlv;

typedef struct {
    uint16_t cc_len;
    uint8_t mapping_version;
    uint16_t mle;
    uint16_t mlc;
    t4t_ndef_file_control_tlv ndef_file_control_tlv;
} t4t_cc_file;

/**
 * @brief The ISO/IEC-7816 select command to select the NDEF file
 * for an NFC Type 4 Tag.
 */
static const uint8_t T4T_APDU_NDEF_FILE_SELECT[] = {
    APDU_CLA_DEFAULT, 
    APDU_INS_SELECT,
    APDU_SELECT_P1_SELECT_MF_DF_OR_EF, 
    APDU_SELECT_P2_FIRST_OR_ONLY_OCCURRENCE | APDU_SELECT_P2_NO_RESPONSE_DATA,
    0x02,       /* Length of Command*/
    0xE1, 0x04, /* NDEF file identifier */
    0x00        /* Length of Expected Response */
};

/**
 * @brief The ISO/IEC-7816 select command to select the Capability Container (CC) file
 * for an NFC Type 4 Tag.
 */
static const uint8_t T4T_APDU_CC_FILE_SELECT[] = {
    APDU_CLA_DEFAULT, 
    APDU_INS_SELECT,
    APDU_SELECT_P1_SELECT_MF_DF_OR_EF, 
    APDU_SELECT_P2_FIRST_OR_ONLY_OCCURRENCE | APDU_SELECT_P2_NO_RESPONSE_DATA,
    0x02,       /* Length of Command*/
    0xE1, 0x03, /* CC file identifier */
    0x00        /* Length of Expected Response */
};

/**
 * @brief The ISO/IEC-7816 select command to select the NDEF Tag Application
 * for an NFC Type 4 Tag.
 */
static const uint8_t T4T_APDU_NDEF_TAG_APPLICATION_SELECT[] = {
    APDU_CLA_DEFAULT, APDU_INS_SELECT,
    APDU_SELECT_P1_SELECT_BY_DF_NAME, APDU_SELECT_P2_FIRST_OR_ONLY_OCCURRENCE,
    0x07, /* Length of Command*/
    0xD2, 0x76, 0x00, 0x00, 0x85, 0x01, 0x01, /* NDEF Identifier */
    0x00  /* Length of Expected Response */
};

typedef struct {
    bool selected_ndef_application;
    bool selected_cc_file;
    bool selected_ndef_file;
    t4t_cc_file cc_file;
    uint8_t *ndef_file;
} nfc_t4t_t;

int t4t_init(nfc_t4t_t *tag, uint8_t *ndef_file, size_t max_ndef_file_size);
