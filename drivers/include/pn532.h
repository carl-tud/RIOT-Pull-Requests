/*
 * SPDX-FileCopyrightText: 2016 TriaGnoSys GmbH
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#pragma once

/**
 * @defgroup    drivers_pn532 PN532 NFC Reader
 * @ingroup     drivers_netdev
 * @brief       PN532 NFC radio device driver
 *
 * @{
 * @file
 * @brief       PN532 driver
 *
 * @author      Víctor Ariño <victor.arino@triagnosys.com>
 */

#ifdef __cplusplus
extern "C" {
#endif

#include "kernel_defines.h"
#include "mutex.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "periph/gpio.h"
#include <stdint.h>

#include "net/nfc/nfc.h"

#if !IS_USED(MODULE_PN532_I2C) && !IS_USED(MODULE_PN532_SPI)
#error Please use either pn532_i2c and/or pn532_spi module to enable \
    the functionality on this device
#endif

/**
 * @brief   Data structure with the configuration parameters
 */
typedef struct {
    union {
#if IS_USED(MODULE_PN532_I2C) || DOXYGEN
        i2c_t i2c;              /**< I2C device */
#endif
#if IS_USED(MODULE_PN532_SPI) || DOXYGEN
        spi_t spi;              /**< SPI device */
#endif
    };
    gpio_t reset;               /**< Reset pin */
    gpio_t irq;                 /**< Interrupt pin */
#if IS_USED(MODULE_PN532_SPI) || DOXYGEN
    gpio_t nss;                 /**< Chip Select pin (only SPI) */
#endif
} pn532_params_t;

/**
 * @brief   Working mode of the PN532
 */
typedef enum {
    PN532_I2C,
    PN532_SPI
} pn532_mode_t;

/**
 * @brief   Device descriptor for the PN532
 */
typedef struct {
    const pn532_params_t *conf;     /**< Configuration struct */
    pn532_mode_t mode;              /**< Working mode (i2c, spi) */
    mutex_t trap;                   /**< Mutex to wait for chip response */
} pn532_t;

/**
 * @defgroup drivers_pn532_config     PN532 NFC Radio driver compile configuration
 * @ingroup config_drivers_netdev
 * @{
 */
/**
 * @brief   Internal buffer size
 *
 * @note A small buffer size is enough for most applications, however if large NDEF
 * files are to be written this size shall be increased. Otherwise the files
 * can be written in chunks.
 */
#ifndef CONFIG_PN532_BUFFER_LEN
#define CONFIG_PN532_BUFFER_LEN     (64)
#endif
/** @} */

/**
 * @name    Helpers to extract firmware information from word
 * @{
 */
#define PN532_IC_VERSION(fwver)  ((fwver >> 24) & 0xff)
#define PN532_FW_VERSION(fwver)  ((fwver >> 16) & 0xff)
#define PN532_FW_REVISION(fwver) ((fwver >>  8) & 0xff)
#define PN532_FW_FEATURES(fwver) ((fwver) & 0xff)
/** @} */

#define PN532_PARAM_NAD_USED              (0x01) 
#define PN532_PARAM_DID_USED              (0x02) 
#define PN532_PARAM_AUTOMATIC_ATR_RES     (0x04) 
#define PN532_PARAM_AUTOMATIC_RATS        (0x10)
#define PN532_PARAM_ISO14443A_4_PICC      (0x20) 
#define PN532_PARAM_REMOVE_PRE_POST_AMBLE (0x40)

// Register addresses
#define PN532_REG_Control_switch_rng 0x6106
#define PN532_REG_CIU_Mode 0x6301
#define PN532_REG_CIU_TxMode 0x6302
#define PN532_REG_CIU_RxMode 0x6303
#define PN532_REG_CIU_TxControl 0x6304
#define PN532_REG_CIU_TxAuto 0x6305
#define PN532_REG_CIU_TxSel 0x6306
#define PN532_REG_CIU_RxSel 0x6307
#define PN532_REG_CIU_RxThreshold 0x6308
#define PN532_REG_CIU_Demod 0x6309
#define PN532_REG_CIU_FelNFC1 0x630A
#define PN532_REG_CIU_FelNFC2 0x630B
#define PN532_REG_CIU_MifNFC 0x630C
#define PN532_REG_CIU_ManualRCV 0x630D
#define PN532_REG_CIU_TypeB 0x630E
// #define PN532_REG_- 0x630F
// #define PN532_REG_- 0x6310
#define PN532_REG_CIU_CRCResultMSB 0x6311
#define PN532_REG_CIU_CRCResultLSB 0x6312
#define PN532_REG_CIU_GsNOFF 0x6313
#define PN532_REG_CIU_ModWidth 0x6314
#define PN532_REG_CIU_TxBitPhase 0x6315
#define PN532_REG_CIU_RFCfg 0x6316
#define PN532_REG_CIU_GsNOn 0x6317
#define PN532_REG_CIU_CWGsP 0x6318
#define PN532_REG_CIU_ModGsP 0x6319
#define PN532_REG_CIU_TMode 0x631A
#define PN532_REG_CIU_TPrescaler 0x631B
#define PN532_REG_CIU_TReloadVal_hi 0x631C
#define PN532_REG_CIU_TReloadVal_lo 0x631D
#define PN532_REG_CIU_TCounterVal_hi 0x631E
#define PN532_REG_CIU_TCounterVal_lo 0x631F
// #define PN532_REG_- 0x6320
#define PN532_REG_CIU_TestSel1 0x6321
#define PN532_REG_CIU_TestSel2 0x6322
#define PN532_REG_CIU_TestPinEn 0x6323
#define PN532_REG_CIU_TestPinValue 0x6324
#define PN532_REG_CIU_TestBus 0x6325
#define PN532_REG_CIU_AutoTest 0x6326
#define PN532_REG_CIU_Version 0x6327
#define PN532_REG_CIU_AnalogTest 0x6328
#define PN532_REG_CIU_TestDAC1 0x6329
#define PN532_REG_CIU_TestDAC2 0x632A
#define PN532_REG_CIU_TestADC 0x632B
// #define PN532_REG_- 0x632C
// #define PN532_REG_- 0x632D
// #define PN532_REG_- 0x632E
#define PN532_REG_CIU_RFlevelDet 0x632F
#define PN532_REG_CIU_SIC_CLK_en 0x6330
#define PN532_REG_CIU_Command 0x6331
#define PN532_REG_CIU_CommIEn 0x6332
#define PN532_REG_CIU_DivIEn 0x6333
#define PN532_REG_CIU_CommIrq 0x6334
#define PN532_REG_CIU_DivIrq 0x6335
#define PN532_REG_CIU_Error 0x6336
#define PN532_REG_CIU_Status1 0x6337
#define PN532_REG_CIU_Status2 0x6338
#define PN532_REG_CIU_FIFOData 0x6339
#define PN532_REG_CIU_FIFOLevel 0x633A
#define PN532_REG_CIU_WaterLevel 0x633B
#define PN532_REG_CIU_Control 0x633C
#define PN532_REG_CIU_BitFraming 0x633D
#define PN532_REG_CIU_Coll 0x633E

#define PN532_SFR_P3 0xFFB0

#define PN532_SFR_P3CFGA 0xFFFC
#define PN532_SFR_P3CFGB 0xFFFD
#define PN532_SFR_P7CFGA 0xFFF4
#define PN532_SFR_P7CFGB 0xFFF5
#define PN532_SFR_P7 0xFFF7

/**
 * @brief   Possible SAM configurations
 */
typedef enum {
    PN532_SAM_NORMAL = 1,
    PN532_SAM_VIRTUAL,
    PN532_SAM_WIRED,
    PN532_SAM_DUAL
} pn532_sam_conf_mode_t;

/**
 * @brief   PN532 supported targets
 */
typedef enum {
    PN532_BR_106_ISO_14443_A = 0,
    PN532_BR_212_FELICA,
    PN532_BR_424_FELICA,
    PN532_BR_106_ISO_14443_B,
    PN532_BR_106_JEWEL
} pn532_target_t;

/**
 * @brief   ISO14443A Card types
 */
typedef enum {
    ISO14443A_UNKNOWN,
    ISO14443A_MIFARE,
    ISO14443A_TYPE4
} nfc_iso14443a_type_t;

/**
 * @brief   ISO14443A tag description
 */
typedef struct {
    char target;                /**< Target */
    char auth;                  /**< Card has been authenticated. Do not modify manually */
    char id_len;                /**< Length of the ID field */
    char sel_res;               /**< SEL_RES */
    unsigned sns_res;           /**< SNS_RES */
    nfc_iso14443a_type_t type;  /**< Type of ISO14443A card */
    char id[8];                 /**< Card ID (length given by id_len) */
} nfc_iso14443a_t;

/**
 * @brief   Mifare keys
 */
typedef enum {
    PN532_MIFARE_KEY_A = 0x60,
    PN532_MIFARE_KEY_B = 0x61
} pn532_mifare_key_t;

typedef union {
    uint8_t mifare_params[6];
    uint8_t felica_params[18];
    uint8_t nfcid3t[10];
} pn532_target_params_t;

/**
 * @brief   Obtain Tag 4 data length from buffer
 *
 * This is useful in case the length has been read and one intents to read the
 * data.
 */
#define PN532_ISO14443A_4_LEN_FROM_BUFFER(b) ((b[0] << 8) | b[1])

/**
 * @brief   Hard reset the chipset
 *
 * The chipset is reset by toggling the reset pins
 *
 * @param[in]  dev          target device
 *
 */
void pn532_reset(const pn532_t *dev);

/**
 * @brief   Initialize the module and peripherals
 *
 * This is the first method to be called in order to interact with the pn532.
 * It configures the GPIOs and the i2c/spi interface (depending on @p mode).
 *
 *  @param[in]  dev         target device
 *  @param[in]  params      configuration parameters
 *  @param[in]  mode        initialization mode
 *
 * @return                  0 on success
 * @return                  <0 i2c/spi initialization error, the value is given
 *                          by the i2c/spi init method.
 */
int pn532_init(pn532_t *dev, const pn532_params_t *params, pn532_mode_t mode);

#if IS_USED(MODULE_PN532_I2C) || DOXYGEN
/**
 * @brief   Initialization of PN532 using i2c
 *
 * @see pn532_init for parameter and return value details
 * @note Use `pn532_i2c` module to use this function.
 */
static inline int pn532_init_i2c(pn532_t *dev, const pn532_params_t *params)
{
    return pn532_init(dev, params, PN532_I2C);
}
#endif

#if IS_USED(MODULE_PN532_SPI) || DOXYGEN
/**
 * @brief   Initialization of PN532 using spi
 *
 * @see pn532_init for parameter and return value details
 * @note Use `pn532_spi` module to use this function.
 */
static inline int pn532_init_spi(pn532_t *dev, const pn532_params_t *params)
{
    return pn532_init(dev, params, PN532_SPI);
}
#endif

/**
 * @brief   Get the firmware version of the pn532
 *
 * The firmware version returned is a 4 byte long value:
 *  - ic version,
 *  - fw version,
 *  - fw revision
 *  - feature support
 *
 * @param[in]  dev          target device
 * @param[out] fw_ver       encoded firmware version
 *
 * @return                  0 on success
 */
int pn532_fw_version(pn532_t *dev, uint32_t *fw_ver);

/**
 * @brief   Read register of the pn532
 *
 * Refer to the datasheet for a comprehensive list of registers and meanings.
 * For SFR registers the high byte must be set to 0xff.
 *
 * http://www.nxp.com/documents/user_manual/141520.pdf
 *
 * @param[in]  dev          target device
 * @param[out] out          value of the register
 * @param[in]  addr         address of the register to read
 *
 * @return                  0 on success
 */
int pn532_read_reg(pn532_t *dev, char *out, unsigned addr);

/**
 * @brief   Write register of the pn532
 *
 * Refer to the datasheet for a comprehensive list of registers and meanings.
 *
 * http://www.nxp.com/documents/user_manual/141520.pdf
 *
 * @param[in]  dev          target device
 * @param[in]  addr         address of the register to read
 * @param[in]  val          value to write in the register
 *
 * @return                  0 on success
 */
int pn532_write_reg(pn532_t *dev, unsigned addr, char val);

int pn532_update_reg(pn532_t *dev, unsigned addr, char val, char mask);

/**
 * @brief   Set new settings for the Security Access Module
 *
 * @param[in]  dev          target device
 * @param[in]  mode         new mode for the SAM
 * @param[in]  timeout      timeout for Virtual Card mode with LSB of 50ms.
 *                          (0 = infinite and 0xFF = 12.75s)
 *
 * @return                  0 on success
 */
int pn532_sam_configuration(pn532_t *dev, pn532_sam_conf_mode_t mode, unsigned timeout);

/**
 * @brief   Get one ISO14443-A passive target
 *
 * This method blocks until a target is detected.
 *
 * @param[in]  dev          target device
 * @param[out] out          target to be stored
 * @param[in]  max_retries  maximum number of attempts to activate a target
 *                          (0xff blocks indefinitely)
 *
 * @return                  0 on success
 * @return                  -1 when no card detected (if non blocking)
 */
int pn532_get_passive_iso14443a(pn532_t *dev, nfc_iso14443a_t *out, unsigned max_retries);

/**
 * @brief   Authenticate a Mifare classic card
 *
 * This operation must be done before reading or writing the segment.
 *
 * @param[in]  dev          target device
 * @param[in]  card         card to use
 * @param[in]  keyid        which key to use
 * @param[in]  key          buffer containing the key
 * @param[in]  block        which block to authenticate
 *
 * @return                  0 on success
 */
int pn532_mifareclassic_authenticate(pn532_t *dev, nfc_iso14443a_t *card,
                                     pn532_mifare_key_t keyid, char *key, unsigned block);

/**
 * @brief   Read a block of a Mifare classic card
 *
 * The block size is 16 bytes and it must be authenticated before read.
 *
 * @param[in]  dev          target device
 * @param[out] odata        buffer where to store the data
 * @param[in]  card         card to use
 * @param[in]  block        which block to read
 *
 * @return                  0 on success
 */
int pn532_mifareclassic_read(pn532_t *dev, char *odata, nfc_iso14443a_t *card, unsigned block);

/**
 * @brief   Write a block of a Mifare classic card
 *
 * The block size is 16 bytes and it must be authenticated before written.
 *
 * @param[in]  dev          target device
 * @param[in]  idata        buffer containing the data to write
 * @param[in]  card         card to use
 * @param[in]  block        which block to write to
 *
 * @return                  0 on success
 */
int pn532_mifareclassic_write(pn532_t *dev, char *idata, nfc_iso14443a_t *card, unsigned block);

/**
 * @brief   Read a block of a Mifare Ultralight card
 *
 * The block size is 32 bytes and it must be authenticated before read.
 *
 * @param[in]  dev          target device
 * @param[out] odata        buffer where to store the data
 * @param[in]  card         card to use
 * @param[in]  page         which block to read
 *
 * @return                  0 on success
 */
int pn532_mifareulight_read(pn532_t *dev, char *odata, nfc_iso14443a_t *card, unsigned page);

/**
 * @brief   Activate the NDEF file of a ISO14443-A Type 4 tag
 *
 * @param[in]  dev          target device
 * @param[in]  card         card to activate
 *
 * @return                  0 on success
 */
int pn532_iso14443a_4_activate(pn532_t *dev, nfc_iso14443a_t *card);

/**
 * @brief   Read data from the NDEF file of a ISO14443-A Type 4 tag
 *
 * The first two bytes of an NDEF file are the length of the data. Afterwards,
 * at offset 0x02 starts the data itself. If one tries to read further than the
 * end of the data no data is returned.
 *
 * @param[in]  dev          target device
 * @param[out] odata        buffer where to store the data
 * @param[in]  card         card to activate
 * @param[in]  offset       offset where to start reading
 * @param[in]  len          length to read
 *
 * @return                  0 on success
 */
int pn532_iso14443a_4_read(pn532_t *dev, char *odata, nfc_iso14443a_t *card, unsigned offset,
                           char len);

/**
 * @brief   Deselect a previously selected passive card
 *
 * @param[in]  dev          target device
 * @param[in] target_id     id of the target to deselect (0x00 for all)
 */
void pn532_deselect_passive(pn532_t *dev, unsigned target_id);

/**
 * @brief   Release an active passive card
 *
 * @param[in]  dev          target device
 * @param[in] target_id     id of the target to release (0x00 for all)
 */
void pn532_release_passive(pn532_t *dev, unsigned target_id);

void pn532_set_parameters(pn532_t *dev, uint8_t flags);

int pn532_init_tag(pn532_t *dev, nfc_application_type_t app_type);

#ifdef __cplusplus
}
#endif

/** @} */
