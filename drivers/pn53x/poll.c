#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "kernel_defines.h"
#include "ztimer.h"
#include "mutex.h"
#include "pn53x.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "periph/uart.h"
#include "msg.h"
#include "byteorder.h"
#include "architecture.h"
#include "macros/utils.h"
#include "iolist.h"
#include "unaligned.h"

#include "log.h"

#include "net/nfc/nfc_error.h"

#include "pn53x.h"

#define ENABLE_DEBUG CONFIG_PN53_DEBUG
#include "debug.h"

static inline void __debug_hex(const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i += 1) {
        printf("%02X ", buffer[i]);
    }
}

#define PN53_DEBUG_HEX(bytes, size) \
    do {                                \
      if (ENABLE_DEBUG) {             \
          __debug_hex(bytes, size);   \
      }                               \
    } while (0)

#define PN53_DEBUG(command, ...) DEBUG("pn53x." command ": " __VA_ARGS__)
#define PN53_DEBUG_REGISTER(...) PN53_DEBUG("register", __VA_ARGS__)

#define __IOLIST(buffer, size, next) ((iolist_t) { \
    .iol_base = (void*)buffer, \
    .iol_len = (size_t)size, \
    .iol_next = next, \
})

ssize_t pn53_in_jump_for_dep(pn53_dev_t* dev, nfc_field_mode_t mode, nfc_bitrate_t bitrate,
    pn53_nfc_dep_arg_t arg, nfc_dep_id_t* nfc_dep_id, uint8_t* general_bytes, size_t general_length,
    nfc_dep_activation_response_t** response
) {
    assert(bitrate == NFC_BITRATE_106K || bitrate == NFC_BITRATE_212K || bitrate == NFC_BITRATE_424K);
    assert(general_length == 0 || general_bytes);

    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_IN_JUMP_FOR_DEP,
        (uint8_t)mode,
        nfc_bitrate_to_index(bitrate),
        0
    };

    uint8_t* next = &command[3];
    iolist_t _command = __IOLIST(command, sizeof(command), NULL);
    iolist_t _rw_tag = __IOLIST(arg._any,
        bitrate == NFC_BITRATE_106K ? sizeof (nfc_a_id_t) : sizeof(nfc_f_polling_command_payload_t),
        NULL);
    iolist_t _dep_id = __IOLIST(nfc_dep_id, sizeof(nfc_dep_id_t), NULL);
    iolist_t _general = __IOLIST(general_bytes, general_length, NULL);

    iolist_t* current = &_command;

    if (mode == NFC_FIELD_MODE_READER_WRITER_TAG) {
        *next |= !!arg._any & 1;
        current->iol_next = &_rw_tag;
        current = &_rw_tag;
    }

    if (nfc_dep_id && !(mode == NFC_FIELD_MODE_READER_WRITER_TAG && bitrate >= NFC_BITRATE_212K)) {
        *next |= 2;
        current->iol_next = &_dep_id;
        current = &_dep_id;
    }

    if (general_length > 0) {
        *next |= 4;
        current->iol_next = &_general;
        current = &_general;
    }

    uint8_t* _response = NULL;
    ssize_t res = pn53_hci_transceive_command(&dev->connection, &_command, &_response, dev->command_timeout);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*_response++);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
        if ((size_t)res < (2 + sizeof(nfc_dep_activation_response_t))) {
            PN53_DEBUG("InJumpForDep", "missing Tg and ATR_RES\n");
            return -EBADMSG;
        }
        uint8_t logical = *_response++;
        if (logical != 1 && logical != 2) {
            return -EBADMSG;
        }
        PN53_DEBUG("InJumpForDep", "activated Tg=%u in peer field mode\n", logical);
        dev->nfc_targets[logical - 1] = (pn53_logical_target_t) {
            .managed_transport = PN53_MANAGED_TRANSPORT_NFC_DEP,
            .super = {
                .field_mode = NFC_FIELD_MODE_PEERS,
                .parameters.polling.bitrate = bitrate,
                .higher_layer.nfc_dep.atr = * (nfc_dep_activation_response_t*)_response
            }
        };
        if (response) {
            *response = (nfc_dep_activation_response_t*)_response;
        }
    }
    return res;
}

ssize_t pn53_in_atr(pn53_dev_t* dev, nfcdev_connection_id_t connection_id, nfc_dep_id_t* nfc_dep_id, uint8_t* general_bytes, size_t general_length,
    nfc_dep_activation_response_t** response
) {
    assert(general_length == 0 || general_bytes);
    pn53_logical_target_t* target = pn53_target(dev, connection_id);
    if (!target) {
        return -ENOTCONN;
    }

    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_IN_JUMP_FOR_DEP,
        (uint8_t)(connection_id + 1),
        0
    };

    uint8_t* next = &command[2];
    iolist_t _command = __IOLIST(command, sizeof(command), NULL);
    iolist_t _dep_id = __IOLIST(nfc_dep_id, sizeof(nfc_dep_id_t), NULL);
    iolist_t _general = __IOLIST(general_bytes, general_length, NULL);

    iolist_t* current = &_command;

    if (nfc_dep_id && target->super.parameters.polling.bitrate == NFC_BITRATE_106K) {
        *next |= 1;
        current->iol_next = &_dep_id;
        current = &_dep_id;
    }

    if (general_length > 0) {
        *next |= 2;
        current->iol_next = &_general;
        current = &_general;
    }

    uint8_t* _response = NULL;
    ssize_t res = pn53_hci_transceive_command(&dev->connection, &_command, &_response, dev->command_timeout);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*_response++);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
        if ((size_t)res < (1 + sizeof(nfc_dep_activation_response_t))) {
            PN53_DEBUG("InAtr", "missing Tg and ATR_RES\n");
            return -EBADMSG;
        }
        PN53_DEBUG("InAtr", "activated NFC-DEP for Tg=%u in r/w+tag field mode\n", connection_id + 1);
        target->managed_transport = PN53_MANAGED_TRANSPORT_NFC_DEP;
        target->super.higher_layer.nfc_dep.atr = *(nfc_dep_activation_response_t*)_response;
        if (response) {
            *response = (nfc_dep_activation_response_t*)_response;
        }
    }
    return res;
}

