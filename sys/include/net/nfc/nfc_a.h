#pragma once

#include <stdint.h>

/**
 * @brief Sense Response (SENS_RES) command
 */
#define NFC_A_SENS_RES_ANTICOLLISION_NFCID1_SIZE_MASK (0xC0)

#define NFC_A_SEL_RES_T2T_VALUE (0x00)
#define NFC_A_SEL_RES_T2T_MASK  (0x60)

#define NFC_A_SEL_RES_T4T_VALUE (0x20)
#define NFC_A_SEL_RES_T4T_MASK  (0x20)

#define NFC_A_SEL_RES_DEP_VALUE (0x40)
#define NFC_A_SEL_RES_DEP_MASK  (0x40)

#define NFC_A_SEL_RES_NFCID1_COMPLETE_VALUE (0x00)
#define NFC_A_SEL_RES_NFCID1_COMPLETE_MASK  (0x04)

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

/* also known as SAK */
typedef uint8_t nfc_a_sel_res_t; 
