#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define NFC_V_ID_LENGTH  (8)

typedef uint8_t nfc_v_id_t[NFC_V_ID_LENGTH];

#pragma pack(push, 1)

typedef enum {
    NFC_V_COMMAND_INVENTORY                                         = 0x01,
    NFC_V_COMMAND_STAY_QUIET                                        = 0x02,
    NFC_V_COMMAND_BLOCK_READ_SINGLE                                 = 0x20,
    NFC_V_COMMAND_BLOCK_WRITE_SINGLE                                = 0x21,
    NFC_V_COMMAND_BLOCK_LOCK                                        = 0x22,
    NFC_V_COMMAND_BLOCK_READ_MULTIPLE                               = 0x23,
    NFC_V_COMMAND_BLOCK_WRITE_MULTIPLE                              = 0x24,
    NFC_V_COMMAND_SELECT                                            = 0x25,
    NFC_V_COMMAND_RESET_TO_READY                                    = 0x26,
    NFC_V_COMMAND_AFI_WRITE                                         = 0x27,
    NFC_V_COMMAND_AFI_LOCK                                          = 0x28,
    NFC_V_COMMAND_DSFID_WRITE                                       = 0x29,
    NFC_V_COMMAND_DSFID_LOCK                                        = 0x2A,
    NFC_V_COMMAND_GET_SYSTEM_INFORMATION                            = 0x2B,
    NFC_V_COMMAND_BLOCK_GET_MULTIPLE_SECURITY_STATUS                = 0x2C,
    NFC_V_COMMAND_BLOCK_READ_FAST_MULTIPLE                          = 0x2D,
    NFC_V_COMMAND_BLOCK_READ_EXTENDED_SINGLE                        = 0x30,
    NFC_V_COMMAND_BLOCK_WRITE_EXTENDED_SINGLE                       = 0x31,
    NFC_V_COMMAND_BLOCK_LOCK_EXTENDED                               = 0x32,
    NFC_V_COMMAND_BLOCK_READ_EXTENDED_MULTIPLE                      = 0x33,
    NFC_V_COMMAND_BLOCK_WRITE_EXTENDED_MULTIPLE                     = 0x34,
    NFC_V_COMMAND_AUTHENTICATE                                      = 0x35,
    NFC_V_COMMAND_KEY_UPDATE                                        = 0x36,
    NFC_V_COMMAND_AUTHCOMM_CRYPTO_FORMAT_INDICATOR                  = 0x37,
    NFC_V_COMMAND_SECURECOMM_CRYPTO_FORMAT_INDICATOR                = 0x38,
    NFC_V_COMMAND_CHALLENGE                                         = 0x39,
    NFC_V_COMMAND_READ_BUFFER                                       = 0x3A,
    NFC_V_COMMAND_EXTENDED_GET_SYSTEM_INFORMATION                   = 0x3B,
    NFC_V_COMMAND_BLOCK_GET_EXTENDED_MULTIPLE_SECURITY_STATUS       = 0x3C,
    NFC_V_COMMAND_BLOCK_READ_FAST_EXTENDED_MULTIPLE                 = 0x3D
} nfc_v_command_code_t;

typedef union {
    struct {
        bool dual_subcarriers : 1;
        bool high_data_rate : 1;
        bool inventory : 1;
        bool extended_protocol_format : 1;
        bool to_selected : 1;
        bool to_addressed : 1;
        bool command_option : 1;
        bool legacy_flag : 1;
    };
    uint8_t flags;
} nfc_v_request_flags_t;

typedef union {
    struct {
        bool dual_subcarriers : 1;
        bool high_data_rate : 1;
        bool inventory : 1;
        bool _rfu0 : 1;
        bool afi_present : 1;
        bool single_slot : 1;
        bool command_option : 1;
        bool legacy_flag : 1;
    };
    uint8_t flags;
} nfc_v_request_flags_inventory_t;

typedef union {
    struct {
        bool error : 1;
        bool valid : 1;
        bool final : 1;
        bool _rfu0 : 1;
        uint8_t block_security_status_length_exponent : 2;
        bool waiting_time_extension_requested : 1;
        bool _rfu1 : 1;
    };
    uint8_t flags;
} nfc_v_response_flags_t;

typedef enum {
    NFC_V_ERROR_UNSUPPORTED_COMMAND         = 1,
    NFC_V_ERROR_MALFORMED_COMMAND           = 2,
    NFC_V_ERROR_UNSUPPORTED_COMMAND_OPTION  = 3,
    NFC_V_ERROR_COMMAND_EXCEEDED_TIME_LIMIT = 4,
    NFC_V_ERROR_GENERIC                     = 0x0f,
    NFC_V_ERROR_BLOCK_ABSENT                = 0x10,
    NFC_V_ERROR_BLOCK_ALREADY_LOCKED        = 0x11,
    NFC_V_ERROR_BLOCK_IMMUTABLE_LOCKED      = 0x12,
    NFC_V_ERROR_BLOCK_CORRUPTED             = 0x13,
    NFC_V_ERROR_BLOCK_UNSUCCESSFULLY_LOCKED = 0x14,
    NFC_V_ERROR_BLOCK_PROTECTED             = 0x15,
    NFC_V_ERROR_CRYPTO                      = 0x40,
} nfc_v_error_code_t;

typedef struct {
    nfc_v_response_flags_t flags;
    uint8_t storage_format_identifier;
    nfc_v_id_t id;
} nfc_v_inventory_response_t;

#pragma pack(pop)

typedef struct {
    nfc_v_request_flags_inventory_t flags;
    uint8_t mask_length;
    uint8_t mask[];
} nfc_v_inventory_request_info_t;

typedef struct {
    uint8_t storage_format_identifier;
    size_t id_prefix_length;
    nfc_v_id_t* id;
} nfc_v_polling_filter_t;

typedef struct {
    struct {
        nfc_v_inventory_request_info_t* frames;
        size_t frame_count;
    };

    nfc_v_polling_filter_t* filter;
} nfc_v_tag_polling_config_t;

typedef struct {
    nfc_v_id_t id;
    uint8_t storage_format_identifier;
    uint8_t block_security_status_length_exponent : 2;
} nfc_v_tag_t;

void nfc_v_print_tag(const nfc_v_tag_t* tag);

bool nfc_v_polling_filter_matches(const nfc_v_polling_filter_t* filter, const nfc_v_tag_t* tag);
