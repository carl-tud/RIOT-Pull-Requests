#include "net/nfc/mifare/mifare_classic_rw.h"
#include "net/nfc/ndef/ndef.h"
#include "pn532.h"
#include "log.h"

static uint8_t ndef_buf[256];

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

    nfc_mifare_classic_rw_t rw = {
        .dev = &dev,
    };

    dev.ops->init(&dev, &config);

    ndef_t ndef;
    ndef_init(&ndef, ndef_buf, sizeof(ndef_buf));

    /* Initialized PN532 device */
    LOG_DEBUG("Initialized PN532 device\n");

    nfc_mifare_classic_rw_read_ndef(&rw, &ndef, &dev);

    return 0;
}
