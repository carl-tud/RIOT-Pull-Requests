/*
 * SPDX-FileCopyrightText: 2016 TriaGnoSys GmbH
 * SPDX-License-Identifier: LGPL-2.1-only
 */

/**
 * @ingroup  drivers_pn532
 *
 * @{
 * @file
 * @brief   PN532 driver
 *
 * @author   <victor.arino@triagnosys.com>
 * @}
 */

#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "kernel_defines.h"
#include "ztimer.h"
#include "mutex.h"
#include "pn532.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "periph/uart.h"
#include "msg.h"

#include "log.h"

#include "net/nfc/nfc_error.h"

#define ENABLE_DEBUG                1
#include "debug.h"

#define PN532_I2C_ADDRESS           (0x24)

/* Commands */
#define CMD_FIRMWARE_VERSION        (0x02)
#define CMD_READ_REG                (0x06)
#define CMD_WRITE_REG               (0x08)
#define CMD_READ_GPIO               (0x0c)
#define CMD_WRITE_GPIO              (0x0E)
#define CMD_SET_PARAMETERS          (0x12)
#define CMD_SAM_CONFIG              (0x14)
#define CMD_POWER_DOWN              (0x16)
#define CMD_RF_CONFIG               (0x32)
#define CMD_DATA_EXCHANGE           (0x40)
#define CMD_DESELECT                (0x44)
#define CMD_LIST_PASSIVE            (0x4a)
#define CMD_RELEASE                 (0x52)
#define CMD_GET_TARGET_STATUS       (0x8a)
#define CMD_INIT_AS_TARGET          (0x8c)
#define CMD_GET_INITIATOR_COMMAND   (0x88)
#define CMD_RESPONSE_TO_INITIATOR   (0x90)
#define CMD_GET_DATA                (0x86)
#define CMD_SET_DATA                (0x8e)


/* Mifare specific commands */
#define MIFARE_CMD_READ             (0x30)
#define MIFARE_CMD_WRITE            (0xA0)

/* RF register settings */
#define RF_CONFIG_MAX_RETRIES       (0x05)
#define RF_CONFIGURATION            (0x32)
#define RF_REGULATION_TEST          (0x58)

/* Buffer operations */
#define BUFF_CMD_START              (6)
#define BUFF_DATA_START             (BUFF_CMD_START + 1)
#define RAPDU_DATA_BEGIN            (1)
#define RAPDU_MAX_DATA_LEN          (CONFIG_PN532_BUFFER_LEN - BUFF_DATA_START - 5)
#define CAPDU_MAX_DATA_LEN          (CONFIG_PN532_BUFFER_LEN - BUFF_DATA_START - 1)

/* Constants and magic numbers */
#define RESET_TOGGLE_SLEEP_MS       (400)
#define RESET_BACKOFF_MS            (10)
#define HOST_TO_PN532               (0xD4)
#define PN532_TO_HOST               (0xD5)
#define SPI_DATA_WRITE              (0x80)
#define SPI_STATUS_READING          (0x40)
#define SPI_DATA_READ               (0xC0)
#define SPI_WRITE_DELAY_US          (2000)

/* Target modes */
#define TARGET_MODE_PASSIVE         (0x01)
#define TARGET_MODE_P2P             (0x02)
#define TARGET_MODE_PICC            (0x04)

/* SPI bus parameters */
#define SPI_MODE                    (SPI_MODE_0)
#define SPI_CLK                     (SPI_CLK_1MHZ)

/* PN53X_REG_CIU_TxAuto */
#define SYMBOL_FORCE_100_ASK      (0x40)
#define SYMBOL_AUTO_WAKE_UP       (0x20)
#define SYMBOL_INITIAL_RF_ON      (0x04)

/* Length for passive listings */
#define LIST_PASSIVE_LEN_14443(num)   (num * 20)

#define UART_BAUDRATE               (115200U)

/* RF Configurations */
static const uint8_t nfc_a_rf_config[] = {0x5A, 0xF4, 0x3F, 0x11, 0x4D, 0x85, 0x61, 0x6F, 0x26, 
    0x62, 0x87};
// static const uint8_t nfc_b_rf_config[] = {0xFF, 0x04, 0x85};
static const uint8_t nfc_f_rf_config[] = {0x69, 0xFF, 0x3F, 0x11, 0x41, 0x85, 0x61, 0x6F};

#if IS_ACTIVE(ENABLE_DEBUG)
#define PRINTBUFF printbuff
static void printbuff(uint8_t *buff, unsigned len)
{
    while (len) {
        len--;
        printf("%02x ", *buff++);
    }
    printf("\n");
}
#else
#define PRINTBUFF(...)
#endif

/* Forward declarations */
static int send_rcv(pn532_t *dev, uint8_t *buff, unsigned sendl, unsigned recvl);
static int _rf_configure(pn532_t *dev, unsigned cfg_item, const uint8_t *data, unsigned len);

static int block_with_timeout(pn532_t *dev, uint32_t timeout_sec) {

    if (timeout_sec == 0) {
        mutex_lock(&dev->trap);
        return 0;
    } else {
        ztimer_t timer = {0};
        ztimer_mutex_unlock(ZTIMER_SEC, &timer, timeout_sec, &dev->trap);
        mutex_lock(&dev->trap);
        bool triggered = !ztimer_remove(ZTIMER_SEC, &timer);
        if (triggered) {
            return NFC_ERR_TIMEOUT;
        } else {
            return 0;
        }
    }
}


static void _nfc_event(void *dev)
{
    // DEBUG("pn532: irq triggered\n");
    mutex_unlock(&((pn532_t *)dev)->trap);
}

void pn532_reset(const pn532_t *dev)
{
    assert(dev != NULL);

    gpio_clear(dev->conf->reset);
    ztimer_sleep(ZTIMER_MSEC, RESET_TOGGLE_SLEEP_MS);
    gpio_set(dev->conf->reset);
    ztimer_sleep(ZTIMER_MSEC, RESET_BACKOFF_MS);
}

uint8_t uart_buffer[128] = {0};
uint8_t uart_index = 0;

#if IS_USED(MODULE_PN532_UART)
static void uart_rx_cb(void *dev, uint8_t byte) {
    (void) dev;
    DEBUG("pn532: uart irq triggered with byte %02x\n", byte);
    uart_buffer[uart_index++] = byte;
    mutex_unlock(&((pn532_t *)dev)->cb); 
}
#endif



