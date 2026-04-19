#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define NFC_DEP_ID_LENGTH (10u)

#define NFC_DEP_CMD0_REQUEST  (0xD4)
#define NFC_DEP_CMD0_RESPONSE (0xD5)

#define NFC_DEP_MAX_PAYLOAD_SIZE (253)

typedef enum __attribute__((packed)) {
    NFC_DEP_PDU_CODE_ACTIVATION_REQUEST             = 0x00,
    NFC_DEP_PDU_CODE_ACTIVATION_RESPONSE            = 0x01,
    NFC_DEP_PDU_CODE_WAKEUP_REQUEST                 = 0x02,
    NFC_DEP_PDU_CODE_WAKEUP_RESPONSE                = 0x03,
    NFC_DEP_PDU_CODE_PARAMETER_SELECTION_REQUEST    = 0x04,
    NFC_DEP_PDU_CODE_PARAMETER_SELECTION_RESPONSE   = 0x05,
    NFC_DEP_PDU_CODE_DATA_EXCHANGE_REQUEST          = 0x06,
    NFC_DEP_PDU_CODE_DATA_EXCHANGE_RESPONSE         = 0x07,
    NFC_DEP_PDU_CODE_DESELECT_REQUEST               = 0x08,
    NFC_DEP_PDU_CODE_DESELECT_RESPONSE              = 0x09,
    NFC_DEP_PDU_CODE_RELEASE_REQUEST                = 0x0A,
    NFC_DEP_PDU_CODE_RELEASE_RESPONSE               = 0x0B
} nfc_dep_instruction_t;

typedef struct __attribute__((__packed__)) {
    uint8_t length;
    uint8_t direction;
    nfc_dep_instruction_t instruction;
} nfc_dep_header_t;

#define NFC_DEP_ADDITIONAL_BITRATE_SUPPORTED(capabilities, bitrate) \
    ((capabilities) & ((bitrate) >> 3) != 0)

#define NFC_DEP_LENGTH_REDUCTION_IN_BYTES(reduction_index) \
    ((reduction_index) == 0b11 ? 252 : ((reduction_index) + 1) * 64)

typedef enum __attribute__((packed)) {
    NFC_DEP_PAYLOAD_LIMIT_64    = 0,
    NFC_DEP_PAYLOAD_LIMIT_128   = 1,
    NFC_DEP_PAYLOAD_LIMIT_192   = 2,
    NFC_DEP_PAYLOAD_LIMIT_252   = 3,
} nfc_dep_payload_reduction_index_t;

typedef struct __attribute__((packed)) {
    uint8_t id[10];
    uint8_t device_id;
    uint8_t supported_bitrates_tx;
    uint8_t supported_bitrates_rx;

    union {
        struct {
            bool nad_used : 1;
            bool general_bytes_available : 1;
            uint8_t _rfu0 : 2;
            nfc_dep_payload_reduction_index_t payload_reduction : 2;
            uint8_t _rfu1 : 1;
            bool nfc_secure_supported : 1;
        } __attribute__((packed));

        uint8_t parameters;
    } __attribute__((packed));

    uint8_t general_bytes[];
} nfc_dep_activation_request_t;

typedef struct __attribute__((packed)) {
    uint8_t id[NFC_DEP_ID_LENGTH];
    uint8_t device_id;
    uint8_t supported_bitrates_tx;
    uint8_t supported_bitrates_rx;

    nfc_time_index_t response_waiting_time;

    union {
        struct {
            bool nad_used : 1;
            bool general_bytes_available : 1;
            uint8_t _rfu0 : 2;
            nfc_dep_payload_reduction_index_t payload_reduction : 2;
            uint8_t _rfu1 : 1;
            bool nfc_secure_supported : 1;
        } __attribute__((packed));

        uint8_t parameters;
    } __attribute__((packed));

    uint8_t general_bytes[];
} nfc_dep_activation_response_t;

typedef struct __attribute__((packed)) {
    uint8_t device_id;

    union {
        struct {
            /// Can cast to @ref nfc_bitrate_t
            uint8_t bitrate_tx : 3;
            /// Can cast to @ref nfc_bitrate_t
            uint8_t bitrate_rx : 3;
            uint8_t _rfu : 2;
        } __attribute__((packed));

        uint8_t bitrate_selection;
    } __attribute__((packed));
    
    union {
        struct {
            nfc_dep_payload_reduction_index_t payload_reduction : 2;
            uint8_t reserved : 6;
        } __attribute__((packed));

        uint8_t lr;
    } __attribute__((packed));

} nfc_dep_parameter_selection_request_t;

