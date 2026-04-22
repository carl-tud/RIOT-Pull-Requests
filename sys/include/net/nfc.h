#pragma once

#include "net/nfc/constants.h"
#include "net/nfc/nfc_a.h"
#include "net/nfc/nfc_b.h"
#include "net/nfc/nfc_f.h"
#include "net/nfc/nfc_v.h"
#include "net/nfc/iso_dep.h"
#include "net/nfc/nfc_dep.h"
#include "net/nfc/nfc_error.h"

#include "compiler_hints.h"

/// Passive tag characteristics obtained during polling (initialization and activation)
typedef struct {
    nfc_technology_t technology;

    union {
        nfc_a_tag_t a;
        nfc_b_tag_t b;
        nfc_f_tag_t f;
        nfc_v_tag_t v;
    };
} nfc_tag_t;


typedef struct {
    /// Passive tag initialization result
    ///
    /// If @ref nfc_target_t::field_mode is NFC_FIELD_MODE_READER_WRITER_TAG
    nfc_tag_t tag;

    struct {
        /// Initiator-defined parameters used during initialization and activation
        struct {
            /// Bitrate the initiator used to modulate and demodulate polling commands and responses
            ///
            /// Ignored if @ref nfc_target_t::technology is @ref NFC_TECHNOLOGY_V.
            /// This bitrate remained valid at least until other parameters were negotiated.
            nfc_bitrate_t bitrate;
        } polling;

        /// Negociated parameters for initiator-to-target communication
        struct {
            size_t max_packet_length;
            nfc_bitrate_t bitrate;
        } downstream;

        /// Negociated parameters for target-to-initiator communication
        struct {
            size_t max_packet_length;
            nfc_bitrate_t bitrate;
        } upstream;
    } parameters;

    union {
        struct {
            size_t atr_length;
            nfc_dep_activation_response_t* atr;
        } nfc_dep;
    } higher_layer;

    nfc_field_mode_t field_mode : 1;
} nfc_target_t;
