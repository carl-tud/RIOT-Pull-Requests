/*
 * Copyright (C) 2016 TriaGnoSys GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Test application for the PN532 NFC reader
 *
 * @author      Víctor Ariño <victor.arino@triagnosys.com>
 *
 * @}
 */

#include "board.h"
#include "macros/utils.h"
#include "net/nfc.h"
#include "pn532.h"
#include "pn532_params.h"
#include "ztimer.h"
#include "architecture.h"

#define LOG_LEVEL LOG_INFO
#include "log.h"

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
    pn532_dev_t pn532 = {};
    nfcdev_t dev = {};

    ssize_t res = 0;

    if ((res = nfcdev_init_pn532(&dev, &pn532, &config)) < 0) {
        printf("init: error %" PRIdSIZE " \n", res);
        return (int)res;
    }

    puts("");
    puts("Polling");

    static nfc_a_rats_payload_t rats = {

    };

    static nfcdev_tag_polling_config_t polling_config_a = {
        .technology = NFC_TECHNOLOGY_A,
        .a = {
            .frames = NULL,
            .frame_count = 0,
            .id = NULL,
            .filter = NULL,
            .rats = &rats,
        }
    };



    static nfcdev_tag_polling_config_t polling_config_b = {
        .technology = NFC_TECHNOLOGY_B,
        .b = {
            .frames = NULL,
            .frame_count = 0,
            .filter = NULL,
            .attrib_length = 0,
            .attrib = NULL
        }
    };

    static nfcdev_polling_loop_t polling_loops[] = {
        {
            .bitrate = NFC_BITRATE_106K,
            .timing = {
                .interval = NFCDEV_POLLING_INTERVAL_BUILTIN /* ms */,
                .retries = 256,
                .guard_time = 0,
            },
            .tag = &polling_config_a,
            .field_mode = NFC_FIELD_MODE_READER_WRITER_TAG
        },
        {
            .bitrate = NFC_BITRATE_106K,
            .timing = {
                .interval = NFCDEV_POLLING_INTERVAL_BUILTIN /* ms */,
                .retries = 4,
                .guard_time = 0,
            },
            .tag = &polling_config_b,
            .field_mode = NFC_FIELD_MODE_READER_WRITER_TAG,
            .higher_layer.bitrate_selector = NFC_SELECT_FASTEST_UP_TO(NFC_BITRATE_848K)
        }
    };

    static nfcdev_polling_config_t polling_config = {
        .loops = polling_loops,
        .loop_count = ARRAY_SIZE(polling_loops),
        .repetitions = 3
    };

    nfc_target_t targets[2];
    if ((res = nfcdev_poll(&dev, &polling_config, targets, NULL, ARRAY_SIZE(targets))) < 0) {
        printf("poll: error %" PRIdSIZE " \n", res);
    }
    printf("poll: targets: %" PRIdSIZE " \n", res);
    if (res > 0) {
        for (size_t i = 0; i < (size_t)res; i += 1) {
            printf("found ");
            nfc_print_target(&targets[i]);
            printf("\n");
        }
    }


    return 0;
    puts("");

    if ((res = pn53_set_field_enablement(&pn532, true, false)) < 0) {
        printf("field on: error %" PRIdSIZE " \n", res);
    }

    nfcdev_radio_config_t radio_config = {
        .technology = NFC_TECHNOLOGY_A,
        .bitrate = NFC_BITRATE_106K,
        .generate_field = true
    };

    puts("");
    puts("Configuring radio");

    if ((res = nfcdev_configure_radio(&dev, &radio_config, &radio_config, NFC_ROLE_INITIATOR)) < 0) {
        printf("radio config: error %" PRIdSIZE " \n", res);
    }

    puts("");

    if ((res = pn53_set_field_enablement(&pn532, false, false)) < 0) {
        printf("field off: error %" PRIdSIZE " \n", res);
    }

    puts("");

    if ((res = pn53_set_field_enablement(&pn532, true, false)) < 0) {
        printf("field on: error %" PRIdSIZE " \n", res);
    }

    puts("");
    puts("Transmitting and receiving frame");

    uint8_t frame[] = {0x6a, 0x01, 0x00, 0x00, 0x00};
    uint8_t* frame_rx = NULL;

    if ((res = nfcdev_transceive(&dev,
        nfc_a_polling_frame_all.frame, nfc_a_polling_frame_all.length,
        &frame_rx, 0, 5,
        NFCDEV_INTERFACE_FRAME
    )) < 0) {
        printf("fifo tx: error %" PRIdSIZE " \n", res);
    }

    puts("");

    if ((res = pn53_set_field_enablement(&pn532, true, false)) < 0) {
        printf("field on: error %" PRIdSIZE " \n", res);
    }

    puts("");

    if ((res = nfcdev_send(&dev, frame, sizeof(frame), NFCDEV_INTERFACE_PACKET)) < 0) {
        printf("fifo tx: error %" PRIdSIZE " \n", res);
    }

