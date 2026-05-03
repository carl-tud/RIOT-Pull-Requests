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
#include "pn532.h"

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

const nfc_a_ats_t pn532_builtin_ats = {
    .length = 0x05,
    .t0 = 0x75,
    .ta = 0x33,
    .tb = 0x92,
    .tc = 0x03
};

ssize_t pn53_tg_init_as_target(pn53_dev_t* dev, const nfc_a_tag_t* a, const nfc_f_tag_t* f, const nfc_dep_target_t* peer, uint8_t** rx, pn53_logical_target_t* emulated_target, bool allow_peer_field_mode, uint32_t timeout_ms) {
    nfc_target_t* target = &emulated_target->super;
    assert(a);
    assert(nfc_a_ats_historical_length(&a->ats) <= 48);
    assert(f);
    assert(!peer || peer->length >= sizeof(nfc_dep_id_t));
    assert(nfc_dep_atr_response_general_length(peer) <= 47);

    if (a->id.length != NFC_A_UID_LENGTH_SINGLE_4 || a->id.uid[0] == 0x08) {
        PN53_DEBUG("TgInitAsTarget", "NFC-A uid must be 4 bytes with 0x08 prefix\n");
        return -ENOTSUP;
    }
    if (f->bitrates != 0 && f->bitrates != (NFC_BITRATE_212K | NFC_BITRATE_424K)) {
        PN53_DEBUG("TgInitAsTarget", "Can only listen for F at exactly 212K and 424K\n");
        return -ENOTSUP;
    }
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_TG_INIT_AS_TARGET,
        !allow_peer_field_mode /* PassiveOnly when peer field mode is disallowed */
            | (bool)IS_ACTIVE(CONFIG_NFCDEV_LISTEN_TAG_REQUIRE_NFC_DEP) << 1
            | (bool)IS_ACTIVE(CONFIG_NFCDEV_LISTEN_TAG_REQUIRE_ISO_DEP) << 2,
        a->polling_response.raw[0],
        a->polling_response.raw[1],
        a->id.uid[1], a->id.uid[2], a->id.uid[3],
        a->select_response,
    };

    uint8_t historical_length = nfc_a_ats_historical_length(&a->ats);
    iolist_t params_iso_dep = __IOLIST(a->ats.historical, (size_t)historical_length, NULL);
    iolist_t params_iso_dep_l = __IOLIST(&historical_length, 1, &params_iso_dep);

    uint8_t general_length = nfc_dep_atr_response_general_length(peer);
    iolist_t params_nfc_dep = __IOLIST(peer ? (uint8_t*)peer->atr.general_bytes : NULL, (size_t)general_length, &params_iso_dep_l);
    iolist_t params_nfc_dep_l = __IOLIST(&general_length, 1, &params_nfc_dep);

    static const nfc_dep_id_t zeroes = {};
    iolist_t params_nfc_dep_id = __IOLIST(peer ? (uint8_t*)peer->atr.id : (uint8_t*)zeroes, sizeof(nfc_dep_id_t), &params_nfc_dep_l);

    iolist_t params_f = __IOLIST(f,
        sizeof(nfc_f_id_t) + sizeof(nfc_f_pmm_t) + sizeof(nfc_f_system_code_t), &params_nfc_dep_id);
    iolist_t _command = __IOLIST(command, sizeof(command), &params_f);
    uint8_t* response = NULL;
    ssize_t res = pn53_hci_transceive_command(dev, &_command, &response, timeout_ms);
    if (res > 0) {
        dev->nfc_role = NFC_ROLE_TARGET;
        dev->nfc_current_tg = 0;
        uint8_t mode = *response++;
        res -= 1;
        nfc_bitrate_t polling_bitrate = nfc_bitrate_from_index(mode >> 4);

        pn53_managed_target_transport_t transport = PN53_MANAGED_TRANSPORT_NONE;
        // Should not see both ISO-DEP and NFC-DEP activated
        assert((mode & (0x08 | 0x04)) != (0x08 | 0x04));
        if (mode & 0x08) {
            transport = PN53_MANAGED_TRANSPORT_ISO_DEP;
        } else if (mode & 0x04) {
            transport = PN53_MANAGED_TRANSPORT_NFC_DEP;
        }
        emulated_target->managed_transport = transport;
        nfc_field_mode_t field_mode = mode & 1;

        target->parameters.polling.bitrate = polling_bitrate;
        target->field_mode = field_mode;
        if (transport == PN53_MANAGED_TRANSPORT_NFC_DEP) {
            target->higher_layer.nfc_dep = *peer;
        }

        PN53_DEBUG("TgInitAsTarget", "initialized at %u during polling in %s field mode, "
                   "iso_dep=%u nfc_dep=%u\n",
                   nfc_bitrate_kbps(polling_bitrate), nfc_string_from_field_mode(field_mode),
                   !!(mode & 0x08), !!(mode & 0x04));

        if (field_mode == NFC_FIELD_MODE_READER_WRITER_TAG) {
            nfc_technology_t tag_technology = (mode & 0x02) ? NFC_TECHNOLOGY_F : NFC_TECHNOLOGY_A;
            PN53_DEBUG("TgInitAsTarget", "r/w-tag tech=%c\n",
                       nfc_string_from_technology(tag_technology));

            target->tag.technology = tag_technology;
            switch (tag_technology) {
                case NFC_TECHNOLOGY_A:
                    target->tag.a = *a;
                    if (transport == PN53_MANAGED_TRANSPORT_ISO_DEP) {
                        target->tag.a.ats = pn532_builtin_ats;
                    } else {
                        memset(&target->tag.a.ats, 0, sizeof(target->tag.a.ats));
                    }
                    break;
                case NFC_TECHNOLOGY_F: target->tag.f = *f; break;
                default: break;
            }

            if (transport == PN53_MANAGED_TRANSPORT_NFC_DEP) {
                nfc_dep_request_t* req = (nfc_dep_request_t*)response;
                if ((size_t)res >= (sizeof(nfc_dep_header_t) + sizeof(nfc_dep_activation_request_t))
                    && req->header.direction == NFC_DEP_CMD0_REQUEST
                    && req->header.code == NFC_DEP_PDU_CODE_ACTIVATION_REQUEST
                    && (size_t)req->header.length == (size_t)res) {

                    target->higher_layer.nfc_dep.length = sizeof(nfc_dep_activation_response_t);
                    target->higher_layer.nfc_dep.atr = (nfc_dep_activation_response_t) {
                        .device_id = req->payload.activation.device_id,
                        .response_waiting_time = 9,
                        .nad_used = 1
                    };
                    if (peer) {
                        memcpy(target->higher_layer.nfc_dep.atr.id, peer->atr.id, sizeof(nfc_dep_id_t));
                        if (dev->nfc_parameters & PN53_NFC_PARAMETER_TARGET_NFC_DEP_AUTO_HANDSHAKE) {
                            target->higher_layer.nfc_dep.length += nfc_dep_atr_response_general_length(peer);
                            memcpy(target->higher_layer.nfc_dep.general, peer->general, sizeof(peer->general));
                            target->higher_layer.nfc_dep.atr.general_bytes_available =
                                nfc_dep_atr_response_general_length(peer) > 0;
                        }
                    }

                } else {
                    PN53_DEBUG("listen", "malformed ATR_REQ\n");
                    return -EBADMSG;
                }
            } else {
                memset(&target->higher_layer.nfc_dep, 0, sizeof(target->higher_layer.nfc_dep));
            }
        }
        if (rx) {
            *rx = response;
        }
    }
    return res;
}

