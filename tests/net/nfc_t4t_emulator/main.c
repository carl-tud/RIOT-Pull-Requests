#include "pn532.h"
#include "net/nfc/t4t/t4t_emulator.h"
#include "net/nfc/t4t/t4t.h"
#include "net/nfc/nfc_a.h"
#include "net/nfc/apdu/apdu.h"
#include "net/nfc/ndef/ndef.h"

#include "log.h"

int main(void) {

    pn532_config_t config = {
        .params = {
            .spi = SPI_DEV(0),
            .reset = GPIO_PIN(1, 1),
            .nss = GPIO_PIN(1, 3),
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

    uint8_t ndef_memory[64] = {0};
    ndef_t ndef;
    ndef_init(&ndef, ndef_memory, sizeof(ndef_memory));

    nfc_t4t_t tag;

    nfc_t4t_emulator_t emulator = {
        .dev = &dev,
        .tag = &tag,
    };

    nfc_a_nfcid1_t nfcid1 = {
        .len = NFC_A_NFCID1_LEN4,
        .nfcid = {0x08, 0xAD, 0xBE, 0xEF}
    };

    dev.ops->init(&dev, &config);

    /* Initialized PN532 device */
    LOG_DEBUG("Initialized PN532 device\n");

    t4t_emulator_start(&emulator, &dev, &tag, &nfcid1);

    return 0;
}
