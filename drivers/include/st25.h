#pragma once

#include "board.h"
#include "periph/gpio.h"
#include "periph/spi.h"
#include "periph/i2c.h"
#include "mutex.h"

#include "net/nfcdev.h"
#include "net/nfc/nfc_a.h"

typedef enum {
    ST25_I2C,
    ST25_SPI
} st25_mode_t;

typedef struct {
    union {
        spi_t spi;                  /**< SPI device */
        i2c_t i2c;                  /**< I2C device */
    };
    gpio_t nss;                 /**< Chip Select pin (only SPI) */
    gpio_t irq;                 /**< Interrupt pin */
    st25_mode_t mode;          /**< Working mode (I2C or SPI) */
} st25_params_t;

/**
 * @brief   Device descriptor for the ST25
 */
typedef struct {
    const st25_params_t *conf;      /**< Configuration struct */
    mutex_t trap;                   /**< Mutex to wait for chip response */
    uint32_t irq_status;             /**< contents of the main interrupt register  */
} st25_t;

int st25_init(nfcdev_t *dev, const void *config);

int st25_poll_a(nfcdev_t *dev, nfc_a_listener_config_t *config);

int st25_listen_a(nfcdev_t *dev, const nfc_a_listener_config_t *config);

static const nfcdev_ops_t st25_nfcdev_ops = {
    .init = st25_init,
    .poll_a = st25_poll_a,
    .listen_a = st25_listen_a,
};
