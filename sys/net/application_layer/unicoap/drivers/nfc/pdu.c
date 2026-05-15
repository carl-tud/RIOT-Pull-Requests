/*
 * SPDX-FileCopyrightText: 2026 Carl Seifert <carl.seifert@tu-dresden.de>
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @file
 * @ingroup net_unicoap_drivers_rfc7252_common
 * @brief   Framing and PDU parser implementation of common RFC 7252 driver
 * @author  Carl Seifert <carl.seifert@tu-dresden.de>
 */

#include <stdint.h>
#include <errno.h>

#include "compiler_hints.h"

#include "net/unicoap/message.h"

#define ENABLE_DEBUG CONFIG_UNICOAP_DEBUG_LOGGING
#include "debug.h"
#include "private.h"

#define _PDU_DEBUG(...) _UNICOAP_PREFIX_DEBUG(".pdu.nfc", __VA_ARGS__)

/*  0               1
 *  7 6 5 4 3 2 1 0 7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Preface   |      Code     |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Token (if any, TKL bytes) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |   Options (if any) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |1 1 1 1 1 1 1 1| Payload (if any) ...
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * If not in compatibility mode:
 * If the code is 0.00, the sender SHALL set TKL to 0, omit Code and any following fields,
 * leading to a 1-byte PDU
 *
 *
 * Preface from initiator (reader/writer):
 *  0
 *  7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+
 * |1|R|0|M|  TKL  |
 * +-+-+-+-+-+-+-+-+
 * Bits 0 to 3 (TKL - Token Length)
         Length of token as per RFC 7252
 * Bit 4 (M - Message ID)
 *       MUST be set to 0 on the first message sent by the initiator and
 *       MUST be incremented on succesful receipt of a target message on the next message sent
 *       by the initiator.
 *       MUST be set to 0 in compatibility mode.
 * Bit 5 MUST be set to 0 and is RFU
 * Bit 6 (R - Reset)
         MUST be 1 to indicate initiator lacks context to process message sent by target before and
         MUST otherwise be set to 0
 * Bit 7 MUST be set to 1

 * - must not be 0x00 for NFC-F polling command, bit 7 prevents that
 * - must not be 0x50==0b0101_0000 for NFC-A HLTA and NFC-B HLTB, bit 7 prevents that
 * - must not be 0x05==0b0000_0101 for NFC-B polling command, bit 7 prevents that
 * - must not be 0x1D==0b0001_1101 for NFC-B ATTRIB, bit 7 prevents that
 * - should not be 0xE0==0b1110_0000 for RATS, hence bit 5 == 0
 * - should not be 0x93 for NFC-A ANTICOLLISION/SELECT, should not matter because of state machine
 * - should not be 0x95 for NFC-A ANTICOLLISION/SELECT, should not matter because of state machine
 * - should not be 0x97 for NFC-A ANTICOLLISION/SELECT, should not matter because of state machine
 * - should not be 0xF0 for NFC-DEP over NFC-A Start Byte, bit 7 prevents that
 * - should not match an ISO-DEP PFB, but that should not happen
 *     - because we never entered PROTOCOL state
 *     - also NFC-DEP uses 0xF0 as start byte over NFC-A and that matches S(PARAMETERS) PFB
 * - bit 7 should be 1 for ISO/IEC 7816-4 interindustry
 * - 0b1101 prefix could be mistaken for PPS, should not matter because not in ISO-DEP protocol state
 *
 * Preface from target (tag):
 *  0
 *  7 6 5 4 3 2 1 0
 * +-+-+-+-+-+-+-+-+
 * |1|R|IND|  TKL  |
 * +-+-+-+-+-+-+-+-+
 * Bits 0 to 3 (TKL - Token Length)
         Length of token as per RFC 7252
 * Bits 4 and 5 (IND - Indication)
 *       0 -- RFU, MUST NOT be 00
 *       1 -- L: Initiator should send another downstream message later,
                  target is busy
 *       2 -- Q: Initiator should send another downstream message quickly
                  (wants confirmation or has upstreammessage to send)
 *       3 -- F: Initiator does not need to send another downstream message,
                  target is finished sending upstream messages
 * Bit 6 (R - Reset)
         MUST be 1 to indicate the target lacks context to process message sent by target before and
         MUST otherwise be set to 0
 * Bit 7 MUST be set to 1
 *
 * - must not be 0x01 for NFC-F polling response, bit 7 prevents that
 * - must not be 0x50==0b0101_0000 for NFC-B polling response, bit 7 prevents that
 * - should not be WUPA/REQA/TIMESLOT/etc., but is 8 bits, not 7,
 *   even if readers intercept 0x26/0x52 (REQA/WUPA) to automatically send short frame
 *   this would not qualify because you cannot achive 0x26/0x52 due to bit 7 set to 1
 */