//    printf("fifo rx: ");
//    printbuff(frame_rx, (size_t)res);

//    pn532_set_parameters(&dev, PN532_NFC_PARAMETER_INITIATOR_ISO_DEP_AUTO_HANDSHAKE);

    puts("");
    puts("Polling NFC-A ...");

    if ((res = pn53_in_list_passive_targets_a(&pn532, ARRAY_SIZE(targets),
                                           NULL, targets, 2 * MS_PER_SEC)
    ) < 0) {
        printf("NFC-A: error %" PRIdSIZE " \n", res);
    } else {
        for (size_t i = 0; i < (size_t)res; i += 1) {
            nfc_a_tag_t* tag = &targets[i].tag.a;
            printf("NFC-A: uid has length=%" PRIuSIZE " iso_dep=%i nfc_dep=%i\n",
                   nfc_a_id_length(tag->polling_response.uid_size_indicator),
                   nfc_a_supports_iso_dep(tag->select_response),
                   nfc_a_supports_nfc_dep(tag->select_response)
                   );

            printf("NFC-A: uid=");
            printbuff(tag->id.uid, (size_t)tag->id.length);

            if (tag->ats.length > 0) {
                printf("NFC-A: ats=");
                printbuff((uint8_t*)&tag->ats,
                          (size_t)tag->ats.length);
            }
        }
    }

    pn53_register_address_t regs[] = {
        PN53_REGISTER_TX_AUTO,
        PN53_REGISTER_TX_MODE,
        PN53_REGISTER_RX_MODE,
    };

    uint8_t* regvals;
    if ((res = pn53_read_registers(&pn532, regs, &regvals, ARRAY_SIZE(regs))) < 0) {
        return (int)res;
    }

    puts("");

    if ((res = nfcdev_configure_radio(&dev, &radio_config, &radio_config, NFC_ROLE_INITIATOR)) < 0) {
        printf("radio config: error %" PRIdSIZE " \n", res);
    }

    puts("");

    if ((res = pn53_set_field_enablement(&pn532, true, false)) < 0) {
        printf("field on: error %" PRIdSIZE " \n", res);
    }

    puts("");

    if ((res = nfcdev_send(&dev,
        nfc_a_polling_frame_all.frame, nfc_a_polling_frame_all.length,
        NFCDEV_INTERFACE_FRAME
    )) < 0) {
        printf("fifo tx: error %" PRIdSIZE " \n", res);
        return (int)res;
    }

    puts("");

    uint8_t buf[10];
    if ((res = nfcdev_receive(&dev,
        buf, sizeof(buf), 1 * MS_PER_SEC,
        NFCDEV_INTERFACE_FRAME
    )) < 0) {
        printf("fifo rx: error %" PRIdSIZE " \n", res);
    }

    if (res > 0) {
        printbuff(buf, (size_t)res);
    }

    puts("");

    if ((res = nfcdev_send(&dev, frame, sizeof(frame), NFCDEV_INTERFACE_PACKET)) < 0) {
        printf("fifo tx: error %" PRIdSIZE " \n", res);
        return (int)res;
    }

    puts("");
    puts("Polling NFC-B ...");

    if ((res = pn53_in_list_passive_targets_b(&pn532, ARRAY_SIZE(targets),
                                           NFC_BITRATE_106K, 0x00, NFC_POLLING_METHOD_PROBABILISTIC,
                                              targets, 2 * MS_PER_SEC)
    ) < 0) {
        printf("NFC-B: error %" PRIdSIZE " \n", res);
    } else {
        for (size_t i = 0; i < (size_t)res; i += 1) {
            nfc_b_tag_t* tag = &targets[i].tag.b;
            printf("NFC-B: id=");
            printbuff((uint8_t*)tag->polling_response.id, sizeof(tag->polling_response.id));
        }
    }

    if ((res = pn53_read_registers(&pn532, regs, &regvals, ARRAY_SIZE(regs))) < 0) {
        return (int)res;
    }

    puts("");
    puts("Polling NFC-F ...");

    nfc_f_polling_command_payload_t polf = {
        .system_code = 0xffff,
        .additional_request = NFC_F_POLLING_REQUEST_SYSTEM_CODE,
        .timeslots = 0x0f
    };
    if ((res = pn53_in_list_passive_targets_f(&pn532, ARRAY_SIZE(targets),
                                           NFC_BITRATE_212K, &polf, targets, 2 * MS_PER_SEC)
    ) < 0) {
        printf("NFC-F: error %" PRIdSIZE " \n", res);
    } else {
        for (size_t i = 0; i < (size_t)res; i += 1) {
            nfc_f_tag_t* tag = &targets[i].tag.f;
            printf("NFC-F: idm=");
            printbuff(tag->id.raw, sizeof(tag->id));
            printf("NFC-F: pmm=");
            printbuff(tag->pmm.raw, sizeof(tag->pmm));
            printf("NFC-F: system=%04x\n", tag->system_code);
        }
    }

    if ((res = pn53_read_registers(&pn532, regs, &regvals, ARRAY_SIZE(regs))) < 0) {
        return (int)res;
    }


