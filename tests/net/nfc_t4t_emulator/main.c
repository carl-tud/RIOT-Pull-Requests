#include "pn532.h"
#include "periph_conf.h"
#include "net/nfc/t4t/t4t_emulator.h"
#include "net/nfc/t4t/t4t.h"
#include "net/nfc/nfc_a.h"
#include "net/nfc/apdu/apdu.h"
#include "net/nfc/ndef/ndef.h"

#include "log.h"
#include "architecture.h"

#include <stdio.h>

//void print_spi_config(size_t i) {
//    printf("spi config[%" PRIuSIZE "] %p\n", i, spi_config[i].dev);
//    printf("spi config[%" PRIuSIZE "] MOSI %" PRIuSIZE " \n", i, (size_t)spi_config[i].mosi);
//    printf("spi config[%" PRIuSIZE "] MISO %" PRIuSIZE " \n", i, (size_t)spi_config[i].miso);
//    printf("spi config[%" PRIuSIZE "] CLK  %" PRIuSIZE "\n", i, (size_t)spi_config[i].sclk);
//}

int main(void) {

//    for (size_t i = 0; i < ARRAY_SIZE(spi_config); i += 1) {
//        print_spi_config(i);
//    }

    pn532_config_t config = {
        .params = {
            .spi = SPI_DEV(0),
            .reset = GPIO_PIN(0, 7),
            .nss = GPIO_PIN(1, 8),
            .irq = GPIO_PIN(0, 26)
        },
        .mode = PN53_BUS_SPI
    };

    pn532_t pn532_dev;

    nfcdev_t dev = {
        .dev = &pn532_dev,
        .ops = &pn532_ops,
        .config = &config,
    };

    uint8_t ndef_memory[64] = {0};
    ndef_t ndef;
    ndef_init(&ndef, ndef_memory, sizeof(ndef_memory));

    nfc_t4t_t tag;

    nfc_t4t_emulator_t emulator = {
        .dev = &dev,
        .tag = &tag,
    };

    nfc_a_id_t uid = {
        .len = NFC_A_NFCID1_LEN4,
        .nfcid = {0x08, 0xAD, 0xBE, 0xEF}
    };

    dev.ops->init(&dev, &config);

    /* Initialized PN532 device */
    LOG_DEBUG("Initialized PN532 device\n");

    t4t_emulator_start(&emulator, &dev, &tag, &uid);

    return 0;
}