int pn53_in_psl(pn53_dev_t* dev, nfcdev_connection_id_t connection_id, nfc_bitrate_t downstream, nfc_bitrate_t upstream) {
    pn53_logical_target_t* target = pn53_target(dev, connection_id);
    if (!target) {
        return -ENOTCONN;
    }
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_IN_PSL,
        (uint8_t)(connection_id),
        (uint8_t)nfc_bitrate_to_index(downstream),
        (uint8_t)nfc_bitrate_to_index(upstream)
    };
    uint8_t* _response;
    ssize_t res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), &_response, dev->command_timeout);
    if (res > 0) {
        pn53_status_code_t status = pn53_status_code(*_response);
        if (status != PN53_STATUS_SUCCESS) {
            return -PN53_ERRNO_FROM_STATUS_CODE(status);
        }
    }
    return 0;
}

ssize_t pn53_parse_passive_targets(
    uint8_t* response, size_t length, pn53_logical_target_t* targets, uint8_t max_targets, void* arg,
    ssize_t (*parse)(uint8_t* response, size_t length, pn53_logical_target_t* target, void* arg)
) {
    memset(targets, 0, sizeof(pn53_logical_target_t) * max_targets);

    if (length < 1) {
        PN53_DEBUG("InList", "missing NbTg\n");
        goto malformed;
    }

    uint8_t target_count = *response++;
    PN53_DEBUG("InList", "%u targets\n", (unsigned int)target_count);
    if (target_count > max_targets) {
        PN53_DEBUG("InList", "... but max=%u\n", (unsigned int)max_targets);
        goto malformed;
    }
    length -= 1;

    for (uint8_t i = 0; i < target_count; i += 1) {
        if (length < 1) {
            PN53_DEBUG("InList", "missing Tg\n");
            goto malformed;
        }

        uint8_t tg = *response++;
        assert(tg == i + 1);
        length -= 1;

        ssize_t remaining = parse(response, length, &targets[i], arg);
        if (remaining < 0) {
            goto malformed;
        }
        PN53_DEBUG("InList", "initialized Tg=%u in r/w+tag field mode\n", tg);
        targets[i].super.field_mode = NFC_FIELD_MODE_READER_WRITER_TAG;

        response += length - (size_t)remaining;
        length = (size_t)remaining;
    }
    if (length > 0) {
        PN53_DEBUG("InList", "excess data %" PRIdSIZE " bytes\n", length);
    }
    return (ssize_t)target_count;

malformed:
    memset(targets, 0, sizeof(pn53_logical_target_t) * max_targets);
    return -EBADMSG;
}