typedef struct __attribute__((packed)) {
    uint8_t preface;
    uint8_t code;
} unicoap_header_nfc_t;

#define COAP_NFC_PREFACE_FIXED (0x80)
#define COAP_NFC_PREFACE_RESET (0x40)
#define COAP_NFC_PREFACE_TARGET_INDICATION (0x30)
#define COAP_NFC_PREFACE_INITIATOR_RFU (0x20)
#define COAP_NFC_PREFACE_INITIATOR_MESSAGE_ID (0x10)
#define COAP_NFC_PREFACE_TKL (0x0f)

/// PDU embedding map into frames/packets
///
/// - Could say prepend length byte over NFC-A/NFC-B/NFC-V to match NFC-F
/// - should not interfere with ISO-DEP PFB because not entered ISO-DEP protocol state

/* PDU to packet/frame map
 * =======================
 *
 * NFC-A/-B/-F/-V: TODO
 *
 * Compatibility mode (CoAP over ISO/IEC 7816-4 over ISO-DEP)
 * CLA = preface
 * INS = Code
 * P1 = Token byte 1 (if present and TKL <= 2)
 * P1 = Token byte 2 (if present and TKL <= 2)
 * Lc bytes = remaining bytes
 *              0 bytes long if no remaining bytes
 *              1 byte  long if remaining bytes <= 0xff
 *              3 byte  long if remaining bytes > 0xff: Set Lc=0 and append 16-bit length field
 * Payload:
 * Token (if present and TKL > 2)
 *
 * (CoAP options, payload separator and CoAP payload as per RFC 7252 and as in normal CoAP over NFC)
 */

static inline bool _has_fixed_bit(const unicoap_header_nfc_t* header) {
    return (header->preface & COAP_NFC_PREFACE_FIXED) == COAP_NFC_PREFACE_FIXED;
}

static inline void _set_fixed_bit(unicoap_header_nfc_t* header) {
    header->preface |= COAP_NFC_PREFACE_FIXED;
}

static inline bool _is_rfu_zero(const unicoap_header_nfc_t* header) {
    return (header->preface & 0x20) == 0;
}

static inline bool _get_reset(const unicoap_header_nfc_t* header) {
    return (header->preface & COAP_NFC_PREFACE_RESET) >> 6;
}

static inline void _set_reset(unicoap_header_nfc_t* header, bool reset) {
    header->preface &= ~COAP_NFC_PREFACE_RESET;
    header->preface |= (reset & 1) << 6;
}

static inline uint8_t _get_indication(const unicoap_header_nfc_t* header) {
    return (header->preface & COAP_NFC_PREFACE_TARGET_INDICATION) >> 4;
}

static inline void _set_indication(unicoap_header_nfc_t* header, unicoap_nfc_indication_t indication) {
    assert(indication != UNICOAP_NFC_INDICATION_RFU);
    assert((indication & ~(COAP_NFC_PREFACE_TARGET_INDICATION >> 4)) == 0);
    header->preface &= ~COAP_NFC_PREFACE_TARGET_INDICATION;
    header->preface |= indication << 4;
}

static inline uint8_t _get_code(const unicoap_header_nfc_t* header){
    return header->code;
}

static inline void _set_code(unicoap_header_nfc_t* header, uint8_t code) {
    header->code = code;
}

static inline bool _get_message_id(const unicoap_header_nfc_t* header) {
    return (header->preface & COAP_NFC_PREFACE_INITIATOR_MESSAGE_ID) >> 4;
}

static inline void _set_message_id(unicoap_header_nfc_t* header, bool message_id) {
    header->preface &= ~COAP_NFC_PREFACE_INITIATOR_MESSAGE_ID;
    header->preface |= (message_id & 1) << 4;
}

static inline uint8_t _get_token_length(const unicoap_header_nfc_t* header) {
    return header->preface & COAP_NFC_PREFACE_TKL;
}

static inline void _set_token_length(unicoap_header_nfc_t* header, uint8_t token_length) {
    assert((token_length & ~0xf) == 0);
    header->preface &= ~COAP_NFC_PREFACE_TKL;
    header->preface |= token_length;
}