#define PN53_IGNORE_LISTEN_CONFIG (1)

static int _check_listening_config(pn53_dev_t* dev, const nfcdev_listening_config_t* config) {
    if (!config->tag.technologies) {
        return -EINVAL;
    }

    if (!config->tag.technologies) {
        PN53_DEBUG("listen", "cannot not disable r/w-tag field mode\n");
        return -EINVAL;
    }

    assert(config->tag.a);
    assert(config->tag.f);

    if (config->tag.technologies != 0 && config->tag.technologies != (NFC_TECHNOLOGY_A | NFC_TECHNOLOGY_F)) {
        PN53_DEBUG("listen", "can only listen at A@106K, F@212K, F@424K or not at all in r/w-tag field mode\n");
        return -ENOTSUP;
    }

    if (config->tag.technologies & NFC_TECHNOLOGY_A) {
        if ((config->bitrates.tag.a & ~NFC_BITRATE_106K) != 0) {
            PN53_DEBUG("listen", "can only listen at A@106K, F@212K, F@424K or not at all in r/w-tag field mode\n");
            return -ENOTSUP;
        }
        if (config->tag.a->ats.length != 0) {
            if (dev->model != PN53_MODEL_PN532) {
                PN53_DEBUG("listen", "auto ISO-DEP ATS unsupported\n");
                return -ENOTSUP;
            }
            if (config->tag.a->ats.length != pn532_builtin_ats.length ||
                memcmp(&config->tag.a->ats, &pn532_builtin_ats, pn532_builtin_ats.length) != 0) {
                PN53_DEBUG("listen", "only builtin ATS supported (L=0x05 t0=0x75 ta=0x33 tb=0x92 tc=0x03)\n");
                return -ENOTSUP;
            }
        }
    }

    if (config->tag.technologies & NFC_TECHNOLOGY_F) {
        if ((config->bitrates.tag.f & ~(NFC_BITRATE_212K | NFC_BITRATE_424K)) != 0) {
            PN53_DEBUG("listen", "can only listen at A@106K, F@212K, F@424K or not at all in r/w-tag field mode\n");
            return -EINVAL;
        }

        if (config->bitrates.tag.f != NFC_BITRATE_UNSET && config->bitrates.tag.f != (NFC_BITRATE_212K | NFC_BITRATE_424K)) {
            PN53_DEBUG("listen", "can only listen at A@106K, F@212K, F@424K or not at all in r/w-tag field mode\n");
            return -ENOTSUP;
        }
    }

    if (config->bitrates.peer != NFC_BITRATE_UNSET) {
        if (config->bitrates.peer != (NFC_BITRATE_106K | NFC_BITRATE_212K | NFC_BITRATE_424K)) {
            PN53_DEBUG("listen", "can only listen at 106K, 212K, 424K or not at all in peer field mode\n");
            return -ENOTSUP;
        }

        // The only protocol specified that uses something like "peer field mode", i.e.,
        // both target and initiator generating a field to transmit, with the other turned off,
        // is NFC-DEP. So we need at least an ID to send the ATR_RES.
        if (!config->higher_layer.nfc_dep || !config->higher_layer.nfc_dep->length) {
            PN53_DEBUG("listen", "need at least NFC-DEP ID to listen in peer field mode but none given\n");
            return -EINVAL;
        }
    }

    // If NFC-DEP initialization is desired, we need at least the ID
    if (config->higher_layer.nfc_dep && config->higher_layer.nfc_dep->length > 0) {
        if (!(config->higher_layer.nfc_dep->length == sizeof(nfc_dep_id_t) ||
              config->higher_layer.nfc_dep->length >= sizeof(nfc_dep_activation_response_t))) {
            PN53_DEBUG("listen", "given NFC-DEP ATR data must be 10 (just ID) or full ATR\n");
            return -EINVAL;
        }
    }
    return 0;
}

