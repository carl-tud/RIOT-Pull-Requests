
#include "net/nfc/t4t/t4t.h"
#include "net/nfc/t4t/t4t_rw.h"
#include "assert.h"
#include "log.h"
#include "memory.h"

#define SELECT_RESPONSE_LENGTH 16
#define READ_RESPONSE_LENGTH 64

#define UPDATE_CAPDU_LENGTH 64
#define UPDATE_RAPDU_LENGTH 8

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

    if (response_len < 2) {
        LOG_ERROR("[T4T RW] SELECT NDEF application response too short\n");
        return -1;
    }

    if (response[response_len - 2] != 0x90 || response[response_len - 1] != 0x00) {
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

    if (response_len - 2 != T4T_CC_FILE_SIZE) {
        LOG_ERROR("[T4T RW] Wrong CC file size, must be %u but is %u\n", T4T_CC_FILE_SIZE, 
        response_len - 2);
        return -1;
    }

    /* copy t4t_cc_file */
    cc_file->cc_len = 
        ((uint16_t)response[0] << 8) | response[1];
    
    cc_file->mapping_version = response[2];
    
    cc_file->mle = 
        ((uint16_t)response[3] << 8) | response[4];
    
    cc_file->mlc = 
        ((uint16_t)response[5] << 8) | response[6];

    /* NDEF file control TLV */
    cc_file->ndef_file_control_tlv.type = response[7];
    cc_file->ndef_file_control_tlv.length = response[8];
    
    cc_file->ndef_file_control_tlv.file_id = 
        ((uint16_t)response[9] << 8) | response[10];
    
    cc_file->ndef_file_control_tlv.max_ndef_size = 
        ((uint16_t)response[11] << 8) | response[12];
    
    cc_file->ndef_file_control_tlv.read_access = response[13];
    cc_file->ndef_file_control_tlv.write_access = response[14];

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
     uint16_t file_size, uint16_t maximum_rapdu_size) {
    assert(rw != NULL);
    assert(buffer != NULL);

    uint8_t response[READ_RESPONSE_LENGTH];
    size_t response_len = READ_RESPONSE_LENGTH;

    uint8_t read_nlen_apdu[] = {
        APDU_CLA_DEFAULT,
        APDU_INS_READ_BINARY,
        0x00,
        0x00,
        0x02, /* Le = 2 to read NLEN */
    };

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, read_nlen_apdu,
        sizeof(read_nlen_apdu), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send READ NDEF file NLEN\n");
        return -1;
    }

    /* the first two bytes contain the size of the NDEF message */
    uint16_t nlen = response[0] << 8 | response[1];
    buffer[0] = response[0];
    buffer[1] = response[1];

    LOG_DEBUG("[T4T RW] NDEF file length: %u bytes\n", nlen);

    uint16_t offset = 2;
    /* read until the entire NDEF file is read */
    while (((offset - 2u) < nlen) && (offset < file_size) && (offset < buffer_size)) {
        uint16_t le = (nlen - (offset - 2) < (READ_RESPONSE_LENGTH - 2)) ?
            (nlen - (offset - 2)) : (READ_RESPONSE_LENGTH - 2);
        le = (le < maximum_rapdu_size - 2) ? le : (maximum_rapdu_size - 2);
        uint8_t read_ndef_apdu[] = {
            APDU_CLA_DEFAULT,
            APDU_INS_READ_BINARY,
            (offset >> 8) & 0xFF,
            offset & 0xFF,
            le,
        };

        response_len = 64;
        ret = rw->dev->ops->initiator_exchange_data(rw->dev, read_ndef_apdu,
            sizeof(read_ndef_apdu), response, &response_len);
        if (ret != 0) {
            LOG_ERROR("[T4T RW] Failed to send READ NDEF file APDU\n");
            return -1;
        }

        uint16_t payload_len = response_len - 2;

        if (!rapdu_is_valid(response, response_len)) {
            LOG_ERROR("[T4T RW] READ NDEF file APDU response invalid\n");
            return -1;
        }

        if (offset + payload_len > buffer_size) {
            LOG_ERROR("[T4T RW] NDEF file buffer too small\n");
            return -1;
        }

        memcpy(buffer + offset, response, payload_len);

        offset += payload_len;
    }

    return 0;
    
}

static int nfc_t4t_write_ndef_file(nfc_t4t_rw_t *rw, const uint8_t *buffer, size_t buffer_size,
    uint16_t maximum_capdu_size) {
    assert(rw != NULL);
    assert(buffer != NULL);

    /* write the 2 byte nlen first */
    uint16_t nlen = (uint16_t) buffer_size;

    uint8_t write_nlen_apdu[] = {
        APDU_CLA_DEFAULT,
        APDU_INS_UPDATE_BINARY,
        0x00, /* offset 0 */
        0x00, /* offset 0 */
        0x02, /* Lc = 2 */
        (nlen >> 8) & 0xFF,
        nlen & 0xFF,
    };
    uint8_t write_nlen_rapdu[UPDATE_RAPDU_LENGTH];
    size_t response_len = UPDATE_RAPDU_LENGTH;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, write_nlen_apdu,
        sizeof(write_nlen_apdu), write_nlen_rapdu, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send UPDATE NDEF file NLEN APDU\n");
        return -1;
    }