int _pn532_init(pn532_t *dev, const pn532_params_t *params, pn532_mode_t mode)
{
    assert(dev != NULL);

    dev->conf = params;

    #if IS_USED(MODULE_PN532_I2C) || IS_USED(MODULE_PN532_SPI)
    gpio_init_int(dev->conf->irq, GPIO_IN_PD, GPIO_FALLING,
                   _nfc_event, (void *)dev);
    
    #endif

    gpio_init(dev->conf->reset, GPIO_OUT);
    gpio_set(dev->conf->reset);


    dev->mode = mode;
    if (mode == PN532_SPI) {
#if IS_USED(MODULE_PN532_SPI)
        /* we handle the CS line manually... */
        gpio_init(dev->conf->nss, GPIO_OUT);
        gpio_set(dev->conf->nss);
#endif
    } else if (mode == PN532_UART) {
#if IS_USED(MODULE_PN532_UART)
        DEBUG("pn532: initializing uart\n");
        mutex_init(&dev->cb);
        mutex_lock(&dev->cb);
        int ret = uart_init(dev->conf->uart, UART_BAUDRATE, uart_rx_cb, (void *) dev);
        if (ret < 0) {
            DEBUG("pn532: uart_init failed with %d\n", ret);
            return ret;
        }
        ztimer_sleep(ZTIMER_MSEC, 1000);
        // uart_mode(dev->conf->uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
        // uart_poweron(dev->conf->uart);
#endif
    }

    pn532_reset(dev);

    #if !IS_USED(MODULE_PN532_UART)
    mutex_init(&dev->trap);
    mutex_lock(&dev->trap);
    #endif

    int error = pn532_sam_configuration(dev, 0x01, 0xA0, true);
    if (error != 0) {
        DEBUG("pn532: sam configuration failed with %d\n", error);
        return error;
    }

    uint8_t cfg1[] = {0x00, 0x0B, 0x0A};
    _rf_configure(dev, 0x02, cfg1, sizeof(cfg1));
    uint8_t cfg2[] = {0x00};
    _rf_configure(dev, 0x04, cfg2, sizeof(cfg2));
    uint8_t cfg3[] = {0x01, 0x00, 0x01};
    _rf_configure(dev, 0x05, cfg3, sizeof(cfg3));

    DEBUG("pn532: setting parameters to 0\n");
    pn532_set_parameters(dev, 0b00000000);

    DEBUG("pn532: init complete\n");

    return 0;
}

int pn532_init(nfcdev_t *nfcdev, const void *dev_config) {
    pn532_t *dev = (pn532_t *)nfcdev->dev;
    const pn532_config_t *config = (const pn532_config_t *)dev_config;
    int ret = _pn532_init(dev, &config->params, config->mode);
    if (ret != 0) {
        return -1;
    }
    nfcdev->state = NFCDEV_STATE_DISABLED;
    return 0;

}

static uint8_t chksum(uint8_t *b, unsigned len)
{
    uint8_t c = 0x00;

    while (len--) {
        c -= *b++;
    }
    return c;
}

#if IS_USED(MODULE_PN532_SPI) // || IS_USED(MODULE_PN532_UART)
static void reverse(uint8_t *buff, unsigned len)
{
    while (len--) {
        buff[len] = (buff[len] & 0xF0) >> 4 | (buff[len] & 0x0F) << 4;
        buff[len] = (buff[len] & 0xCC) >> 2 | (buff[len] & 0x33) << 2;
        buff[len] = (buff[len] & 0xAA) >> 1 | (buff[len] & 0x55) << 1;
    }
}
#endif

#if IS_USED(MODULE_PN532_SPI)
static int _read_status(const pn532_t *dev, uint8_t *status)
{

    if (dev->mode == PN532_SPI) {
        spi_acquire(dev->conf->spi, SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
        gpio_clear(dev->conf->nss);
        spi_transfer_byte(dev->conf->spi, SPI_CS_UNDEF, true, SPI_STATUS_READING);
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, status, 1);
        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);
        return 0;
    }

    return -1;
}
#endif

int _read_blocking(pn532_t *dev, uint8_t *buff, unsigned len) {

    int ret = -1;
#if IS_USED(MODULE_PN532_SPI)
    if (dev->mode == PN532_SPI) {
        uint8_t status = 0x00;
        /* receive the status byte */
        do {
            _read_status(dev, &status);
            ztimer_sleep(ZTIMER_USEC, 100);
        } while ((status & 0x80) == 0);
        /* status is 0x01 (0x80 reversed) here */

        spi_acquire(dev->conf->spi, SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
        gpio_clear(dev->conf->nss);

        spi_transfer_byte(dev->conf->spi, SPI_CS_UNDEF, true, SPI_DATA_READ);
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, &buff[1], len);
        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);

        buff[0] = 0x80;

        reverse(buff, len);
        ret = (int) len + 1;
    }
#endif
#if IS_USED(MODULE_PN532_UART)
    if (dev->mode == PN532_UART) {
        /* uart does not have a status byte, simply block until we have read the required 
        amount of bytes */;
        while (uart_index < len) {
            DEBUG("pn532: waiting for byte %u/%u\n", uart_index + 1, len);
            mutex_lock(&dev->cb);
        }
        ret = (int) uart_index;
        memcpy(buff, uart_buffer, uart_index);
        uart_index = 0;
    }
#endif
    // DEBUG("pn532: <- ");
    // PRINTBUFF(buff, len);
    return ret;
}

static int _write(const pn532_t *dev, uint8_t *buff, unsigned len)
{
    int ret = -1;

    (void)buff;
    (void)len;

    #if IS_USED(MODULE_PN532_SPI)
    DEBUG("pn532: -> ");
    PRINTBUFF(buff, len);
    #endif

    switch (dev->mode) {
#if IS_USED(MODULE_PN532_I2C)
    case PN532_I2C:
        i2c_acquire(dev->conf->i2c);
        ret = i2c_write_bytes(dev->conf->i2c, PN532_I2C_ADDRESS, buff, len, 0);
        if (ret == 0) {
            ret = (int)len;
        }
        i2c_release(dev->conf->i2c);
        break;
#endif
#if IS_USED(MODULE_PN532_SPI)
    case PN532_SPI:
        spi_acquire(dev->conf->spi, SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
        gpio_clear(dev->conf->nss);
        ztimer_sleep(ZTIMER_USEC, SPI_WRITE_DELAY_US);
        reverse(buff, len);
        spi_transfer_byte(dev->conf->spi, SPI_CS_UNDEF, true, SPI_DATA_WRITE);
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, buff, NULL, len);
        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);
        ret = (int)len;
        break;
#endif
#if IS_USED(MODULE_PN532_UART)
    case PN532_UART:
        DEBUG("pn532: writing uart\n");
        // reverse(buff, len);
        uart_write(dev->conf->uart, buff, len);
        ret = (int)len;
        break;
#endif
    default:
        DEBUG("pn532: invalid mode (%i)!\n", dev->mode);
    }
    //DEBUG("pn532: -> ");
    // PRINTBUFF(buff, len);
    ztimer_sleep(ZTIMER_USEC, 1000);
    return ret;
}

