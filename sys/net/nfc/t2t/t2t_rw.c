/* Reader write of the T2T */

#include "net/nfc/t2t/t2t_rw.h"
#include "net/nfc/t2t/t2t.h"
#include "net/nfc/ndef/ndef.h"

#include <memory.h>

#include "log.h"
#include "assert.h"

static int nfc_t2t_rw_send_read(nfc_t2t_rw_t *rw, uint8_t block_no, uint8_t *data) {
    uint8_t cmd_buffer[2];
    cmd_buffer[0] = NFC_T2T_READ_COMMAND;
    cmd_buffer[1] = block_no;


    size_t response_len = 16;
    LOG_DEBUG("[T2T RW] Sending READ command for block %u\n", block_no);
    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 2, data, 
        &response_len);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error during READ command\n");
        return ret;
    }

    if (response_len != 16) {
        LOG_ERROR("[T2T RW] Did not receive 16 bytes in response to READ command\n");
        return -1;
    }

    return 0;
}

static int nfc_t2t_rw_send_write(nfc_t2t_rw_t *rw, uint8_t block_no, const uint8_t *data, 
    size_t data_len) {
    if (data_len != NFC_T2T_BLOCK_SIZE) {
        LOG_ERROR("[T2T RW] Data length for WRITE command must be %d bytes\n", NFC_T2T_BLOCK_SIZE);
        return -1;
    }

    uint8_t cmd_buffer[6];
    cmd_buffer[0] = NFC_T2T_WRITE_COMMAND;
    cmd_buffer[1] = block_no;
    memcpy(&cmd_buffer[2], data, NFC_T2T_BLOCK_SIZE);

    uint8_t resp_buffer[16];
    size_t resp_len = 16;

    LOG_DEBUG("[T2T RW] Sending WRITE command for block %u\n", block_no);
    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 6, resp_buffer, &resp_len);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error during WRITE command\n");
        return ret;
    }

    /* check for ack */
    if (resp_len != 1 || resp_buffer[0] != NFC_T2T_ACK) {
        LOG_ERROR("[T2T RW] Did not receive ACK after WRITE command\n");
        return -1;
    }

    return 0;
}

static int nfc_t2t_rw_read_cc(nfc_t2t_rw_t *rw, t2t_cc_t *cc) {
    assert(rw != NULL);
    assert(cc != NULL);

    uint8_t data[16];
    int ret = nfc_t2t_rw_send_read(rw, 0, data);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error reading CC\n");
        return ret;
    }

    memcpy(cc, data + NFC_T2T_INTERNAL_SIZE + NFC_T2T_LOCK_BYTES_SIZE, sizeof(t2t_cc_t));

    return 0;
}

static int t2t_poll(nfc_t2t_rw_t *rw, nfc_a_listener_config_t *config) {
    int ret = rw->dev->ops->poll_a(rw->dev, config);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error polling for T2T\n");
        return ret;
    }

    if ((config->sel_res & NFC_A_SEL_RES_T2T_MASK) != NFC_A_SEL_RES_T2T_VALUE) {
        LOG_ERROR("[T2T RW] Not a Type 2 Tag (SEL_RES: %02X)\n", config->sel_res);
        return -1;
    }

    LOG_DEBUG("[T2T RW] Found Type 2 Tag\n");

    return 0;
}

// static int nfc_t2t_rw_send_halt(nfc_t2t_rw_t *rw) {
//     uint8_t cmd_buffer[1];
//     cmd_buffer[0] = NFC_T2T_HALT_COMMAND;

//     size_t response_len = 0;
//     LOG_DEBUG("[T2T RW] Sending HALT command\n");
//     /* don't receive a response as the tag won't return anything */
//     int ret = rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 1, NULL, 
//         &response_len);

//     if (ret != 0) {
//         LOG_ERROR("[T2T RW] Error during HALT command\n");
//         return ret;
//     }

//     return 0;
// }

