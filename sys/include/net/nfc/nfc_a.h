#pragma once

#include <stdint.h>

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

/**
 * @brief Mask for the NFCID1 size field in the anticollision
 * information byte of SENS_RES
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
