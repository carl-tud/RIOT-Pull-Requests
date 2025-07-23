#include "st25.h"
#include "periph/spi.h"
#include "periph/gpio.h"
#include "ztimer.h"
#include "memory.h"
#include "log.h"
#include "board.h"
#include "kernel_defines.h"

#define ENABLE_DEBUG 1
#include "debug.h"

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
#define CMD_CLEAR_RSSI                    (0xDA)
#define CMD_CLEAR_FIFO                    (0xDB)
#define CMD_ENTER_TRANSPARENT_MODE            (0xDC)
#define CMD_MEASURE_POWER_SUPPLY           (0xDF)
#define CMD_START_GENERAL_PURPOSE_TIMER    (0xE0)
#define CMD_START_WAKE_UP_TIMER            (0xE1)
#define CMD_START_MASK_RECEIVE_TIMER       (0xE2)
#define CMD_START_NO_RESPONSE_TIMER        (0xE3)
#define CMD_START_PP_TIMER                 (0xE4)
#define CMD_STOP_NO_RESPONSE_TIMER          (0xE5)
#define CMD_TRIGGER_RC_CALIBRATION         (0xEA)
#define CMD_REGISTER_SPACE_B_ACCESS      (0xFB)
#define CMD_TEST_ACCESS                  (0xFC)

/* TODO: add more commands */


#define SPI_MODE_REGISTER_WRITE       (0x00)
#define SPI_MODE_REGISTER_READ        (0x40)
#define SPI_MODE_FIFO_LOAD            (0x80)
#define SPI_MODE_FIFO_READ            (0x9F)
#define SPI_MODE_DIRECT_COMMAND       (0xC0)
/* TODO: add more SPI modes */



#define REG_OPERATION_CONTROL       (0x02)
#define REG_MAIN_INTERRUPT   (0x1A)
#define REG_FIFO_STATUS_1   (0x1E)
#define REG_FIFO_STATUS_2   (0x1F)

#define SPACE_B (0x40) /* Space B register flag */

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

#if IS_ACTIVE(ENABLE_DEBUG)
#define PRINTBUFF printbuff
static void printbuff(uint8_t *buff, unsigned len)
{
    while (len) {
        len--;
        printf("%02x ", *buff++);
    }
    printf("\n");
}
#else
#define PRINTBUFF(...)
#endif

static void wait_ready(st25_t *dev)
{
    mutex_lock(&dev->trap);
}

static int _write(const st25_t *dev, uint8_t *buff, unsigned len)
{
    assert(dev != NULL);
    assert(buff != NULL);
    assert(len > 0);

    spi_acquire(dev->conf->spi, dev->conf->nss, SPI_MODE_1, SPI_CLK_5MHZ);
    gpio_clear(dev->conf->nss);

    spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, buff, NULL, len);

    gpio_set(dev->conf->nss);
    spi_release(dev->conf->spi);

    DEBUG("st25 -> ");
    PRINTBUFF(buff, len);

    return 0;
}

static int _write_and_read(const st25_t *dev, uint8_t *read, unsigned len, 
    uint8_t *to_write, unsigned to_write_len)
{
    assert(dev != NULL);
    assert(to_write != NULL);
    assert(read != NULL);
    assert(len > 0);

    spi_acquire(dev->conf->spi, dev->conf->nss, SPI_MODE_1, SPI_CLK_5MHZ);
    gpio_clear(dev->conf->nss);

    spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true,
                        to_write, NULL, to_write_len);
    spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, read, len);

    gpio_set(dev->conf->nss);
    spi_release(dev->conf->spi);

    DEBUG("st25 -> ");
    PRINTBUFF(to_write, to_write_len);

    DEBUG("st25 <- ");
    PRINTBUFF(read, len);

    return len;
}

static int _write_reg(const st25_t *dev, uint8_t reg, const uint8_t *data, unsigned len)
{
    assert(dev != NULL);
    assert(data != NULL);
    assert(reg <= 0x3F);

    uint8_t buff[BUFFER_LENGTH];
    if ((reg & SPACE_B) != 0U) {
        /* we are in register space B */
        buff[0] = CMD_REGISTER_SPACE_B_ACCESS;
        buff[1] = SPI_MODE_REGISTER_WRITE | reg;
        memcpy(&buff[2], data, len);
    } else {
        buff[0] = SPI_MODE_REGISTER_WRITE | reg;
        memcpy(&buff[1], data, len);
    }

    return _write(dev, buff, len + 1);
}