ssize_t pn53_parse_passive_target_a(uint8_t* response, size_t length, pn53_logical_target_t* target, void* auto_rats_enabled) {
    assert(target);
    target->super.tag.technology = NFC_TECHNOLOGY_A;
    target->super.parameters.polling.bitrate = NFC_BITRATE_106K;

    nfc_a_tag_t* tag = &target->super.tag.a;

    if (length < 4) {
        PN53_DEBUG("InList.a", "target header too short\n");
        return -EBADMSG;
    }

    tag->polling_response.raw[0] = *response++;
    tag->polling_response.raw[1] = *response++;
    tag->select_response = *response++;
    PN53_DEBUG("List.a", "atqa=%02x%02x sak=%02x\n",
               tag->polling_response.raw[0], tag->polling_response.raw[1], tag->select_response);

    nfc_a_id_t* id = (nfc_a_id_t*)response++;
    length -= 4;

    if (length < (size_t)id->length) {
        PN53_DEBUG("InList.a", "id cut off, "
                   "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                   (size_t)id->length, length);
        return -EBADMSG;
    }
    memcpy(&tag->id, id, 1 + (size_t)id->length);
    response += tag->id.length;
    length -= tag->id.length;

    if (auto_rats_enabled && nfc_a_supports_iso_dep(tag->select_response) && length > 0) {
        nfc_a_ats_t* ats = (nfc_a_ats_t*)response;
        if (length < (size_t)ats->length) {
            PN53_DEBUG("InList.a", "ATS cut off, expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                       (size_t)ats->length, length);
            return -EBADMSG;
        }
        PN53_DEBUG("InList.a", "ATS length=%" PRIuSIZE "\n", (size_t)ats->length);

        memcpy(&tag->ats, ats, (size_t)ats->length);
        target->managed_transport = PN53_MANAGED_TRANSPORT_ISO_DEP;

        response += ats->length;
        length -= ats->length;
    }
    return length;
}

ssize_t pn53_parse_passive_target_b(uint8_t* response, size_t length, pn53_logical_target_t* target, void* arg) {
    (void)arg;
    assert(target);
    target->super.tag.technology = NFC_TECHNOLOGY_B;
    target->super.parameters.polling.bitrate = NFC_BITRATE_106K;
    nfc_b_tag_t* tag = &target->super.tag.b;

    if (length < (sizeof(nfc_b_polling_response_payload_t) + 2 /* 0x50, ATTRIB length */)) {
        PN53_DEBUG("InList.b", "target header too short\n");
        return -EBADMSG;
    }

    /* 0x50, the ATQB frame code / prefix is missing from the PN532/PN533 manuals.
     * Carl's PN532 on the Adafruit PN532 shield sends an addition 0x01 byte before 0x50. */

    // TODO: use while loop

    uint8_t next = *response++;
    length -= 1;
    if (next == 1 || next == 2) {
        PN53_DEBUG("InList.b", "0x%02x in place of 0x50 ATQB prefix, skipping\n", next);
        next = *response++;
        length -= 1;
    }
    if (next != NFC_B_FRAME_CODE_POLLING_RESPONSE) {
        PN53_DEBUG("InList.b", "malformed, missing 0x50 ATQB prefix\n");
        return -EBADMSG;
    }

    nfc_b_polling_response_payload_t* polling_response = (nfc_b_polling_response_payload_t*)response;
    memcpy(&tag->polling_response, polling_response, sizeof(nfc_b_polling_response_payload_t));

    response += sizeof(nfc_b_polling_response_payload_t);
    length -= sizeof(nfc_b_polling_response_payload_t);

    if (length > 0) {
        uint8_t attrib_length = *response++;
        length -= 1;

        if (length < (size_t)attrib_length) {
            PN53_DEBUG("InList.b", "ATTRIB cut off, "
                       "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                       (size_t)attrib_length, (size_t)length);
            return -EBADMSG;
        }

        if (attrib_length > 0) {
            tag->attrib_response_length = (size_t)attrib_length;
            tag->attrib = *(nfc_b_attrib_response_t*)response;

            response += (size_t)attrib_length;
            length -= attrib_length;
        }
    }
    return length;
}

ssize_t pn53_parse_passive_target_f(uint8_t* response, size_t length, pn53_logical_target_t* target, void* additional_request) {
    assert(target);
    target->super.tag.technology = NFC_TECHNOLOGY_F;
    nfc_f_tag_t* tag = &target->super.tag.f;

    if (length < (sizeof(nfc_f_polling_response_t) - sizeof(nfc_f_polling_response_payload_t))) {
        PN53_DEBUG("InList.f", "target header too short\n");
        return -EBADMSG;
    }

    nfc_f_polling_response_t* pol = (nfc_f_polling_response_t*)response;
    if (length < (size_t)pol->header.length) {
        PN53_DEBUG("InList.f", "POL_RES cut off, expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                   (size_t)pol->header.length, length);
        return -EBADMSG;
    }
    if (pol->header.code != NFC_F_PACKET_CODE_POLLING_RESPONSE) {
        PN53_DEBUG("InList.f", "invalid response code %02x\n", (uint8_t)pol->header.code);
        return -EBADMSG;
    }

    size_t expected = sizeof(nfc_f_polling_response_t);
    if (additional_request == NFC_F_POLLING_REQUEST_NOTHING) {
        expected -= sizeof(nfc_f_polling_response_payload_t);
    }

    if (pol->header.length != expected) {
        PN53_DEBUG("InList.f", "POL_RES had invalid length, "
                   "expected %" PRIuSIZE ", have %" PRIuSIZE "\n",
                   expected, (size_t)pol->header.length);
        return -EBADMSG;
    }
    length -= expected;

    switch (*(nfc_f_polling_additional_request_t*)additional_request) {
        case NFC_F_POLLING_REQUEST_SYSTEM_CODE:
            tag->system_code = byteorder_lebuftohs(pol->payload);
            break;
        case NFC_F_POLLING_REQUEST_BITRATES:
            tag->bitrates = (nfc_bitrate_t)pol->payload[1] << 1;
            break;
        default: break;
    }

    tag->id = pol->id;
    tag->pmm = pol->pmm;
    return length;
}

ssize_t pn53_in_list_passive_targets_a(pn53_dev_t* dev, uint8_t max_targets, nfc_a_id_t* id, nfc_target_t* targets, uint32_t timeout_ms) {
    assert(max_targets > 0);
    assert(max_targets <= ARRAY_SIZE(dev->nfc_targets));
    assert(max_targets <= ((dev->model == PN53_MODEL_PN532) ? 2 : 1));

    ssize_t res = 0;
    uint8_t command[3 + 10 + 2] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)PN53_TECHNOLOGY_A_106K,
        NFC_A_UID_CASCADE_TAG, 0, 0, 0, NFC_A_UID_CASCADE_TAG
    };

    size_t length = 3;

    if (id && (id->length > 0)) {
        PN53_DEBUG("InList.a", "anticol with given id of length %u\n", id->length);
        length += id->length;
        switch (id->length) {
            case NFC_A_UID_LENGTH_SINGLE_4:
                memcpy(&command[3], id->uid, NFC_A_UID_LENGTH_SINGLE_4);
                break;
            case NFC_A_UID_LENGTH_DOUBLE_7:
                length += 1;
                memcpy(&command[4], id->uid, NFC_A_UID_LENGTH_DOUBLE_7);
                break;
            case NFC_A_UID_LENGTH_TRIPLE_10:
                length += 2;
                memcpy(&command[4], id->uid, 3);
                memcpy(&command[8], &id->uid[3], 7);
                break;
            default:
                break;
        }
    }

    dev->nfc_role = NFC_ROLE_INITIATOR;
    uint8_t* response;
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, length, &response, timeout_ms)) < 0) {
        return res;
    }
    if ((res = pn53_parse_passive_targets(response, (size_t)res, dev->nfc_targets, max_targets,
        dev->nfc_parameters & (1 << 4) ? response : NULL, pn53_parse_passive_target_a
    )) < 0) {
        return res;
    }
    if (targets) {
        for (uint8_t i = 0; i < max_targets; i += 1) {
            targets[i] = dev->nfc_targets[i].super;
        }
    }
    return res;
}

ssize_t pn53_in_list_passive_targets_b(pn53_dev_t* dev, uint8_t max_targets, nfc_bitrate_t bitrate,
                                uint8_t application_family, nfc_b_polling_method_t method,
                                nfc_target_t* targets, uint32_t timeout_ms
) {
    assert(max_targets > 0);
    assert(max_targets <= ARRAY_SIZE(dev->nfc_targets));
    assert(max_targets <= ((dev->model == PN53_MODEL_PN532) ? 2 : 1));

    ssize_t res = 0;
    pn53_technology_baudrate_t brty;
    switch (bitrate) {
        case NFC_BITRATE_106K: brty = PN53_TECHNOLOGY_B_106K; break;
        case NFC_BITRATE_212K: brty = PN53_TECHNOLOGY_B_212K; break;
        case NFC_BITRATE_424K:
            if (dev->model == PN53_MODEL_PN533) {
                brty = PN53_TECHNOLOGY_B_424K; break;
            }
            // fallthrough
        case NFC_BITRATE_848K:
            if (dev->model == PN53_MODEL_PN533) {
                brty = PN53_TECHNOLOGY_B_848K; break;
            }
            // fallthrough
        default:
            PN53_DEBUG("InList.b", "unsupported bitrate %u kbit/s\n", nfc_bitrate_kbps(bitrate));
            return -ENOTSUP;
    }

    uint8_t* response;
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)brty,
        application_family,
        (uint8_t)method
    };

    dev->nfc_role = NFC_ROLE_INITIATOR;
    // Only append method if method argument != 0
    size_t length = sizeof(command) - ((method != 0) ? 0 : 1);
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, length, &response, timeout_ms)) < 0) {
        return res;
    }
    if ((res = pn53_parse_passive_targets(response, (size_t)res, dev->nfc_targets, max_targets,
        NULL, pn53_parse_passive_target_b
    )) < 0) {
        return res;
    }
    if (targets) {
        for (uint8_t i = 0; i < max_targets; i += 1) {
            targets[i] = dev->nfc_targets[i].super;
        }
    }
    return res;
}

