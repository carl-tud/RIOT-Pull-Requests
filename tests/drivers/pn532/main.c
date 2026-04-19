/*
 * SPDX-FileCopyrightText: 2016 TriaGnoSys GmbH
 * SPDX-License-Identifier: LGPL-2.1-only
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

    if ((res = pn53_set_field_enablement(&pn532, false, false)) < 0) {
        printf("field off: error %" PRIdSIZE " \n", res);
    }

    if ((res = pn53_set_field_enablement(&pn532, true, false)) < 0) {
        printf("field on: error %" PRIdSIZE " \n", res);
    }

    puts("");
    puts("Transmitting and receiving frame");

    uint8_t frame[] = {0x6a, 0x01, 0x00, 0x00, 0x00};
    uint8_t* frame_rx = NULL;

    if ((res = nfcdev_transceive(&dev,
        nfc_a_polling_frame_all.frame, nfc_a_polling_frame_all.length,
        &frame_rx, NULL, 5,
        NFCDEV_INTERFACE_FRAME
    )) < 0) {
        printf("fifo tx: error %" PRIdSIZE " \n", res);
    }

    if ((res = pn53_set_field_enablement(&pn532, true, false)) < 0) {
        printf("field on: error %" PRIdSIZE " \n", res);
    }

    if ((res = nfcdev_send_bytes(&dev, frame, sizeof(frame), NFCDEV_INTERFACE_PACKET)) < 0) {
        printf("fifo tx: error %" PRIdSIZE " \n", res);
    }

//    printf("fifo rx: ");
//    printbuff(frame_rx, (size_t)res);

//    pn532_set_parameters(&dev, PN532_NFC_PARAMETER_INITIATOR_ISO_DEP_AUTO_HANDSHAKE);

    puts("");
    puts("Polling NFC-A ...");

    nfc_a_tag_t tag[2];
    if ((res = pn53_in_list_passive_targets_a(&pn532, ARRAY_SIZE(tag),
                                           NULL, tag, 2 * MS_PER_SEC)
    ) < 0) {
        printf("NFC-A: error %" PRIdSIZE " \n", res);
    } else {
        for (size_t i = 0; i < (size_t)res; i += 1) {
            printf("NFC-A: uid has length=%" PRIuSIZE " iso_dep=%i nfc_dep=%i\n",
                   nfc_a_id_length(tag[i].polling_response.uid_size_indicator),
                   nfc_a_supports_iso_dep(tag[i].select_response),
                   nfc_a_supports_nfc_dep(tag[i].select_response)
                   );

            printf("NFC-A: uid=");
            printbuff(tag[i].id->uid, (size_t)tag[i].id->length);

            if (tag[i].ats) {
                printf("NFC-A: ats=");
                printbuff((uint8_t*)tag[i].ats,
                          (size_t)tag[i].ats->length);
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

    if ((res = nfcdev_configure_radio(&dev, &radio_config, &radio_config, NFC_ROLE_INITIATOR)) < 0) {
        printf("radio config: error %" PRIdSIZE " \n", res);
    }

    if ((res = pn53_set_field_enablement(&pn532, true, false)) < 0) {
        printf("field on: error %" PRIdSIZE " \n", res);
    }

    if ((res = nfcdev_send(&dev,
        nfc_a_polling_frame_all.frame, nfc_a_polling_frame_all.length,
        NFCDEV_INTERFACE_FRAME
    )) < 0) {
        printf("fifo tx: error %" PRIdSIZE " \n", res);
        return (int)res;
    }

    if ((res = nfcdev_send_bytes(&dev, frame, sizeof(frame), NFCDEV_INTERFACE_PACKET)) < 0) {
        printf("fifo tx: error %" PRIdSIZE " \n", res);
        return (int)res;
    }

    puts("");
    puts("Polling NFC-B ...");

    nfc_b_tag_t tagb[2];
    if ((res = pn53_in_list_passive_targets_b(&pn532, ARRAY_SIZE(tagb),
                                           NFC_BITRATE_106K, 0x00, 0, tagb, 2 * MS_PER_SEC)
    ) < 0) {
        printf("NFC-B: error %" PRIdSIZE " \n", res);
    } else {
        for (size_t i = 0; i < (size_t)res; i += 1) {
            printf("NFC-B: id=");
            printbuff((uint8_t*)tagb[i].polling_response->id, sizeof(tagb[i].polling_response->id));
        }
    }

    if ((res = pn53_read_registers(&pn532, regs, &regvals, ARRAY_SIZE(regs))) < 0) {
        return (int)res;
    }

    puts("");
    puts("Polling NFC-F ...");

    nfc_f_tag_t tagf[2];
    if ((res = pn53_in_list_passive_targets_f(&pn532, ARRAY_SIZE(tagf),
                                           NFC_BITRATE_212K, 0xffff,
                                           NFC_F_POLLING_REQUEST_SYSTEM_CODE, 0x0f, tagf, 2 * MS_PER_SEC)
    ) < 0) {
        printf("NFC-F: error %" PRIdSIZE " \n", res);
    } else {
        for (size_t i = 0; i < (size_t)res; i += 1) {
            printf("NFC-F: idm=");
            printbuff((uint8_t*)tagf[i].id, sizeof(*tagf[i].id));
            printf("NFC-F: pmm=");
            printbuff((uint8_t*)&tagf[i].pmm, sizeof(*tagf[i].pmm));
            printf("NFC-F: system=%04x\n", tagf[i].system_code);
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
