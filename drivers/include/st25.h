#pragma once

#include "board.h"
#include "periph/gpio.h"
#include "periph/spi.h"
#include "mutex.h"

typedef struct {
    spi_t spi;                  /**< SPI device */
    gpio_t irq;                 /**< Interrupt pin */
    gpio_t nss;                 /**< Chip Select pin (only SPI) */
} st25_params_t;

/**
 * @brief   Device descriptor for the ST25
 */
typedef struct {
    const st25_params_t *conf;      /**< Configuration struct */
    mutex_t trap;                   /**< Mutex to wait for chip response */
    uint8_t irq_status;             /* contents of the main interrupt register  */
} st25_t;

int st25_init(st25_t *dev, const st25_params_t *params);

int st25_poll_nfc_a(st25_t *dev);