ssize_t pn53_in_list_passive_targets_f(pn53_dev_t* dev, uint8_t max_targets, nfc_bitrate_t bitrate,
    nfc_f_polling_command_payload_t* payload, nfc_target_t* targets, uint32_t timeout_ms
) {
    assert(payload);
    assert(max_targets > 0);
    assert(max_targets <= ARRAY_SIZE(dev->nfc_targets));
    assert(max_targets <= ((dev->model == PN53_MODEL_PN532) ? 2 : 1));

    ssize_t res = 0;
    pn53_technology_baudrate_t brty;
    switch (bitrate) {
        case NFC_BITRATE_212K: brty = PN53_TECHNOLOGY_F_212K; break;
        case NFC_BITRATE_424K: brty = PN53_TECHNOLOGY_F_424K; break;
        default:
            PN53_DEBUG("InList.f", "unsupported bitrate %u kbit/s\n", nfc_bitrate_kbps(bitrate));
            return -ENOTSUP;
    }

    uint8_t* response;
    uint8_t command[3 + sizeof(nfc_f_polling_command_t) - 1 /* without length */] = {
        (uint8_t)PN53_COMMAND_IN_LIST_PASSIVE_TARGET,
        max_targets,
        (uint8_t)brty
    };
    nfc_f_polling_command_t* pol = (nfc_f_polling_command_t*)(&command[2]);
    pol->header.code = NFC_F_PACKET_CODE_POLLING_COMMAND;
    pol->payload = *payload;

    dev->nfc_role = NFC_ROLE_INITIATOR;
    // Only append method if method argument != 0
    if ((res = pn53_hci_transceive_command2(&dev->connection, command, sizeof(command), &response, timeout_ms)) < 0) {
        return res;
    }
    if ((res = pn53_parse_passive_targets(response, (size_t)res, dev->nfc_targets, max_targets,
        &payload->additional_request, pn53_parse_passive_target_f
    )) < 0) {
        return res;
    }
    for (uint8_t i = 0; i < (uint8_t)res; i += 1) {
        dev->nfc_targets[i].super.parameters.polling.bitrate = bitrate;
    }
    if (targets) {
        for (uint8_t i = 0; i < max_targets; i += 1) {
            targets[i] = dev->nfc_targets[i].super;
        }
    }
    return res;
}

static ssize_t nfcdev_poll_pn53_in_list(nfcdev_t* nfcdev, const nfcdev_polling_loop_t* loop, ssize_t frame_ix, uint8_t max_targets, uint32_t timeout_ms) {
    pn53_dev_t* dev = nfcdev->dev;
    assert(loop->tag);
    ssize_t res = 0;
    switch (loop->tag->technology) {
        case NFC_TECHNOLOGY_A:
            if (pn53_bitfield_get(dev->nfc_parameters,
                  PN53_NFC_PARAMETER_INITIATOR_ISO_DEP_AUTO_HANDSHAKE) !=
                (!!loop->tag->a.rats & 1)
            ) {
                PN53_DEBUG("poll", "need to adjust auto RATS\n");
                if ((res = pn53_set_parameters_enablement(dev,
                     PN53_NFC_PARAMETER_INITIATOR_ISO_DEP_AUTO_HANDSHAKE, !!loop->tag->a.rats
                )) < 0) {
                    return res;
                }
            }
            if ((res = pn53_in_list_passive_targets_a(dev, max_targets, loop->tag->a.id, NULL, timeout_ms)) < 0) {
                return res;
            }
            break;
        case NFC_TECHNOLOGY_B: {
            nfc_b_tag_polling_config_t* config = &loop->tag->b;
            assert(
                (frame_ix < 0) ||
                (frame_ix < (ssize_t)config->frame_count &&
                 config->frames[frame_ix].length == sizeof(nfc_b_polling_command_t) &&
                 ((nfc_b_polling_command_t*)config->frames[frame_ix].frame)->code == NFC_B_FRAME_CODE_POLLING &&
                 ((nfc_b_polling_command_t*)config->frames[frame_ix].frame)->payload.raw[1] == 0)
            );

            uint8_t afi = frame_ix < 0 ? 0xff : ((nfc_b_polling_command_t*)config->frames[frame_ix].frame)->payload.application_family;
            nfc_bitrate_t higher_layer_bitrate = loop->tag->b.attrib ?
                iso_dep_bitrate_from_divisor_power(loop->tag->b.attrib->down_bitrate_divisor_power) :
                NFC_BITRATE_106K;

            res = pn53_in_list_passive_targets_b(dev, max_targets,
                 higher_layer_bitrate, afi, loop->tag->b.method, NULL, timeout_ms);
            if (res < 0) {
                return res;
            }
            break;
        }
        case NFC_TECHNOLOGY_F:
            assert((frame_ix < 0) || (frame_ix < (ssize_t)loop->tag->f.frame_count));
            static nfc_f_polling_command_payload_t default_polreq =  {
                .system_code = 0xffff,
                .additional_request = NFC_F_POLLING_REQUEST_SYSTEM_CODE,
                .timeslots = 0
            };
            res = pn53_in_list_passive_targets_f(dev, max_targets,
                loop->bitrate, frame_ix < 0 ? &default_polreq : &loop->tag->f.frames[frame_ix],
                NULL, timeout_ms);
            if (res < 0) {
                return res;
            }
            break;
        default:
            assert(false);
            UNREACHABLE();
            return -1;
    }

    size_t found = 0;
    for (nfcdev_connection_id_t i = 0; i < (uint8_t)res; i += 1) {
        if (nfcdev_polling_filter_matches(loop->tag, &dev->nfc_targets[i].super.tag)) {
            found += 1;
        } else {
            PN53_DEBUG("poll", "releasing non-matching target Tg=%u\n", i+1);
            if ((res = pn53_release(dev, i)) < 0) {
                PN53_DEBUG("poll", "failed to release target Tg=%u to matching tag filter\n", +1);
            }
        }
    }
    return found;
}


static bool _poll_config_a_contains(const nfc_a_tag_polling_config_t* config, const nfc_a_polling_frame_t* frame) {
    for (size_t i = 0; i < config->frame_count; i += 1) {
        assert(config->frames);
        if (nfc_a_polling_frame_is_equal(&config->frames[i], frame)) {
            return true;
        }
    }
    return false;
}

#define PN53_SKIP_POLLING_LOOP (1)