static int _read(const pn532_t *dev, uint8_t *buff, unsigned len)
{
    int ret = -1;

    (void)buff;
    (void)len;

    switch (dev->mode) {
#if IS_USED(MODULE_PN532_I2C)
    case PN532_I2C:
        i2c_acquire(dev->conf->i2c);
        /* len+1 for RDY after read is accepted */
        ret = i2c_read_bytes(dev->conf->i2c, PN532_I2C_ADDRESS, buff, len + 1, 0);
        if (ret == 0) {
            ret = (int)len + 1;
        }
        i2c_release(dev->conf->i2c);
        break;
#endif
#if IS_USED(MODULE_PN532_SPI)
    case PN532_SPI:
        spi_acquire(dev->conf->spi, SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
        gpio_clear(dev->conf->nss);
        spi_transfer_byte(dev->conf->spi, SPI_CS_UNDEF, true, SPI_DATA_READ);
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, &buff[1], len);
        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);

        buff[0] = 0x80;
        reverse(buff, len);
        ret = (int)len + 1;
        break;
#endif
    default:
        DEBUG("pn532: invalid mode (%i)!\n", dev->mode);
    }
    if (ret > 0) {
        DEBUG("pn532: <- ");
        PRINTBUFF(buff, len);
    }

    /* wait for a while */
    ztimer_sleep(ZTIMER_USEC, SPI_WRITE_DELAY_US);
    return ret;
}

static int send_cmd(const pn532_t *dev, uint8_t *buff, unsigned len)
{
    unsigned pos;
    uint8_t checksum;

    buff[0] = 0x00;
    buff[1] = 0x00;
    buff[2] = 0xFF;

    len += 1;
    if (len < 0xff) {
        buff[3] = (uint8_t)len;
        buff[4] = 0x00 - buff[3];
        pos = 5;

    }
    else {
        buff[3] = 0xff;
        buff[4] = 0xff;
        buff[5] = (len >> 8) & 0xff;
        buff[6] = (len) & 0xff;
        buff[7] = 0x00 - buff[5] - buff[6];
        pos = 8;
    }

    buff[pos] = HOST_TO_PN532;
    checksum = chksum(&buff[pos], len);

    len += pos;
    buff[len++] = checksum;
    buff[len++] = 0x00;

    return _write(dev, buff, len);
}

/* Returns >0 payload len (or <0 received len but not as expected) */
static int read_command(pn532_t *dev, uint8_t *buff, unsigned len, int expected_cmd)
{
    int r;
    unsigned j, fi, lp, lc;

    /* apply framing overhead */
    len += 8;
    if (len >= 0xff) {
        len += 3;
    }

    r = _read(dev, buff, len);

    /* Validate frame structure and CRCs
     *
     * Note that all offsets are shifted by one since the first byte is always
     * 0x01. */
    if ((r < (int)len) || (buff[1] != 0x00) || (buff[2] != 0x00) || (buff[3] != 0xFF)) {
        return -r;
    }

    if (buff[4] == 0xff && buff[5] == 0xff) {
        /* extended frame */
        lp = buff[6] << 8 | buff[7];
        lc = (buff[6] + buff[7] + buff[8]) & 0xff;
        fi = 9;

    }
    else {
        /* normal frame */
        lp = buff[4];
        lc = (buff[4] + buff[5]) & 0xff;
        fi = 6;
    }

    if (lc != 0 || lp >= 265 || buff[fi] != PN532_TO_HOST) {
        return -r;
    }

    if (lp == 0) {
        return -r;
    }

    if (chksum(&buff[fi], lp) != buff[fi + lp]) {
        return -r;
    }

    if (buff[fi + 1] != expected_cmd) {
        return -r;
    }

    /* Move the meaningful data to the beginning of the buffer */
    /* start copying after command byte */
    for (j = 0, fi += 2, lp -= 2; j < lp; fi++, j++) {
        buff[j] = buff[fi];
    }

    DEBUG("pn532: in cmd ");
    PRINTBUFF(buff, lp);

    return lp;
}

/* Returns 0 if OK, <0 otherwise */
static int send_check_ack(pn532_t *dev, uint8_t *buff, unsigned len)
{
    if (send_cmd(dev, buff, len) > 0) {
        static uint8_t ack[] = { 0x00, 0x00, 0xff, 0x00, 0xff, 0x00 };

        block_with_timeout(dev, 2);
        if (_read(dev, buff, sizeof(ack)) != sizeof(ack) + 1) {
            DEBUG("pn532: ack read error\n");
            return -2;
        }
 
        if (memcmp(&buff[1], ack, sizeof(ack)) != 0) {
            DEBUG("pn532: invalid ack\n");
            return -3;
        }

        block_with_timeout(dev, 2);
        return 0;
    }

    DEBUG("pn532: send error\n");
    return -1;
}

/* sendl: send length, recvl: receive payload length
    returns received length or <0 on error
*/
static int send_rcv(pn532_t *dev, uint8_t *buff, unsigned sendl, unsigned recvl)
{
    assert(dev != NULL);

    int expected_cmd = buff[BUFF_CMD_START] + 1;

    if (send_check_ack(dev, buff, sendl + 1)) {
        return 0;
    }

    recvl += 1; /* cmd response */
    return read_command(dev, buff, recvl, expected_cmd);
}

int pn532_fw_version(pn532_t *dev, uint32_t *fw_ver)
{
    unsigned ret = -1;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START] = CMD_FIRMWARE_VERSION;

    if (send_rcv(dev, buff, 0, 4) == 4) {
        *fw_ver =  ((uint32_t)buff[0] << 24);   /* ic version */
        *fw_ver += ((uint32_t)buff[1] << 16);   /* fw ver */
        *fw_ver += ((uint32_t)buff[2] << 8);    /* fw rev */
        *fw_ver += (buff[3]);                   /* feature support */
        ret = 0;
    }

    return ret;
}

