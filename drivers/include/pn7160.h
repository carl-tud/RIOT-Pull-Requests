#pragma once

#include "kernel_defines.h"
#include "mutex.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "net/nfcdev.h"

typedef enum {
    PN7160_I2C,
    PN7160_SPI
} pn7160_mode_t;

typedef struct {
    #if IS_USED(MODULE_PN7160_I2C) || DOXYGEN
            i2c_t i2c;              /**< I2C device */
    #endif
    #if IS_USED(MODULE_PN7160_SPI) || DOXYGEN
            spi_t spi;              /**< SPI device */
    #endif
    gpio_t reset;               /**< Reset pin */
    gpio_t irq;                 /**< Interrupt pin */
    #if IS_USED(MODULE_PN7160_SPI) || DOXYGEN
        gpio_t nss;                 /**< Chip Select pin (only SPI) */
    #endif
    pn7160_mode_t mode;             /**< Working mode (i2c, spi) */
} pn7160_config_t;

typedef struct {
    const pn7160_config_t *conf;    /**< Configuration struct */
    mutex_t trap;                   /**< Mutex to wait for chip response */
    bool is_mifare_classic;         /**< Whether mifare classic is used */
} pn7160_t;

int pn7160_init(nfcdev_t *dev, const void *params);

int pn7160_poll_a(nfcdev_t *nfcdev, nfc_a_listener_config_t *config);

int pn7160_listen_a(nfcdev_t *nfcdev, const nfc_a_listener_config_t *config);

int pn7160_initiator_exchange_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len,
                                  uint8_t *rcv, size_t *receive_len);

int pn7160_target_send_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len);

int pn7160_target_receive_data(nfcdev_t *nfcdev, uint8_t *rcv, size_t *receive_len);

int pn7160_reset(pn7160_t *dev);

int pn7160_mifare_classic_authenticate(nfcdev_t *nfcdev, uint8_t block, 
    const nfc_a_nfcid1_t *nfcid1, bool is_key_a, const uint8_t *key);

static const nfcdev_ops_t pn7160_ops = {
    .init = pn7160_init,
    .poll_a = pn7160_poll_a,
    .listen_a = pn7160_listen_a,
    .initiator_exchange_data = pn7160_initiator_exchange_data,
    .target_send_data = pn7160_target_send_data,
    .target_receive_data = pn7160_target_receive_data,
    .mifare_classic_authenticate = pn7160_mifare_classic_authenticate
};
