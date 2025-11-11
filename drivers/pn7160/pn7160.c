#include "pn7160.h"

#include "periph/spi.h"
#include "net/nfc/nci.h"
#include "net/nfc/nfc_a.h"
#include "net/nfc/mifare/mifare_classic.h"
#include "log.h"
#include "ztimer.h"
#include "memory.h"

#define PN7160_PAYLOAD_START                (3u)

#define PN7160_NCI_PROTOCOL_MIFARE_CLASSIC       (0x80u)

#define NCI_RF_INTERFACE_TAG_CMD          (0x80u)

#define PN7160_SPI_TRANSFER_DIRECTION_WRITE (0x00)
#define PN7160_SPI_TRANSFER_DIRECTION_READ  (0xFF)

#define PN7160_TAG_CMD_XCHG_DATA_ID         (0x10u)
#define PN7160_TAG_CMD_MFC_AUTHENTICATE_ID  (0x40u)

#define PN7160_SPI_TIMEOUT_MS               (5u)

#define SPI_MODE                            (SPI_MODE_0)
#define SPI_CLK                             (SPI_CLK_1MHZ)

#define PN7160_BUFFER_LEN                     (128u)

static const uint8_t PN7160_CORE_CONFIG[] = { 0x01, 0xA0, 0x0E, 0x0B, 
    0x11, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x10, 0x0C};

static const uint8_t PN7160_NFC_A_LISTEN_MODE_ROUTING[] = {
    0x00, 0x02, 
    0x01, 0x03, 0x00, 0x01, 0x04,  /* protocol based routing (ISO-DEP) */
    0x00, 0x03, 0x00, 0x01, 0x00   /* technology based routing (NFC-A) */
};

// static const uint8_t PN7160_RF_CONFIG[] = {
//     0x09,
//     0xA0, 0x0D, 0x03, 0x78, 0x0D, 0x02,
//     0xA0, 0x0D, 0x03, 0x78, 0x14, 0x02,
//     0xA0, 0x0D, 0x06, 0x4C, 0x44, 0x65, 0x09, 0x00, 0x00,
//     0xA0, 0x0D, 0x06, 0x4C, 0x2D, 0x05, 0x35, 0x1E, 0x01,
//     0xA0, 0x0D, 0x06, 0x82, 0x4A, 0x55, 0x07, 0x00, 0x07,
//     0xA0, 0x0D, 0x06, 0x44, 0x44, 0x03, 0x04, 0xC4, 0x00,
//     0xA0, 0x0D, 0x06, 0x46, 0x30, 0x50, 0x00, 0x18, 0x00,
//     0xA0, 0x0D, 0x06, 0x48, 0x30, 0x50, 0x00, 0x18, 0x00,
//     0xA0, 0x0D, 0x06, 0x4A, 0x30, 0x50, 0x00, 0x08, 0x00
// };

// static const uint8_t PN7160_TAG_DETECTOR_DISABLE[] = {
//      0x01, 0xA0, 0x40, 0x01, 0x00
// };

// static const uint8_t PN7160_TOTAL_DURATION[]={
//     0x01,         /* CORE_SET_CONFIG_CMD */
//     0x00, 0x02, 0xFE, 0x01                            
// };

// static const uint8_t PN7160_CLOCK_SEL_XTAL[] = {
//      0x01, 0xA0, 0x0E, 0x01, 0x00
// };

#if DEVELHELP == 1
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

static int block_with_timeout(pn7160_t *dev, uint32_t timeout_sec) {
    if (timeout_sec == 0) {
        mutex_lock(&dev->trap);
        return 0;
    } else {
        ztimer_t timer = {0};
        ztimer_mutex_unlock(ZTIMER_SEC, &timer, timeout_sec, &dev->trap);
        mutex_lock(&dev->trap);
        bool triggered = !ztimer_remove(ZTIMER_SEC, &timer);
        if (triggered) {
            return NFC_ERR_TIMEOUT;
        } else {
            return 0;
        }
    }
}


