#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "assert.h"
#include "net/nfc/iso_dep.h"

// MARK: - Pseudo-unique identifier
/// @name Pseudo-unique identifier
/// @{

#define NFC_B_ID_LENGTH  (4)

typedef uint8_t nfc_b_id_t[NFC_B_ID_LENGTH];

/// @}

typedef union __attribute__((packed)) {
    struct {
        uint8_t application_family;
        uint8_t slot_count_power : 3;
        bool wake_up : 1;
        bool extended_atqb_supported : 1;
        uint8_t _rfu : 3;
    } __attribute__((packed));
    
    uint8_t raw[2];
} nfc_b_polling_command_payload_t;

#define NFC_B_FRAME_CODE_POLLING (0x05)

typedef union __attribute__((packed)) {
    struct {
        uint8_t code;
        nfc_b_polling_command_payload_t payload;
    } __attribute__((packed));

    uint8_t raw[sizeof(nfc_b_polling_command_payload_t) + 1];
} nfc_b_polling_command_t;

#define NFC_B_SLOT_COUNT(power) (1 << (power))

static inline uint8_t nfc_b_slot_count(uint8_t power) {
    assert(power <= 0b111);
    return NFC_B_SLOT_COUNT(power);
}

typedef enum __attribute__((packed)) {
    NFC_POLLING_METHOD_PROBABILISTIC = 1,
    NFC_POLLING_METHOD_TIMESLOT = 2,
} nfc_b_polling_method_t;

#define NFC_B_SLOT_COUNT_POWER(count) ((uint8_t)__builtin_ctz((uint8_t)count))

static inline uint8_t nfc_b_slot_count_power(uint8_t count) {
    return NFC_B_SLOT_COUNT_POWER(count);
}

typedef struct {
    uint8_t* frame;
    nfcdev_frame_length_t length;
} nfc_b_polling_frame_t;

/// @}

// MARK: - Polling response
/// @name Polling response
/// @{

typedef union __attribute__((packed)) {
    struct {
        nfc_b_id_t id;

        union {
            struct {
                uint8_t application_family;
                uint8_t aid_crc[2];
                uint8_t application_count_total : 4;
                uint8_t application_count_family : 4;
            } __attribute__((packed));

            uint8_t application_data[4];
        } __attribute__((packed));

        union {
            struct {
                iso_dep_bitrate_capabilities_t bitrates;

                bool iso_dep_supported : 1;
                uint8_t min_tr2 : 2;
                bool _rfu0 : 1;
                iso_dep_frame_size_t frame_size : 4;

                bool cid_supported : 1;
                bool nad_supported : 1;
                bool application_data_standardized : 1;
                bool _rfu1 : 1;
                nfc_time_index_t frame_waiting_time : 4;
                
                /// Extension to ATQB, check length and ensure @ref nfc_b_polling_command_t::extended_atqb_supported
                struct {
                    uint8_t _rfu2 : 4;
                    nfc_time_index_t startup_frame_guard_time : 4;
                } __attribute__((packed)) extension;
            } __attribute__((packed));

            uint8_t protocol_info[4];
        };
    } __attribute__((packed));

    uint8_t raw[12];
} nfc_b_polling_response_payload_t;

#define NFC_B_FRAME_CODE_POLLING_RESPONSE (0x50)

typedef union __attribute__((packed)) {
    struct {
        uint8_t code;
        nfc_b_polling_response_payload_t payload;
    } __attribute__((packed));

    uint8_t raw[sizeof(nfc_b_polling_response_payload_t) + 1];
} nfc_b_polling_response_t;

/// @}

// MARK: - ATTRIB command
/// @name ATTRIB command
/// @{