int pn532_read_reg(pn532_t *dev, uint8_t *out, unsigned addr)
{
    int ret = -1;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_READ_REG;
    buff[BUFF_DATA_START    ] = (addr >> 8) & 0xff;
    buff[BUFF_DATA_START + 1] = addr & 0xff;

    if (send_rcv(dev, buff, 2, 1) == 1) {
        *out = buff[8];
        ret = 0;
    }

    return ret;
}

int pn532_write_reg(pn532_t *dev, unsigned addr, uint8_t val)
{
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_WRITE_REG;
    buff[BUFF_DATA_START    ] = (addr >> 8) & 0xff;
    buff[BUFF_DATA_START + 1] = addr & 0xff;
    buff[BUFF_DATA_START + 2] = val;

    return send_rcv(dev, buff, 3, 0);
}

int pn532_update_reg(pn532_t *dev, unsigned addr, uint8_t val, uint8_t mask)
{
    uint8_t reg_val;
    int ret = pn532_read_reg(dev, &reg_val, addr);
    if (ret < 0) {
        return ret;
    }

    reg_val &= ~mask;
    reg_val |= (val & mask);

    return pn532_write_reg(dev, addr, reg_val);
}

static int _rf_configure(pn532_t *dev, unsigned cfg_item, const uint8_t *config,
                         unsigned cfg_len)
{
    DEBUG("pn532: rf config %02x\n", cfg_item);
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    buff[BUFF_CMD_START ] = CMD_RF_CONFIG;
    buff[BUFF_DATA_START] = cfg_item;
    for (unsigned i = 1; i <= cfg_len; i++) {
        buff[BUFF_DATA_START + i] = *config++;
    }

    return send_rcv(dev, buff, cfg_len + 1, 0);
}

static int _set_act_retries(pn532_t *dev, unsigned max_retries)
{
    uint8_t rtrcfg[] = { 0xff, 0x01, max_retries & 0xff };

    return _rf_configure(dev, RF_CONFIG_MAX_RETRIES, rtrcfg, sizeof(rtrcfg));
}

int pn532_sam_configuration(pn532_t *dev, pn532_sam_conf_mode_t mode, unsigned timeout, 
    bool use_irq)
{
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_SAM_CONFIG;
    buff[BUFF_DATA_START    ] = (uint8_t)mode;
    buff[BUFF_DATA_START + 1] = (uint8_t)(timeout / 50);
    buff[BUFF_DATA_START + 2] = use_irq ? 0x01 : 0x00;

    return send_rcv(dev, buff, 3, 0);
}

static int _list_passive_targets(pn532_t *dev, uint8_t *buff, pn532_target_t target,
                                 unsigned max, unsigned recvl)
{
    buff[BUFF_CMD_START]      = CMD_LIST_PASSIVE;
    buff[BUFF_DATA_START]     = (uint8_t) max;
    buff[BUFF_DATA_START + 1] = (uint8_t)target;

    /* requested len depends on expected target num and type */
    return send_rcv(dev, buff, 2, recvl);
}

int pn532_get_passive_iso14443a(pn532_t *dev, nfc_iso14443a_t *out,
                                unsigned max_retries)
{
    int ret = -1;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    if (_set_act_retries(dev, max_retries) == 0) {
        ret = _list_passive_targets(dev, buff, PN532_BR_106_ISO_14443_A, 1,
                                    LIST_PASSIVE_LEN_14443(1));
    }

    if (ret > 0 && buff[0] > 0) {
        out->target = buff[1];
        out->sns_res = (buff[2] << 8) | buff[3];
        out->sel_res = buff[4];
        out->id_len  = buff[5];
        out->type = ISO14443A_UNKNOWN;

        for (int i = 0; i < out->id_len; i++) {
            out->id[i] = buff[6 + i];
        }

        /* try to find out the type */
        if (out->id_len == 4) {
            out->type = ISO14443A_MIFARE;
        }
        else if (out->id_len == 7) {
            /* In the case of type 4, the first byte of RATS is the length
             * of RATS including the length itself (6+7) */
            if (buff[13] == ret - 13) {
                out->type = ISO14443A_TYPE4;
            }
        }
        ret = 0;
    }
    else {
        ret = -1;
    }

    return ret;
}

void pn532_deselect_passive(pn532_t *dev, unsigned target_id)
{
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START ] = CMD_DESELECT;
    buff[BUFF_DATA_START] = target_id;

    send_rcv(dev, buff, 1, 1);
}

void pn532_release_passive(pn532_t *dev, unsigned target_id)
{
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START ] = CMD_RELEASE;
    buff[BUFF_DATA_START] = target_id;

    send_rcv(dev, buff, 1, 1);
}

int pn532_mifareclassic_authenticate(pn532_t *dev, nfc_iso14443a_t *card,
                                     pn532_mifare_key_t keyid, uint8_t *key, unsigned block)
{
    int ret = -1;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
    buff[BUFF_DATA_START    ] = card->target;
    buff[BUFF_DATA_START + 1] = keyid;
    buff[BUFF_DATA_START + 2] = block; /* current block */

    /*
     * The card ID directly follows the key in the buffer
     * The key consists of 6 bytes and starts at offset 3
     */
    for (int i = 0; i < 6; i++) {
        buff[BUFF_DATA_START + 3 + i] = key[i];
    }

    for (int i = 0; i < card->id_len; i++) {
        buff[BUFF_DATA_START + 9 + i] = card->id[i];
    }

    ret = send_rcv(dev, buff, 9 + card->id_len, 1);
    if (ret == 1) {
        ret = buff[0];
        card->auth = 1;
    }

    return ret;
}

int pn532_mifareclassic_write(pn532_t *dev, uint8_t *idata, nfc_iso14443a_t *card,
                              unsigned block)
{
    int ret = -1;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    if (card->auth) {

        buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
        buff[BUFF_DATA_START    ] = card->target;
        buff[BUFF_DATA_START + 1] = MIFARE_CMD_WRITE;
        buff[BUFF_DATA_START + 2] = block; /* current block */
        memcpy(&buff[BUFF_DATA_START + 3], idata, MIFARE_CLASSIC_BLOCK_SIZE);

        if (send_rcv(dev, buff, 19, 1) == 1) {
            ret = buff[0];
        }

    }
    return ret;
}

