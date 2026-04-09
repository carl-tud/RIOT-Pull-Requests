#pragma once

#include <stdint.h>
#include "assert.h"
#include "net/nfc/iso_dep.h"

#define NFC_B_ID_LENGTH  (4u)

#define NFC_B_POLLING_COMMAND_PREFIX (0x05)

typedef union __attribute__((packed)) {
    struct {
        uint8_t prefix;
        uint8_t application_family;
        uint8_t slot_count_power : 3;
        bool wake_up : 1;
        bool extended_atqb_supported : 1;
        uint8_t _rfu : 3;
    } __attribute__((packed));
    
    uint8_t raw[3];
} nfc_b_polling_command_t;

#define _nfc_b_slot_count(power) (1 << (power))

static inline uint8_t nfc_b_slot_count(uint8_t power) {
    assert(power <= 0b111);
    return _nfc_b_slot_count(power);
}

#define _nfc_b_slot_count_power(count) ((uint8_t)__builtin_ctz((uint8_t)count))

static inline uint8_t nfc_b_slot_count_power(uint8_t count) {
    return _nfc_b_slot_count_power(count);
}

#define NFC_B_POLLING_RESPONSE_PREFIX (0x50)

#define NFC_B_POLLING_RESPONSE_APPLICATION_DATA_LENGTH (4u)
#define NFC_B_POLLING_RESPONSE_PROTOCOL_INFO_LENGTH    (4u)

typedef union __attribute__((packed)) {
    struct {
        uint8_t prefix;
        uint8_t identifier[4];

        union {
            struct {
                uint8_t application_family;
                uint8_t aid_crc[2];
                uint8_t application_count_total : 4;
                uint8_t application_count_family : 4;
            } __attribute__((packed));

            uint8_t application_data[NFC_B_POLLING_RESPONSE_APPLICATION_DATA_LENGTH];
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

            uint8_t protocol_info[NFC_B_POLLING_RESPONSE_PROTOCOL_INFO_LENGTH];
        };
    } __attribute__((packed));

    uint8_t raw[13];
} nfc_b_polling_response_t;

#define NFC_B_POLLING_RESPONSE_LENGTH (sizeof(nfc_b_polling_response_t) - 1)
#define NFC_B_POLLING_RESPONSE_EXTENDED_LENGTH sizeof(nfc_b_polling_response_t)

#define NFC_B_ATTRIB_PREFIX (0x1D)

typedef union __attribute__((packed)) {
    struct {
        uint8_t prefix;
        uint8_t identifier[4];

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

    uint8_t raw[9];
} nfc_b_attrib_command_t;

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

#define NFC_B_SLEEP_COMMAND_PREFIX (0x50)

typedef union __attribute__((packed)) {
    struct {
        uint8_t prefix;
        uint8_t identifier[4];
    } __attribute__((packed));

    uint8_t raw[3];
} nfc_b_sleep_command_t;

#define NFC_B_SLEEP_RESPONSE (0x00)

typedef uint8_t nfc_b_sleep_response_t;

typedef struct {
    nfc_b_polling_response_t polling_response;
} nfc_b_listener_config_t;