typedef union __attribute__((packed)) {
    struct {
        nfc_b_id_t identifier;

        union {
            struct {
                uint8_t _rfu0 : 2;
                bool sof_suppressable : 1;
                bool eof_suppressable : 1;
                uint8_t min_tr1 : 2;
                uint8_t min_tr0 : 2;
            } __attribute__((packed));

            uint8_t param1;
        } __attribute__((packed));

        union {
            struct {
                iso_dep_frame_size_t : 4;
                uint8_t down_bitrate_divisor_power : 2;
                uint8_t up_bitrate_divisor_power : 2;
            } __attribute__((packed));

            uint8_t param2;
        } __attribute__((packed));

        union {
            struct {
                bool iso_dep_supported : 1;
                uint8_t min_tr2 : 2;
                uint8_t _rfu1 : 5;
            } __attribute__((packed));

            uint8_t param3;
        } __attribute__((packed));

        union {
            struct {
                uint8_t cid : 4;
                uint8_t _rfu2 : 4;
            } __attribute__((packed));

            uint8_t param4;
        } __attribute__((packed));

        uint8_t higher_layer[];
    } __attribute__((packed));

    uint8_t raw[8];
} nfc_b_attrib_command_payload_t;

#define NFC_B_FRAME_CODE_ATTRIB (0x1D)

typedef union __attribute__((packed)) {
    struct {
        uint8_t code;
        nfc_b_attrib_command_payload_t payload;
    } __attribute__((packed));

    uint8_t raw[sizeof(nfc_b_attrib_command_payload_t) + 1];
} nfc_b_attrib_command_t;

/// @}

// MARK: - ATTRIB response
/// @name ATTRIB response
/// @{

typedef struct __attribute__((packed)) {
    union {
        struct {
            uint8_t cid : 4;
            uint8_t max_buffer_length_index : 4;
        } __attribute__((packed));

        uint8_t start_byte;
    } __attribute__((packed));

    uint8_t higher_layer[];
} nfc_b_attrib_response_t;

#define NFC_B_ATTRIB_MAX_BUFFER_LENGTH_INDEX_UNKNOWN (0)

#define _nfc_b_attrib_max_buffer_length(index, max_frame_size) \
    ((max_frame_size) * (1 << ((index) - 1)))

static inline uint8_t nfc_b_attrib_max_buffer_length(uint8_t index, size_t max_frame_size) {
    assert(index <= 0xf);
    return _nfc_b_attrib_max_buffer_length(index, max_frame_size);
}

/// @}

// MARK: - Halt command
/// @name Halt command
/// @{

typedef union __attribute__((packed)) {
    nfc_b_id_t identifier;
} nfc_b_halt_command_payload_t;

#define NFC_B_FRAME_CODE_HALT (0x50)

typedef union __attribute__((packed)) {
    struct {
        uint8_t code;
        nfc_b_halt_command_payload_t payload;
    } __attribute__((packed));

    uint8_t raw[sizeof(nfc_b_halt_command_payload_t) + 1];
} nfc_b_halt_command_t;

/// @}

// MARK: - Halt response
/// @name Halt response
/// @{

#define NFC_B_FRAME_CODE_HALT_RESPONSE (0x00)

typedef struct __attribute__((packed)) {
    uint8_t code;
} nfc_b_halt_response_t;

/// @}

typedef struct {
    nfc_b_id_t* id;
    uint8_t application_family_mask;
    uint8_t aid_crc[2];
    iso_dep_bitrate_capabilities_t bitrates;
    bool require_iso_dep_supported : 1;
    bool require_cid_supported : 1;
    bool require_nad_supported : 1;
    bool require_application_data_standardized : 1;
} nfc_b_polling_filter_t;

typedef struct {
    nfc_b_polling_frame_t* frames;
    size_t frame_count;

    nfc_b_polling_method_t method;

    nfc_b_polling_filter_t* filter;

    nfc_b_attrib_command_t* attrib;
} nfc_b_tag_polling_config_t;

typedef struct {
    nfc_b_polling_response_payload_t* polling_response;
    size_t attrib_response_length;
    nfc_b_attrib_response_t* attrib;
} nfc_b_tag_t;
