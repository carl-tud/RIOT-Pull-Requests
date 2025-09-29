#include "pn7160.h"
#include "net/nfcdev.h"
#include "net/nfc/t2t/t2t.h"
#include "net/nfc/t2t/t2t_rw.h"

#include "log.h"

int main(void) {
    LOG_DEBUG("pn7160 test\n");

    pn7160_t dev;
    pn7160_params_t params = {
        .spi = SPI_DEV(0),
        .reset = GPIO_PIN(1, 6),
        .irq = GPIO_PIN(1, 7),
        .nss = GPIO_PIN(1, 8),
        .mode = PN7160_SPI
    };

    nfcdev_t nfc_device = {
        .dev = &dev,
        .config = (void *) &params,
        .ops = &pn7160_ops,
        .state = NFCDEV_STATE_DISABLED
    };

    nfc_t2t_rw_t t2t_rw;

    ndef_t ndef;
    uint8_t ndef_buf[256];
    ndef_init(&ndef, ndef_buf, sizeof(ndef_buf));

    nfc_device.ops->init(&nfc_device, &params);
    nfc_t2t_rw_read_ndef(&t2t_rw, &ndef, &nfc_device);

    ndef_record_desc_t records[4];
    ndef_parse(&ndef, records, 4);
    ndef_pretty_print(records, 1);
    LOG_DEBUG("pn7160 test done\n");

    return 0;
}
