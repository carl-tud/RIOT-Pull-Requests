#include "pn532.h"
#include "net/nfcdev.h"
#include "net/nfc/llcp.h"

#include <stdio.h>

static nfc_role_t role = NFC_ROLE_TARGET;

static void printbuff(uint8_t* buff, size_t length) {
    while (length) {
        length--;
        printf("%02x ", *buff++);
    }
    puts("");
}

int main(void) {
    const pn532_connection_config_t config = {
        .bus = {
            .kind = PN53_BUS_SPI,
            .spi = SPI_DEV(0),
        },
#if IS_USED(MODULE_PN532_SPI)
            .chip_select = GPIO_PIN(1, 8),
#endif
            .reset = GPIO_PIN(0, 7),
            .irq = GPIO_PIN(0, 26),
    };
    static pn532_dev_t pn532 = {};
    static nfcdev_t dev = {};

    ssize_t res = 0;

    if ((res = nfcdev_init_pn532(&dev, &pn532, &config)) < 0) {
        printf("init: error %" PRIdSIZE " \n", res);
        return (int)res;
    }

    puts("device initiliazed");

    static nfcdev_tag_polling_config_t polling_config_a = {
        .technology = NFC_TECHNOLOGY_A
    };


    static nfcdev_tag_polling_config_t polling_config_f = {
        .technology = NFC_TECHNOLOGY_F,
    };

    static const nfc_dep_activation_request_t atr_req = {
        .id = { 1,2,3,4,5,6,7,8,9,10 },
        .device_id = 0x42,
        .general_bytes_available = true,
        .payload_reduction = NFC_DEP_PAYLOAD_LIMIT_252,
        .general_bytes = {
            NFC_DEP_GENERAL_BYTE0_LLCP,
            NFC_DEP_GENERAL_BYTE1_LLCP,
            NFC_DEP_GENERAL_BYTE2_LLCP,

            LLCP_PARAMETER_TLV_VERSION,
                0x01,
                0x10,
        }
    };

    static nfc_dep_target_t nfc_dep_target =  {
        .length = sizeof(nfc_dep_activation_response_t) + 6,
        .atr = {
            .id = { 1,2,3,4,5,6,7,8,9,10 },
            .device_id = 0x42,
            .general_bytes_available = true,
        },
        .general = {
            NFC_DEP_GENERAL_BYTE0_LLCP,
            NFC_DEP_GENERAL_BYTE1_LLCP,
            NFC_DEP_GENERAL_BYTE2_LLCP,

            LLCP_PARAMETER_TLV_VERSION,
                0x01,
                0x10,
        }
    };

    static nfcdev_polling_loop_t polling_loops[] = {
        {
            .bitrate = NFC_BITRATE_106K,
            .timing = {
                .interval = NFCDEV_POLLING_INTERVAL_BUILTIN /* ms */,
                .retries = 5,
                .guard_time = 10,
            },
            .tag = &polling_config_a,
            .higher_layer.nfc_dep = {
                .length = sizeof(nfc_dep_activation_request_t) + 6,
                .atr = &atr_req
            },
            .higher_layer.bitrate_selector = NFC_SELECT_FASTEST_UP_TO(NFC_BITRATE_848K),
            .field_mode = NFC_FIELD_MODE_READER_WRITER_TAG,
        },
        {
            .bitrate = NFC_BITRATE_212K,
            .timing = {
                .interval = NFCDEV_POLLING_INTERVAL_BUILTIN /* ms */,
                .retries = 5,
                .guard_time = 10,
            },
            .tag = &polling_config_f,
            .higher_layer.nfc_dep = {
                .length = sizeof(nfc_dep_activation_request_t) + 6,
                .atr = &atr_req
            },
            .higher_layer.bitrate_selector = NFC_SELECT_FASTEST_UP_TO(NFC_BITRATE_848K),
            .field_mode = NFC_FIELD_MODE_READER_WRITER_TAG,
        },
        {
            .bitrate = NFC_BITRATE_212K,
            .timing = {
                .interval = NFCDEV_POLLING_INTERVAL_BUILTIN /* ms */,
                .retries = 5,
                .guard_time = 10,
            },
            .higher_layer.nfc_dep = {
                .length = sizeof(nfc_dep_activation_request_t),
                .atr = &atr_req
            },
            .higher_layer.bitrate_selector = NFC_SELECT_FASTEST_UP_TO(NFC_BITRATE_848K),
            .field_mode = NFC_FIELD_MODE_PEERS,
        },
    };

    static nfcdev_polling_config_t polling_config = {
        .loops = polling_loops,
        .loop_count = ARRAY_SIZE(polling_loops),
        .repetitions = NFCDEV_POLLING_REPETIONS_INFINITE
    };

    static nfc_a_tag_t tag_a = {
        .id = {
            .length = 4,
            .uid = { 0x80, 0x42, 0x00, 0x66 }
        },
        .polling_response = {
            .uid_size_indicator = 0,
            .bit_frame_anticollision = 4,
        },
        .select_response = NFC_A_SELECT_RESPONSE_FLAG_NFC_DEP,
    };
    tag_a.ats = pn532_builtin_ats;

    static nfc_f_tag_t tag_f = {
        .id.raw = { 1,0xFE,3,4,5,6,7,8 },
        .pmm.raw = {},
        .system_code = NFC_F_SYSTEM_CODE_NDEF
    };

    static nfcdev_listening_config_t listening_config = {
        .tag = {
            .technologies = NFC_TECHNOLOGY_A | NFC_TECHNOLOGY_F,
            .a = &tag_a,
            .f = &tag_f
        },
        .higher_layer.nfc_dep = &nfc_dep_target,
        .bitrates = {
            .tag.a = NFC_BITRATE_106K,
            .tag.f = NFC_BITRATE_212K | NFC_BITRATE_424K,
            .peer = NFC_BITRATE_106K | NFC_BITRATE_212K | NFC_BITRATE_424K,
        }
    };

    nfc_target_t target;

    puts("");

    uint8_t* general_bytes = NULL;
    size_t general_length = 0;

    switch (role) {
        case NFC_ROLE_INITIATOR: {
            puts("Polling");

            if ((res = nfcdev_poll(&dev, &polling_config, &target, NULL, 1)) < 0) {
                printf("poll: error %" PRIdSIZE " \n", res);
                return res;
            }
            if (target.higher_layer.nfc_dep.length < sizeof(nfc_dep_activation_response_t)) {
                puts("poll: ATR_RES missing");
                return -1;
            }
            general_bytes = target.higher_layer.nfc_dep.atr.general_bytes;
            general_length = nfc_dep_atr_response_general_length(&target.higher_layer.nfc_dep);
            break;
        }
        case NFC_ROLE_TARGET: {
            puts("Listening");

            if ((res = nfcdev_listen(&dev, &listening_config, &target, PN53_TIMEOUT_NEVER)) < 0) {
                printf("listen: error %" PRIdSIZE " \n", res);
                return res;
            }
            nfc_dep_activation_request_t* atr = NULL;
            if ((res = pn53_listen_get_atr_request(&pn532, &atr)) < 0 || (size_t)res < sizeof(nfc_dep_activation_request_t)) {
                puts("listen: ATR_REQ missing");
                return -1;
            }

            general_bytes = atr->general_bytes;
            general_length = _nfc_dep_atr_request_general_length((size_t)res);
            break;
        }
        default:
            puts("role invalid");
            return -1;
    }

    puts("");
    printf("target: ");
    nfc_print_target(&target);
    printf("\n");

    static uint8_t prefix[] = {
        NFC_DEP_GENERAL_BYTE0_LLCP,
        NFC_DEP_GENERAL_BYTE1_LLCP,
        NFC_DEP_GENERAL_BYTE2_LLCP,
    };
    if (general_length < 3 || memcmp(general_bytes, prefix, sizeof(prefix)) != 0) {
        puts("missing Ffm general bytes prefix");
        return -1;
    }

    puts("general bytes:");
    printbuff(general_bytes, general_length);
    puts("");

    static nfc_llcp_controller_t controller = {};
    if (nfc_llcp_controller_init(&controller, &dev, general_bytes + sizeof(prefix), general_length - sizeof(prefix)) < 0) {
        puts("failed to init controller");
        return -1;
    }

    puts("LLCP running");

    static nfc_llcp_socket_sync_t socket = {
        .super = {
            .ssap = 1,
            .dsap = 0,
            .service_name = "urn:nfc:sn:snep"
        }
    };
    if (nfc_llcp_controller_add_socket_sync(&controller, &socket, LLCP_SOCKET_MODE_ACCEPTING) < 0) {
        puts("failed to add LLCP socket");
        return -1;
    }

    puts("LLCP socket registered, waiting for message");

    static uint8_t buffer[200];
    if ((res = nfc_llcp_socket_receive_sync(&socket, buffer, sizeof(buffer), 10 * 1000)) < 0) {
        printf("failed to receive from LLCP peer: %" PRIiSIZE "\n", res);
        return -1;
    }

    puts("got over LLCP:");
    printbuff(buffer, (size_t)res);
}
