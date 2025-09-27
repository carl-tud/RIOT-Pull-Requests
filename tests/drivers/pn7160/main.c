#include "pn7160.h"

#include "log.h"

int main(void) {
    LOG_DEBUG("pn7160 test\n");

    pn7160_params_t params = {
        .spi = SPI_DEV(0),
        .reset = GPIO_PIN(1, 6),
        .nss = GPIO_PIN(1, 8),
        .irq = GPIO_PIN(1, 7),
        .mode = PN7160_SPI
    };

    pn7160_t dev;
    int ret = pn7160_init(&dev, &params);
    if (ret < 0) {
        LOG_DEBUG("pn7160: init failed with %d\n", ret);
        return ret;
    }

    pn7160_reset(&dev);

    LOG_DEBUG("pn7160 test done\n");

    return 0;
}
