#include "net/nfc/t2t/t2t_rw.h"
#include "net/nfcdev.h"
#include "pn532.h"
#include "ztimer.h"

#include "log.h"

static uint8_t ndef_buffer[256];

int main(void) {
    pn532_config_t config = {
        .params = {
            .spi = SPI_DEV(0),
            .nss = GPIO_PIN(1, 3),
            .reset = GPIO_PIN(1, 1),
            .irq = GPIO_PIN(1, 2)
        },
        .mode = PN532_SPI
    };

    pn532_t pn532_dev;

    nfcdev_t dev = {
        .dev = &pn532_dev,
        .ops = &pn532_ops,
        .config = &config,
    };

    nfc_t2t_rw_t rw = {
        .dev = &dev,
    };

    dev.ops->init(&dev, &config);

    /* Initialized PN532 device */
    LOG_DEBUG("Initialized PN532 device\n");

    ndef_t ndef_msg;
    ndef_init(&ndef_msg, ndef_buffer, sizeof(ndef_buffer));

    ztimer_now_t start_time = ztimer_now(ZTIMER_USEC);
    nfc_t2t_rw_read_ndef(&rw, &ndef_msg, &dev);
    ztimer_now_t end_time = ztimer_now(ZTIMER_USEC);

    LOG_DEBUG("NFC T2T Read Time: %u us\n", (unsigned)(end_time - start_time));
}
