
#include "net/nfc/t4t/t4t.h"

static int nfc_t4t_select_ndef_application(nfc_t4t_rw_t *rw) {
    assert(rw != NULL);

    uint8_t response[256];
    size_t response_len = 0;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, select_ndef_apdu,
        sizeof(select_ndef_apdu), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send SELECT NDEF application APDU\n");
        return -1;
    }

    if (response_len < 2 || response[response_len - 2] != 0x90 || response[response_len - 1] != 0x00) {
        LOG_ERROR("[T4T RW] SELECT NDEF application failed with status %02X%02X\n",
            response[response_len - 2], response[response_len - 1]);
        return -1;
    }

    LOG_DEBUG("[T4T RW] SELECT NDEF application successful\n");
    return 0;
}

static int nfc_t4t_select_cc_file(nfc_t4t_rw_t *rw) {
    assert(rw != NULL);

    uint8_t response[16];
    size_t response_len = 0;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, select_cc_file_apdu,
        sizeof(select_cc_file_apdu), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send SELECT CC file APDU\n");
        return -1;
    }

    if (response_len < 2 || response[response_len - 2] != APDU_SW1_NO_ERROR || 
        response[response_len - 1] != APDU_SW2_NO_ERROR) {
        LOG_ERROR("[T4T RW] SELECT CC file failed with status %02X%02X\n",
            response[response_len - 2], response[response_len - 1]);
        return -1;
    }

    LOG_DEBUG("[T4T RW] SELECT CC file successful\n");
    return 0;
}

static int nfc_t4t_read_cc_file(nfc_t4t_rw_t *rw, nfc_t4t_cc_file_t *cc_file) {
    assert(rw != NULL);
    assert(cc_buffer != NULL);
    assert(cc_length != NULL);

    uint8_t response[32];
    size_t response_len = 0;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, read_cc_file_apdu,
        sizeof(read_cc_file_apdu), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send READ CC file APDU\n");
        return -1;
    }

    if (!apdu_is_valid(response, response_len)) {
        LOG_ERROR("[T4T RW] READ CC file APDU response invalid\n");
        return -1;
    }

    uint8_t data_leng = apdu_get_lc(response, response_len);
    uint8_t *data = apdu_get_data(response, response_len);

    if (data_len != sizeof(nfc_t4t_cc_file_t)) {
        LOG_ERROR("[T4T RW] CC file too short\n");
        return -1;
    }

    memcpy(cc_file, data, sizeof(nfc_t4t_cc_file_t));
    return 0;
}

static int nfc_t4t_select_ndef_file(nfc_t4t_rw_t *rw) {
    assert(rw != NULL);

    uint8_t response[32];
    size_t response_len = 0;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, select_ndef_file_apdu,
        sizeof(select_ndef_file_apdu), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send SELECT NDEF file APDU\n");
        return -1;
    }

    if (response_len < 2 || response[response_len - 2] != APDU_SW1_NO_ERROR || 
        response[response_len - 1] != APDU_SW2_NO_ERROR) {
        LOG_ERROR("[T4T RW] SELECT NDEF file failed with status %02X%02X\n",
            response[response_len - 2], response[response_len - 1]);
        return -1;
    }

    LOG_DEBUG("[T4T RW] SELECT NDEF file successful\n");
    return 0;
}

static int nfc_t4t_read_ndef_file(nfc_t4t_rw_t *rw, uint8_t *ndef_file, size_t ndef_file_size,
     uint16_t file_size, uint16_t maximum_capdu_size) {
    assert(rw != NULL);

    const uint8_t le = 64;

    uint8_t response[64];
    uint8_t response_len = 0;

    uint8_t read_nlen_apdu[] = {
        APDU_CLA_DEFAULT,
        APDU_INS_READ_BINARY,
        0x00,
        0x00,
        0x02, // Le = 2 to read NLEN
    };

    rw->dev->ops->initiator_exchange_data(rw->dev, read_nlen_apdu,
        sizeof(read_nlen_apdu), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send READ NDEF file NLEN\n");
        return -1;
    }

    uint16_t nlen = data[0] << 8 | data[1];


    uint16_t offset = 2;
    /* read until the entire NDEF file is read */
    while ((offset < nlen) && (offset < file_size)) {
        uint8_t read_ndef_apdu[] = {
            APDU_CLA_DEFAULT,
            APDU_INS_READ_BINARY,
            (offset >> 8) & 0xFF,
            offset & 0xFF,
            le, // Le
        };

        size_t response_len = 0;
        ret = rw->dev->ops->initiator_exchange_data(rw->dev, read_ndef_apdu,
            sizeof(read_ndef_apdu), response, &response_len);
        if (ret != 0) {
            LOG_ERROR("[T4T RW] Failed to send READ NDEF file APDU\n");
            return -1;
        }

        if (!rapdu_is_valid(response, response_len)) {
            LOG_ERROR("[T4T RW] READ NDEF file APDU response invalid\n");
            return -1;
        }

        if (offset - 2 + response_len - 2 > ndef_file_size) {
            LOG_ERROR("[T4T RW] NDEF file buffer too small\n");
            return -1;
        }

        memcpy(ndef_file + (offset - 2), data, response_len - 2);

        offset += response_len - 2;

        if ((response_len - 2) < le) {
            break; /* End of file reached */
        }
    }
    
}

