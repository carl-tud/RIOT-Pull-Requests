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
            tag->attrib.higher_layer = attrib_length > sizeof(nfc_b_attrib_response_t)
                ? response + sizeof(nfc_b_attrib_response_t) : NULL;

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

static int _fix_attrib(nfcdev_t* nfcdev, const nfcdev_polling_loop_t* loop, nfc_target_t* targets, ssize_t res, uint8_t afi, uint32_t timeout_ms) {
    if (loop->tag->b.attrib && loop->tag->b.attrib_length >= sizeof(nfc_b_attrib_command_payload_t)) {
        PN53_DEBUG("poll", "overwriting built-in ATTRIB\n");

        nfc_b_attrib_command_payload_t builtin_attrib = {
            .raw = { 0x00, 0x05, 0x00, 0x01 }
        };
        nfc_b_tag_t* tag = &targets[(size_t)res].tag.b;
        bool iso_dep_activated = tag->polling_response.iso_dep_supported;
        builtin_attrib.iso_dep_supported = iso_dep_activated;

        // Found a target, check if ATTRIB sent by PN53x was unfortunate
        if (memcmp(builtin_attrib.raw, loop->tag->b.attrib->raw, sizeof(nfc_b_attrib_command_payload_t)) != 0) {
            PN53_DEBUG("poll", "ATTRIB varies from PN53-builtin\n");
            if (iso_dep_activated) {
                uint8_t s_deselect[] = {
                    ISO_DEP_PCB_MASK_BLOCK_TYPE_VALUE_S
                        | ISO_DEP_PCB_S_BLOCK_MASK_KIND_VALUE_DESELECT,
                    tag->attrib.cid
                };

                if ((res = nfcdev_transceive(nfcdev,
                     &s_deselect, targets[(size_t)res].tag.b.attrib.cid == 0 ? 1 : 2,
                     NULL, 0, timeout_ms, NFCDEV_INTERFACE_PACKET
                )) < 0) {
                    return res;
                }

                nfc_b_polling_command_t wupb = {
                    .code = NFC_B_FRAME_CODE_POLLING,
                    .payload = {
                        .application_family = afi
                    }
                };
                // default PN53 REQB payload is 00 00, so we use exactly that payload, but set
                // the wake up bit, so that the REQB turns into a WUPB.
                wupb.payload.wake_up = true;

                if ((res = nfcdev_transceive(nfcdev,
                     &wupb, sizeof(wupb), NULL, 0, timeout_ms, NFCDEV_INTERFACE_PACKET
                )) < 0) {
                    return res;
                }

                nfc_b_attrib_command_t command = (nfc_b_attrib_command_t) {
                    .code = NFC_B_FRAME_CODE_ATTRIB,
                };
                memcpy(&command.identifier, tag->polling_response.id, sizeof(nfc_b_id_t));
                memcpy(&command.payload, loop->tag->b.attrib, sizeof(nfc_b_attrib_command_payload_t));
                iolist_t _higher_layer = {
                    .iol_base = loop->tag->b.attrib->higher_layer,
                    .iol_len = loop->tag->b.attrib_length - sizeof(nfc_b_attrib_command_payload_t)
                };
                iolist_t _command = {
                    .iol_base = command.raw,
                    .iol_len = sizeof(command),
                    .iol_next = &_higher_layer
                };
                nfc_b_attrib_response_t* attrib_response = NULL;
                if ((res = nfcdev_transceive_chunks(nfcdev,
                    &_command, &attrib_response, 0, timeout_ms, NFCDEV_INTERFACE_PACKET
                )) < 0) {
                    return res;
                }

                tag->attrib = *attrib_response;
                tag->attrib.higher_layer = (size_t)res > sizeof(nfc_b_attrib_response_t)
                    ? (uint8_t*)attrib_response + sizeof(nfc_b_attrib_response_t) : NULL;
            }
        }
    }
    return 0;
}