ssize_t unicoap_pdu_parse_nfc(uint8_t* pdu, size_t size, unicoap_message_t* message,
                                  unicoap_message_properties_t* properties
) {
    size_t min_length = properties->nfc.compatibility_mode ?
        4 /* CLA=Preface, INS=Code, P1, P2 */ :
        1 /* Preface */;

    if (size < min_length) {
        _PDU_DEBUG("msg too short\n");
        return -EBADMSG;
    }

    const unicoap_header_nfc_t* header = (unicoap_header_nfc_t*)pdu;

    if (!_has_fixed_bit(header)) {
        _PDU_DEBUG("malformed, bit 7 not set\n");
        return -EBADMSG;
    }

    if (properties->nfc.direction == UNICOAP_NFC_DIRECTION_DOWNSTREAM && !_is_rfu_zero(header)) {
        _PDU_DEBUG("malformed, bit 5 set in downstream PDU\n");
        return -EBADMSG;
    }

    if (_get_token_length(header) != 0 && (size == 1 || _get_code(header) == UNICOAP_CODE_EMPTY)) {
        _PDU_DEBUG("malformed, token in 0.00 empty message\n");
        return -EBADMSG;
    }

    if (!properties->nfc.compatibility_mode && size == 1) {
        _PDU_DEBUG("well-formed, code 0.00 absent\n");
        message->code = UNICOAP_CODE_EMPTY;
        properties->token = NULL;
        properties->token_length = 0;
        properties->nfc.id = 0;
        properties->nfc.reset = 0;
        properties->nfc.indication = UNICOAP_NFC_INDICATION_RFU;
        return size;
    }

    if (_get_code(header) == UNICOAP_CODE_EMPTY) {
        if (properties->nfc.compatibility_mode) {
            if (size > 4) {
                _PDU_DEBUG("0.00 empty msg is too long\n");
                return -EBADMSG;
            }
            if (pdu[2] != 0 || pdu[3] != 0) {
                _PDU_DEBUG("0.00 empty msg has P1=0x%02x P2=0x%02x instead of 0\n",
                           pdu[2], pdu[3]);
                return -EBADMSG;
            }
        } else {
            if (size > sizeof(unicoap_header_nfc_t)) {
                _PDU_DEBUG("0.00 empty msg is too long\n");
                return -EBADMSG;
            }
        }

    }

    uint8_t* end = pdu + size;
    uint8_t* cursor = pdu + sizeof(unicoap_header_nfc_t);

    message->code = _get_code(header);
    properties->nfc.reset = _get_reset(header);
    switch (properties->nfc.direction) {
        case UNICOAP_NFC_DIRECTION_DOWNSTREAM:
            properties->nfc.id = _get_message_id(header);
            break;
        case UNICOAP_NFC_DIRECTION_UPSTREAM:
            properties->nfc.indication = _get_indication(header);
            break;
        default:
            UNREACHABLE();
            break;
    }

    properties->token_length = _get_token_length(header);
    if (properties->token_length > CONFIG_UNICOAP_EXTERNAL_TOKEN_LENGTH_MAX) {
        /* From RFC 7252, Section 3
         * https://datatracker.ietf.org/doc/html/rfc7252#section-3
         * Lengths 9-15 are
         * reserved, MUST NOT be sent, and MUST be processed as a message
         * format error. */
        _PDU_DEBUG("invalid token length %" PRIu8 "\n", properties->token_length);
        return -EBADMSG;
    }

    if (properties->nfc.compatibility_mode) {
        if (properties->token_length <= 2) {
            properties->token = cursor;
        }
        /* Skip over P1/P2. */
        cursor += 2;
        if (properties->token_length > 2) {
            if (size < 5) {
                _PDU_DEBUG("missing Lc, expecting token in payload\n");
                return -EBADMSG;
            }
        }

        if (size < 5) {
            /* No Lc, so no payload, and if there's a token, it must have been in P1/P2. */
            return size;
        }

        size_t Lc = (size_t)*cursor++;
        if (Lc == 0) {
            _PDU_DEBUG("extended APDU\n");
            if (size < 7) {
                _PDU_DEBUG("missing extended Lc, expecting token in payload\n");
                return -EBADMSG;
            }
            /* Read more significant part, then less significant part. */
            Lc += ((size_t)*cursor++) << 8;
            Lc += ((size_t)*cursor++);

            if (size - 7 /* CLA, INS, P1, P2, 00, Lc(MSB), Lc(LSB) */ < Lc) {
                _PDU_DEBUG("payload truncated in APDU\n");
                return -EBADMSG;
            }
        } else {
            if (size - 5 /* CLA, INS, P1, P2, Lc */ < Lc) {
                _PDU_DEBUG("payload truncated in APDU\n");
                return -EBADMSG;
            }
        }

        if (properties->token_length > 2) {
            properties->token = cursor;
            cursor += properties->token_length;
        }
    } else {
        properties->token = cursor;
        cursor += properties->token_length;
    }

    if (cursor > end) {
        _PDU_DEBUG("invalid token length %" PRIu8 ", overflow\n", properties->token_length);
        return -EBADMSG;
    }

    return unicoap_pdu_parse_options_and_payload(cursor, end, message);
}