static void nfc_event(void *dev)
{
    mutex_unlock(&((pn7160_t *)dev)->trap);
}

static int _read(const pn7160_t *dev, uint8_t *buff)
{
    int ret = -1;
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
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, buff + NCI_HEADER_SIZE, 
            payload_len);

        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);

        ret = (int)(NCI_HEADER_SIZE + payload_len);
        break;
    case PN7160_I2C:
        /* I2C read implementation */
        break;
    default:
        break;
    }

    LOG_DEBUG("pn7160: <- ");
    PRINTBUFF(buff, ret > 0 ? (size_t)ret : 0);
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

    ztimer_sleep(ZTIMER_MSEC, 1);

    LOG_DEBUG("pn7160: -> ");
    PRINTBUFF((uint8_t *)buff, len);

    return ret;
}

/* returns the length of the received data */
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

    ret = block_with_timeout(dev, 2);
    if (ret < 0) {
        return ret;
    }

    ret = _read(dev, buff);
    if (ret < 0) {
        return ret;
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

static int send_cmd_rcv_rsp(pn7160_t *dev, uint8_t gid, uint8_t oid, 
    uint8_t *buff, uint8_t payload_len)
{
    assert (dev != NULL);
    construct_packet_header(buff, NCI_MT_CMD, 0, gid, oid, payload_len);
    int len = send_recv((pn7160_t *)dev, buff, PN7160_PAYLOAD_START + payload_len);

    if (len < 0) {
        LOG_ERROR("pn7160: send_recv failed!\n");
        return -1;
    }

    /* check the header if it contains the correct mt, gid and oid */
    if (len < (int) NCI_HEADER_SIZE) {
        LOG_ERROR("pn7160: invalid len (%i)!\n", len);
        return -1;
    }

    if (NCI_GET_MT(buff[0]) != NCI_MT_RSP) {
        LOG_ERROR("pn7160: invalid mt (%i)!\n", NCI_GET_MT(buff[0]));
        return -1;
    }

    if (NCI_GET_GID(buff[0]) != gid) {
        LOG_ERROR("pn7160: invalid gid (%i)!\n", NCI_GET_GID(buff[0]));
        return -1;
    }

    if (NCI_GET_OID(buff[1]) != oid) {
        LOG_ERROR("pn7160: invalid oid (%i)!\n", NCI_GET_OID(buff[1]));
        return -1;
    }

    /* the first byte of the payload is the status */
    if (buff[PN7160_PAYLOAD_START] != NCI_STATUS_OK) {
        LOG_ERROR("pn7160: rsp not ok (%i)!\n", buff[PN7160_PAYLOAD_START]);
        return -1;
    }

    return 0;
}

static int send_cmd_rcv_ntf(pn7160_t *dev, uint8_t gid, uint8_t oid, 
    uint8_t *buff, uint8_t payload_len)
{
    assert (dev != NULL);
    int ret = send_cmd_rcv_rsp(dev, gid, oid, buff, payload_len);
    if (ret < 0) {
        return ret;
    }

    /* wait for the NTF */
    ret = block_with_timeout(dev, 2);
    if (ret < 0) {
        return ret;
    }

    _read((pn7160_t *)dev, buff);


    return 0;
}

static void _disable_standby(pn7160_t *dev, uint8_t *buff)
{
    buff[PN7160_PAYLOAD_START] = 0x00; /* disable standby */
    construct_packet_header(buff, NCI_MT_CMD, 0, NCI_GID_PROPRIETARY, 0x00, 1);
    send_cmd_rcv_rsp(dev, NCI_GID_PROPRIETARY, 0x0, buff, 1);
}

static int _core_init(pn7160_t *dev, uint8_t *buff) {
    assert (dev != NULL);
    buff[PN7160_PAYLOAD_START    ] = 0x00;
    buff[PN7160_PAYLOAD_START + 1] = 0x00; 
    return send_cmd_rcv_rsp(dev, NCI_GID_CORE, NCI_OID_CORE_INIT, buff, 2);
}

static int _reset_sequence(pn7160_t *dev, uint8_t *buff) {
    assert(dev != NULL);

    buff[PN7160_PAYLOAD_START] = 0x01; /* reset type: soft reset */

    send_cmd_rcv_ntf(dev, NCI_GID_CORE, NCI_OID_CORE_RESET, buff, 1);

    _core_init(dev, buff);

    LOG_DEBUG("pn7160: reset done\n");

    return 0;
}

static int _core_set_config_cmd(pn7160_t *dev, uint8_t *buff, size_t len) {
    assert (dev != NULL);
    return send_cmd_rcv_rsp(dev, NCI_GID_CORE, NCI_OID_CORE_SET_CONFIG, buff, len);
}

static int _set_listen_mode_routing_cmd(pn7160_t *dev, uint8_t *buff, size_t len) {
    assert (dev != NULL);
    return send_cmd_rcv_rsp(dev, NCI_GID_RF_MGMT, NCI_OID_RF_SET_LISTEN_MODE_ROUTING, 
        buff, len);
}

static int _nfc_a_set_listen_mode_routing(pn7160_t *dev, uint8_t *buff) {
    assert (dev != NULL);
    memcpy(&buff[PN7160_PAYLOAD_START], PN7160_NFC_A_LISTEN_MODE_ROUTING, 
        sizeof(PN7160_NFC_A_LISTEN_MODE_ROUTING));
    return _set_listen_mode_routing_cmd(dev, buff, sizeof(PN7160_NFC_A_LISTEN_MODE_ROUTING));
}

static int _rf_discover_map_cmd(const pn7160_t *dev, uint8_t rf_protocol, 
    uint8_t mode, uint8_t rf_interface) {
    assert (dev != NULL);
    uint8_t buff[PN7160_BUFFER_LEN];
    buff[PN7160_PAYLOAD_START    ] = 0x01;          /* Number of configurations */
    buff[PN7160_PAYLOAD_START + 1] = rf_protocol;  /* RF protocol*/
    buff[PN7160_PAYLOAD_START + 2] = mode;         /* Mode (Listen/Poll) */
    buff[PN7160_PAYLOAD_START + 3] = rf_interface; /* RF interface */

    send_cmd_rcv_rsp((pn7160_t *)dev, NCI_GID_RF_MGMT, 
        NCI_OID_RF_DISCOVER_MAP, buff, 4);

    return 0;
}

// static int _rf_deactivate_cmd(const pn7160_t *dev, uint8_t type) {
//     assert (dev != NULL);
//     uint8_t buff[PN7160_BUFFER_LEN];
//     buff[PN7160_PAYLOAD_START] = type; /* Deactivation type */

//     send_cmd_rcv_ntf((pn7160_t *)dev, NCI_GID_RF_MGMT,
//         NCI_OID_RF_DEACTIVATE, buff, 1);

//     if (NCI_GET_GID(buff[0]) != NCI_GID_RF_MGMT) {
//         LOG_ERROR("pn7160: invalid gid (%i)!\n", NCI_GET_GID(buff[0]));
//         return -1;
//     }

//     if (NCI_GET_OID(buff[1]) != NCI_OID_RF_DEACTIVATE) {
//         LOG_ERROR("pn7160: invalid ntf oid (%i)!\n", NCI_GET_OID(buff[1]));
//         return -1;
//     }

//     return 0;
// }

static int _rf_discover_cmd(pn7160_t *dev, uint8_t technology_and_mode, uint8_t *buff) {
    assert (dev != NULL);
    buff[PN7160_PAYLOAD_START]     = 0x01;       /* num of configurations */
    buff[PN7160_PAYLOAD_START + 1] = technology_and_mode;
    buff[PN7160_PAYLOAD_START + 2] = 0x01;

    send_cmd_rcv_ntf((pn7160_t *)dev, NCI_GID_RF_MGMT,
        NCI_OID_RF_DISCOVER, buff, 3);

    if (NCI_GET_GID(buff[0]) != NCI_GID_RF_MGMT) {
        LOG_ERROR("pn7160: invalid gid (%i)!\n", NCI_GET_GID(buff[0]));
        return -1;
    }

    if (NCI_GET_OID(buff[1]) != NCI_OID_RF_INTF_ACTIVATED) {
        LOG_ERROR("pn7160: invalid ntf oid (%i)!\n", NCI_GET_OID(buff[1]));
        return -1;
    }

    /* check if the tag is a MIFARE Classic tag */
    if (buff[PN7160_PAYLOAD_START + 1] == PN7160_NCI_PROTOCOL_MIFARE_CLASSIC) {
        dev->is_mifare_classic = true;
    } else {
        dev->is_mifare_classic = false;
    }

    return 0;
}

static int _configuration_3_3V(pn7160_t *dev, uint8_t *buff) {
    memcpy(&buff[PN7160_PAYLOAD_START], PN7160_CORE_CONFIG, sizeof(PN7160_CORE_CONFIG));
    return _core_set_config_cmd(dev, buff, sizeof(PN7160_CORE_CONFIG));
}

// static int _set_rf_config(pn7160_t *dev, uint8_t *buff) {
//     memcpy(&buff[PN7160_PAYLOAD_START], PN7160_RF_CONFIG, sizeof(PN7160_RF_CONFIG));
//     return _core_set_config_cmd(dev, buff, sizeof(PN7160_RF_CONFIG));
// }



int pn7160_init(nfcdev_t *nfcdev, const void *params)
{
    assert(nfcdev != NULL);
    assert(params != NULL);
    pn7160_t *dev = (pn7160_t *) nfcdev->dev;
    dev->conf = (pn7160_config_t *) params;

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

    LOG_DEBUG("pn7160: init done\n");

    uint8_t buff[PN7160_BUFFER_LEN];
    int ret = _reset_sequence(dev, buff);
    if (ret < 0) {
        return ret;
    }

    ret = _configuration_3_3V(dev, buff);
    if (ret < 0) {
        return ret;
    }
    
    nfcdev->state = NFCDEV_STATE_DISABLED;

    return 0;
}

int pn7160_poll_f(nfcdev_t *dev, nfc_f_listener_config_t *config) {
    (void)dev;
    (void)config;
    return -1;
}

int pn7160_poll_a(nfcdev_t *dev, nfc_a_listener_config_t *config) {
    assert (dev != NULL);
    assert (config != NULL);

    uint8_t buff[PN7160_BUFFER_LEN];

    /* put the device into idle state */
    _reset_sequence(dev->dev, buff);

    // _rf_deactivate_cmd((pn7160_t *)dev->dev, 0x00); /* idle mode */


    /* this is done so the device operates properly at 3.3V */
    // _configuration_3_3V(dev->dev, buff);

    /* LOG_DEBUG("pn7160: discover map...\n"); */

    /* we need to map the TAG-CMD interface to the MIFARE Classic Protocol*/
    _rf_discover_map_cmd((pn7160_t *)dev->dev,
        PN7160_NCI_PROTOCOL_MIFARE_CLASSIC, /* RF protocol */
        NCI_RF_MODE_POLL,                   /* Poll mode */
        NCI_RF_INTERFACE_TAG_CMD);          /* RF interface */

    LOG_DEBUG("pn7160: poll for NFC-A device...\n");

    int ret = _rf_discover_cmd((pn7160_t *)dev->dev, 
        NCI_NFC_A_PASSIVE_POLL_MODE, buff); /* NFC-A technology */

    if (ret < 0) {
        LOG_ERROR("pn7160: poll for NFC-A device failed\n");
        return ret;
    }

    /* we now have the discovered technologies */
    if (buff[PN7160_PAYLOAD_START + 3] != NCI_NFC_A_PASSIVE_POLL_MODE) {
        LOG_ERROR("pn7160: not NFC-A passive poll mode (%u)!\n", 
            (unsigned)buff[PN7160_PAYLOAD_START + 3]);
        return -1;
    }

    // uint8_t length_of_parameters = buff[PN7160_PAYLOAD_START + 6];

    /* RF technology specific parameters (NFC-A) */
    config->sens_res.anticollision_information = buff[PN7160_PAYLOAD_START + 7];
    config->sens_res.platform_information      = buff[PN7160_PAYLOAD_START + 8];
    config->nfcid1.len                         = buff[PN7160_PAYLOAD_START + 9];
    memcpy(config->nfcid1.nfcid, &buff[PN7160_PAYLOAD_START + 10], config->nfcid1.len);

    uint8_t sel_res_len = buff[PN7160_PAYLOAD_START + 10 + config->nfcid1.len];
    if (sel_res_len != 1) {
        return NFC_ERR_POLL_NO_TARGET;
    }
    config->sel_res = buff[PN7160_PAYLOAD_START + 11 + config->nfcid1.len];

    return 0;
}

// static int _clock_sel_xtal(pn7160_t *dev, uint8_t *buff) {
//     assert (dev != NULL);
//     memcpy(&buff[PN7160_PAYLOAD_START], PN7160_CLOCK_SEL_XTAL, sizeof(PN7160_CLOCK_SEL_XTAL));
//     return _core_set_config_cmd(dev, buff, sizeof(PN7160_CLOCK_SEL_XTAL));
// }

// static int _disable_tag_detector(pn7160_t *dev, uint8_t *buff) {
//     assert (dev != NULL);
//     memcpy(&buff[PN7160_PAYLOAD_START], PN7160_TAG_DETECTOR_DISABLE, sizeof(PN7160_TAG_DETECTOR_DISABLE));
//     return _core_set_config_cmd(dev, buff, sizeof(PN7160_TAG_DETECTOR_DISABLE));
// }

// static int _set_total_duration(pn7160_t *dev, uint8_t *buff) {
//     assert (dev != NULL);
//     memcpy(&buff[PN7160_PAYLOAD_START], PN7160_TOTAL_DURATION, sizeof(PN7160_TOTAL_DURATION));
//     return _core_set_config_cmd(dev, buff, sizeof(PN7160_TOTAL_DURATION));
// }

int pn7160_listen_a(nfcdev_t *dev, const nfc_a_listener_config_t *config) {
    assert (dev != NULL);
    assert (config != NULL);

    if ((config->sel_res & NFC_A_SEL_RES_T2T_MASK) == NFC_A_SEL_RES_T2T_VALUE) {
        LOG_ERROR("pn7160: T2T emulation not supported\n");
        return -1;
    } else if ((config->sel_res & NFC_A_SEL_RES_T4T_MASK) == NFC_A_SEL_RES_T4T_VALUE) {
        // _core_set_config_cmd(dev, buff, sizeof(buff));
    } else {
        LOG_ERROR("pn7160: unsupported SEL_RES (%02X)!\n", config->sel_res);
        return -1;
    }
    uint8_t buff[PN7160_BUFFER_LEN];


    /* this is done so the device operates properly at 3.3V */
    _configuration_3_3V(dev->dev, buff);
    _disable_standby(dev->dev, buff);
    // _set_rf_config(dev->dev, buff);
    // _set_total_duration(dev->dev, buff);

    _reset_sequence(dev->dev, buff);

    _rf_discover_map_cmd(dev->dev, 
        NCI_PROTOCOL_ISO_DEP,       /* RF protocol */
        NCI_RF_MODE_LISTEN,         /* Listen mode */
        NCI_RF_INTERFACE_ISO_DEP);  /* RF interface */
    
    _nfc_a_set_listen_mode_routing(dev->dev, buff);

    assert(config->nfcid1.len == 4 || config->nfcid1.len == 7 || config->nfcid1.len == 10);


    buff[PN7160_PAYLOAD_START] = 0x06;       /* num of parameters */

    /* The buffer will be filled with correct NFC-A listener config data */
    /* SEL_RES */
    buff[PN7160_PAYLOAD_START + 1] = NCI_LA_SEL_INFO;
    buff[PN7160_PAYLOAD_START + 2] = 0x01;
    buff[PN7160_PAYLOAD_START + 3] = config->sel_res;

    /* SENS_RES anticollision */
    buff[PN7160_PAYLOAD_START + 4] = NCI_LA_BIT_FRAME_SDD;
    buff[PN7160_PAYLOAD_START + 5] = 0x01;
    buff[PN7160_PAYLOAD_START + 6] = config->sens_res.anticollision_information;

    /* SENS_RES platform information */
    buff[PN7160_PAYLOAD_START + 7] = NCI_LA_PLATFORM_CONFIG;
    buff[PN7160_PAYLOAD_START + 8] = 0x01;
    buff[PN7160_PAYLOAD_START + 9] = config->sens_res.platform_information;

    buff[PN7160_PAYLOAD_START + 10] = NCI_LI_A_RATS_TB1;
    buff[PN7160_PAYLOAD_START + 11] = 0x01;
    buff[PN7160_PAYLOAD_START + 12] = 0xF0;

    buff[PN7160_PAYLOAD_START + 13] = NCI_LI_A_RATS_TC1;
    buff[PN7160_PAYLOAD_START + 14] = 0x01;
    buff[PN7160_PAYLOAD_START + 15] = 0x00;

    /* NFCID1 */
    buff[PN7160_PAYLOAD_START + 16] = NCI_LA_NFCID1;
    buff[PN7160_PAYLOAD_START + 17] = config->nfcid1.len;
    memcpy(&buff[PN7160_PAYLOAD_START + 18], config->nfcid1.nfcid, config->nfcid1.len);

    int ret = _core_set_config_cmd((pn7160_t *)dev->dev, buff, 18 + config->nfcid1.len);
    if (ret < 0) {
        LOG_ERROR("pn7160: set config failed\n");
        return ret;
    }

    _nfc_a_set_listen_mode_routing((pn7160_t *)dev->dev, buff);

    /* prepare buffer for discover command */
    buff[PN7160_PAYLOAD_START]     = 0x01; /* num of configurations */
    buff[PN7160_PAYLOAD_START + 1] = NCI_NFC_A_PASSIVE_LISTEN_MODE; /* NFC-A technology */
    buff[PN7160_PAYLOAD_START + 2] = 0x01; /* frequency */


    ret = _rf_discover_cmd((pn7160_t *)dev->dev, 
        NCI_NFC_A_PASSIVE_LISTEN_MODE, /* NFC-A technology */
        buff);
    if (ret < 0) {
        LOG_ERROR("pn7160: listen for NFC-A device failed\n");
        return ret;
    }
    return 0;
}

static int _send_and_rcv_ntf_message(pn7160_t *dev, uint8_t *buff, size_t len) {
    assert (dev != NULL);
    construct_packet_header(buff, NCI_MT_DATA, 0, 0, 0, len);
    int ret = send_recv((pn7160_t *)dev, buff, NCI_HEADER_SIZE + len);
    if (ret <= 0) {
        return -1;
    }

    /* this should be a credits NTF */
    if (NCI_GET_MT(buff[0]) != NCI_MT_NTF) {
        LOG_ERROR("pn7160: invalid mt (%i)!\n", NCI_GET_MT(buff[0]));
        return -1;
    }

    if (NCI_GET_GID(buff[0]) != NCI_GID_CORE) {
        LOG_ERROR("pn7160: invalid gid (%i)!\n", NCI_GET_GID(buff[0]));
        return -1;
    }

    if (NCI_GET_OID(buff[1]) != NCI_OID_CORE_CONN_CREDITS) {
        LOG_ERROR("pn7160: invalid oid (%i)!\n", NCI_GET_OID(buff[1]));
        return -1;
    }

    return 0;
}

static int _send_and_rcv_data_message(pn7160_t *dev, uint8_t *buff, size_t len) {
    assert (dev != NULL);
    construct_packet_header(buff, NCI_MT_DATA, 0, 0, 0, len);
    int ret = send_recv((pn7160_t *)dev, buff, NCI_HEADER_SIZE + len);

    /* this should be a credits NTF */
    if (NCI_GET_MT(buff[0]) != NCI_MT_NTF) {
        LOG_ERROR("pn7160: invalid mt (%i)!\n", NCI_GET_MT(buff[0]));
        return -1;
    }

    if (NCI_GET_GID(buff[0]) != NCI_GID_CORE) {
        LOG_ERROR("pn7160: invalid gid (%i)!\n", NCI_GET_GID(buff[0]));
        return -1;
    }

    if (NCI_GET_OID(buff[1]) != NCI_OID_CORE_CONN_CREDITS) {
        LOG_ERROR("pn7160: invalid oid (%i)!\n", NCI_GET_OID(buff[1]));
        return -1;
    }

    ret = block_with_timeout(dev, 2);
    if (ret < 0) {
        return ret;
    }

    ret = _read((pn7160_t *)dev, buff);
    if (ret < 0) {
        return ret;
    }

    if (NCI_GET_MT(buff[0]) != NCI_MT_DATA) {
        LOG_ERROR("pn7160: invalid mt (%i)!\n", NCI_GET_MT(buff[0]));
        return -1;
    }

    ret--;

    /* the last byte must be 0x00 */
    if (buff[ret] != 0x00) {
        LOG_ERROR("pn7160: invalid last byte (%i)!\n", buff[ret]);
        return -1;
    }

    return ret;
}

int pn7160_initiator_exchange_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len,
                                  uint8_t *rcv, size_t *receive_len) {
    assert(nfcdev != NULL);
    assert(send != NULL);
    assert(send_len > 0);
    assert(receive_len != NULL);

    pn7160_t *dev = (pn7160_t *) nfcdev->dev;
    uint8_t buff[PN7160_BUFFER_LEN];

    if (send_len > (PN7160_BUFFER_LEN - NCI_HEADER_SIZE)) {
        LOG_ERROR("pn7160: send_len too large (%u)!\n", (unsigned)send_len);
        return -1;
    }

    if (!dev->is_mifare_classic) {
        memcpy(&buff[PN7160_PAYLOAD_START], send, send_len);
    } else {
        /* MIFARE Classic tags operate with the TAG CMD interface and need a header */
        memcpy(&buff[PN7160_PAYLOAD_START + 1], send, send_len);
        buff[PN7160_PAYLOAD_START] = PN7160_TAG_CMD_XCHG_DATA_ID;
        send_len++;
    }

    int data_len = 0;
    if (*receive_len == 0 || rcv == NULL) {
        /* don't wait for data and return after receiving the NTF */
        return _send_and_rcv_ntf_message(dev, buff, send_len);
    } else {
        /* also receive data */
        data_len = _send_and_rcv_data_message(dev, buff, send_len) - NCI_HEADER_SIZE;
    }

    if (dev->is_mifare_classic)  {
        /* subtract the 1 byte TAG-CMD header */
        data_len--;
    }

    if (data_len < 0) {
        return -1;
    }

    if ((size_t) data_len > *receive_len) {
        LOG_ERROR("pn7160: buffer is too small (%u < %u)!\n", (unsigned)*receive_len, 
            (unsigned)data_len);
        *receive_len = 0;
        return -1;
    }

    *receive_len = data_len;

    if (!dev->is_mifare_classic) {
        memcpy(rcv, &buff[PN7160_PAYLOAD_START], *receive_len);
    } else {
        memcpy(rcv, &buff[PN7160_PAYLOAD_START + 1], *receive_len);
    }

    return 0;
}