static int pn532_mifare_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
                             unsigned block, unsigned len)
{
    int ret = -1;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
    buff[BUFF_DATA_START    ] = card->target;
    buff[BUFF_DATA_START + 1] = MIFARE_CMD_READ;
    buff[BUFF_DATA_START + 2] = block; /* current block */

    if (send_rcv(dev, buff, 3, len + 1) == (int)(len + 1)) {
        memcpy(odata, &buff[1], len);
        ret = 0;
    }

    return ret;
}

int pn532_mifareclassic_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
                             unsigned block)
{
    if (card->auth) {
        return pn532_mifare_read(dev, odata, card, block, MIFARE_CLASSIC_BLOCK_SIZE);
    }
    else {
        return -1;
    }
}

int pn532_mifareulight_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
                            unsigned page)
{
    return pn532_mifare_read(dev, odata, card, page, 32);
}

static int send_rcv_apdu(pn532_t *dev, uint8_t *buff, unsigned slen, unsigned rlen)
{
    int ret;

    rlen += 3;
    if ((rlen >= RAPDU_MAX_DATA_LEN) || (slen >= CAPDU_MAX_DATA_LEN)) {
        return -1;
    }

    ret = send_rcv(dev, buff, slen, rlen);
    if ((ret == (int)rlen) && (buff[0] == 0x00)) {
        ret = (buff[rlen - 2] << 8) | buff[rlen - 1];
        if (ret == (int)0x9000) {
            ret = 0;
        }
    }

    return ret;
}

int pn532_iso14443a_4_activate(pn532_t *dev, nfc_iso14443a_t *card)
{
    int ret;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    /* select app ndef tag */
    buff[BUFF_CMD_START      ] = CMD_DATA_EXCHANGE;
    buff[BUFF_DATA_START     ] = card->target;
    buff[BUFF_DATA_START +  1] = 0x00;
    buff[BUFF_DATA_START +  2] = 0xa4;
    buff[BUFF_DATA_START +  3] = 0x04;
    buff[BUFF_DATA_START +  4] = 0x00;
    buff[BUFF_DATA_START +  5] = 0x07;
    buff[BUFF_DATA_START +  6] = 0xD2;
    buff[BUFF_DATA_START +  7] = 0x76;
    buff[BUFF_DATA_START +  8] = 0x00;
    buff[BUFF_DATA_START +  9] = 0x00;
    buff[BUFF_DATA_START + 10] = 0x85;
    buff[BUFF_DATA_START + 11] = 0x01;
    buff[BUFF_DATA_START + 12] = 0x01;
    buff[BUFF_DATA_START + 13] = 0x00;

    DEBUG("pn532: select app\n");
    ret = send_rcv_apdu(dev, buff, 14, 0);

    /* select ndef file */
    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
    buff[BUFF_DATA_START    ] = card->target;
    buff[BUFF_DATA_START + 1] = 0x00;
    buff[BUFF_DATA_START + 2] = 0xa4;
    buff[BUFF_DATA_START + 3] = 0x00;
    buff[BUFF_DATA_START + 4] = 0x0c;
    buff[BUFF_DATA_START + 5] = 0x02;
    buff[BUFF_DATA_START + 6] = 0x00;
    buff[BUFF_DATA_START + 7] = 0x01;

    if (ret == 0) {
        DEBUG("pn532: select file\n");
        ret = send_rcv_apdu(dev, buff, 8, 0);
    }

    return ret;
}

int pn532_iso14443a_4_read(pn532_t *dev, uint8_t *odata, nfc_iso14443a_t *card,
                           unsigned offset, uint8_t len)
{
    int ret;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
    buff[BUFF_DATA_START    ] = card->target;
    buff[BUFF_DATA_START + 1] = 0x00;
    buff[BUFF_DATA_START + 2] = 0xb0;
    buff[BUFF_DATA_START + 3] = (offset >> 8) & 0xff;
    buff[BUFF_DATA_START + 4] = offset & 0xff;
    buff[BUFF_DATA_START + 5] = len;

    ret = send_rcv_apdu(dev, buff, 6, len);
    if (ret == 0) {
        memcpy(odata, &buff[RAPDU_DATA_BEGIN], len);
    }

    return ret;
}

int change_rf_field(pn532_t *dev, bool on) {
    uint8_t command[] = {RF_CONFIGURATION, 0x01, (on) ? 0x01 : 0x00};
    return send_rcv(dev, command, 2, 0);
}

static void _load_fifo_data(pn532_t *dev, const uint8_t *data, unsigned len) {
    /* clear FIFO data */
    pn532_write_reg(dev, PN532_REG_CIU_FIFOLevel, 0x80);

    for (unsigned i = 0; i < len; i++) {
        pn532_write_reg(dev, PN532_REG_CIU_FIFOData, data[i]);
    }
}

static void _read_fifo_data(pn532_t *dev, uint8_t *data, unsigned len) {
    for (unsigned i = 0; i < len; i++) {
        pn532_read_reg(dev, &(data[i]), PN532_REG_CIU_FIFOData);
    }
}