static int _check_polling_loop(const nfcdev_polling_loop_t* loop) {
    switch (loop->field_mode) {
        case NFC_FIELD_MODE_READER_WRITER_TAG:
            switch (loop->tag->technology) {
                case NFC_TECHNOLOGY_A: {
                    if (loop->timing.interval != NFCDEV_POLLING_INTERVAL_BUILTIN) {
                        if (loop->timing.interval < PN53_POLL_INTERVAL_RW_MODE_A_B_MS - 1) {
                            PN53_DEBUG("poll", "polling interval too narrow\n");
                            return -ENOTSUP;
                        }
                    }
                    if (loop->bitrate != NFC_BITRATE_106K) {
                        PN53_DEBUG("poll", "can only poll @ 106 kbps\n");
                        return -ENOTSUP;
                    }
                    if (loop->tag->a.frame_count != 0 && !_poll_config_a_contains(&loop->tag->a, loop->tag->a.id ? &nfc_a_polling_frame_all : &nfc_a_polling_frame_only_awake)) {
                        PN53_DEBUG("poll", "polling loop must contain REQA/WUPA\n");
                        return -ENOTSUP;
                    }
                    nfc_a_tag_polling_config_t* config = &loop->tag->a;
                    if (config->frame_count > 0) {
                        for (size_t frame_ix = 0; frame_ix <= config->frame_count; frame_ix += 1) {
                            nfc_a_polling_frame_t* frame = &config->frames[frame_ix];
                            if (frame->interface > NFCDEV_INTERFACE_PACKET) {
                                PN53_DEBUG("poll", "cannot use interface higher than BITS/FRAME/PACKET\n");
                                return -EINVAL;
                            }
                        }
                    }
                    break;
                }
                case NFC_TECHNOLOGY_B: {
                    if (loop->timing.interval != NFCDEV_POLLING_INTERVAL_BUILTIN) {
                        if (loop->timing.interval < PN53_POLL_INTERVAL_RW_MODE_A_B_MS - 1) {
                            PN53_DEBUG("poll", "polling interval too narrow\n");
                            return -ENOTSUP;
                        }
                    }
                    if (loop->bitrate != NFC_BITRATE_106K) {
                        return -ENOTSUP;
                    }
                    nfc_b_tag_polling_config_t* config = &loop->tag->b;
                    if (config->frame_count > 0) {
                        for (size_t frame_ix = 0; frame_ix <= config->frame_count; frame_ix += 1) {
                            nfc_b_polling_frame_t* frame = &config->frames[frame_ix];
                            if (frame->interface == NFCDEV_INTERFACE_BITS) {
                                PN53_DEBUG("poll", "cannot use BITS interface in NFC-B comms\n");
                                return -EINVAL;
                            }
                            if (frame->interface > NFCDEV_INTERFACE_PACKET) {
                                PN53_DEBUG("poll", "cannot use interface higher than FRAME/PACKET\n");
                                return -EINVAL;
                            }

                            if ((frame->interface == NFCDEV_INTERFACE_PACKET && /* CRC by controller */
                                 frame->length == (sizeof(nfc_b_polling_command_t))) ||
                                (frame->interface == NFCDEV_INTERFACE_FRAME && /* CRC by host */
                                 frame->length == (sizeof(nfc_b_polling_command_t) + 2))
                            ) {
                                nfc_b_polling_command_t* command = (nfc_b_polling_command_t*)frame->frame;
                                if (command->code == NFC_B_FRAME_CODE_POLLING) {
                                    if (command->payload.raw[1] != 0) {
                                        // PN53x have no extended ATQB support, no WUPB support
                                        return -ENOTSUP;
                                    }
                                }
                            }
                        }
                    }

                    break;
                }
                case NFC_TECHNOLOGY_F:
                    if ((loop->bitrate & ~(NFC_BITRATE_212K | NFC_BITRATE_424K)) != 0) {
                        PN53_DEBUG("poll", "only F@212 and F@424 are standardized!\n");
                        return -EINVAL;
                    }
                    nfc_f_tag_polling_config_t* config = &loop->tag->f;
                    if (loop->timing.interval != NFCDEV_POLLING_INTERVAL_BUILTIN && config->frame_count > 0) {
                        for (size_t frame_ix = 0; frame_ix <= config->frame_count; frame_ix += 1) {
                            uint32_t response_window = nfc_f_polling_response_time_ms(config->frames[frame_ix].timeslots);
                            if (loop->timing.interval < (response_window - 1)) {
                                PN53_DEBUG("poll", "specified interval of %" PRIu32 " ms for NFC-F,"
                                           " but given timeslot number (TSN) defines response time of"
                                           " %" PRIu32 " ms\n", loop->timing.interval, response_window);
                                return -EINVAL;
                            }
                        }
                    }

                    break;
                default:
                    return -ENOTSUP;
            }

        case NFC_FIELD_MODE_PEERS:
            break;
        default:
            assert(false);
            UNREACHABLE();
            return -1;
    }

    return 0;
}

static uint32_t _builtin_interval_rw_tag(const nfcdev_polling_loop_t* loop) {
    switch (loop->tag->technology) {
        case NFC_TECHNOLOGY_A:
        case NFC_TECHNOLOGY_B:
            return PN53_POLL_INTERVAL_RW_MODE_A_B_MS;
        case NFC_TECHNOLOGY_F:
            return nfc_f_polling_response_time_ms(
                loop->tag->f.frame_count == 0 ? 0 : loop->tag->f.frames[0].timeslots);
        default:
            assert(false);
            UNREACHABLE();
    }
}

static bool _is_builtin_polling_frame(const nfcdev_tag_polling_config_t* config, size_t frame_ix) {
    assert(config);
    switch (config->technology) {
        case NFC_TECHNOLOGY_A:
            return nfc_a_polling_frame_is_equal(&config->a.frames[frame_ix],
                config->a.id ? &nfc_a_polling_frame_all : &nfc_a_polling_frame_only_awake);
        case NFC_TECHNOLOGY_B:
            nfc_b_polling_command_t* reqb = (nfc_b_polling_command_t*)config->b.frames[frame_ix].frame;
            return config->b.frames[frame_ix].length == sizeof(nfc_b_polling_command_t) &&
                reqb->code == NFC_B_FRAME_CODE_POLLING &&
                reqb->raw[1] == 0;
        case NFC_TECHNOLOGY_F:
            // NFC-F polling frames are typed to nfc_f_polling_command_payload_t, so
            // we don't need to check as PN53 support customising all NFC-F Polling Request
            // fields.
            return true;
        default:
            UNREACHABLE();
            return false;
    }
}

