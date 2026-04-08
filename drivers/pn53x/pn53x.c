/*
 * Copyright (C) 2016 TriaGnoSys GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

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

#define PN53_HCI_DEBUG(...) DEBUG("pn53x.hci: " __VA_ARGS__)
#define PN53_DEBUG(...) DEBUG("pn53x: " __VA_ARGS__)



#define PN53_BUS_I2C_ADDRESS           (0x24)

/* SPI bus parameters */
#define SPI_MODE                    (SPI_MODE_0)
#define SPI_CLK                     (SPI_CLK_1MHZ)

#define UART_BAUDRATE               (115200U)


ssize_t pn53_hci_transceive_command(pn53_connection_t* connection, iolist_t* command,
                            uint8_t** response, uint32_t timeout_ms) {
    assert(command);
    uint8_t code = *(uint8_t*)command->iol_base;
    ssize_t res = 0;
    if ((res = pn53_hci_transceive(connection, command, response, timeout_ms)) < 0) {
        return res;
    }

    if (res == 0) {
        return -PN53_ERROR_CONNECTION_RESPONSE_MISSING;
    }

    if (response && *response && **response != (code + 1)) {
        return -PN53_ERROR_CONNECTION_RESPONSE_MISMATCH;
    }
    *response += 1;
    return res - 1;
}

static inline ssize_t _hci_execute(pn53_connection_t* connection, uint8_t* command, size_t length,
                            uint8_t** response, uint32_t timeout_ms) {
    iolist_t iolist = { .iol_base = (void*)command, .iol_len = length };
    return pn53_hci_transceive_command(connection, &iolist, response, timeout_ms);
}

int pn53_check_communication(pn53_dev_t* dev) {
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_DIAGNOSE,
        0x00, /* Echo test */
        0x42, 'R', 'I', 'O', 'T', 0x42,
    };
    uint8_t* response;
    ssize_t res = _hci_execute(&dev->connection, command, sizeof(command),
                                &response, CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS);
    if (res < 0) {
        return (int)res;
    }

    PN53_DEBUG("echo: ");
    PN53_DEBUG_HEX(response, (size_t)res);
    DEBUG("\n");

    if (res != sizeof(command) - 1) {
        PN53_DEBUG("comms check failed (echo)\n");
        return -1;
    }
    if (memcmp(&command[1], response, sizeof(command) - 1) != 0) {
        PN53_DEBUG("comms check failed (echo)\n");
        return -1;
    }

    PN53_DEBUG("comms check successful (echo)\n");
    return 0;
}

typedef struct __attribute((packed)) {
    bool nfc_a : 1;
    bool nfc_b : 1;
    bool nfc_dep : 1;
    uint8_t _padding : 5;
} _protocol_support_t;

typedef struct __attribute((packed)) {
    uint8_t ic;
    uint8_t version;
    uint8_t revision;
    _protocol_support_t support;
} _firmware_version_t;

int pn53_init(pn53_dev_t* dev, const pn53_connection_config_t* config) {
    assert(dev);
    assert(config);
    int res = 0;

    dev->connection.config = config;
    if ((res = pn53_hci_init(&dev->connection)) < 0) {
        return res;
    }
    PN53_DEBUG("HCI initialized\n");

    pn53_hci_reset(&dev->connection);
    PN53_DEBUG("reset done\n");

    /* Start with conservative minimum before knowing model */
    dev->connection.max_packet_length = 200;
    if ((res = (int)pn53_sam_configuration(dev, PN53_SAM_NORMAL, 0xA0, true)) < 0) {
        PN53_DEBUG("SAMConfiguration failed with %i\n", res);
    }
    if (IS_ACTIVE(CONFIG_PN53_SELFCHECK)) {
        if ((res = pn53_check_communication(dev)) < 0) {
            PN53_DEBUG("communication test failed\n");
            return res;
        }
    }

    uint8_t command = (uint8_t)PN53_COMMAND_GET_FIRMWARE_VERSION;
    uint8_t* response;

    if ((res = _hci_execute(&dev->connection, &command, sizeof(command), &response, CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS)) < 0) {
        PN53_DEBUG("unable to get firmware version\n");
        return res;
    }

    _firmware_version_t* fw = (_firmware_version_t*)response;
    if (IS_ACTIVE(ENABLE_DEBUG)) {
        PN53_DEBUG("connected to <pn53x ic=0x%02X version=%i revision=%i nfc={a=%i b=%i dep=%i}>\n",
                   fw->ic, (int)fw->version, (int)fw->revision,
                   fw->support.nfc_a, fw->support.nfc_b, fw->support.nfc_dep);
    }


//    uint8_t cfg1[] = {0x00, 0x0B, 0x0A};
//    int ret = _rf_configure(dev, 0x02, cfg1, sizeof(cfg1));
//    if (ret != 0) {
//        LOG_ERROR("pn532: rf configuration 1 failed with %d\n", ret);
//        return ret;
//    }
//    uint8_t cfg2[] = {0x00};
//    ret = _rf_configure(dev, 0x04, cfg2, sizeof(cfg2));
//    if (ret != 0) {
//        LOG_ERROR("pn532: rf configuration 2 failed with %d\n", ret);
//        return ret;
//    }
//    uint8_t cfg3[] = {0x01, 0x00, 0x01};
//    ret = _rf_configure(dev, 0x05, cfg3, sizeof(cfg3));
//    if (ret != 0) {
//        LOG_ERROR("pn532: rf configuration 3 failed with %d\n", ret);
//        return ret;
//    }
//
//    LOG_DEBUG("pn532: setting parameters to 0\n");
//    pn532_set_parameters(dev, 0b00000000);
//
//    LOG_DEBUG("pn532: init complete\n");

    return 0;
}