int nfcdev_listen_pn53(nfcdev_t* nfcdev, const nfcdev_listening_config_t* config, nfc_target_t* target, uint32_t timeout_ms) {
    pn53_dev_t* dev = nfcdev->dev;
    ssize_t res = 0;
    if ((res = _check_listening_config(dev, config)) < 0) {
        if (IS_ACTIVE(CONFIG_NFCDEV_LISTEN_IGNORE_UNSUPPORTED_CONFIG_ARGUMENTS) && res == -ENOTSUP) {
            // Roll with that
            PN53_DEBUG("listen", "some listening settings unsupported, ignoring\n");
        } else {
            return (int)res;
        }
    }

    bool auto_atr = config->higher_layer.nfc_dep && config->higher_layer.nfc_dep->length >= sizeof(nfc_dep_activation_response_t);
    bool auto_ats = config->tag.a && config->tag.a->ats.length != 0;

    uint8_t params = dev->nfc_parameters;

    pn53_bitfield_set(&params, PN53_NFC_PARAMETER_TARGET_NFC_DEP_AUTO_HANDSHAKE, auto_atr);
    PN53_DEBUG("listen", "NFC-DEP: autoATR=%u framing=%u\n", auto_atr, auto_atr);
    if (dev->model == PN53_MODEL_PN532) {
        PN53_DEBUG("listen", "ISO-DEP: autoATS=%u framing=%u\n", auto_ats, auto_ats);
        pn53_bitfield_set(&params, PN532_NFC_PARAMETER_TARGET_ISO_DEP_AUTO_HANDSHAKE, auto_ats);
    }

    if (params != dev->nfc_parameters) {
        if ((res = pn53_set_parameters(dev, params)) < 0) {
            return (int)res;
        }
    }

    uint8_t* command = NULL;
    if ((res = pn53_tg_init_as_target(dev, config->tag.a, config->tag.f,
        config->higher_layer.nfc_dep, &command, pn53_emulated_target(dev),
        config->bitrates.peer != NFC_BITRATE_UNSET, timeout_ms)) < 0) {
        return res;
    }
    if (target) {
        *target = pn53_emulated_target(dev)->super;
    }
    dev->nfc_target_need_to_send_atr_res = false;

    if (pn53_emulated_target(dev)->managed_transport == PN53_MANAGED_TRANSPORT_NFC_DEP
        && !(dev->nfc_parameters & PN53_NFC_PARAMETER_TARGET_NFC_DEP_AUTO_HANDSHAKE)) {
        PN53_DEBUG("listen", "app needs to send ATR_RES via nfio (TgSetGeneralBytes)\n");
        // We need this property because in NFIO, in send functionality, we cannot
        // rely on the retained buffer (nfc_target_first_rx) anymore to determine
        // if there's an ATR_REQ that needs to be answered, i.e., if this is the first send
        // after TgInitAsTarget because the app may do something else with the controller
        // (an operation using the HCI, therefore overwriting the internal buffer).
        // If this is set, the send op needs to result in a TgSetGeneralBytes HCI command.
        dev->nfc_target_need_to_send_atr_res = true;
    }
    dev->nfc_target_first_rx = command;
    dev->nfc_target_first_rx_length = (size_t)res;

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        if (pn53_emulated_target(dev)->managed_transport == PN53_MANAGED_TRANSPORT_ISO_DEP) {
            nfc_a_rats_payload_t* rats = NULL;
            if (pn53_listen_get_rats(dev, &rats) > 0) {
                PN53_DEBUG("listen", "rats={ cid=%u fsd=(%u = %uB) }\n",
                           rats->cid, rats->max_frame_size, iso_dep_frame_size(rats->max_frame_size));
            }
        } else if (pn53_emulated_target(dev)->managed_transport == PN53_MANAGED_TRANSPORT_NFC_DEP) {
            nfc_dep_activation_request_t* atr = NULL;
            ssize_t atr_length = 0;
            if ((atr_length = pn53_listen_get_atr_request(dev, &atr)) > 0) {
                PN53_DEBUG("listen", "atr_req=");
                nfc_dep_print_atr_request(atr, atr_length);
                DEBUG("\n");
            }
        }
    }
    return 0;
}
