
#include "net/nfc/t4t/t4t.h"
#include "net/nfc/t4t/t4t_rw.h"
#include "assert.h"
#include "log.h"
#include "memory.h"

#define SELECT_RESPONSE_LENGTH 16
#define READ_RESPONSE_LENGTH 64

static int nfc_t4t_select_ndef_application(nfc_t4t_rw_t *rw) {
    assert(rw != NULL);

    uint8_t response[SELECT_RESPONSE_LENGTH];
    size_t response_len = SELECT_RESPONSE_LENGTH;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, T4T_APDU_NDEF_TAG_APPLICATION_SELECT,
        sizeof(T4T_APDU_NDEF_TAG_APPLICATION_SELECT), response, &response_len);
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

    uint8_t response[SELECT_RESPONSE_LENGTH];
    size_t response_len = SELECT_RESPONSE_LENGTH;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, T4T_APDU_CC_FILE_SELECT,
        sizeof(T4T_APDU_CC_FILE_SELECT), response, &response_len);
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

static int nfc_t4t_read_cc_file(nfc_t4t_rw_t *rw, t4t_cc_file_t *cc_file) {
    assert(rw != NULL);
    assert(cc_file != NULL);

    uint8_t response[READ_RESPONSE_LENGTH];
    size_t response_len = READ_RESPONSE_LENGTH;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, T4T_APDU_CC_FILE_READ,
        sizeof(T4T_APDU_CC_FILE_READ), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send READ CC file APDU\n");
        return -1;
    }

    if (!rapdu_is_valid(response, response_len)) {
        LOG_ERROR("[T4T RW] READ CC file APDU response invalid\n");
        return -1;
    }

    if (response_len - 2 != sizeof(t4t_cc_file_t)) {
        LOG_ERROR("[T4T RW] CC file too short\n");
        return -1;
    }

    memcpy(cc_file, response, sizeof(t4t_cc_file_t));
    return 0;
}

static int nfc_t4t_select_ndef_file(nfc_t4t_rw_t *rw) {
    assert(rw != NULL);

    uint8_t response[SELECT_RESPONSE_LENGTH];
    size_t response_len = 32;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, T4T_APDU_NDEF_FILE_SELECT,
        sizeof(T4T_APDU_NDEF_FILE_SELECT), response, &response_len);
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

static int nfc_t4t_read_ndef_file(nfc_t4t_rw_t *rw, uint8_t *buffer, size_t buffer_size,
     uint16_t file_size, uint16_t maximum_capdu_size) {
    assert(rw != NULL);

    /* read maximum_capdu_size bytes at once, but never read more than  */
    const uint8_t le = (maximum_capdu_size < 64) ? maximum_capdu_size : 64;

    uint8_t response[READ_RESPONSE_LENGTH];
    size_t response_len = READ_RESPONSE_LENGTH;

    uint8_t read_nlen_apdu[] = {
        APDU_CLA_DEFAULT,
        APDU_INS_READ_BINARY,
        0x00,
        0x00,
        0x02, // Le = 2 to read NLEN
    };

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, read_nlen_apdu,
        sizeof(read_nlen_apdu), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send READ NDEF file NLEN\n");
        return -1;
    }

    /* the first two bytes contain the size of the NDEF message */
    uint16_t nlen = response[0] << 8 | response[1];


    uint16_t offset = 2;
    /* read until the entire NDEF file is read */
    while ((offset < nlen) && (offset < file_size) && ((offset - 2u) < buffer_size)) {
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

        if (offset - 2 + response_len - 2 > buffer_size) {
            LOG_ERROR("[T4T RW] NDEF file buffer too small\n");
            return -1;
        }

        memcpy(buffer + (offset - 2), response, response_len - 2);

        offset += response_len - 2;

        if ((response_len - 2) < le) {
            break; /* End of file reached */
        }
    }

    return 0;
    
}

static int nfc_t4t_read_ndef(nfc_t4t_rw_t *rw, uint8_t *buffer, size_t buffer_size,
     uint16_t file_size, uint16_t maximum_capdu_size) {
    
    int ret;
    ret = nfc_t4t_read_ndef_file(rw, buffer, buffer_size, file_size, maximum_capdu_size);
    if (ret != 0) {
        return ret;
    }

    uint16_t ndef_length = (buffer[0] << 8) | buffer[1];
    memmove(buffer, buffer + 2, ndef_length);
    return 0;
}

int nfc_t4t_rw_poll(nfc_t4t_rw_t *rw, nfcdev_t *dev, nfc_listener_config_t *config) {
    assert(rw != NULL);
    assert(dev != NULL);

    rw->dev = dev;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T4T RW] Device not initialized\n");
        return -1;
    }

    if (rw->dev->ops->poll_a != NULL) {
        int ret = rw->dev->ops->poll_a(dev, (nfc_a_listener_config_t *) &config->config);
        if (ret == 0) {
            config->technology = NFC_TECHNOLOGY_A;
            return 0;
        }
    }

    if (rw->dev->ops->poll_b != NULL) {
        int ret = rw->dev->ops->poll_b(dev, (nfc_b_listener_config_t *) &config->config);
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

    uint16_t maximum_capdu_size = tag->cc_file.mlc;

    /* select the NDEF file*/
    ret = nfc_t4t_select_ndef_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF file failed\n");
        return -1;
    }

    /* read the NDEF file*/
    ret = nfc_t4t_read_ndef_file(rw, tag->ndef_file, tag->max_ndef_file_size,
        tag->cc_file.ndef_file_control_tlv.max_ndef_size, maximum_capdu_size);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Reading NDEF file failed\n");
        return -1;
    }

    return 0;
}

int nfc_t4t_rw_read_ndef(nfc_t4t_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    rw->dev = dev;

    nfc_listener_config_t config;
    int ret = nfc_t4t_rw_poll(rw, rw->dev, &config);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Polling failed\n");
        return ret;
    }

    ret = nfc_t4t_select_ndef_application(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF application failed\n");
        return ret;
    }

    /* first select the CC container */
    ret = nfc_t4t_select_cc_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting CC file failed\n");
        return ret;
    }

    t4t_cc_file_t cc_file;
    ret = nfc_t4t_read_cc_file(rw, &cc_file);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Reading CC file failed\n");
        return ret;
    }

    uint16_t maximum_capdu_size = cc_file.mlc;
    uint16_t ndef_file_size = ndef_file_size;

    ret = nfc_t4t_select_ndef_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF file failed\n");
        return -1;
    }

    ret = nfc_t4t_read_ndef(rw, ndef->buffer.memory, ndef_get_capacity(ndef), 
        ndef_file_size, maximum_capdu_size);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Reading NDEF file failed\n");
        return -1;
    }

    ndef_from_buffer(ndef);

    return 0;
}

int nfc_t4t_rw_write_ndef(nfc_t4t_rw_t *rw, const ndef_t *ndef) {
    assert(rw != NULL);
    assert(ndef != NULL);

    return -1;
}