//int pn532_init(nfcdev_t *nfcdev, const void *dev_config) {
//    pn532_t *dev = (pn532_t *)nfcdev->dev;
//    const pn532_config_t *config = (const pn532_config_t *)dev_config;
//    nfcdev->ops = &pn532_ops;
//    int ret = _pn532_init(dev, &config->params, config->mode);
//    if (ret != 0) {
//        return -1;
//    }
//    nfcdev->state = NFCDEV_STATE_DISABLED;
//    return 0;
//}


//int pn532_fw_version(pn532_t *dev, uint32_t *fw_ver)
//{
//    unsigned ret = -1;
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START] = CMD_FIRMWARE_VERSION;
//
//    if (send_rcv(dev, buff, 0, 4, STANDARD_TIMEOUT_SEC) == 4) {
//        *fw_ver =  ((uint32_t)buff[0] << 24);   /* ic version */
//        *fw_ver += ((uint32_t)buff[1] << 16);   /* fw ver */
//        *fw_ver += ((uint32_t)buff[2] << 8);    /* fw rev */
//        *fw_ver += (buff[3]);                   /* feature support */
//        ret = 0;
//    }
//
//    return ret;
//}
//
//int pn532_read_reg(pn532_t *dev, uint8_t *out, unsigned addr)
//{
//    int ret = -1;
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START     ] = CMD_READ_REG;
//    buff[BUFF_DATA_START    ] = (addr >> 8) & 0xff;
//    buff[BUFF_DATA_START + 1] = addr & 0xff;
//
//    if (send_rcv(dev, buff, 2, 1, STANDARD_TIMEOUT_SEC) == 1) {
//        *out = buff[8];
//        ret = 0;
//    }
//
//    return ret;
//}
//
//int pn532_write_reg(pn532_t *dev, unsigned addr, uint8_t val)
//{
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START     ] = CMD_WRITE_REG;
//    buff[BUFF_DATA_START    ] = (addr >> 8) & 0xff;
//    buff[BUFF_DATA_START + 1] = addr & 0xff;
//    buff[BUFF_DATA_START + 2] = val;
//
//    return send_rcv(dev, buff, 3, 0, STANDARD_TIMEOUT_SEC);
//}
//
//int pn532_update_reg(pn532_t *dev, unsigned addr, uint8_t val, uint8_t mask)
//{
//    uint8_t reg_val;
//    int ret = pn532_read_reg(dev, &reg_val, addr);
//    if (ret < 0) {
//        return ret;
//    }
//
//    reg_val &= ~mask;
//    reg_val |= (val & mask);
//
//    return pn532_write_reg(dev, addr, reg_val);
//}
//
//static int _rf_configure(pn532_t *dev, unsigned cfg_item, const uint8_t *config,
//                         unsigned cfg_len)
//{
//    LOG_DEBUG("pn532: rf config %02x\n", cfg_item);
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START ] = CMD_RF_CONFIG;
//    buff[BUFF_DATA_START] = cfg_item;
//    for (unsigned i = 1; i <= cfg_len; i++) {
//        buff[BUFF_DATA_START + i] = *config++;
//    }
//
//    return send_rcv(dev, buff, cfg_len + 1, 0, STANDARD_TIMEOUT_SEC);
//}
//
//int _set_act_retries(pn532_t *dev, unsigned max_retries)
//{
//    uint8_t rtrcfg[] = { 0xff, 0x01, max_retries & 0xff };
//
//    return _rf_configure(dev, RF_CONFIG_MAX_RETRIES, rtrcfg, sizeof(rtrcfg));
//}
//
ssize_t pn53_sam_configuration(pn53_dev_t* dev, pn53_sam_mode_t mode, uint8_t timeout, bool use_irq) {
    uint8_t command[] = {
        (uint8_t)PN53_COMMAND_SAM_CONFIGURATION,
        (uint8_t)mode,
        timeout,
        (uint8_t)use_irq
    };
    return _hci_execute(&dev->connection, command, sizeof(command), NULL, CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS);
}
//
//static int _list_passive_targets(pn532_t *dev, uint8_t *buff, pn532_target_t target,
//                                 unsigned max, unsigned recvl)
//{
//    buff[BUFF_CMD_START]      = CMD_LIST_PASSIVE;
//    buff[BUFF_DATA_START]     = (uint8_t) max;
//    buff[BUFF_DATA_START + 1] = (uint8_t)target;
//
//    /* requested len depends on expected target num and type */
//    return send_rcv(dev, buff, 2, recvl, STANDARD_TIMEOUT_SEC);
//}
//
//// int pn532_get_passive_iso14443a(pn532_t *dev, nfc_iso14443a_t *out,
//                                 unsigned max_retries)
// {
//     int ret = -1;
//     uint8_t buff[CONFIG_PN532_BUFFER_LEN];

//     if (_set_act_retries(dev, max_retries) == 0) {
//         ret = _list_passive_targets(dev, buff, PN532_BR_106_ISO_14443_A, 1,
//                                     LIST_PASSIVE_LEN_14443(1));
//     }

//     if (ret > 0 && buff[0] > 0) {
//         out->target = buff[1];
//         out->sns_res = (buff[2] << 8) | buff[3];
//         out->sel_res = buff[4];
//         out->id_len  = buff[5];
//         out->type = ISO14443A_UNKNOWN;

//         for (int i = 0; i < out->id_len; i++) {
//             out->id[i] = buff[6 + i];
//         }

//         /* try to find out the type */
//         if (out->id_len == 4) {
//             out->type = ISO14443A_MIFARE;
//         }
//         else if (out->id_len == 7) {
//             /* In the case of type 4, the first byte of RATS is the length
//              * of RATS including the length itself (6+7) */
//             if (buff[13] == ret - 13) {
//                 out->type = ISO14443A_TYPE4;
//             }
//         }
//         ret = 0;
//     }
//     else {
//         ret = -1;
//     }