int nfc_t4t_rw_poll(nfc_t4t_rw_t *rw, nfcdev_t *dev, nfc_listener_config_t *config) {
    assert(rw != NULL);
    assert(dev != NULL);

    rw->dev = dev;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T4T RW] Device not initialized\n");
        return -1;
    }

    config->listener = rw->dev->listener;
    if (rw->dev->ops->poll_a != NULL) {
        int ret = rw->dev->ops->poll_a(dev, &config->config);
        if (ret == 0) {
            config->technology = NFC_TECHNOLOGY_A;
            return 0;
        }
    }

    if (rw->dev->ops->poll_b != NULL) {
        int ret = rw->dev->ops->poll_b(dev, &config->config);
        if (ret == 0) {
            config->technology = NFC_TECHNOLOGY_B;
            return 0;
        }
    }

    return NFC_ERR_POLL_NO_TARGET;
}

int nfc_t4t_rw_read_tag(nfc_t4t_rw_t *rw, nfc_t4t_t *tag, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(tag != NULL);

    rw->dev = dev;
    rw->tag = tag;

    nfc_listener_config_t config;
    int ret = nfc_t4t_rw_poll(rw, dev, &config);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Polling failed\n");
        return -1;
    }

    /* select the NDEF application */
    ret = nfc_t4t_select_ndef_application(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF application failed\n");
        return -1;
    }

    /* first select the CC container */
    ret = nfc_t4t_select_cc_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting CC file failed\n");
        return -1;
    }

    /* read the CC file */
    ret = nfc_t4t_read_cc_file(rw, &tag->cc_file);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Reading CC file failed\n");
        return -1;
    }

    uint16_t maximum_capdu_size = tag->cc_file.max_capdu_size;
    uint16_t ndef_file_size = (tag->cc_file.ndef_file_size[0] << 8) | tag->cc_file.ndef_file_size[1];
    if (ndef_file_size > sizeof(tag->ndef_file_buffer)) {
        LOG_ERROR("[T4T RW] NDEF file size %u exceeds buffer size %u\n",
            ndef_file_size, (uint32_t)sizeof(tag->ndef_file_buffer));
        return -1;
    }

    /* select the NDEF file*/
    ret = nfc_t4t_select_ndef_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF file failed\n");
        return -1;
    }

    /* read the NDEF file*/
    ret = nfc_t4t_read_ndef_file(rw, maximum_capdu_size);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Reading NDEF file failed\n");
        return -1;
    }

    return 0;
}

int nfc_t4t_rw_read_ndef(nfc_t4t_rw_t *rw, ndef_t *ndef) {
    assert(rw != NULL);
    assert(ndef != NULL);
    assert(ndef_len != NULL);

    nfc_listener_config_t config;
    nfc_t4t_rw_poll(rw, rw->dev, &config);

    ret = nfc_t4t_select_ndef_application(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF application failed\n");
        return -1;
    }

    /* first select the CC container */
    ret = nfc_t4t_select_cc_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting CC file failed\n");
        return -1;
    }

    ret = nfc_t4t_read_cc_file(rw, &tag->cc_file);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Reading CC file failed\n");
        return -1;
    }

    uint16_t maximum_capdu_size = tag->cc_file.max_capdu_size;
    uint16_t ndef_file_size = (tag->cc_file.ndef_file_size[0] << 8) | tag->cc_file.ndef_file_size[1];
    if (ndef_file_size > sizeof(tag->ndef_file_buffer)) {
        LOG_ERROR("[T4T RW] NDEF file size %u exceeds buffer size %u\n",
            ndef_file_size, (uint32_t)sizeof(tag->ndef_file_buffer));
        return -1;
    }

    ret = nfc_t4t_select_ndef_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF file failed\n");
        return -1;
    }

    ret = nfc_t4t_read_ndef_file(rw, ndef->buffer.memory, ndef_get_capacity(ndef), 
        maximum_capdu_size);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Reading NDEF file failed\n");
        return -1;
    }

    return 0;
}

int nfc_t4t_rw_write_ndef(nfc_t4t_rw_t *rw, const ndef_t *ndef) {
    assert(rw != NULL);
    assert(ndef != NULL);

    
}
