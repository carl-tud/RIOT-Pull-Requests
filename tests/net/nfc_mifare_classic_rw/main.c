#include "net/nfc/mifare/mifare_classic_rw.h"
#include "net/nfc/ndef/ndef.h"
#include "pn532.h"

#include <stdio.h>

#include "log.h"

static uint8_t ndef_buf[256];

static pn532_t pn532_dev;

int main(void) {
    printf("Hello, World!\n");
    LOG_DEBUG("MIFARE Classic Read/Write example\n");
    pn532_config_t config = {
        .params = {
            .spi = SPI_DEV(0),
            .nss = GPIO_PIN(1, 3),
            .reset = GPIO_PIN(1, 1),
            .irq = GPIO_PIN(1, 2)
        },
        .mode = PN532_SPI
    };



    nfcdev_t dev = {
        .dev = &pn532_dev,
        .ops = &pn532_ops,
        .config = (void *) &config,
    };

    nfc_mifare_classic_rw_t rw = {
        .dev = &dev,
    };

    dev.ops->init(&dev, (void *)&config);

    ndef_t ndef;
    ndef_init(&ndef, ndef_buf, sizeof(ndef_buf));

    /* Initialized PN532 device */
    LOG_DEBUG("Initialized PN532 device\n");


    nfc_mifare_classic_rw_read_ndef(&rw, &ndef, &dev);

    ndef_record_desc_t record_descs[MAX_NDEF_RECORD_COUNT];
    ndef_parse(&ndef, record_descs, MAX_NDEF_RECORD_COUNT);

    ndef_pretty_print(record_descs, ndef.record_count);

    return 0;
}
