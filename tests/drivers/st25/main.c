#include "st25.h"
#include "board.h"
#include <stdio.h>

int main(void) {
    st25_params_t params = {
        .spi = SPI_DEV(0),
        .nss = GPIO_PIN(0, 0),
    };

    st25_t device;

    int ret = st25_init(&device, &params);
    if (ret < 0) {
        printf("Failed to initialize ST25 device: %d\n", ret);
        return ret;
    }


    return 0;
}

