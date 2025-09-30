#pragma once

#include <stdint.h>

typedef struct {
    uint8_t afi;
    uint8_t crc[2];
    uint8_t number_of_applications;
} nfc_b_sens_res_application_data_t;

typedef struct {
    uint8_t nfcid0[4];
    nfc_b_sens_res_application_data_t application_data;
    uint8_t protocol_info[4];
} nfc_b_sensb_res_t;

typedef struct {
    nfc_b_sensb_res_t sensb_res;
} nfc_b_listener_config_t;