/* writes the NDEF message to a type 2 tag */
int nfc_t2t_rw_write_ndef(nfc_t2t_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    rw->dev = dev;

    nfc_a_listener_config_t config = {0};
    int ret = t2t_poll(rw, &config);
    if (ret != 0) {
        return ret;
    }

    /* read the first 4 blocks to get the length of the NDEF message */
    t2t_cc_t cc;
    ret = nfc_t2t_rw_read_cc(rw, &cc);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error reading CC\n");
        return ret;
    }

    if (cc.magic_number != NFC_T2T_CC_MAGIC_NUMBER) {
        LOG_ERROR("[T2T RW] CC of T2T not NDEF compatible: %02X\n", cc.magic_number);
        return -1;
    }

    uint16_t ndef_length = ndef_get_size(ndef);
    uint8_t maximum_ndef_size = cc.memory_size * 8;
    if (ndef_length + NFC_T2T_RESERVED_SIZE + NFC_TLV_MINIMUM_SIZE > maximum_ndef_size) {
        LOG_ERROR("[T2T RW] NDEF message too large for tag\n");
        return -1;
    }

    if (ndef_length == 0) {
        LOG_DEBUG("[T2T RW] NDEF message is empty, writing only terminator TLV\n");
    }

    /* write the NDEF message */
    uint8_t data[NFC_T2T_BLOCK_SIZE];

    uint16_t remaining_ndef_length = ndef_length;
    uint16_t first_byte_position = 0;

    /* prepare first block with NDEF TLV */
    data[0] = NFC_T2T_NDEF_TLV_TYPE;
    if (ndef_length > 0xFE) {
        /* 3 bytes length field */
        data[1] = 0xFF;
        data[2] = (ndef_length >> 8) & 0xFF;
        data[3] = ndef_length & 0xFF;
        first_byte_position = 4;
    } else {
        /* 1 byte length field */
        data[1] = ndef_length & 0xFF;
        first_byte_position = 2;
    }

    uint8_t block_no = 4;

    /* a T2T write command writes 4 bytes (1 block) */
    uint16_t to_write = NFC_T2T_BLOCK_SIZE - first_byte_position;

    memcpy(&data[first_byte_position], ndef->buffer.memory, to_write);
    nfc_t2t_rw_send_write(rw, block_no, data, NFC_T2T_BLOCK_SIZE);
    block_no++;

    bool written_terminator_tlv = false;
    uint16_t ndef_offset = to_write;
    while (ndef_offset < ndef_length) {
        remaining_ndef_length = ndef_length - ndef_offset;

        if (remaining_ndef_length >= NFC_T2T_BLOCK_SIZE) {
            to_write = NFC_T2T_BLOCK_SIZE;
        } else {
            /* last block, need to add terminator TLV */
            to_write = remaining_ndef_length;
            memset(data, 0, NFC_T2T_BLOCK_SIZE);
            data[to_write] = NFC_T2T_TERMINATOR_TLV_TYPE;
            written_terminator_tlv = true;
        }

        memcpy(&data, ndef->buffer.memory + ndef_offset, to_write);
        nfc_t2t_rw_send_write(rw, block_no, data, NFC_T2T_BLOCK_SIZE);

        block_no++;
        ndef_offset += to_write;
    }

    if (!written_terminator_tlv) {
        /* need to write terminator TLV */
        memset(data, 0, NFC_T2T_BLOCK_SIZE);
        data[0] = NFC_T2T_TERMINATOR_TLV_TYPE;
        nfc_t2t_rw_send_write(rw, block_no, data, NFC_T2T_BLOCK_SIZE);
    }

    /* send HALT command */
    /* ret = nfc_t2t_rw_send_halt(rw);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error sending HALT command\n");
        return ret;
    } */

    return 0;
}

int nfc_t2t_rw_set_read_only(nfc_t2t_rw_t *rw, nfcdev_t *dev) {
    assert(rw != NULL);

    rw->dev = dev;

    nfc_a_listener_config_t config = {0};
    int ret = t2t_poll(rw, &config);
    if (ret != 0) {
        return ret;
    }

    /* read the first 4 blocks to get the length of the NDEF message */
    t2t_cc_t cc;
    ret = nfc_t2t_rw_read_cc(rw, &cc);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error reading CC\n");
        return ret;
    }

    cc.read_write_access = (uint8_t) NFC_T2T_CC_READ_ONLY;

    /* write back the modified CC */
    uint8_t data[NFC_T2T_BLOCK_SIZE];
    ret = nfc_t2t_rw_send_read(rw, 1, data);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error reading block 1 for CC write back\n");
        return ret;
    }

    memcpy(data, &cc, sizeof(t2t_cc_t));

    ret = nfc_t2t_rw_send_write(rw, 1, data, NFC_T2T_BLOCK_SIZE);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error writing back modified CC\n");
        return ret;
    }

    /* send HALT command */
    /* ret = nfc_t2t_rw_send_halt(rw);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error sending HALT command\n");
        return ret;
    } */

    return 0;
}