int _init_as_target_nfc_f(pn532_t *dev, uint8_t *nfc_f_params) {
    pn532_write_reg(dev, PN532_REG_CIU_Command, 0x00);

    /* set NFC F rf config */
    _rf_configure(dev, 0x0B, nfc_f_rf_config, 8);

    /* load the NFC F params into the FIFO buffer */
    uint8_t params_buffer[25] = {0};
    memcpy(params_buffer + 2 + 3 + 1, nfc_f_params, 18);


    pn532_write_reg(dev, PN532_REG_CIU_Command, 0x00); /* idle command */
    pn532_write_reg(dev, PN532_REG_CIU_FIFOLevel, 0b10000000); /* clear fifo */
    _load_fifo_data(dev, params_buffer, 25); /* write params into fifo */

    pn532_write_reg(dev, PN532_REG_CIU_Command, 0b00000001); /* configure command */

    pn532_write_reg(dev, PN532_REG_CIU_Control,   0b00000000);
    pn532_write_reg(dev, PN532_REG_CIU_Mode,      0b00111111);
    pn532_write_reg(dev, PN532_REG_CIU_FelNFC2,   0b10000000);
    pn532_write_reg(dev, PN532_REG_CIU_TxMode,    0b10010010); /* this must be changed based on the bitrate*/
    pn532_write_reg(dev, PN532_REG_CIU_RxMode,    0b10011010); /* this must be changed based on the bitrate */
    pn532_write_reg(dev, PN532_REG_CIU_TxControl, 0b10000000);
    pn532_write_reg(dev, PN532_REG_CIU_TxAuto,    0b00100000);
    pn532_write_reg(dev, PN532_REG_CIU_Demod,     0b01100001);
    pn532_write_reg(dev, PN532_REG_CIU_CommIrq,   0b01111111);
    pn532_write_reg(dev, PN532_REG_CIU_DivIrq,    0b01111111);
    pn532_write_reg(dev, PN532_REG_CIU_Command,   0b00001101);

    uint8_t commirq, status_1, status_2, divirq;
    while (true) {
        ztimer_sleep(ZTIMER_MSEC, 10);
        pn532_read_reg(dev, &commirq,  PN532_REG_CIU_CommIrq);
        pn532_read_reg(dev, &status_1, PN532_REG_CIU_Status1);
        pn532_read_reg(dev, &status_2, PN532_REG_CIU_Status2);
        pn532_read_reg(dev, &divirq,   PN532_REG_CIU_DivIrq);
        DEBUG("pn532: CIU comm irq %02x\n", commirq);
        if ((commirq & 0b00110000) == 0b00110000) {
            pn532_write_reg(dev, PN532_REG_CIU_CommIrq, 0b00110000); /* clear IRQ */
            uint8_t fifo_size;
            pn532_read_reg(dev, &fifo_size, PN532_REG_CIU_FIFOLevel);

            uint8_t fifo_data[128] = {0};
            _read_fifo_data(dev, fifo_data, fifo_size);


            if (fifo_size == 0) {
                return -1; /* no data in FIFO */
            }

            DEBUG("pn532: fifo size is %d\n", fifo_size);
            if (fifo_size == fifo_data[0]) {
                DEBUG("pn532: fifo data is ");
                PRINTBUFF(fifo_data, fifo_size);

                return 0; /* success */
            
            }
            pn532_write_reg(dev, PN532_REG_CIU_Command, 0b00001101); /* restart command */
        }
        
    }
}

static int _init_as_target(pn532_t *dev, uint8_t mode,
    uint8_t *mifare_params, uint8_t *felica_params, uint8_t *nfcid3t, uint8_t *buff) {
    assert(dev != NULL);
    assert(buff != NULL);

    DEBUG("pn532: setting CIU Mode\n");
    int ret = pn532_write_reg(dev, PN532_REG_CIU_Mode, 0b00111111);
    if (ret != 0) {
        return ret;
    }

    buff[BUFF_CMD_START] = CMD_INIT_AS_TARGET;

    /* target mode */
    buff[BUFF_DATA_START] = mode;
    
    if (mifare_params != NULL) {
        _rf_configure(dev, 0x0A, nfc_a_rf_config, sizeof(nfc_a_rf_config));
        memcpy(&buff[BUFF_DATA_START + 1], mifare_params, 6);
    }

    if (felica_params != NULL) {
        memcpy(&buff[BUFF_DATA_START + 1 + 6], felica_params, 18);
    }

    if (nfcid3t != NULL) {
        memcpy(&buff[BUFF_DATA_START + 1 + 6 + 18], nfcid3t, 10);
    }
    /* AutoRFCA: on, RFCA: off
    uint8_t enable_auto_rfca = 0b00000010;
    _rf_configure(dev, 0x01, &enable_auto_rfca, 1);  */

    DEBUG("pn532: init as target mode %i\n", mode);
    /* recv len depends on the technology used */

    return send_rcv(dev, buff, 1 + 6 + 18 + 10 + 2, 20);
}

void pn532_set_parameters(pn532_t *dev, uint8_t flags) {
    assert(dev != NULL);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START] = CMD_SET_PARAMETERS;
    buff[BUFF_DATA_START] = flags;

    send_rcv(dev, buff, 1, 1);
}

void pn532_set_power_down(pn532_t *dev, uint8_t wake_up_enable, uint8_t generate_irq) {
    assert(dev != NULL);
    DEBUG("pn532: setting power down\n");

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_POWER_DOWN;
    buff[BUFF_DATA_START    ] = wake_up_enable;
    buff[BUFF_DATA_START + 1] = generate_irq;

    send_rcv(dev, buff, 2, 0);
}

uint8_t pn532_get_target_status(pn532_t *dev) {
    assert(dev != NULL);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    uint8_t status = 0xff;
    buff[BUFF_CMD_START] = CMD_GET_TARGET_STATUS;
    if (send_rcv(dev, buff, 0, 2) == 2) {
        status = buff[0];
        /* discard baud rate in byte 2 */
    }

    return status;
}

/* int pn532_init_tag(pn532_t *dev, nfc_application_type_t app_type) {
    assert(dev != NULL);

    if (app_type == NFC_APPLICATION_TYPE_T2T) {
        DEBUG("pn532: init target as T2T\n");
        uint8_t mifare_params[] = {
            0x00, 0x00, 0x58, 0xC8, 0xB5, 0x00
        };
        return _init_as_target(dev, 0b00000000, mifare_params, NULL, NULL);
    } else if (app_type == NFC_APPLICATION_TYPE_T3T) {
        DEBUG("pn532: init target as T3T\n");
        uint8_t felica_params[] = {
            0x02, 0xFE, 0x04, 0x04, 0x05, 0x06, 0x07, 0x08,
            0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
            0x12, 0xFC 
        };
        return _init_as_target_nfc_f(dev, felica_params);
    }

    return -1;
} */

