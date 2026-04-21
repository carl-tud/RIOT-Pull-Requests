#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "net/nfc/constants.h"
#include "net/nfc/nfc_dep.h"

#define NFC_F_SYNC0 0xb2
#define NFC_F_SYNC1 0x4d


// MARK: - IDm
/// @name IDm
/// @{

#define NFC_F_ID_LENGTH  (8u)

typedef union __attribute__((packed)) {
    struct {
        uint8_t mcode[2];
        uint8_t cin[6];
    } __attribute__((packed));

    uint8_t raw[NFC_F_ID_LENGTH];
} nfc_f_id_t;

/// @}

typedef enum __attribute__((packed)) {
    NFC_F_PACKET_CODE_POLLING_COMMAND       = 0,
    NFC_F_PACKET_CODE_POLLING_RESPONSE      = 1,
    NFC_F_PACKET_CODE_DEP_REQUEST           = 0xD4,
    NFC_F_PACKET_CODE_DEP_RESPONSE          = 0xD5,
} nfc_f_packet_code_t;

typedef uint16_t nfc_f_system_code_t;

#define NFC_F_SYSTEM_CODE_ALL (0xffff)

typedef struct __attribute__((packed)) {
    /// Packet length including this length byte, code, and data
    uint8_t length;
    nfc_f_packet_code_t code;
} nfc_f_packet_header_t;

#define NFC_F_NFC_DEP_ID_MCODE0 (0x01)
#define NFC_F_NFC_DEP_ID_MCODE1 (0xFE)

static inline bool nfc_f_supports_nfc_dep(const nfc_f_id_t* idm) {
    return idm->mcode[0] == NFC_F_NFC_DEP_ID_MCODE0 &&
        idm->mcode[1] == NFC_F_NFC_DEP_ID_MCODE1;
}

typedef union __attribute__((packed)) {
    struct {
        uint8_t rom;
        uint8_t ic;

        struct {
            uint8_t variable;
            uint8_t fixed;
            uint8_t authenticated;
            uint8_t read;
            uint8_t write;
            uint8_t other;
        } __attribute__((packed)) max_response_times;
    } __attribute__((packed));

    uint8_t raw[NFC_F_ID_LENGTH];
} nfc_f_pmm_t;

#define _NFC_F_MAX_RESPONSE_TIME_MS(A, B, E, n) \
    (256 * 16 / NFC_CARRIER_FREQUENCY_HZ * (((B)+1) * (n) + (A) + 1) * (4 << (E)))

#define NFC_F_MAX_RESPONSE_TIME_MS(byte, n) \
    _NFC_F_MAX_RESPONSE_TIME_MS(byte & 0b111, (byte >> 3) & 0b111, byte >> 6, n)

typedef struct __attribute__((packed)) {
    nfc_f_packet_header_t super;
    nfc_f_id_t id;
    nfc_f_pmm_t pmm;
} nfc_f_packet_header_response_t;

typedef enum __attribute__((packed)) {
    NFC_F_POLLING_REQUEST_NOTHING           = 0,
    NFC_F_POLLING_REQUEST_SYSTEM_CODE       = 1,
    NFC_F_POLLING_REQUEST_BITRATES          = 2,
} nfc_f_polling_additional_request_t;

typedef struct __attribute__((packed)) {
    nfc_f_system_code_t system_code;
    nfc_f_polling_additional_request_t additional_request;
    uint8_t timeslots;
} nfc_f_polling_command_payload_t;

static inline uint32_t nfc_f_polling_response_time_ms(uint8_t tsn) {
    return (242 /* 2.42 ms */ + (tsn+1) * 121 /* 1.21 ms */) / 100;
}

typedef uint8_t nfc_f_polling_response_payload_t[2];

typedef struct __attribute__((packed)) {
    nfc_f_packet_header_t header;
    nfc_f_polling_command_payload_t payload;
} nfc_f_polling_command_t;

typedef struct __attribute__((packed)) {
    nfc_f_packet_header_t header;
    nfc_f_id_t id;
    nfc_f_pmm_t pmm;
    nfc_f_polling_response_payload_t payload;
} nfc_f_polling_response_t;

typedef struct __attribute__((packed)) {
    nfc_f_packet_header_t header;
    union __attribute__((packed)) {
        nfc_f_polling_command_payload_t polling;
    } payload;
} nfc_f_command_t;

typedef struct __attribute__((packed)) {
    nfc_f_packet_header_t header;
    nfc_f_id_t id;
    nfc_f_pmm_t pmm;
    union __attribute__((packed)) {
        nfc_f_polling_response_payload_t polling;
    } payload;
} nfc_f_response_t;

typedef struct {
    nfc_f_id_t* id;
    // bitrate and system code filter cannot be used together, can only have one additional request
    nfc_bitrate_t bitrates;
    size_t system_code_count;
    nfc_f_system_code_t system_codes[];
} nfc_f_polling_filter_t;

typedef struct {
    struct {
        nfc_f_polling_command_payload_t* frames;
        size_t frame_count;
    };

    nfc_f_polling_filter_t* filter;
} nfc_f_tag_polling_config_t;

typedef struct {
    nfc_f_id_t id;
    nfc_f_pmm_t pmm;
    nfc_f_system_code_t system_code;
    nfc_bitrate_t bitrates;
} nfc_f_tag_t;