//     return ret;
// }

//void pn532_deselect_passive(pn532_t *dev)
//{
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START ] = CMD_DESELECT;
//    buff[BUFF_DATA_START] = 0x00; /* all targets */
//
//    send_rcv(dev, buff, 1, 1, STANDARD_TIMEOUT_SEC);
//}
//
//void pn532_release_passive(pn532_t *dev)
//{
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START ] = CMD_RELEASE;
//    buff[BUFF_DATA_START] = 0x00;
//
//    send_rcv(dev, buff, 1, 1, STANDARD_TIMEOUT_SEC);
//}
//
//// int pn532_mifareclassic_authenticate(pn532_t *dev, nfc_iso14443a_t *card,
////                                      pn532_mifare_key_t keyid, uint8_t *key, unsigned block)
//// {
////     int ret = -1;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////     buff[BUFF_DATA_START    ] = card->target;
////     buff[BUFF_DATA_START + 1] = keyid;
////     buff[BUFF_DATA_START + 2] = block; /* current block */
//
////     /*
////      * The card ID directly follows the key in the buffer
////      * The key consists of 6 bytes and starts at offset 3
////      */
////     for (int i = 0; i < 6; i++) {
////         buff[BUFF_DATA_START + 3 + i] = key[i];
////     }
//
////     for (int i = 0; i < card->id_len; i++) {
////         buff[BUFF_DATA_START + 9 + i] = card->id[i];
////     }
//
////     ret = send_rcv(dev, buff, 9 + card->id_len, 1, STANDARD_TIMEOUT_SEC);
////     if (ret == 1) {
////         ret = buff[0];
////         card->auth = 1;
////     }
//
////     return ret;
//// }
//
//// int pn532_mifareclassic_write(pn532_t *dev, uint8_t *idata, nfc_iso14443a_t *card,
////                               unsigned block)
//// {
////     int ret = -1;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     if (card->auth) {
//
////         buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////         buff[BUFF_DATA_START    ] = card->target;
////         buff[BUFF_DATA_START + 1] = MIFARE_CMD_WRITE;
////         buff[BUFF_DATA_START + 2] = block; /* current block */
////         memcpy(&buff[BUFF_DATA_START + 3], idata, MIFARE_CLASSIC_BLOCK_SIZE);
//
////         if (send_rcv(dev, buff, 19, 1, STANDARD_TIMEOUT_SEC) == 1) {
////             ret = buff[0];
////         }
//
////     }
////     return ret;
//// }
//
//// static int pn532_mifare_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
////                              unsigned block, unsigned len)
//// {
////     int ret = -1;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////     buff[BUFF_DATA_START    ] = card->target;
////     buff[BUFF_DATA_START + 1] = MIFARE_CMD_READ;
////     buff[BUFF_DATA_START + 2] = block; /* current block */
//
////     if (send_rcv(dev, buff, 3, len + 1, STANDARD_TIMEOUT_SEC) == (int)(len + 1)) {
////         memcpy(odata, &buff[1], len);
////         ret = 0;
////     }
//
////     return ret;
//// }
//
//// int pn532_mifareclassic_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
////                              unsigned block)
//// {
////     if (card->auth) {
////         return pn532_mifare_read(dev, odata, card, block, MIFARE_CLASSIC_BLOCK_SIZE);
////     }
////     else {
////         return -1;
////     }
//// }
//
//// int pn532_mifareulight_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
////                             unsigned page)
//// {
////     return pn532_mifare_read(dev, odata, card, page, 32);
//// }
//
//// static int send_rcv_apdu(pn532_t *dev, uint8_t *buff, unsigned slen, unsigned rlen)
//// {
////     int ret;
//
////     rlen += 3;
////     if ((rlen >= RAPDU_MAX_DATA_LEN) || (slen >= CAPDU_MAX_DATA_LEN)) {
////         return -1;
////     }
//
////     ret = send_rcv(dev, buff, slen, rlen);
////     if ((ret == (int)rlen) && (buff[0] == 0x00)) {
////         ret = (buff[rlen - 2] << 8) | buff[rlen - 1];
////         if (ret == (int)0x9000) {
////             ret = 0;
////         }
////     }
//
////     return ret;
//// }
//
//// int pn532_iso14443a_4_activate(pn532_t *dev, nfc_iso14443a_t *card)
//// {
////     int ret;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     /* select app ndef tag */
////     buff[BUFF_CMD_START      ] = CMD_DATA_EXCHANGE;
////     buff[BUFF_DATA_START     ] = card->target;
////     buff[BUFF_DATA_START +  1] = 0x00;
////     buff[BUFF_DATA_START +  2] = 0xa4;
////     buff[BUFF_DATA_START +  3] = 0x04;
////     buff[BUFF_DATA_START +  4] = 0x00;
////     buff[BUFF_DATA_START +  5] = 0x07;
////     buff[BUFF_DATA_START +  6] = 0xD2;
////     buff[BUFF_DATA_START +  7] = 0x76;
////     buff[BUFF_DATA_START +  8] = 0x00;
////     buff[BUFF_DATA_START +  9] = 0x00;
////     buff[BUFF_DATA_START + 10] = 0x85;
////     buff[BUFF_DATA_START + 11] = 0x01;
////     buff[BUFF_DATA_START + 12] = 0x01;
////     buff[BUFF_DATA_START + 13] = 0x00;
//
////     LOG_DEBUG("pn532: select app\n");
////     ret = send_rcv_apdu(dev, buff, 14, 0);
//
////     /* select ndef file */
////     buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////     buff[BUFF_DATA_START    ] = card->target;
////     buff[BUFF_DATA_START + 1] = 0x00;
////     buff[BUFF_DATA_START + 2] = 0xa4;
////     buff[BUFF_DATA_START + 3] = 0x00;
////     buff[BUFF_DATA_START + 4] = 0x0c;
////     buff[BUFF_DATA_START + 5] = 0x02;
////     buff[BUFF_DATA_START + 6] = 0x00;
////     buff[BUFF_DATA_START + 7] = 0x01;
//
////     if (ret == 0) {
////         LOG_DEBUG("pn532: select file\n");
////         ret = send_rcv_apdu(dev, buff, 8, 0);
////     }
//
////     return ret;
//// }
//
//// int pn532_iso14443a_4_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
////                            unsigned offset, uint8_t len)
//// {
////     int ret;
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
////     buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
////     buff[BUFF_DATA_START    ] = card->target;
////     buff[BUFF_DATA_START + 1] = 0x00;
////     buff[BUFF_DATA_START + 2] = 0xb0;
////     buff[BUFF_DATA_START + 3] = (offset >> 8) & 0xff;
////     buff[BUFF_DATA_START + 4] = offset & 0xff;
////     buff[BUFF_DATA_START + 5] = len;
//
////     ret = send_rcv_apdu(dev, buff, 6, len);
////     if (ret == 0) {
////         memcpy(odata, &buff[RAPDU_DATA_BEGIN], len);
////     }
//
////     return ret;
//// }
//
//int change_rf_field(pn532_t *dev, bool on) {
//    uint8_t command[] = {RF_CONFIGURATION, 0x01, (on) ? 0x01 : 0x00};
//    return send_rcv(dev, command, 2, 0, STANDARD_TIMEOUT_SEC);
//}
//
//static void _load_fifo_data(pn532_t *dev, const uint8_t *data, unsigned len) {
//    /* clear FIFO data */
//    pn532_write_reg(dev, PN532_REG_CIU_FIFOLevel, 0x80);
//
//    for (unsigned i = 0; i < len; i++) {
//        pn532_write_reg(dev, PN532_REG_CIU_FIFOData, data[i]);
//    }
//}
//
//static void _read_fifo_data(pn532_t *dev, uint8_t *data, unsigned len) {
//    for (unsigned i = 0; i < len; i++) {
//        pn532_read_reg(dev, &(data[i]), PN532_REG_CIU_FIFOData);
//    }
//}
//
//
//int _init_as_target_nfc_f(pn532_t *dev, uint8_t *nfc_f_params) {
//    pn532_write_reg(dev, PN532_REG_CIU_Command, 0x00);
//
//    /* set NFC F rf config */
//    _rf_configure(dev, 0x0B, nfc_f_rf_config, 8);
//
//    /* load the NFC F params into the FIFO buffer */
//    uint8_t params_buffer[25] = {0};
//    memcpy(params_buffer + 2 + 3 + 1, nfc_f_params, 18);
//
//
//    pn532_write_reg(dev, PN532_REG_CIU_Command, 0x00); /* idle command */
//    pn532_write_reg(dev, PN532_REG_CIU_FIFOLevel, 0b10000000); /* clear fifo */
//    _load_fifo_data(dev, params_buffer, 25); /* write params into fifo */
//
//    pn532_write_reg(dev, PN532_REG_CIU_Command, 0b00000001); /* configure command */
//
//    pn532_write_reg(dev, PN532_REG_CIU_Control,   0b00000000);
//    pn532_write_reg(dev, PN532_REG_CIU_Mode,      0b00111111);
//    pn532_write_reg(dev, PN532_REG_CIU_FelNFC2,   0b10000000);
//    pn532_write_reg(dev, PN532_REG_CIU_TxMode,    0b10010010); /* this must be changed based on the bitrate*/
//    pn532_write_reg(dev, PN532_REG_CIU_RxMode,    0b10011010); /* this must be changed based on the bitrate */
//    pn532_write_reg(dev, PN532_REG_CIU_TxControl, 0b10000000);
//    pn532_write_reg(dev, PN532_REG_CIU_TxAuto,    0b00100000);
//    pn532_write_reg(dev, PN532_REG_CIU_Demod,     0b01100001);
//    pn532_write_reg(dev, PN532_REG_CIU_CommIrq,   0b01111111);
//    pn532_write_reg(dev, PN532_REG_CIU_DivIrq,    0b01111111);
//    pn532_write_reg(dev, PN532_REG_CIU_Command,   0b00001101);
//
//    uint8_t commirq, status_1, status_2, divirq;
//    while (true) {
//        ztimer_sleep(ZTIMER_MSEC, 10);
//        pn532_read_reg(dev, &commirq,  PN532_REG_CIU_CommIrq);
//        pn532_read_reg(dev, &status_1, PN532_REG_CIU_Status1);
//        pn532_read_reg(dev, &status_2, PN532_REG_CIU_Status2);
//        pn532_read_reg(dev, &divirq,   PN532_REG_CIU_DivIrq);
//        LOG_DEBUG("pn532: CIU comm irq %02x\n", commirq);
//        if ((commirq & 0b00110000) == 0b00110000) {
//            pn532_write_reg(dev, PN532_REG_CIU_CommIrq, 0b00110000); /* clear IRQ */
//            uint8_t fifo_size;
//            pn532_read_reg(dev, &fifo_size, PN532_REG_CIU_FIFOLevel);
//
//            uint8_t fifo_data[128] = {0};
//            _read_fifo_data(dev, fifo_data, fifo_size);
//
//
//            if (fifo_size == 0) {
//                return -1; /* no data in FIFO */
//            }
//
//            LOG_DEBUG("pn532: fifo size is %d\n", fifo_size);
//            if (fifo_size == fifo_data[0]) {
//                LOG_DEBUG("pn532: fifo data is ");
//                PRINTBUFF(fifo_data, fifo_size);
//
//                return 0; /* success */
//            
//            }
//            pn532_write_reg(dev, PN532_REG_CIU_Command, 0b00001101); /* restart command */
//        }
//        
//    }
//}
//
//static int _tg_init_as_target(pn532_t *dev, uint8_t mode,
//    uint8_t *mifare_params, uint8_t *felica_params, uint8_t *nfcid3t, uint8_t *buff,
//    size_t timeout_sec) {
//    assert(dev != NULL);
//    assert(buff != NULL);
//
//    LOG_DEBUG("pn532: setting CIU Mode\n");
//    int ret = pn532_write_reg(dev, PN532_REG_CIU_Mode, 0b00111111);
//    if (ret != 0) {
//        return ret;
//    }
//
//    buff[BUFF_CMD_START] = CMD_INIT_AS_TARGET;
//
//    /* target mode */
//    buff[BUFF_DATA_START] = mode;
//    
//    if (mifare_params != NULL) {
//        _rf_configure(dev, 0x0A, nfc_a_rf_config, sizeof(nfc_a_rf_config));
//        memcpy(&buff[BUFF_DATA_START + 1], mifare_params, 6);
//    }
//
//    if (felica_params != NULL) {
//        memcpy(&buff[BUFF_DATA_START + 1 + 6], felica_params, 18);
//    }
//
//    if (nfcid3t != NULL) {
//        memcpy(&buff[BUFF_DATA_START + 1 + 6 + 18], nfcid3t, 10);
//    }
//    /* AutoRFCA: on, RFCA: off
//    uint8_t enable_auto_rfca = 0b00000010;
//    _rf_configure(dev, 0x01, &enable_auto_rfca, 1);  */
//
//    LOG_DEBUG("pn532: init as target mode %i\n", mode);
//    /* recv len depends on the technology used */
//
//    return send_rcv(dev, buff, 1 + 6 + 18 + 10 + 2, 20, timeout_sec);
//}
//
//void pn532_set_parameters(pn532_t *dev, uint8_t flags) {
//    assert(dev != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START] = CMD_SET_PARAMETERS;
//    buff[BUFF_DATA_START] = flags;
//
//    send_rcv(dev, buff, 1, 1, STANDARD_TIMEOUT_SEC);
//}
//
//void pn532_set_power_down(pn532_t *dev, uint8_t wake_up_enable, uint8_t generate_irq) {
//    assert(dev != NULL);
//    LOG_DEBUG("pn532: setting power down\n");
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START     ] = CMD_POWER_DOWN;
//    buff[BUFF_DATA_START    ] = wake_up_enable;
//    buff[BUFF_DATA_START + 1] = generate_irq;
//
//    send_rcv(dev, buff, 2, 0, STANDARD_TIMEOUT_SEC);
//}
//
//uint8_t pn532_get_target_status(pn532_t *dev) {
//    assert(dev != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    uint8_t status = 0xff;
//    buff[BUFF_CMD_START] = CMD_GET_TARGET_STATUS;
//    if (send_rcv(dev, buff, 0, 2, STANDARD_TIMEOUT_SEC) == 2) {
//        status = buff[0];
//        /* discard baud rate in byte 2 */
//    }
//
//    return status;
//}
//
///* int pn532_init_tag(pn532_t *dev, nfc_application_type_t app_type) {
//    assert(dev != NULL);
//
//    if (app_type == NFC_APPLICATION_TYPE_T2T) {
//        LOG_DEBUG("pn532: init target as T2T\n");
//        uint8_t mifare_params[] = {
//            0x00, 0x00, 0x58, 0xC8, 0xB5, 0x00
//        };
//        return _init_as_target(dev, 0b00000000, mifare_params, NULL, NULL);
//    } else if (app_type == NFC_APPLICATION_TYPE_T3T) {
//        LOG_DEBUG("pn532: init target as T3T\n");
//        uint8_t felica_params[] = {
//            0x02, 0xFE, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08,
//            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
//            0x12, 0xFC 
//        };
//        return _init_as_target_nfc_f(dev, felica_params);
//    }
//
//    return -1;
//} */
//
//int pn532_listen_a(nfcdev_t *nfcdev, const nfc_a_listener_config_t *config) {
//    LOG_DEBUG("pn532: init target as NFC-A\n");
//    assert(nfcdev != NULL);
//    assert(config != NULL);
//
//    assert(config->nfcid1.len == 4); /* only 4 byte nfcid1 supported */
//    assert(config->nfcid1.nfcid[0] == 0x08); /* the first byte must be 0x08 for the PN532*/
//
//    /* the NFC-A params are called Mifare params in the PN532 manual */
//    uint8_t mifare_params[6] = {0};  /* SENS_RES, NFCID1t, SEL_RES */
//
//    mifare_params[0] = config->sens_res.anticollision_information;
//    mifare_params[1] = config->sens_res.platform_information;
//
//    mifare_params[2] = config->nfcid1.nfcid[1];
//    mifare_params[3] = config->nfcid1.nfcid[2];
//    mifare_params[4] = config->nfcid1.nfcid[3];
//
//    mifare_params[5] = config->sel_res;
//
//    uint8_t mode = 0x00;
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN] = {0};
//    if (config->sel_res && NFC_A_SEL_RES_T4T_VALUE) {
//        /* we enable automatic handeling of NFC-DEP for T4T */
//        pn532_set_parameters((pn532_t *) nfcdev->dev, 0b00100000); /* enable NFC-DEP */
//        mode &= 0b00000100;
//    } else {
//        pn532_set_parameters((pn532_t *) nfcdev->dev, 0b00000000); /* disable NFC-DEP */
//    }
//
//
//    int ret = _tg_init_as_target((pn532_t *) nfcdev->dev, mode, mifare_params, NULL, NULL, buff, 
//        LISTEN_TIMEOUT_SEC);
//    if (ret <= 1) {
//        return ret;
//    }
//
//    pn532_t *dev = (pn532_t *) nfcdev->dev;
//
//    /* the buff now contains the first command we received from the initiator */
//    if (0b00001000 & buff[0]) {
//        LOG_DEBUG("pn532: received ISO-DEP RATS\n");
//        /* The PN532 is initialized as ISO14443A-4 target (NFC-DEP), the buffer should
//         * contain the RATS sent by the initiator. We don't need to save it inside the
//         * initiator buffer. Therefore, we do nothing.
//         */
//        dev->iso_dep = true;
//    } else if ((0b00001000 & buff[0]) == 0x00) {
//        /* We should be a passive target. Potentially a T2T or a proprietary tag. */
//        dev->initiator_command_len = ret - 1;
//        LOG_DEBUG("pn532: received initiator command len %d\n", dev->initiator_command_len);
//        assert(dev->initiator_command_len <= CONFIG_PN532_INITIATOR_COMMAND_BUFFER_LEN);
//        memcpy(dev->initiator_command, &buff[1], dev->initiator_command_len);
//        dev->iso_dep = false;
//    } else {
//        LOG_ERROR("pn532: unknown initiator command received %02x\n", buff[0]);
//        return -1;
//    }
//
//    return 0;
//}
//
///* Polls for an NFC-A tag */
//int pn532_poll_a(nfcdev_t *nfcdev, nfc_a_listener_config_t *config) {
//    assert(nfcdev != NULL);
//    assert(config != NULL);
//
//    /* enable automatic RATS */
//    pn532_set_parameters(nfcdev->dev, 0b00010000);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    int ret = _list_passive_targets(nfcdev->dev, buff, PN532_BR_106_ISO_14443_A, 1,
//                         LIST_PASSIVE_LEN_14443(1));
//    if (ret <= 0) {
//        LOG_ERROR("pn532: error during polling\n");
//        return NFC_ERR_POLL_NO_TARGET;
//    }
//
//    if (buff[0] != 1) {
//        LOG_ERROR("pn532: error during polling\n");
//        return NFC_ERR_POLL_NO_TARGET;
//    }
//
//    config->sens_res.anticollision_information = buff[3];
//    config->sens_res.platform_information      = buff[2];
//    config->sel_res = buff[4];
//    config->nfcid1.len = buff[5];
//    memcpy(config->nfcid1.nfcid, &buff[6], config->nfcid1.len);
//
//    // uint8_t *target_data = buff[1];
//    return 0;
//}
//
///* Polls for targets in the area independent of technology */
//int pn532_poll(nfcdev_t *nfcdev, nfc_listener_config_t *config) {
//    assert(nfcdev != NULL);
//    assert(config != NULL);
//
//    nfc_a_listener_config_t a_config;
//    nfc_b_listener_config_t b_config;
//    nfc_f_listener_config_t f_config;
//
//    if (0 == pn532_poll_a(nfcdev, &a_config)) {
//        config->technology = NFC_TECHNOLOGY_A;
//        memcpy(&config->config.a, &a_config, sizeof(nfc_a_listener_config_t));
//    } else if (0 == pn532_poll_b(nfcdev, &b_config)) {
//        config->technology = NFC_TECHNOLOGY_B;
//        memcpy(&config->config.b, &b_config, sizeof(nfc_b_listener_config_t));
//    } else if (0 == pn532_poll_f(nfcdev, &f_config)) {
//        config->technology = NFC_TECHNOLOGY_F;
//        memcpy(&config->config.f, &f_config, sizeof(nfc_f_listener_config_t));
//    } else {
//        return NFC_ERR_POLL_NO_TARGET;
//    }
//
//    return 0;
//}
//
///* Polls for an NFC-B tag */
//int pn532_poll_b(nfcdev_t *nfcdev, nfc_b_listener_config_t *config) {
//    assert(nfcdev != NULL);
//    assert(config != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    int ret = _list_passive_targets(nfcdev->dev, buff, PN532_BR_106_ISO_14443_B, 1,
//                         21);
//    if (ret != 0) {
//        return ret;
//    }
//
//    if (buff[0] != 1) {
//        LOG_ERROR("pn532: error during polling\n");
//        return NFC_ERR_POLL_NO_TARGET;
//    }
//
//    memcpy(&(config->sensb_res.nfcid0), &buff[2], NFC_B_NFCID0_LEN);
//    memcpy(&(config->sensb_res.application_data), &buff[2 + NFC_B_NFCID0_LEN], NFC_B_APP_DATA_LEN);
//    memcpy(&(config->sensb_res.protocol_info), &buff[2 + NFC_B_NFCID0_LEN + NFC_B_APP_DATA_LEN],
//           NFC_B_PROT_INFO_LEN);
//
//    // uint8_t *target_data = buff[1];
//    return 0;
//}
//
//int pn532_poll_f(nfcdev_t *nfcdev, nfc_f_listener_config_t *config) {
//    assert(nfcdev != NULL);
//    assert(config != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    int ret = _list_passive_targets(nfcdev->dev, buff, PN532_BR_212_FELICA, 1, 21);
//    if (ret != 0) {
//        return ret;
//    }
//
//    if (buff[0] != 1) {
//        LOG_ERROR("pn532: error during polling\n");
//        return -1;
//    }
//
//    memcpy(config->sensf_res.nfcid2, &buff[5], NFC_F_NFCID2_LEN);
//
//    // uint8_t *target_data = buff[1];
//    return 0;
//}
//
//int pn532_mifare_classic_authenticate(nfcdev_t *nfcdev, uint8_t block, 
//    const nfc_a_nfcid1_t *nfcid1, bool is_key_a, const uint8_t *key) {
//    assert(nfcdev != NULL);
//    assert(key != NULL);
//    assert(nfcid1 != NULL);
//    assert(nfcid1->len == NFC_A_NFCID1_LEN4);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
//    buff[BUFF_DATA_START    ] = 1; /* tg number must always be 1 */
//    buff[BUFF_DATA_START + 1] = is_key_a ? MIFARE_CLASSIC_AUTH_A : MIFARE_CLASSIC_AUTH_B;
//    buff[BUFF_DATA_START + 2] = block; /* current block */
//
//
//    memcpy(&buff[BUFF_DATA_START + 3], key, 6);
//
//    memcpy(&buff[BUFF_DATA_START + 9], nfcid1->nfcid, nfcid1->len);
//
//    int ret_len = send_rcv((pn532_t *) nfcdev->dev, buff, 9 + nfcid1->len, 1, STANDARD_TIMEOUT_SEC);
//    if (ret_len > 0) {
//        if (buff[0] != 0x00) {
//            /* error */
//            LOG_DEBUG("pn532: error in mifare classic auth %02x\n", buff[0]);
//            return NFC_ERR_AUTH;
//        }
//        return 0;
//    } else {
//        return NFC_ERR_AUTH;
//    }
//}
//
//int pn532_initiator_exchange_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len,
//                                  uint8_t *rcv, size_t *receive_len) {
//    assert(nfcdev != NULL);
//    assert(nfcdev->dev != NULL);
//    assert(BUFF_DATA_START + 1 + send_len <= CONFIG_PN532_BUFFER_LEN);
//    assert(receive_len != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//
//    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
//    buff[BUFF_DATA_START    ] = 1; /* tg number must always be 1 */
//
//    memcpy(&buff[BUFF_DATA_START + 1], send, send_len);
//    int ret_len = send_rcv(nfcdev->dev, buff, send_len + 1, CONFIG_PN532_BUFFER_LEN - 8, 
//        STANDARD_TIMEOUT_SEC);
//
//    /* receive_len is only the data received, not the status byte */
//    if (ret_len > 0) {
//        if (*receive_len < (size_t)(ret_len - 1)) {
//            /* the receive buffer is too small */
//            LOG_ERROR("pn532: receive buffer too small\n");
//            *receive_len = 0;
//            return -1;
//        }
//
//        *receive_len = ret_len - 1;
//        if (buff[0] != 0x00) {
//            /* error */
//            LOG_ERROR("pn532: error in data exchange %02x\n", buff[0]);
//            *receive_len = 0;
//            return -1;
//        }
//
//        LOG_DEBUG("pn532: received %u bytes\n", *receive_len);
//        /* copy the data into the receive buffer, excluding the status byte */
//        memcpy(rcv, buff + 1, *receive_len);
//    }
//    return 0;
//}
//
///* returns the length of bytes sent */
//static int _tg_response_to_initiator(pn532_t *dev, const uint8_t *send, size_t send_len) {
//    LOG_DEBUG("pn532: response to initiator\n");
//    assert (send != NULL);
//    assert (send_len > 0);
//    assert(BUFF_DATA_START + send_len <= CONFIG_PN532_BUFFER_LEN);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START] = CMD_RESPONSE_TO_INITIATOR;
//
//    memcpy(&buff[BUFF_DATA_START], send, send_len);
//
//    /* receive 1 status byte */
//    int ret_len = send_rcv(dev, buff, send_len, 1, STANDARD_TIMEOUT_SEC);
//    if (ret_len != 1) {
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    if (buff[0] != 0x00) {
//        LOG_ERROR("pn532: error in response to initiator %02x\n", buff[0]);
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    return ret_len;
//}
//
//// static int _tg_get_target_status(pn532_t *dev, uint8_t *status, uint8_t *baud_rate) {
////     LOG_DEBUG("pn532: get target status\n");
////     assert(status != NULL);
////     assert(baud_rate != NULL);
//
////     uint8_t buff[CONFIG_PN532_BUFFER_LEN];
////     buff[BUFF_CMD_START] = CMD_GET_TARGET_STATUS;
//
////     int ret_len = send_rcv(dev, buff, 0, 1, STANDARD_TIMEOUT_SEC);
////     if (ret_len != 1) {
////         return NFC_ERR_COMMUNICATION;
////     }
//
////     *status = buff[0];
////     *baud_rate = buff[1];
////     return ret_len;
//// }
//
///* this is used to receive */
//static int _tg_get_initiator_command(pn532_t *dev, uint8_t *rcv, size_t *receive_len) {
//    LOG_DEBUG("pn532: get initiator command\n");
//    assert(rcv != NULL);
//    assert(receive_len != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START] = CMD_GET_INITIATOR_COMMAND;
//
//    int ret_len = send_rcv(dev, buff, 0, CONFIG_PN532_BUFFER_LEN - 10, STANDARD_TIMEOUT_SEC);
//
//    if (ret_len == 0) {
//        LOG_ERROR("pn532: no data received from initiator\n");
//        *receive_len = 0;
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    if (ret_len <= 1) {
//        *receive_len = 0;
//        return ret_len;
//    }
//
//    /* check if ret_len is bigger than receive_len */
//    if (ret_len - 1 > (int)*receive_len) {
//        LOG_ERROR("pn532: receive buffer too small\n");
//        *receive_len = 0;
//        return -1;
//    }
//
//    if (buff[0] != 0x00) {
//        LOG_ERROR("pn532: error in get initiator command %02x\n", buff[0]);
//        *receive_len = 0;
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    *receive_len = ret_len - 1;
//    memcpy(rcv, buff + 1, *receive_len);
//
//    return ret_len;
//}
//
///* for NFC-DEP and ISO-DEP */
//static int _tg_get_data(pn532_t *dev, uint8_t *rcv, size_t *receive_len) {
//    LOG_DEBUG("pn532: get data\n");
//    assert(dev != NULL);
//    assert(rcv != NULL);
//    assert(receive_len != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START] = CMD_GET_DATA;
//
//    int ret_len = send_rcv(dev, buff, 0, CONFIG_PN532_BUFFER_LEN - 8, STANDARD_TIMEOUT_SEC);
//
//    if (ret_len < 0) {
//        *receive_len = 0;
//        return NFC_ERR_COMMUNICATION;
//    }
//
//#define ERROR_CODE_DESELECTED 0x29
//    if (buff[0] == ERROR_CODE_DESELECTED) {
//        LOG_ERROR("pn532: target deselected by initiator\n");
//        *receive_len = 0;
//        return NFC_ERR_DESELECTED;
//    }
//
//    /* check if ret_len is bigger than receive_len */
//    if (ret_len - 1 > (int)*receive_len) {
//        LOG_ERROR("pn532: receive buffer too small\n");
//        *receive_len = 0;
//        return -1;
//    }
//
//    if (buff[0] != 0x00) {
//        LOG_ERROR("pn532: error in get data %02x\n", buff[0]);
//        *receive_len = 0;
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    *receive_len = ret_len - 1;
//    memcpy(rcv, buff + 1, *receive_len);
//
//    return ret_len;
//}
//
///* for NFC-DEP and ISO-DEP */
//static int _tg_set_data(pn532_t *dev, const uint8_t *send, size_t send_len) {
//    LOG_DEBUG("pn532: set data\n");
//    assert(dev != NULL);
//    assert(send != NULL);
//    assert(send_len > 0);
//    assert(BUFF_DATA_START + send_len <= CONFIG_PN532_BUFFER_LEN);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    buff[BUFF_CMD_START] = CMD_SET_DATA;
//
//    memcpy(&buff[BUFF_DATA_START], send, send_len);
//
//    int ret_len = send_rcv(dev, buff, send_len, 0, STANDARD_TIMEOUT_SEC);
//    if (ret_len != 1) {
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    if (buff[0] != 0x00) {
//        LOG_ERROR("pn532: error in send data %02x\n", buff[0]);
//        return NFC_ERR_COMMUNICATION;
//    }
//
//    return ret_len;
//}
//
//int pn532_target_send_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len) {
//    assert(nfcdev != NULL);
//    assert(nfcdev->dev != NULL);
//
//    if (send_len > CONFIG_PN532_BUFFER_LEN - 8) {
//        LOG_ERROR("pn532: send buffer too large (%u bytes, max %d)\n", send_len, CONFIG_PN532_BUFFER_LEN - 8);
//        return -1;
//    }
//
//    if (((pn532_t *) nfcdev->dev)->iso_dep) {
//        /* for communication with an ISO 14443-4 PICC (ISO-DEP) */
//        return _tg_set_data(nfcdev->dev, send, send_len);
//    } else {
//        /* for communication with a non-ISO-DEP target (T2T, MIFARE Classic, etc.) */
//        _tg_response_to_initiator(nfcdev->dev, send, send_len);
//        return 0;
//    }
//
//
//}
//
//int pn532_target_receive_data(nfcdev_t *nfcdev, uint8_t *rcv, size_t *receive_len) {
//    assert(nfcdev != NULL);
//    assert(nfcdev->dev != NULL);
//    assert(rcv != NULL);
//    assert(receive_len != NULL);
//
//    /* the PN532 init as target command returns after receiving the first command */
//    pn532_t *dev = (pn532_t *) nfcdev->dev;
//
//    if (dev->initiator_command_len > 0) {
//        assert(dev->initiator_command_len <= CONFIG_PN532_INITIATOR_COMMAND_BUFFER_LEN);
//        memcpy(rcv, dev->initiator_command, dev->initiator_command_len);
//        LOG_DEBUG("pn532: returning cached initiator command of length %zu\n", dev->initiator_command_len);
//        *receive_len = dev->initiator_command_len;
//        dev->initiator_command_len = 0; /* clear the buffer */
//        return *receive_len;
//    }
//
//    if (dev->iso_dep) {
//        return _tg_get_data(nfcdev->dev, rcv, receive_len);
//    } else {
//        return _tg_get_initiator_command(nfcdev->dev, rcv, receive_len);
//    }
//}
//
//int pn532_poll_dep(nfcdev_t *nfcdev, nfc_baudrate_t br) {
//    assert(nfcdev != NULL);
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
//    uint8_t target_type;
//    switch (br) {
//        case NFC_BAUDRATE_106K:
//            target_type = 0x00;
//            break;
//        case NFC_BAUDRATE_212K:
//            target_type = 0x01;
//            break;
//        case NFC_BAUDRATE_424K:
//            target_type = 0x02;
//            break;
//        default:
//            return -1;
//    }
//
//    int ret = _list_passive_targets(nfcdev->dev, buff, target_type, 1, 30);
//    if (ret <= 0) {
//        return -1;
//    }
//
//    if (buff[0] != 1) {
//        LOG_ERROR("pn532: error during polling\n");
//        return NFC_ERR_POLL_NO_TARGET;
//    }
//
//    return 0;
//}
//
//int pn532_listen_dep(nfcdev_t *nfcdev, nfc_baudrate_t br, const uint8_t *nfcid3t) {
//    assert(nfcdev != NULL);
//    (void) br;
//
//    uint8_t mode = TG_INIT_AS_TARGET_DEP_ONLY;
//
//    uint8_t buff[CONFIG_PN532_BUFFER_LEN] = {0};
//
//    _tg_init_as_target(nfcdev->dev, mode, NULL, NULL, (uint8_t *) nfcid3t, buff, 
//        LISTEN_TIMEOUT_SEC);
//
//    return 0;
//};
