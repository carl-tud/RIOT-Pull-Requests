#include "net/nfc/generic/generic_rw.h"
#include "pn532.h"
#include "net/nfcdev.h"
#include "net/nfc/ndef/ndef.h"

static uint8_t ndef_buffer[256];

int main(void) {
    nfc_generic_rw_t rw;
    ndef_t ndef;
    ndef_init(&ndef, ndef_buffer, sizeof(ndef_buffer));

    pn532_config_t config = {
        .params = {
            .spi = SPI_DEV(0),
            .nss = GPIO_PIN(1, 1),
            .reset = GPIO_PIN(1, 2),
            .irq = GPIO_PIN(1, 3)
        },
        .mode = PN532_SPI
    };

    pn532_t pn532_dev;

    nfcdev_t dev = {
        .dev = &pn532_dev,
        .ops = &pn532_ops,
        .config = &config,
    };

    dev.dev = &pn532_dev;

    dev.ops->init(&dev, (void *) &config);

    nfc_generic_rw_read_ndef(&rw, &ndef, &dev);
}