static ssize_t nfcdev_poll_pn53_in_list(nfcdev_t* nfcdev, const nfcdev_polling_loop_t* loop, ssize_t frame_ix, nfc_target_t* targets, uint8_t max_targets, uint32_t timeout_ms) {
    pn53_dev_t* dev = nfcdev->dev;
    assert(loop->tag);
    ssize_t res = 0;
    switch (loop->tag->technology) {
        case NFC_TECHNOLOGY_A:
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
                 higher_layer_bitrate, afi, loop->tag->b.method, targets, timeout_ms);
            if (res < 0) {
                return res;
            }
            if (IS_ACTIVE(CONFIG_PN53_NFCDEV_OVERWRITE_ATTRIB) && res > 0) {
                assert(res <= max_targets);
                ssize_t _res = _fix_attrib(nfcdev, loop, targets, res, afi, timeout_ms);
                if (_res < 0) {
                    return _res;
                }
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
                targets, timeout_ms);
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
    for (uint8_t i = 0; i < (uint8_t)res; i += 1) {
        if (nfcdev_polling_filter_matches(loop->tag, &dev->nfc_targets[i].super.tag)) {
            targets[found] = dev->nfc_targets[i].super;
            found += 1;
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
    size_t frame_ix, nfc_target_t* targets, size_t max_targets, uint32_t timeout_ms
) {
    ssize_t res = 0;
    uint32_t delta = ztimer_now(ZTIMER_MSEC);
    if ((res = nfcdev_poll_pn53_in_list(nfcdev, loop, frame_ix,
         targets, max_targets, timeout_ms
    )) < 0) {
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

ssize_t nfcdev_poll_pn53_in_loop_in_list_rw_tag(nfcdev_t* nfcdev, const nfcdev_polling_loop_t* loop, nfc_target_t* targets, size_t max_targets) {
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

        if ((res = nfcdev_poll_pn53_in_list(nfcdev, loop, default_frame_ix, targets, max_targets, timeout_ms)) < 0) {
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
                if ((res = _poll_with_builtin_frame(nfcdev, loop, default_frame_ix, targets, max_targets, interval + 10)) != 0) {
                    return res;
                }
            } else {
                for (ssize_t frame_ix = 0; (size_t)frame_ix <= frame_count; frame_ix += 1) {
                    if (_is_builtin_polling_frame(loop->tag, frame_ix)) {
                        PN53_DEBUG("poll", "[try %" PRIuSIZE "/%" PRIuSIZE "] REQA using InList\n",
                                try, loop->timing.retries);
                        if ((res = _poll_with_builtin_frame(nfcdev, loop, frame_ix, targets, max_targets, interval + 10)) != 0) {
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


ssize_t nfcdev_poll_pn53(nfcdev_t* nfcdev, const nfcdev_polling_config_t* config, nfc_target_t* targets, size_t max_targets) {
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

            switch (loop->field_mode) {
                case NFC_FIELD_MODE_READER_WRITER_TAG:
                    if ((res = nfcdev_poll_pn53_in_loop_in_list_rw_tag(nfcdev, loop, targets, max_targets)) < 0) {
                        return res;
                    }
                case NFC_FIELD_MODE_PEERS:
                    break;
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }
            // TODO: I think we need to stop polling immediately, and cannot acquire more targets...
            PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE "] ["
                       "loop %" PRIuSIZE "/%" PRIuSIZE "] acquired %" PRIuSIZE " targets\n",
                       lap+1, config->repetitions+1, i+1, config->loop_count, (size_t)res);

            assert((size_t)res <= max_targets);
            current_count += (size_t)res;
            max_targets -= (size_t)res;

            if (current_count > max_targets) {
                PN53_DEBUG("poll", "[lap %" PRIuSIZE "/%" PRIuSIZE "] ["
                           "loop %" PRIuSIZE "/%" PRIuSIZE "] acquired total max of %" PRIuSIZE " targets, early exit\n",
                           lap+1, config->repetitions+1, i+1, config->loop_count, current_count);
                return current_count;
            }
        }
    }
    PN53_DEBUG("poll", "acquired %" PRIuSIZE " targets total, late exit\n", current_count);
    return current_count;
}