int pn532_listen_a(nfcdev_t *nfcdev, const nfc_a_listener_config_t *config) {
    DEBUG("pn532: init target as NFC-A\n");
    assert(nfcdev != NULL);
    assert(config != NULL);

    assert(config->nfcid1.len == 4); /* only 4 byte nfcid1 supported */
    assert(config->nfcid1.nfcid[0] == 0x08); /* the first byte must be 0x08 for the PN532*/

    /* the NFC-A params are called Mifare params in the PN532 manual */
    uint8_t mifare_params[6] = {0};  /* SENS_RES, NFCID1t, SEL_RES */

    mifare_params[0] = config->sens_res.anticollision_information;
    mifare_params[1] = config->sens_res.platform_information;

    mifare_params[2] = config->nfcid1.nfcid[1];
    mifare_params[3] = config->nfcid1.nfcid[2];
    mifare_params[4] = config->nfcid1.nfcid[3];

    mifare_params[5] = config->sel_res;

    uint8_t mode = 0x00;
    uint8_t buff[CONFIG_PN532_BUFFER_LEN] = {0};
    if (config->sel_res && NFC_A_SEL_RES_T4T_VALUE) {
        /* we enable automatic handeling of NFC-DEP for T4T */
        pn532_set_parameters((pn532_t *) nfcdev->dev, 0b00100000); /* enable NFC-DEP */
        mode &= 0b00000100;
    } else {
        pn532_set_parameters((pn532_t *) nfcdev->dev, 0b00000000); /* disable NFC-DEP */
    }


    int ret = _init_as_target((pn532_t *) nfcdev->dev, mode, mifare_params, NULL, NULL, buff);
    if (ret <= 1) {
        return ret;
    }

    pn532_t *dev = (pn532_t *) nfcdev->dev;

    /* the buff now contains the first command we received from the initiator */
    if (0b00001000 & buff[0]) {
        /* The PN532 is initialized as ISO14443A-4 target (NFC-DEP), the buffer should
         * contain the RATS sent by the initiator. We don't need to save it inside the
         * initiator buffer. Therefore, we do nothing.
         */
        dev->iso_dep = true;
    } else if ((0b00001000 & buff[0]) == 0x00) {
        /* We should be a passive target. Potentially a T2T or a proprietary tag. */
        dev->initiator_command_len = ret - 1;
        assert(dev->initiator_command_len <= CONFIG_PN532_INITIATOR_COMMAND_BUFFER_LEN);
        memcpy(dev->initiator_command, &buff[1], dev->initiator_command_len);
        dev->iso_dep = false;
    }

    return 0;
}

/* Polls for an NFC-A tag */
int pn532_poll_a(nfcdev_t *nfcdev, nfc_a_listener_config_t *config) {
    assert(nfcdev != NULL);
    assert(config != NULL);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    _list_passive_targets(nfcdev->dev, buff, PN532_BR_106_ISO_14443_A, 1,
                         LIST_PASSIVE_LEN_14443(1));

    if (buff[0] != 1) {
        LOG_DEBUG("pn532: error during polling\n");
        return NFC_ERR_POLL_NO_TARGET;
    }

    config->sens_res.anticollision_information = buff[2];
    config->sens_res.platform_information      = buff[3];
    config->sel_res = buff[4];
    config->nfcid1.len = buff[5];
    memcpy(config->nfcid1.nfcid, &buff[6], config->nfcid1.len);

    // uint8_t *target_data = buff[1];
    return 0;
}

/* Polls for targets in the area independent of technology */
int pn532_poll(nfcdev_t *nfcdev, nfc_listener_config_t *config) {
    assert(nfcdev != NULL);
    assert(config != NULL);

    nfc_a_listener_config_t a_config;
    nfc_b_listener_config_t b_config;
    nfc_f_listener_config_t f_config;

    if (0 == pn532_poll_a(nfcdev, &a_config)) {
        config->technology = NFC_TECHNOLOGY_A;
        memcpy(&config->config.a, &a_config, sizeof(nfc_a_listener_config_t));
    } else if (0 == pn532_poll_b(nfcdev, &b_config)) {
        config->technology = NFC_TECHNOLOGY_B;
        memcpy(&config->config.b, &b_config, sizeof(nfc_b_listener_config_t));
    } else if (0 == pn532_poll_f(nfcdev, &f_config)) {
        config->technology = NFC_TECHNOLOGY_F;
        memcpy(&config->config.f, &f_config, sizeof(nfc_f_listener_config_t));
    } else {
        return NFC_ERR_POLL_NO_TARGET;
    }

    return 0;
}

/* Polls for an NFC-B tag */
int pn532_poll_b(nfcdev_t *nfcdev, nfc_b_listener_config_t *config) {
    assert(nfcdev != NULL);
    assert(config != NULL);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    _list_passive_targets(nfcdev->dev, buff, PN532_BR_106_ISO_14443_B, 1,
                         21);

    if (buff[0] != 1) {
        LOG_DEBUG("pn532: error during polling\n");
        return NFC_ERR_POLL_NO_TARGET;
    }

    memcpy(&(config->sensb_res.nfcid0), &buff[2], NFC_B_NFCID0_LEN);
    memcpy(&(config->sensb_res.application_data), &buff[2 + NFC_B_NFCID0_LEN], NFC_B_APP_DATA_LEN);
    memcpy(&(config->sensb_res.protocol_info), &buff[2 + NFC_B_NFCID0_LEN + NFC_B_APP_DATA_LEN],
           NFC_B_PROT_INFO_LEN);

    // uint8_t *target_data = buff[1];
    return 0;
}

int pn532_poll_f(nfcdev_t *nfcdev, nfc_f_listener_config_t *config) {
    assert(nfcdev != NULL);
    assert(config != NULL);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    _list_passive_targets(nfcdev->dev, buff, PN532_BR_212_FELICA, 1, 21);

    if (buff[0] != 1) {
        LOG_DEBUG("pn532: error during polling\n");
        return -1;
    }

    memcpy(config->sensf_res.nfcid2, &buff[5], NFC_F_NFCID2_LEN);

    // uint8_t *target_data = buff[1];
    return 0;
}

int pn532_mifare_classic_authenticate(nfcdev_t *nfcdev, uint8_t block, 
    const nfc_a_nfcid1_t *nfcid1, bool is_key_a, const uint8_t *key) {
    assert(nfcdev != NULL);
    assert(key != NULL);
    assert(nfcid1 != NULL);
    assert(nfcid1->len == NFC_A_NFCID1_LEN4);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
    buff[BUFF_DATA_START    ] = 1; /* tg number must always be 1 */
    buff[BUFF_DATA_START + 1] = is_key_a ? MIFARE_CLASSIC_AUTH_A : MIFARE_CLASSIC_AUTH_B;
    buff[BUFF_DATA_START + 2] = block; /* current block */


    memcpy(&buff[BUFF_DATA_START + 3], key, 6);

    memcpy(&buff[BUFF_DATA_START + 9], nfcid1->nfcid, nfcid1->len);

    int ret_len = send_rcv((pn532_t *) nfcdev->dev, buff, 9 + nfcid1->len, 1);
    if (ret_len > 0) {
        if (buff[0] != 0x00) {
            /* error */
            DEBUG("pn532: error in mifare classic auth %02x\n", buff[0]);
            return NFC_ERR_AUTH;
        }
        return 0;
    } else {
        return NFC_ERR_AUTH;
    }
}