//    static char data[16];
//    static nfc_iso14443a_t card;
//    static pn532_t pn532;
//    unsigned len;
//    int ret;
//
//    pn53_bus_kind_t mode = IS_ACTIVE(MODULE_PN53X_I2C) ? PN53_BUS_I2C : PN53_BUS_SPI;
//    ret = pn532_init(&pn532, &pn532_conf[0], mode);
//
//    if (ret != 0) {
//        LOG_INFO("init error %d\n", ret);
//        return -1;
//    }
//
//    ztimer_sleep(ZTIMER_MSEC, 200);
//    LOG_INFO("awake\n");
//
//    uint32_t fwver;
//    ret = pn532_fw_version(&pn532, &fwver);
//    if (ret != 0) {
//        LOG_INFO("ver error %d\n", ret);
//        return -1;
//    }
//    LOG_INFO("ver %d.%d\n", (unsigned)PN532_FW_VERSION(fwver), (unsigned)PN532_FW_REVISION(fwver));
//
//    ret = pn532_sam_configuration(&pn532, PN532_SAM_NORMAL, 1000);
//    if (ret != 0) {
//        LOG_INFO("set sam error %d\n", ret);
//        return -1;
//    }
//
//    while (1) {
//        /* Delay not to be always polling the interface */
//        ztimer_sleep(ZTIMER_MSEC, 250);
//
//        ret = pn532_get_passive_iso14443a(&pn532, &card, 0x50);
//        if (ret < 0) {
//            LOG_DEBUG("no card\n");
//            continue;
//        }
//
//        if (card.type == ISO14443A_TYPE4) {
//            if (pn532_iso14443a_4_activate(&pn532, &card) != 0) {
//                LOG_ERROR("act error\n");
//                continue;
//
//            }
//            else if (pn532_iso14443a_4_read(&pn532, data, &card, 0x00, 2) != 0) {
//                LOG_ERROR("len error\n");
//                continue;
//            }
//
//            len = PN532_ISO14443A_4_LEN_FROM_BUFFER(data);
//            len = MIN(len, sizeof(data));
//
//            if (pn532_iso14443a_4_read(&pn532, data, &card, 0x02, len) != 0) {
//                LOG_ERROR("read error\n");
//                continue;
//            }
//
//            LOG_INFO("dumping card contents (%d bytes)\n", len);
//            printbuff(data, len);
//            pn532_release_passive(&pn532, card.target);
//
//        }
//        else if (card.type == ISO14443A_MIFARE) {
//            char key[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
//            char data[32];
//
//            for (int i = 0; i < 64; i++) {
//                LOG_INFO("sector %02d, block %02d | ", i / 4, i);
//                if ((i & 0x03) == 0) {
//                    ret = pn532_mifareclassic_authenticate(&pn532, &card,
//                                                           PN532_MIFARE_KEY_A, key, i);
//                    if (ret != 0) {
//                        LOG_ERROR("auth error\n");
//                        break;
//                    }
//                }
//
//                ret = pn532_mifareclassic_read(&pn532, data, &card, i);
//                if (ret == 0) {
//                    printbuff(data, 16);
//                }
//                else {
//                    LOG_ERROR("read error\n");
//                    break;
//                }
//            }
//
//        }
//        else {
//            LOG_ERROR("unknown card type\n");
//        }
//    }

    return 0;
}
