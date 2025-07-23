#include "st25.h"
#include "board.h"
#include <stdio.h>
#include "log.h"

int main(void) {
    puts("Starting ST25 NFC driver test...\n");
    st25_params_t params = {
        .spi = SPI_DEV(0),
        .nss = GPIO_PIN(0, 0),
        .irq = GPIO_PIN(0, 1)
    };

    st25_t device;

    int ret = st25_init(&device, &params);
    if (ret < 0) {
        printf("Failed to initialize ST25 device: %d\n", ret);
        return ret;
    }

    ret = st25_poll_nfc_a(&device);
    while (true) {}

    return 0;
}

