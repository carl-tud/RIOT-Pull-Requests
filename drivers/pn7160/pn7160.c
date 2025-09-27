#include "pn7160.h"

#include "periph/spi.h"
#include "net/nfc/nci.h"
#include "log.h"

#define PN7160_PAYLOAD_START                (3u)

#define PN7160_SPI_TRANSFER_DIRECTION_WRITE (0x00)
#define PN7160_SPI_TRANSFER_DIRECTION_READ  (0xFF)

#define PN7160_SPI_TIMEOUT_MS               (5u)

#define SPI_MODE                            (SPI_MODE_0)
#define SPI_CLK                             (1000000)

#define PN7160_BUFFER_LEN                     (128u)

static void nfc_event(void *dev)
{
    LOG_DEBUG("pn7160: event\n");
    mutex_unlock(&((pn7160_t *)dev)->trap);
}

int pn7160_init(pn7160_t *dev, const pn7160_params_t *params)
{
    assert(dev != NULL);
    assert(params != NULL);
    dev->conf = params;

    if (dev->conf->mode == PN7160_SPI) {
    #if IS_USED(MODULE_PN7160_SPI)
        /* we handle the CS line manually... */
        gpio_init(dev->conf->nss, GPIO_OUT);
        gpio_set(dev->conf->nss);
    #endif
    } else {
        /* not implemented yet */
        return -1;
    }

    mutex_init(&dev->trap);
    mutex_lock(&dev->trap);

    gpio_init_int(dev->conf->irq, GPIO_IN_PU, GPIO_RISING, nfc_event, (void *)dev);


    // gpio_init(dev->conf->reset, GPIO_OUT);
    // gpio_set(dev->conf->reset);

    return 0;
}

static int _read(const pn7160_t *dev, uint8_t *buff)
{
    int ret = -1;
    LOG_DEBUG("pn7160: read\n");

    switch (dev->conf->mode) {
    case PN7160_SPI:
        /* SPI read implementation */
        spi_acquire(dev->conf->spi, SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
        gpio_clear(dev->conf->nss);

        spi_transfer_byte(dev->conf->spi, SPI_CS_UNDEF, true, PN7160_SPI_TRANSFER_DIRECTION_READ);

        /* read the header first */
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, buff, NCI_HEADER_SIZE);
        size_t payload_len = buff[NCI_PAYLOAD_LENGTH_POS];

        /* make sure that we don't overflow */
        if (payload_len > (PN7160_BUFFER_LEN - NCI_HEADER_SIZE)) {
            LOG_DEBUG("pn7160: payload too large (%u)!\n", (unsigned)payload_len);
            gpio_set(dev->conf->nss);
            spi_release(dev->conf->spi);
            return -1;
        }
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, buff + NCI_HEADER_SIZE, payload_len);

        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);
        LOG_DEBUG("pn7160: read %u bytes\n", (unsigned)(NCI_HEADER_SIZE + payload_len));
        ret = (int)(NCI_HEADER_SIZE + payload_len);
        break;
    case PN7160_I2C:
        /* I2C read implementation */
        break;
    default:
        break;
    }

    return ret;
}

static int _write(const pn7160_t *dev, const uint8_t *buff, size_t len)
{
    int ret = -1;

    switch (dev->conf->mode) {
    case PN7160_SPI:
        /* SPI write implementation */
        spi_acquire(dev->conf->spi, SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
        gpio_clear(dev->conf->nss);

        spi_transfer_byte(dev->conf->spi, SPI_CS_UNDEF, true, PN7160_SPI_TRANSFER_DIRECTION_WRITE);
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, buff, NULL, len);

        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);
        ret = (int)len;
        break;

    case PN7160_I2C:
        /* I2C write implementation */
        break;
    default:
        break;
    }

    return ret;
}

/* Fills the buffer with the packet header for the message type */
static int construct_packet_header(uint8_t *buff, uint8_t mt, uint8_t pbf, 
    uint8_t gid, uint8_t oid, uint8_t payload_len)
{
    switch (mt) {
        case NCI_MT_CMD:
            buff[0] = NCI_CONTROL_BYTE(mt, pbf, gid);
            buff[1] = NCI_OPCODE(oid);
            buff[2] = payload_len;
            break;
        case NCI_MT_DATA:
            buff[0] = NCI_CONTROL_BYTE(mt, pbf, gid);
            buff[1] = NCI_OPCODE(oid);
            buff[2] = payload_len;
            break;
        default: 
            LOG_DEBUG("pn7160: invalid mt (%i)!\n", mt);
            return -1;
    }

    return 0;
}

static int send_recv(pn7160_t *dev, uint8_t *buff, size_t len)
{
    int ret = -1;

    if (dev == NULL || buff == NULL || len == 0) {
        return -1;
    }

    ret = _write(dev, buff, len);
    if (ret < 0) {
        return ret;
    }

    mutex_lock(&dev->trap);

    ret = _read(dev, buff);
    if (ret < 0) {
        return ret;
    }

    return ret;
}

int pn7160_reset(pn7160_t *dev)
{
    LOG_DEBUG("pn7160: reset\n");
    assert(dev != NULL);
    uint8_t buff[PN7160_BUFFER_LEN];

    construct_packet_header(buff, NCI_MT_CMD, 0, NCI_GID_CORE, NCI_OID_CORE_RESET, 1);
    buff[PN7160_PAYLOAD_START] = 0x01; /* reset configuration */
    send_recv(dev, buff, PN7160_PAYLOAD_START + 1);

    if (buff[PN7160_PAYLOAD_START] == 0x00) {
        LOG_DEBUG("pn7160: reset ok\n");
        return 0;
    }

    return -1;
}


