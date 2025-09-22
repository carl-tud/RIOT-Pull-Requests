#include "net/nfcdev.h"
// #include "nrfx_nfct_dev.h"
#include "pn532.h"

#include "log.h"


/* static const nfcdev_ops_t nrfx_nfct_dev_ops = {
    .init = init,
    .listen_a = listen_a,
    .target_exchange_data = target_exchange_data,
    // Add other function pointers as needed
}; */

static const nfcdev_ops_t pn532_ops = {
    .init = pn532_init,
    .poll_a = pn532_poll_a,
    .initiator_exchange_data = pn532_initiator_exchange_data,
};

/* int test_nrfx_nfct_dev(void) {
    nfcdev_t dev  = {
        .ops = &nrfx_nfct_dev_ops,
    };

    dev.ops->init(&dev, NULL);

    nfc_a_listener_config_t config = {
        .nfcid1 = {
            .len = NFC_A_NFCID1_LEN4,
            .nfcid = {0xDE, 0xAD, 0xBE, 0xEF}
        },
        .sel_res = NFC_A_SEL_RES_T2T_VALUE
    };
    dev.ops->listen_a(&dev, &config);
    
    uint8_t rx_buffer[32];
    size_t rx_len = 0;

    // receive only
    dev.ops->target_exchange_data(&dev, NULL, 0, rx_buffer, &rx_len);

    return 0;
} */

int test_pn532(void) {
    static pn532_t dev;
    static pn532_params_t initiator_params = {
        .spi = SPI_DEV(0),
        .nss = GPIO_PIN(1, 6),
        .reset = GPIO_PIN(1, 1),
        .irq = GPIO_PIN(1, 3)
    };

    pn532_config_t config = {
        .params = initiator_params,
        .mode = PN532_SPI
    };

    nfcdev_t nfc_dev  = {
        .dev = &dev,
        .ops = &pn532_ops,
    };

    nfc_dev.ops->init(&nfc_dev, (void *) &config);
    nfc_dev.ops->poll_a(&nfc_dev);

    return 0;
}

int main(void) {
    // Test NRFX NFCT device
    // test_nrfx_nfct_dev();

    // Test PN532 device
    test_pn532();

    return 0;
}