static ssize_t _poll_with_builtin_frame(nfcdev_t* nfcdev, const nfcdev_polling_loop_t* loop,
    size_t frame_ix, size_t max_targets, uint32_t timeout_ms
) {
    ssize_t res = 0;
    uint32_t delta = ztimer_now(ZTIMER_MSEC);
    if ((res = nfcdev_poll_pn53_in_list(nfcdev, loop, frame_ix, max_targets, timeout_ms)) < 0) {
        switch (res) {
            case -PN53_ERROR_CONNECTION_TIMEOUT:
            case -PN53_ERRNO_FROM_STATUS_CODE(PN53_STATUS_ERROR_RF_TIMEOUT):
            case -PN53_ERRNO_FROM_STATUS_CODE(PN53_STATUS_ERROR_TARGET_ANSWER_TIMED_OUT):
                PN53_DEBUG("poll", "InList timed out despite manually set retries\n");
                break;
            default:
                return res;
        }
    }
    if (res > 0) {
        return res;
    }
    if ((delta = ztimer_now(ZTIMER_MSEC) - delta) < loop->timing.interval) {
        uint32_t remaining_wait = loop->timing.interval - delta;
        PN53_DEBUG("poll", "desired interval is %" PRIu32 " ms, so waiting %" PRIu32 " ms\n", loop->timing.interval, remaining_wait);
        if (remaining_wait > 0) {
            ztimer_sleep(ZTIMER_MSEC, remaining_wait);
        }
    }
    return 0;
}

static int _broadcast_custom_polling_frame(nfcdev_t* nfcdev, const nfcdev_tag_polling_config_t* config, size_t frame_ix, uint32_t timeout_ms) {
    assert(config);
    ssize_t res = 0;
    switch (config->technology) {
        case NFC_TECHNOLOGY_A: {
            nfc_a_polling_frame_t* frame = &config->a.frames[frame_ix];
            res = nfcdev_transceive(nfcdev, frame->frame, frame->length, NULL, 0, timeout_ms, frame->interface);
            break;
        }
        case NFC_TECHNOLOGY_B: {
            nfc_b_polling_frame_t* frame = &config->b.frames[frame_ix];
            res = nfcdev_transceive(nfcdev, frame->frame, frame->length, NULL, 0, timeout_ms, frame->interface);
            break;
        }
        case NFC_TECHNOLOGY_F:
            // NFC-F polling frames are typed to nfc_f_polling_command_payload_t, so
            // we don't need to check as PN53 support customising all NFC-F Polling Request
            // fields.
            return -EINVAL;
        default:
            UNREACHABLE();
            return -1;
    }
    if (res < 0) {
        switch (res) {
            case -PN53_ERROR_CONNECTION_TIMEOUT:
            case -PN53_ERRNO_FROM_STATUS_CODE(PN53_STATUS_ERROR_RF_TIMEOUT):
            case -PN53_ERRNO_FROM_STATUS_CODE(PN53_STATUS_ERROR_TARGET_ANSWER_TIMED_OUT):
                PN53_DEBUG("poll", "InList timed out despite manually set retries\n");
                break;
            default:
                return res;
        }
    }
    return 0;
}

ssize_t nfcdev_poll_pn53_in_loop_in_list_rw_tag(nfcdev_t* nfcdev, const nfcdev_polling_loop_t* loop, size_t max_targets) {
    assert(loop->tag->technology == NFC_TECHNOLOGY_A ||
           loop->tag->technology == NFC_TECHNOLOGY_B ||
           loop->tag->technology == NFC_TECHNOLOGY_F);

    pn53_dev_t* dev = nfcdev->dev;
    ssize_t res = 0;
    // max retries are 0xFE because 0xFF means infinite retries,
    // we checked earlier that the tag polling config does either not
    // list any polling frames (we get to decide, built-in) or it lists at least the standard frame,
    // i.e., REQA for NFC-A, REQB for NFC-B, ... for NFC-F,
    // which is what PN53x send. So, if the frame count is 1,
    // that frame is the standard frame, that has been checked.
    pn53_rf_configuration_payload_t rf_config = {
        .item = PN53_RF_CONFIGURATION_ITEM_MAXIMUM_RETRIES,
        .max_retries = {
            .polling_mode_peers = 0xff,
            .parameter_selection = 0x00,
        }
    };
    size_t frame_count = nfcdev_tag_polling_config_frame_count(loop->tag);
    ssize_t default_frame_ix = frame_count > 0 ? 0 : -1;
    uint32_t interval_builtin = _builtin_interval_rw_tag(loop);

    bool interval_is_builtin = (loop->timing.interval == NFCDEV_POLLING_INTERVAL_BUILTIN) ||
        (loop->timing.interval >= (interval_builtin - 1) &&
         loop->timing.interval <= (interval_builtin + 1));

    uint32_t interval = loop->timing.interval == NFCDEV_POLLING_INTERVAL_BUILTIN
        ? interval_builtin : loop->timing.interval;

    // TODO: we can actually modify this such that we can also use one InList by setting the retries to infinity and then calculate the timeout for approximately the retries desired
    if (frame_count <= 1 && interval_is_builtin) {
        PN53_DEBUG("poll", "one InList covers all intervals\n");
        // can configure InListPassiveTarget, otherwise have to do it manually...
        rf_config.max_retries.polling_mode_rw_tag = (uint8_t)(
            loop->timing.retries == NFCDEV_POLLING_RETRIES_INFINITE || loop->timing.retries > 0xfe
                ? 0xff : loop->timing.retries);

        if ((res = pn53_rf_configuration(dev, &rf_config)) < 0) {
            return res;
        }

        uint32_t timeout_ms;
        if (loop->timing.retries == NFCDEV_POLLING_RETRIES_INFINITE) {
            timeout_ms = PN53_TIMEOUT_NEVER;
        } else if (loop->timing.retries > 0xfe) {
            // Need to set timeout such that number of retries is achieved, roughly...
            // 5.6 ms measured built-in interval.
            timeout_ms = ((56) * (loop->timing.retries)) / 10;
            PN53_DEBUG("poll", "approximating %" PRIuSIZE " retries,"
                       " setting timeout of 5 ms * %" PRIuSIZE " = %" PRIu32 " ms\n",
                       loop->timing.retries, loop->timing.retries, timeout_ms);
        } else {
            // Reasonable command timeout.
            // Use a bit of tolerance for the entire command (if it also initializes
            // the tag, then that requires time, and the interval stated in the manual is
            // also approximate, plus small bus transfer times).
            timeout_ms = (interval_builtin + 1) * (loop->timing.retries + 1 /* initial try */) + 10;
        }

        if ((res = nfcdev_poll_pn53_in_list(nfcdev, loop, default_frame_ix, max_targets, timeout_ms)) < 0) {
            switch (res) {
                case -PN53_ERROR_CONNECTION_TIMEOUT:
                case -PN53_ERRNO_FROM_STATUS_CODE(PN53_STATUS_ERROR_RF_TIMEOUT):
                case -PN53_ERRNO_FROM_STATUS_CODE(PN53_STATUS_ERROR_TARGET_ANSWER_TIMED_OUT):
                    if (loop->timing.retries == NFCDEV_POLLING_RETRIES_INFINITE) {
                        PN53_DEBUG("poll", "InList timed out despite manually set retries\n");
                    } else {
                        PN53_DEBUG("poll", "InList did not find any target\n");
                    }
                    break;
                default:
                    return res;
            }
        }
        if (res > 0) {
            return res;
        }
    } else {
        PN53_DEBUG("poll", "need InList per interval\n");
        rf_config.max_retries.polling_mode_rw_tag = 0;
        if ((res = pn53_rf_configuration(dev, &rf_config)) < 0) {
            return res;
        }

        // try wraps around as intended when loop->timing.retries is NFCDEC_POLLING_RETRIES_INIFNITE
        // aka. SIZE_MAX.
        for (size_t try = 0; try <= loop->timing.retries; try += 1) {
            if (frame_count <= 1) {
                if ((res = _poll_with_builtin_frame(nfcdev, loop, default_frame_ix, max_targets, interval + 10)) != 0) {
                    return res;
                }
            } else {
                for (ssize_t frame_ix = 0; (size_t)frame_ix <= frame_count; frame_ix += 1) {
                    if (_is_builtin_polling_frame(loop->tag, frame_ix)) {
                        PN53_DEBUG("poll", "[try %" PRIuSIZE "/%" PRIuSIZE "] REQA using InList\n",
                                try, loop->timing.retries);
                        if ((res = _poll_with_builtin_frame(nfcdev, loop, frame_ix, max_targets, interval + 10)) != 0) {
                            return res;
                        }
                    } else {
                        // TODO: when to re-enable field?
                        // TODO: when to configure radio?
                        PN53_DEBUG("poll.r/w+tag", "[try %" PRIuSIZE "/%" PRIuSIZE "] PLA\n",
                                   try, loop->timing.retries);
                        if ((res = _broadcast_custom_polling_frame(nfcdev, loop->tag, frame_ix, interval + 10)) < 0) {
                            return res;
                        }
                    }
                }
            }
        }
    }
    return 0;
}