int pn7160_target_send_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len)
{
    assert(nfcdev != NULL);
    assert(send != NULL);
    assert(send_len > 0);

    pn7160_t *dev = (pn7160_t *) nfcdev->dev;
    uint8_t buff[PN7160_BUFFER_LEN];

    if (send_len > (PN7160_BUFFER_LEN - NCI_HEADER_SIZE)) {
        LOG_ERROR("pn7160: send_len too large (%u)!\n", (unsigned)send_len);
        return -1;
    }

    memcpy(&buff[PN7160_PAYLOAD_START], send, send_len);

    int ret = _send_and_rcv_data_message(dev, buff, send_len);
    if (ret < 0) {
        return ret;
    }

    return 0;
}

int pn7160_target_receive_data(nfcdev_t *nfcdev, uint8_t *rcv, size_t *receive_len) {
    assert(nfcdev != NULL);
    assert(rcv != NULL);
    assert(receive_len != NULL);

    pn7160_t *dev = (pn7160_t *) nfcdev->dev;
    uint8_t buff[PN7160_BUFFER_LEN];

    int ret = block_with_timeout(dev, 2);
    if (ret < 0) {
        return ret;
    }

    *receive_len = _read(dev, buff);
    if (*receive_len == 0) {
        return -1;
    }

    if (NCI_GET_MT(buff[0]) != NCI_MT_DATA) {
        LOG_ERROR("pn7160: invalid mt (%i)!\n", NCI_GET_MT(buff[0]));
        return -1;
    }

    *receive_len -= NCI_HEADER_SIZE;
    memcpy(rcv, &buff[PN7160_PAYLOAD_START], *receive_len);

    return 0;
}

