#include "st25.h"
#include "board.h"
#include <stdio.h>
#include "log.h"
#include "ztimer.h"

int main(void) {
    puts("Starting ST25 NFC driver test...\n");
    st25_params_t params = {
        .spi = SPI_DEV(0),
        .nss = GPIO_PIN(1, 12),
        .irq = GPIO_PIN(1, 11),
        .mode = ST25_SPI
    };

    st25_t device;

    nfcdev_t device_desc = {
        .dev = &device,
        .ops = &st25_nfcdev_ops,
    };

    int ret = st25_init(&device_desc, &params);
    if (ret < 0) {
        printf("Failed to initialize ST25 device: %d\n", ret);
        return ret;
    }

    nfc_a_listener_config_t config;
    st25_poll_a(&device_desc, &config);

    while(true) {}

    return 0;
}

