#include "st25.h"
#include "periph/spi.h"

#define CMD_SET_DEFAULT             (0xC0)
#define CMD_STOP                    (0xC2)  
#define CMD_TRANSMIT_WITH_CRC       (0xC4)
#define CMD_TRANSMIT_WITHOUT_CRC    (0xC5)
#define CMD_TRANSMIT_REQA           (0xC6)
#define CMD_TRANSMIT_WUPA           (0xC7)
#define CMD_NFC_INITIAL_FIELD_ON    (0xC8)
#define CMD_NFC_RESPONSE_FIELD_ON   (0xC9)
#define CMD_GO_TO_SENSE             (0xCD)
#define CMD_GO_TO_SLEEP             (0xCE)
#define CMD_MASK_RECEIVE_DATA       (0xD0)
#define CMD_MASK_TRANSMIT_DATA      (0xD1)
#define CMD_CHANGE_AM_AMPLITUDE_STATE (0xD2)
#define CMD_MEASURE_AMPLTIUDE        (0xD3)
#define CMD_RESET_RX_GAIN               (0xD5)
#define CMD_ADJUST_REGULATORS           (0xD6)
#define CMD_CALIBRATE_DRIVER_TIMING       (0xD8)
#define CMD_MEASURE_PHASE                 (0xD9)
/* TODO: add more commands */


#define SPI_MODE_REGISTER_WRITE       (0x00)
#define SPI_MODE_REGISTER_READ        (0x40)
#define SPI_MODE_FIFO_LOAD            (0x80)
#define SPI_MODE_FIFO_READ            (0x9F)
/* TODO: add more SPI modes */



#define REG_OPERATION_CONTROL       (0x02)

#define OPERATION_EN        (0x80)
#define OPERATION_RX_EN     (0x40)
#define OPERATION_RX_CHN    (0x20)
#define OPERATION_TX_MAN    (0x10)
#define OPERATION_TX_EN     (0x08)
#define OPERATION_WU        (0x04)
#define OPERATION_EN_FD_CX  (0x03)

#define SPI_WRITE_DELAY_US (2000)

/* Buffer length for SPI transfers */
#define BUFFER_LENGTH (128)  


static int _write(const st25_t *dev, const uint8_t *buff, unsigned len)
{
    assert(dev != NULL);
    assert(buff != NULL);

    spi_acquire(dev->conf->spi, dev->conf->nss, SPI_MODE_0, SPI_CLK_1MHZ);
    gpio_clear(dev->conf->nss);
    ztimer_sleep(ZTIMER_USEC, SPI_WRITE_DELAY_US);
    spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, buff, NULL, len);
    gpio_set(dev->conf->nss);
    spi_release(dev->conf->spi);

    return 0;
}

static int _read(const st25_t *dev, uint8_t *buff, unsigned len)
{
    assert(dev != NULL);
    assert(buff != NULL);

    spi_acquire(dev->conf->spi, dev->conf->nss, SPI_MODE_0, SPI_CLK_1MHZ);
    gpio_clear(dev->conf->nss);
    ztimer_sleep(ZTIMER_USEC, SPI_WRITE_DELAY_US);
    spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, buff, len);
    gpio_set(dev->conf->nss);
    spi_release(dev->conf->spi);

    return len;
}

static int _write_reg(const st25_t *dev, uint8_t reg, const uint8_t *data, unsigned len)
{
    assert(dev != NULL);
    assert(data != NULL);
    assert(reg <= 0x3F);

    uint8_t buff[BUFFER_LENGTH];
    buff[0] = SPI_MODE_REGISTER_WRITE | reg;
    memcpy(&buff[1], data, len);

    return _write(dev, buff, len + 1);
}

static int _read_reg(const st25_t *dev, uint8_t reg, uint8_t *data)
{
    assert(dev != NULL);
    assert(data != NULL);
    assert(reg <= 0x3F);

    uint8_t buff[BUFFER_LENGTH];
    buff[0] = SPI_MODE_REGISTER_READ | reg;

    int ret = _write(dev, buff, 1);
    if (ret < 0) {
        return ret;
    }

    ret = _read(dev, buff, 1);
    return ret;
}

static int _load_fifo(const st25_t *dev, const uint8_t *data, unsigned len)
{
    assert(dev != NULL);
    assert(data != NULL);

    uint8_t buff[BUFFER_LENGTH];
    buff[0] = SPI_MODE_FIFO_LOAD;
    memcpy(&buff[1], data, len);

    return _write(dev, buff, len + 1);
}

static int _read_fifo(const st25_t *dev, uint8_t *data, unsigned len)
{
    assert(dev != NULL);
    assert(data != NULL);

    uint8_t buff[BUFFER_LENGTH];
    buff[0] = SPI_MODE_FIFO_READ;

    int ret = _write(dev, buff, 1);
    if (ret < 0) {
        return ret;
    }

    ret = _read(dev, data, len);
    return ret;
}

static int _send_cmd(const st25_t *dev, uint8_t cmd)
{
    assert(dev != NULL);
    assert(data != NULL);

    return _write(dev, cmd, 1);
}

int st25_poll() {
    _send_cmd(dev, CMD_STOP);
    _send_cmd(dev, CMD_RESET_RX_GAIN);

    /* Configure timers */
    _send_cmd();
}
