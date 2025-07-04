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
#define SENS_RES_ANTICOLLISION_NFCID1_SIZE_MASK (0xC0)

typedef struct {
    uint8_t anticollision_information;
    uint8_t platform_information;
} nfc_a_sens_res_t;

typedef struct {
    uint8_t data;
} nfc_a_sel_res_t;