static int _read_reg(const st25_t *dev, uint8_t reg, uint8_t *data)
{
    assert(dev != NULL);
    assert(data != NULL);
    assert(reg <= 0x3F);

    uint8_t buff[BUFFER_LENGTH];
    uint8_t to_send = 0;
    if ((reg & SPACE_B) != 0U) {
        /* we are in register space B */
        buff[0] = CMD_REGISTER_SPACE_B_ACCESS;
        buff[1] = SPI_MODE_REGISTER_READ | reg;
        to_send = 2;
    } else {
        buff[0] = SPI_MODE_REGISTER_READ | reg;
        to_send = 1;
    }

    int ret = _write_and_read(dev, data, 1, buff, to_send);
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

static int _fifo_status(const st25_t *dev, uint16_t *size) {
    assert(dev != NULL);

    uint8_t status_1 = 0;
    uint8_t status_2 = 0;

    _read_reg(dev, REG_FIFO_STATUS_1, &status_1);
    _read_reg(dev, REG_FIFO_STATUS_2, &status_2);

    if (status_2 & 0x01 || status_2 & 0x02) {
        DEBUG("st25: FIFO underflow/overflow\n");
        return -1;
    }

    *size = (status_1) | (((uint16_t) (status_2 & 0xC0)) << 2);

    return 0;
}

static int _read_fifo(const st25_t *dev, uint8_t *data, uint16_t *len)
{
    assert(dev != NULL);
    assert(data != NULL);

    /* get the amount of bytes in the FIFO */
    _fifo_status(dev, len);

    if (*len == 0) {
        /* no data in FIFO */
        DEBUG("st25: FIFO is empty\n");
        return -1;
    }

    uint8_t operation_mode = SPI_MODE_FIFO_READ;

    /* read the bytes from the FIFO */
    int ret = _write_and_read(dev, data, (unsigned) len, &operation_mode, 1);
    return ret;
}

static int _send_cmd(st25_t *dev, uint8_t cmd)
{
    assert(dev != NULL);

    uint8_t operation_mode = SPI_MODE_DIRECT_COMMAND | cmd;

    return _write(dev, &operation_mode, 1);
}

static void _nfc_event(void *dev)
{
    DEBUG("st25: NFC event triggered\n");
    _read_reg((st25_t *)dev, REG_MAIN_INTERRUPT, &((st25_t *)dev)->irq_status);
    mutex_unlock(&((st25_t *)dev)->trap);
}

int st25_poll_nfc_a(st25_t *dev) {
    DEBUG("st25: Polling for NFC-A...\n");

    _send_cmd(dev, CMD_STOP);
    _send_cmd(dev, CMD_RESET_RX_GAIN);

    uint8_t reg_content = OPERATION_EN;
    _write_reg(dev, REG_OPERATION_CONTROL, &reg_content, 1);
    
    uint8_t operation_control = 0;
    _read_reg(dev, REG_OPERATION_CONTROL, &operation_control);
    DEBUG("st25: Operation Control Register: 0x%02X\n", operation_control);

    _send_cmd(dev, CMD_NFC_INITIAL_FIELD_ON);

    /* TODO: Configure timers */
    _send_cmd(dev, CMD_START_GENERAL_PURPOSE_TIMER);
    _send_cmd(dev, CMD_START_WAKE_UP_TIMER);

    _send_cmd(dev, CMD_TRANSMIT_REQA);

    wait_ready(dev);

    uint8_t buffer[BUFFER_LENGTH];
    uint16_t len;
    _read_fifo(dev, buffer, &len);

    return 0;
}


int st25_init(st25_t *dev, const st25_params_t *params)
{
    assert(dev != NULL);
    assert(params != NULL);

    dev->conf = params;

    /* Initialize SPI */

    /* we handle the CS line manually... */
    gpio_init(dev->conf->nss, GPIO_OUT);
    gpio_set(dev->conf->nss);

    gpio_init_int(dev->conf->irq, GPIO_IN_PD, GPIO_RISING,
                  _nfc_event, (void *)dev);

    DEBUG("st25: Initialized ST25 device\n");

    return 0;
}