ssize_t nfcdev_poll_pn53(nfcdev_t* nfcdev, const nfcdev_polling_config_t* config, nfc_target_t* targets, nfcdev_connection_id_t* connection_ids, size_t max_targets) {
    pn53_dev_t* dev = nfcdev->dev;
    ssize_t res = 0;
    size_t current_count = 0;
    // First, check if we can even do what were supposed to do, so we don't look like
    // a goldfish in front of a blowdryer when we see an unsupported polling loop.
    // If we are supposed to error out, we need to check those before we actually do anything.
    for (size_t i = 0; i < config->loop_count; i += 1) {
        PN53_DEBUG("poll", "[loop %" PRIuSIZE "/%" PRIuSIZE "] checking\n", i+1, config->loop_count);
        if ((res = _check_polling_loop(&config->loops[i])) < 0) {
            return res;
        }
    }

    // trylapswraps around as intended when config->repetitions is NFCDEV_POLLING_REPETIONS_INFINITE
    // aka. SIZE_MAX.
    for (size_t lap = 0; lap <= config->repetitions; lap += 1) {
        PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE"] %" PRIuSIZE " tech loops\n",
                   lap+1, config->repetitions+1, config->loop_count);

        for (size_t i = 0; i < config->loop_count; i += 1) {
            nfcdev_polling_loop_t* loop = &config->loops[i];
            assert(loop->higher_layer.nfc_dep.atr_length == 0 ||
                   loop->higher_layer.nfc_dep.atr_length >= sizeof(loop->higher_layer.nfc_dep.atr));

            PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE "] ["
                       "loop %" PRIuSIZE "/%" PRIuSIZE "] checking\n",
                       lap+1, config->repetitions+1, i+1, config->loop_count);
            if ((res = _check_polling_loop(loop)) < 0) {
                if (res == -ENOTSUP && IS_ACTIVE(CONFIG_NFCDEV_SKIP_UNSUPPORTED_POLLING_LOOPS)) {
                    PN53_DEBUG("poll", "[loop %" PRIuSIZE "/%" PRIuSIZE "] skipping\n", i+1, config->loop_count);
                    continue;
                }
                return res;
            }

            PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE "] ["
                       "loop %" PRIuSIZE "/%" PRIuSIZE "] "
                       "guard=%" PRIu32 "ms interval=%" PRIu32 "ms retries=",
                       lap+1, config->repetitions+1, i+1, config->loop_count,
                       loop->timing.guard_time,
                       loop->timing.interval == NFCDEV_POLLING_INTERVAL_BUILTIN
                       ? 5 : loop->timing.interval);
            if (loop->timing.retries == NFCDEV_POLLING_RETRIES_INFINITE) {
                DEBUG("inf\n");
            } else {
                DEBUG("%" PRIuSIZE "\n", loop->timing.retries);
            }

            if (i > 0 && loop->timing.guard_time > 0) {
                PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE "] ["
                           "loop %" PRIuSIZE "/%" PRIuSIZE "] waiting guard time of %" PRIu32 "\n",
                           lap+1, config->repetitions+1, i+1, config->loop_count,
                           loop->timing.guard_time);
                ztimer_sleep(ZTIMER_MSEC, loop->timing.guard_time);
            }

            nfc_dep_activation_request_t* atr = loop->higher_layer.nfc_dep.atr;
            nfc_dep_activation_response_t* atr_res = NULL;

            switch (loop->field_mode) {
                case NFC_FIELD_MODE_READER_WRITER_TAG:
                    if ((res = nfcdev_poll_pn53_in_loop_in_list_rw_tag(nfcdev, loop, max_targets)) < 0) {
                        return res;
                    }

                    if (res > 0 && loop->higher_layer.nfc_dep.atr_length > 0) {
                        ssize_t _res = 0;
                        for (nfcdev_connection_id_t i = 0; i < ARRAY_SIZE(dev->nfc_targets); i += 1) {
                            pn53_logical_target_t* target = pn53_target(dev, i);
                            if (!target) {
                                continue;
                            }
                            PN53_DEBUG("poll", "NFC-DEP to be actived with in r/w+tag field mode"
                                       " on Tg=%u\n", i+1);

                            if ((res = pn53_in_atr(dev, i+1, atr ? &atr->id : NULL,
                                atr ? atr->general_bytes : NULL,
                                loop->higher_layer.nfc_dep.atr_length == 0 ? 0 :
                                    loop->higher_layer.nfc_dep.atr_length - sizeof(nfc_dep_activation_request_t),
                                &atr_res
                            )) != 0) {
                                return res;
                            }
                        }
                    }
                    break;
                case NFC_FIELD_MODE_PEERS:
                    PN53_DEBUG("poll", "NFC-DEP to be activated in peer field mode\n");
                    if ((res = pn53_in_jump_for_dep(dev, NFC_FIELD_MODE_PEERS, loop->bitrate,
                        (pn53_nfc_dep_arg_t) {},
                        atr ? &atr->id : NULL, atr ? atr->general_bytes : NULL,
                        loop->higher_layer.nfc_dep.atr_length == 0 ? 0 :
                            loop->higher_layer.nfc_dep.atr_length - sizeof(nfc_dep_activation_request_t),
                        &atr_res
                    )) < 0) {
                        return res;
                    }
                    break;
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }

            if (res > 0) {
                PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE "] ["
                    "loop %" PRIuSIZE "/%" PRIuSIZE "] acquired %" PRIuSIZE " targets\n",
                    lap+1, config->repetitions+1, i+1, config->loop_count, (size_t)res);
                assert((size_t)res <= max_targets);

                nfc_bidirectional_bitrate_selector_t* selector = &loop->higher_layer.bitrate_selector;

                if (selector->upstream.strategy != NFC_BITRATE_CHOOSE_CURRENT ||
                    selector->downstream.strategy != NFC_BITRATE_CHOOSE_CURRENT) {
                    ssize_t _res = 0;
                    for (nfcdev_connection_id_t i = 0; i < ARRAY_SIZE(dev->nfc_targets); i += 1) {
                        pn53_logical_target_t* target = pn53_target(dev, i);
                        if (!target) {
                            continue;
                        }

                        target->super.parameters.downstream.bitrate =
                        target->super.parameters.upstream.bitrate =
                        target->super.parameters.polling.bitrate;

                        if (target->managed_transport == PN53_MANAGED_TRANSPORT_ISO_DEP &&
                            target->super.tag.technology == NFC_TECHNOLOGY_A &&
                            target->super.tag.a.ats.length >= 2 && target->super.tag.a.ats.ta_present) {
                            PN53_DEBUG("poll", "PPS on NFC-A ISO-DEP Tg=%u\n", i+1);
                            // S(Parameters) not supported
                            nfc_bitrate_select_bidirectional(selector,
                                &target->super.parameters.downstream.bitrate,
                                &target->super.parameters.upstream.bitrate,
                                ((nfc_bitrate_set_t)target->super.tag.a.ats.ta_info.bitrates.down << 1) | 1,
                                ((nfc_bitrate_set_t)target->super.tag.a.ats.ta_info.bitrates.up << 1) | 1,
                                target->super.tag.a.ats.ta_info.bitrates.same_constraint);
                        }
                        else if (target->managed_transport == PN53_MANAGED_TRANSPORT_NFC_DEP) {
                            PN53_DEBUG("poll", "PSL_REQ NFC-DEP ISO-DEP Tg=%u\n", i+1);
                            // PN53 supports only 106--424K in NFC-DEP mode, so
                            // no need to read BRt and BSt from ATR_RES
                            nfc_bitrate_select_bidirectional(selector,
                                &target->super.parameters.downstream.bitrate,
                                &target->super.parameters.upstream.bitrate,
                                NFC_BITRATE_106K | NFC_BITRATE_212K | NFC_BITRATE_424K,
                                NFC_BITRATE_106K | NFC_BITRATE_212K | NFC_BITRATE_424K,
                                false);
                        }
                        else {
                            continue;
                        }

                        if ((res = pn53_in_psl(dev, i,
                            target->super.parameters.downstream.bitrate,
                            target->super.parameters.upstream.bitrate
                        )) != 0) {
                            return res;
                        }
                    }
                }

                uint8_t found = 0;
                for (nfcdev_connection_id_t i = 0; i < ARRAY_SIZE(dev->nfc_targets); i += 1) {
                    pn53_logical_target_t* target = pn53_target(dev, i);
                    if (!target) {
                        continue;
                    }
                    PN53_DEBUG("poll", "target %u has connection id %u (HCI Tg=%u)\n", found, i, i+1);
                    targets[found] = target->super;
                    if (connection_ids) {
                        connection_ids[found] = i;
                    }
                }
                assert(found == (uint8_t)res);
                return (ssize_t)found;
            }
//            // TODO: I think we need to stop polling immediately, and cannot acquire more targets...
//            PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE "] ["
//                       "loop %" PRIuSIZE "/%" PRIuSIZE "] acquired %" PRIuSIZE " targets\n",
//                       lap+1, config->repetitions+1, i+1, config->loop_count, (size_t)res);
//
//            assert((size_t)res <= max_targets);
//            current_count += (size_t)res;
//            max_targets -= (size_t)res;
//
//            if (current_count > max_targets) {
//                PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE "] ["
//                           "loop %" PRIuSIZE "/%" PRIuSIZE "] acquired total max of %" PRIuSIZE " targets, early exit\n",
//                           lap+1, config->repetitions+1, i+1, config->loop_count, current_count);
//                return current_count;
//            }
        }
    }
//    PN53_DEBUG("poll", "acquired %" PRIuSIZE " targets total, late exit\n", current_count);
//    return current_count;
    return 0;
}
