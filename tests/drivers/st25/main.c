#include "st25.h"
#include "board.h"
#include <stdio.h>
#include "log.h"
#include "ztimer.h"

int main(void) {
    puts("Starting ST25 NFC driver test...\n");
    st25_params_t params = {
        .spi = SPI_DEV(0), // Use SPI device 0
        .nss = GPIO_PIN(1, 12),
        .irq = GPIO_PIN(1, 11),
        .mode = ST25_SPI
    };

    st25_t device;

    int ret = st25_init(&device, &params);
    if (ret < 0) {
        printf("Failed to initialize ST25 device: %d\n", ret);
        return ret;
    }

    st25_listen_nfc_a(&device);

    while(true) {}

    return 0;
}