#define NLEN_SIZE 2u
    uint16_t file_offset = NLEN_SIZE;

    uint8_t update_capdu[UPDATE_CAPDU_LENGTH];

    /* only for st25 */
    // maximum_capdu_size = maximum_capdu_size < (25) ? 
    //      maximum_capdu_size : (25);

    /* write until the entire NDEF file is written */
    while (file_offset - NLEN_SIZE < buffer_size) {

        // uint16_t lc = maximum_capdu_size;
        uint16_t remaining = buffer_size - (file_offset - NLEN_SIZE);
        // this is set low to avoid chaining
        uint16_t lc = remaining < maximum_capdu_size ? remaining 
            : maximum_capdu_size;


        /* lc + 5 bytes header can't be bigger than UPDATE_RAPDU_LENGTH */
        if (lc + 5 > UPDATE_CAPDU_LENGTH) {
            lc = UPDATE_CAPDU_LENGTH - 5;
        }
        update_capdu[0] = APDU_CLA_DEFAULT;
        update_capdu[1] = APDU_INS_UPDATE_BINARY;
        update_capdu[2] = (file_offset >> 8) & 0xFF;
        update_capdu[3] = file_offset & 0xFF;
        update_capdu[4] = lc;
        memcpy(&update_capdu[5], &buffer[file_offset - 2], lc);

        uint8_t update_rapdu[UPDATE_RAPDU_LENGTH];
        response_len = UPDATE_RAPDU_LENGTH;

        int ret = rw->dev->ops->initiator_exchange_data(rw->dev, update_capdu,
            lc + 5, update_rapdu, &response_len);
        if (ret != 0) {
            LOG_ERROR("[T4T RW] Failed to send UPDATE NDEF file APDU\n");
            return -1;
        }

        if (response_len < 2 || update_rapdu[response_len - 2] != APDU_SW1_NO_ERROR || 
            update_rapdu[response_len - 1] != APDU_SW2_NO_ERROR) {
            LOG_ERROR("[T4T RW] UPDATE NDEF file failed with status %02X%02X\n",
                update_rapdu[response_len - 2], update_rapdu[response_len - 1]);
            return -1;
        }

        file_offset += lc;
    }

    return 0;
    
}

static int nfc_t4t_read_ndef(nfc_t4t_rw_t *rw, ndef_t *ndef,
     uint16_t file_size, uint16_t maximum_rapdu_size) {
    
    int ret;
    ret = nfc_t4t_read_ndef_file(rw, ndef->buffer.memory, ndef_get_capacity(ndef), 
    file_size, maximum_rapdu_size);
    if (ret != 0) {
        return ret;
    }

    uint16_t ndef_length = (ndef->buffer.memory[0] << 8) | ndef->buffer.memory[1];

    /* this removes the 2-byte nlen */
    memmove(ndef->buffer.memory, ndef->buffer.memory + 2, ndef_length);

    /* adjust the ndef cursor because we do a manual memcpy instead of using the NDEF API */
    ndef->buffer.cursor = ndef->buffer.memory + ndef_length;

    ndef_from_buffer(ndef);
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
            rw->is_tag_selected = true;
            config->technology = NFC_TECHNOLOGY_A;
            return 0;
        }
    }

    if (rw->dev->ops->poll_b != NULL) {
        int ret = rw->dev->ops->poll_b(dev, (nfc_b_listener_config_t *) &config->config);
        if (ret == 0) {
            rw->is_tag_selected = true;
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
        printf("POLL FAILED\n");
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

    int ret;
    if (!rw->is_tag_selected) {
        ret = nfc_t4t_rw_poll(rw, rw->dev, &config);
        if (ret != 0) {
            LOG_ERROR("[T4T RW] Polling failed\n");
            return ret;
        }
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

    uint16_t maximum_rapdu_size = cc_file.mle;
    uint16_t ndef_file_size = cc_file.ndef_file_control_tlv.max_ndef_size;

    ret = nfc_t4t_select_ndef_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF file failed\n");
        return -1;
    }

    ret = nfc_t4t_read_ndef(rw, ndef, ndef_file_size, maximum_rapdu_size);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Reading NDEF file failed\n");
        return -1;
    }

    return 0;
}

int nfc_t4t_rw_write_ndef(nfc_t4t_rw_t *rw, const ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    /* first check the CC file */
    rw->dev = dev;

    nfc_listener_config_t config;
    int ret;
    if (!rw->is_tag_selected) {
        ret = nfc_t4t_rw_poll(rw, rw->dev, &config);
        if (ret != 0) {
            LOG_ERROR("[T4T RW] Polling failed\n");
            return ret;
        }
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

    if (cc_file.ndef_file_control_tlv.write_access == 0xFF) {
        LOG_ERROR("[T4T RW] NDEF file is read-only\n");
        return -1;
    }


    uint16_t ndef_file_size = cc_file.ndef_file_control_tlv.max_ndef_size;
    if (ndef_get_size(ndef) + 2 > ndef_file_size) {
        LOG_ERROR("[T4T RW] NDEF message too large for tag\n");
        return -1;
    }


    uint16_t maximum_capdu_size = cc_file.mlc;
    ret = nfc_t4t_select_ndef_file(rw);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Selecting NDEF file failed\n");
        return -1;
    }

    ret = nfc_t4t_write_ndef_file(rw, ndef->buffer.memory, ndef_get_size(ndef), 
        maximum_capdu_size);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Writing NDEF file failed\n");
        return -1;
    }

    return 0;
}