int nfc_t2t_rw_read_ndef(nfc_t2t_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    rw->dev = dev;

    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T2T RW] Device not initialized\n");
        return -1;
    }

    nfc_a_listener_config_t config = {0};
    int ret = t2t_poll(rw, &config);
    if (ret != 0) {
        return ret;
    }

    /* read the first 4 blocks to get the length of the NDEF message */
    t2t_cc_t cc;
    ret = nfc_t2t_rw_read_cc(rw, &cc);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error reading CC\n");
        return ret;
    }

    /* read block 4, block 4 must contain the NDEF TLV (T = 0x03) */
    uint8_t data[16];
    ret = nfc_t2t_rw_send_read(rw, 4, data);
    if (ret != 0) {
        LOG_ERROR("[T2T RW] Error reading block 4\n");
        return ret;
    }

    /* Check if the block contains the NDEF TLV */
    if(data[0] != NFC_T2T_NDEF_TLV_TYPE) {
        LOG_ERROR("[T2T RW] Block 4 does not contain NDEF TLV\n");
        return -1;
    }

    /* identify NDEF TLV */
    int remaining_ndef_length = 0;
    int first_byte_position = 0;
    if (data[1] == 0xFF) {
        /* 3 bytes length field */
        remaining_ndef_length = (((uint16_t) (data[2])) << 8) | data[3];
        LOG_DEBUG("[T2T RW] NDEF length: %u (3 bytes length field)\n", 
            (unsigned)remaining_ndef_length);
        first_byte_position = 4;
    } else {
        /* 1 byte length field */
        remaining_ndef_length = data[1];
        LOG_DEBUG("[T2T RW] NDEF length: %u (1 byte length field)\n", 
            (unsigned)remaining_ndef_length);
        first_byte_position = 2;
    }

    ndef_put_into_buffer(ndef, &data[first_byte_position], remaining_ndef_length);
    remaining_ndef_length -= (16 - first_byte_position);

    /* we have already read blocks 4 to 7 */
    uint8_t block_no = 8;
    while (remaining_ndef_length > 0) {
        int ret = nfc_t2t_rw_send_read(rw, block_no, data);
        if (ret != 0) {
            LOG_ERROR("[T2T RW] Error reading block %u\n", block_no);
            return ret;
        }

        if (remaining_ndef_length >= 16) {
            ndef_put_into_buffer(ndef, data, 16);
            remaining_ndef_length -= 16;
        } else {
            ndef_put_into_buffer(ndef, data, remaining_ndef_length);
            remaining_ndef_length = 0;
        }

        block_no += 4;
    }

    /* send HALT command */
    // ret = nfc_t2t_rw_send_halt(rw);
    // if (ret != 0) {
    //     LOG_ERROR("[T2T RW] Error sending HALT command\n");
    //     return ret;
    // }

    ndef_from_buffer(ndef);

    return 0;
}

/* read the entire t2t into the t2t struct*/
int nfc_t2t_rw_read(nfc_t2t_rw_t *rw, nfc_t2t_t *tag, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(tag != NULL);
    assert(dev != NULL);

    rw->dev = dev;
    rw->tag = tag;

    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T2T RW] Device not initialized\n");
        return -1;
    }

    uint8_t cmd_buffer[5];
    uint8_t resp_buffer[16];


    /* Poll for card presence */
    nfc_a_listener_config_t config = {0};
    t2t_poll(rw, &config);

    uint8_t block_no = 0;
    while (resp_buffer[0] != NFC_T2T_NACK && block_no < (NFC_T2T_MEMORY_SIZE / NFC_T2T_BLOCK_SIZE)) {
        cmd_buffer[0] = NFC_T2T_READ_COMMAND;
        cmd_buffer[1] = block_no;

        printf("[T2T RW] Reading block %u to %u\n", block_no, block_no + 3);

        size_t resp_len = 16;
        if(rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 2, resp_buffer, &resp_len) != 0) {
            LOG_ERROR("Error during data exchange\n");
            return -1;
        }

        if(resp_len != 16) {
            LOG_ERROR("Did not receive 16 bytes, stopping read\n");
            return -1;
        }

        memcpy(&(((uint8_t *)(rw->tag))[block_no * NFC_T2T_BLOCK_SIZE]), resp_buffer, 16);
        block_no += 4;
    }
    LOG_DEBUG("[T2T RW] Read entire tag, total blocks: %u\n", block_no);

    return 0;

}