static uint8_t block_to_sector(uint8_t block) {
    if (block >= MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT * MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR) {
        /* large sector */
        return MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT +
            (block - MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT * MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR) /
            MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR;
    } else {
        /* small sector */
        return block / MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR;
    }
}

int pn7160_mifare_classic_authenticate(nfcdev_t *nfcdev, uint8_t block, 
    const nfc_a_nfcid1_t *nfcid1, bool is_key_a, const uint8_t *key) {
        /**
         * MIFARE Classic tags require a special TAG-CMD interface for interaction
         * that has its own header.
         */
        (void) nfcid1;

        pn7160_t *dev = (pn7160_t *) nfcdev->dev;

        uint8_t buff[PN7160_BUFFER_LEN];

        /* the TAG-CMD packet header is inside the payload */
        buff[PN7160_PAYLOAD_START] = PN7160_TAG_CMD_MFC_AUTHENTICATE_ID;

        /* the sector that will be authenticated */
        buff[PN7160_PAYLOAD_START + 1] = block_to_sector(block);

        /* the key selector, use the specified key and set the highest bit depending on key A/B */
        buff[PN7160_PAYLOAD_START + 2] = 0b00010000 | (is_key_a ? 0b00000000 : 0b10000000);

        /* copy the 6 byte key */
        memcpy(&buff[PN7160_PAYLOAD_START + 3], key, 6);


        int ret = _send_and_rcv_data_message(dev, buff, 1 + 1 + 1 + 6);
        if (ret < 0) {
            return -1;
        }

        /* check the rsp header and the status at the end of the payload */
        if (buff[PN7160_PAYLOAD_START] == PN7160_TAG_CMD_MFC_AUTHENTICATE_ID && 
            buff[PN7160_PAYLOAD_START + 1] == 0x00)  {
            dev->is_mifare_classic = true;
            return 0;
        } else {
            return -1;
        }
}
