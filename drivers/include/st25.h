#pragma once

#include "board.h"
#include "periph/gpio.h"
#include "periph/spi.h"

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
} st25_t;