int pn532_initiator_exchange_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len,
                                  uint8_t *rcv, size_t *receive_len) {
    assert(nfcdev != NULL);
    assert(nfcdev->dev != NULL);
    assert(BUFF_DATA_START + 1 + send_len <= CONFIG_PN532_BUFFER_LEN);
    assert(receive_len != NULL);

    *receive_len = 0;

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];

    buff[BUFF_CMD_START     ] = CMD_DATA_EXCHANGE;
    buff[BUFF_DATA_START    ] = 1; /* tg number must always be 1 */

    memcpy(&buff[BUFF_DATA_START + 1], send, send_len);
    int ret_len = send_rcv(nfcdev->dev, buff, send_len + 1, CONFIG_PN532_BUFFER_LEN - 8);

    /* receive_len is only the data received, not the status byte */

    if (ret_len > 0) {
        *receive_len = ret_len - 1;
        if (buff[0] != 0x00) {
            /* error */
            DEBUG("pn532: error in data exchange %02x\n", buff[0]);
            *receive_len = 0;
            return -1;
        }

        DEBUG("pn532: received %u bytes\n", *receive_len);
        /* copy the data into the receive buffer, excluding the status byte */
        memcpy(rcv, buff + 1, *receive_len);
    }



    return 0;
}

/* returns the length of bytes sent */
static int _tg_response_to_initiator(pn532_t *dev, const uint8_t *send, size_t send_len) {
    DEBUG("pn532: response to initiator\n");
    assert (send != NULL);
    assert (send_len > 0);
    assert(BUFF_DATA_START + send_len <= CONFIG_PN532_BUFFER_LEN);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    buff[BUFF_CMD_START] = CMD_RESPONSE_TO_INITIATOR;

    memcpy(&buff[BUFF_DATA_START], send, send_len);

    int ret_len = send_rcv(dev, buff, send_len, 0);
    if (ret_len != 1) {
        return NFC_ERR_COMMUNICATION;
    }

    if (buff[0] != 0x00) {
        DEBUG("pn532: error in response to initiator %02x\n", buff[0]);
        return NFC_ERR_COMMUNICATION;
    }

    return ret_len;
}

/* this is used to receive */
static int _tg_get_initiator_command(pn532_t *dev, uint8_t *rcv, size_t *receive_len) {
    DEBUG("pn532: get initiator command\n");
    assert(rcv != NULL);
    assert(receive_len != NULL);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    buff[BUFF_CMD_START] = CMD_GET_INITIATOR_COMMAND;

    int ret_len = send_rcv(dev, buff, 0, CONFIG_PN532_BUFFER_LEN - 8);

    if (ret_len <= 1) {
        *receive_len = 0;
        return NFC_ERR_COMMUNICATION;
    }

    if (buff[0] != 0x00) {
        DEBUG("pn532: error in get initiator command %02x\n", buff[0]);
        *receive_len = 0;
        return NFC_ERR_COMMUNICATION;
    }

    *receive_len = ret_len - 1;
    memcpy(rcv, buff + 1, *receive_len);

    return ret_len;
}

/* for NFC-DEP and ISO-DEP */
static int _tg_get_data(pn532_t *dev, uint8_t *rcv, size_t *receive_len) {
    DEBUG("pn532: get data\n");
    assert(dev != NULL);
    assert(rcv != NULL);
    assert(receive_len != NULL);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    buff[BUFF_CMD_START] = CMD_GET_DATA;

    int ret_len = send_rcv(dev, buff, 0, CONFIG_PN532_BUFFER_LEN - 8);

    if (ret_len <= 1) {
        *receive_len = 0;
        return NFC_ERR_COMMUNICATION;
    }

    if (buff[0] != 0x00) {
        DEBUG("pn532: error in get data %02x\n", buff[0]);
        *receive_len = 0;
        return NFC_ERR_COMMUNICATION;
    }

    *receive_len = ret_len - 1;
    memcpy(rcv, buff + 1, *receive_len);

    return ret_len;
}

/* for NFC-DEP and ISO-DEP */
static int _tg_set_data(pn532_t *dev, const uint8_t *send, size_t send_len) {
    DEBUG("pn532: set data\n");
    assert(dev != NULL);
    assert(send != NULL);
    assert(send_len > 0);
    assert(BUFF_DATA_START + send_len <= CONFIG_PN532_BUFFER_LEN);

    uint8_t buff[CONFIG_PN532_BUFFER_LEN];
    buff[BUFF_CMD_START] = CMD_SET_DATA;

    memcpy(&buff[BUFF_DATA_START], send, send_len);

    int ret_len = send_rcv(dev, buff, send_len, 0);
    if (ret_len != 1) {
        return NFC_ERR_COMMUNICATION;
    }

    if (buff[0] != 0x00) {
        DEBUG("pn532: error in send data %02x\n", buff[0]);
        return NFC_ERR_COMMUNICATION;
    }

    return ret_len;
}

int pn532_target_send_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len) {
    assert(nfcdev != NULL);
    assert(nfcdev->dev != NULL);

    if (((pn532_t *) nfcdev->dev)->iso_dep) {
        return _tg_set_data(nfcdev->dev, send, send_len);
    } else {
        return _tg_response_to_initiator(nfcdev->dev, send, send_len);
    }
}

int pn532_target_receive_data(nfcdev_t *nfcdev, uint8_t *rcv, size_t *receive_len) {
    assert(nfcdev != NULL);
    assert(nfcdev->dev != NULL);
    assert(rcv != NULL);
    assert(receive_len != NULL);

    /* the PN532 init as target command returns after receiving the first command */
    pn532_t *dev = (pn532_t *) nfcdev->dev;

    if (dev->initiator_command_len > 0) {
        assert(dev->initiator_command_len <= CONFIG_PN532_INITIATOR_COMMAND_BUFFER_LEN);
        memcpy(rcv, dev->initiator_command, dev->initiator_command_len);
        *receive_len = dev->initiator_command_len;
        dev->initiator_command_len = 0; /* clear the buffer */
        return *receive_len;
    }

    if (dev->iso_dep) {
        return _tg_get_data(nfcdev->dev, rcv, receive_len);
    } else {
        return _tg_get_initiator_command(nfcdev->dev, rcv, receive_len);
    }
}