ssize_t unicoap_pdu_build_header_nfc(uint8_t* header, size_t capacity,
                                         const unicoap_message_t* message,
                                         const unicoap_message_properties_t* properties)
{
    assert(properties->token_length <= 0xf);
    /* In compatibility mode (ISO/IEC 7816-4),
     * we may need reserve space for P1, P2 and always for Lc + potentially 16-bit extended Lc. */
    size_t extra = properties->nfc.compatibility_mode ? 5 : 0;
    if (capacity < (sizeof(unicoap_header_nfc_t) + properties->token_length + extra)) {
        return -ENOBUFS;
    }

    *header = 0;
    unicoap_header_nfc_t* _header = (unicoap_header_nfc_t*)header;
    _set_fixed_bit(_header); /* If in compatibility mode, CLA has interindustry bit set. */
    _set_reset(_header, properties->nfc.reset);
    switch (properties->nfc.direction) {
        case UNICOAP_NFC_DIRECTION_DOWNSTREAM:
            _set_message_id(_header, properties->nfc.id);
            break;
        case UNICOAP_NFC_DIRECTION_UPSTREAM:
            _set_indication(_header, properties->nfc.indication);
            break;
        default:
            UNREACHABLE();
            break;
    }

    if (message->code == UNICOAP_CODE_EMPTY && !properties->nfc.compatibility_mode) {
        _PDU_DEBUG("1-byte PDU\n");
        assert(properties->token_length == 0);
        assert(unicoap_message_payload_get_size(message) == 0);
        /* Omit code if 0.00 empty. TKL already zeroed. */
        return 1;
    }

    _set_code(_header, message->code);
    _set_token_length(_header, properties->token_length);
    header += sizeof(unicoap_header_nfc_t);

    if (properties->nfc.compatibility_mode) {
        header[0] = 0; /* P1 */
        header[1] = 0; /* P2 */
        uint8_t* Lc = &header[2];
        size_t apdu_payload_length = unicoap_message_payload_get_size(message);
        if (properties->token_length > 2) {
            /* Cannot encode into P1/P2, needs to go into APDU payload. */
            apdu_payload_length += properties->token_length;
        }
        /* Extended APDU cannot encode payloads longer than whose length fit into u16. */
        if (apdu_payload_length > UINT16_MAX) {
            _PDU_DEBUG("APDU payload longer than 0xffff\n");
            return -EINVAL;
        }

        /* Additional header bytes beyond CLA and INS, right now only P1/P2.
         * This extra capacity is checked to be there first thing in this function. */
        size_t additional_header_bytes = 2;
        if (apdu_payload_length > 0) {
            _PDU_DEBUG("APDU has payload\n");
            bool need_extended_apdu = apdu_payload_length > 0xff;
            /* P1, P2, Lc, and optionally 16-bit extendedn length field with Lc=0 */
            if (need_extended_apdu) {
                Lc[0] = 0;
                Lc[1] = (uint8_t)(apdu_payload_length >> 8);
                Lc[2] = (uint8_t)(apdu_payload_length & 0xff);
                /* Must also encode 0x00, Lc(MSB), Lc(LSB) in addition to CLA, INS, P1, P2. */
                additional_header_bytes += 3;
            } else {
                *Lc = (uint8_t)apdu_payload_length;
                /* Just encode Lc in addition to CLA, INS, P1, P2. */
                additional_header_bytes += 1;
            }
            if (properties->token_length > 2) {
                /* Move cursor into payload field to encode
                 * token in APDU payload (counts towards APDU length field). */
                header += additional_header_bytes /* P1, P2, Lc + optional extended Lc */;
                /* Include Token in additional header bytes now, too.
                 * Need to return correct header length in return statement. */
                additional_header_bytes += properties->token_length;
            }
        }
        memcpy(header, properties->token, properties->token_length);
        if (properties->token_length <= 2) {
            /* Cursor still points to P1, encode
             * token in P1/P2 fields (does not count towards payload field). */
            header += additional_header_bytes /* P1/P2 */;
        }
        return sizeof(unicoap_header_nfc_t) /* CLA=Preface, INS=Code */ + additional_header_bytes;
    } else {
        memcpy(header, properties->token, properties->token_length);
        return sizeof(unicoap_header_nfc_t) + properties->token_length;
    }
}

const char* unicoap_string_from_nfc_indication(unicoap_nfc_indication_t indication) {
    switch ((unsigned int)indication) {
    case UNICOAP_NFC_INDICATION_QUICK:
        return "Q";
    case UNICOAP_NFC_INDICATION_LATER:
        return "L";
    case UNICOAP_NFC_INDICATION_FIN:
        return "FIN";
    case UNICOAP_NFC_INDICATION_RFU:
        return "RFU!";
    default:
        return "?";
    }
}