typedef struct __attribute__((packed)) {
    uint8_t device_id;
} nfc_dep_parameter_selection_response_t;

typedef enum __attribute__((packed)) {
    NFC_DEP_PDU_INFORMATION     = 0,
    NFC_DEP_PDU_PROTECTED       = 1,
    NFC_DEP_PDU_ACK_NACK        = 1 << 1,
    NFC_DEP_PDU_ACK_SUPERVISROY = 1 << 2,
} nfc_dep_pdu_type_t;

typedef enum __attribute__((packed)) {
    NFC_DEP_SUPERVISORY_TYPE_ATTENTION     = 0,
    NFC_DEP_SUPERVISORY_TYPE_TIMEOUT       = 1,
} nfc_dep_supervisory_type_t;

typedef union __attribute__((packed)) {
    struct {
        uint8_t _specific : 5;
        nfc_dep_pdu_type_t type : 3;
    } __attribute__((packed));

    union {
        struct {
            uint8_t packet_number : 2;
            bool did_available : 1;
            bool nad_available : 1;
            bool multiple_information : 1;
            uint8_t _type : 3;
        } __attribute__((packed)) information_pdu;

        struct {
            uint8_t packet_number : 2;
            bool did_available : 1;
            bool nad_available : 1;
            bool multiple_information : 1;
            uint8_t _type : 3;
        } __attribute__((packed)) protected_pdu;

        struct {
            uint8_t packet_number : 2;
            bool did_available : 1;
            bool nad_available : 1;
            bool negative_ack : 1;
            uint8_t _type : 3;
        } __attribute__((packed)) ack_nack_pdu;

        struct {
            uint8_t _fixed : 2;
            bool did_available : 1;
            bool nad_available : 1;
            nfc_dep_supervisory_type_t rtox : 1;
            uint8_t _type : 3;
        } __attribute__((packed)) supervisory_pdu;
    } __attribute__((packed));
} nfc_dep_exchange_control_info_t;

#define NFC_DEP_PDU_TYPE(pfb) ((nfc_dep_pdu_type_t)(((pfb) >> 5) & 0b111))

typedef struct __attribute__((packed)) {
    union {
        uint8_t control_information;
        struct {
            uint8_t packet_number_information : 2;
            uint8_t nak_indicator : 1;
            uint8_t nad_present : 1;
            uint8_t device_id_present : 1;
            uint8_t more_information : 1;
            uint8_t pdu_type : 2;
        } __attribute__((packed));
    } __attribute__((packed));

    uint8_t payload[];
} nfc_dep_data_exchange_request_response_t;

typedef struct __attribute__((packed)) {
    uint8_t id[NFC_DEP_ID_LENGTH];
    uint8_t device_id;
} nfc_dep_wakeup_request;

typedef struct __attribute__((packed)) {
    uint8_t device_id;
} _nfc_dep_device_id_pdu_t;

typedef _nfc_dep_device_id_pdu_t nfc_dep_wakeup_response_t;
typedef _nfc_dep_device_id_pdu_t nfc_dep_deselect_request_t;
typedef _nfc_dep_device_id_pdu_t nfc_dep_deselect_response_t;
typedef _nfc_dep_device_id_pdu_t nfc_dep_release_request_t;
typedef _nfc_dep_device_id_pdu_t nfc_dep_release_response_t;

typedef struct __attribute__((packed)) {
    nfc_dep_header_t header;

    union __attribute__((packed)) {
        nfc_dep_activation_request_t activation;
        nfc_dep_parameter_selection_request_t parameter_selection;
        nfc_dep_data_exchange_request_response_t data_exchange;
        nfc_dep_wakeup_request wakeup;
        nfc_dep_deselect_request_t deselect;
        nfc_dep_release_request_t release;
    } payload;
} nfc_dep_request_t;

typedef struct __attribute__((packed)) {
    nfc_dep_header_t header;

    union __attribute__((packed)) {
        nfc_dep_activation_response_t activation;
        nfc_dep_parameter_selection_response_t parameter_selection;
        nfc_dep_data_exchange_request_response_t data_exchange;
        nfc_dep_wakeup_response_t wakeup;
        nfc_dep_deselect_response_t deselect;
        nfc_dep_release_response_t release;
    } payload;
} nfc_dep_response_t;

typedef struct {
    nfc_bitrate_t baudrate;
    uint8_t id[10]; /* NFCID3t, 10 bytes */
} nfc_dep_listener_config_t;
