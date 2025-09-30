/* Reader write of the T2T */

#include "net/nfc/t2t/t2t_rw.h"
#include "net/nfc/t2t/t2t.h"
#include "net/nfc/ndef/ndef.h"

#include <memory.h>

#include "log.h"

int nfc_t2t_rw_read_ndef(nfc_t2t_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    rw->dev = dev;
    /* reads the NDEF message from a type */

    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T2T RW] Device not initialized\n");
        return -1;
    }

    rw->dev->ops->poll_a(dev, NULL);

    /* read the first 2 blocks to get the length of the NDEF message */
    uint8_t cmd_buffer[5];
    uint8_t resp_buffer[16];
    size_t resp_len = 0;
    cmd_buffer[0] = NFC_T2T_READ_COMMAND;
    cmd_buffer[1] = 0x00; /* block 0 */
    // if(rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 2, resp_buffer, &resp_len) != 0) {
    //     LOG_ERROR("[T2T RW] Error during data exchange\n");
    //     return -1;
    // }
    // if(resp_len != 16) {
    //     LOG_DEBUG("[T2T RW] Did not receive 16 bytes, stopping read\n");
    //     return -1;
    // }

    /* read the block 4, block 4 must contain the NDEF TLV (T = 0x03) */
    cmd_buffer[0] = NFC_T2T_READ_COMMAND;
    cmd_buffer[1] = 0x04; /* block 4 */
    if(rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 2, resp_buffer, &resp_len) != 0) {
        LOG_ERROR("[T2T RW] Error during data exchange\n");
        return -1;
    }
    if(resp_len != 16) {
        LOG_ERROR("[T2T RW] Did not receive 16 bytes, stopping read\n");
        return -1;
    }

    /* Check if the block contains the NDEF TLV */
    if(resp_buffer[0] != NFC_T2T_NDEF_TLV_TYPE) {
        LOG_ERROR("[T2T RW] Block 4 does not contain NDEF TLV\n");
        return -1;
    }

    int remaining_ndef_length = 0;
    uint16_t first_byte_position = 0;
    if (resp_buffer[1] == 0xFF) {
        /* 3 bytes length field */
        remaining_ndef_length = (resp_buffer[2] << 8) | resp_buffer[3];
        LOG_DEBUG("[T2T RW] NDEF length: %u (3 bytes length field)\n", 
            (unsigned)remaining_ndef_length);
        first_byte_position = 4;
    } else {
        /* 1 byte length field */
        remaining_ndef_length = resp_buffer[1];
        LOG_DEBUG("[T2T RW] NDEF length: %u (1 byte length field)\n", 
            (unsigned)remaining_ndef_length);
        first_byte_position = 2;
    }

    ndef_put_into_buffer(ndef, &resp_buffer[first_byte_position], remaining_ndef_length);
    remaining_ndef_length -= (16 - first_byte_position);

    uint8_t block_no = 5;

    while (remaining_ndef_length > 0) {
        cmd_buffer[0] = NFC_T2T_READ_COMMAND;
        cmd_buffer[1] = block_no;

        LOG_DEBUG("[T2T RW] Reading block %u to %u\n", block_no, block_no + 3);
        if(rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 2, resp_buffer, &resp_len) != 0) {
            LOG_ERROR("[T2T RW] Error during data exchange\n");
            return -1;
        }
        if(resp_len != 16) {
            LOG_ERROR("[T2T RW] Did not receive 16 bytes, stopping read\n");
            return -1;
        }
        if (remaining_ndef_length >= 16) {
            ndef_put_into_buffer(ndef, resp_buffer, 16);
            remaining_ndef_length -= 16;
        } else {
            ndef_put_into_buffer(ndef, resp_buffer, remaining_ndef_length);
            remaining_ndef_length = 0;
        }

        block_no += 4;
    }

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
    size_t resp_len = 0;

    /* read until the end of the tag is reached 
     the end of the tag is */

    /* Poll for card presence */
    if (NFC_ERR_POLL_NO_TARGET == rw->dev->ops->poll_a(rw->dev, NULL)) {
        LOG_DEBUG("[T2T RW] No tag found\n");
        return -1;
    }

    uint8_t block_no = 0;
    while (resp_buffer[0] != NFC_T2T_NACK && block_no < 16) {
        cmd_buffer[0] = NFC_T2T_READ_COMMAND;
        cmd_buffer[1] = block_no;

        printf("[T2T RW] Reading block %u to %u\n", block_no, block_no + 3);

        if(rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 2, resp_buffer, &resp_len) != 0) {
            LOG_ERROR("Error during data exchange\n");
            return -1;
        }

        if(resp_len != 16) {
            LOG_DEBUG("Did not receive 16 bytes, stopping read\n");
            return -1;
        }

        memcpy(&(((uint8_t *)(rw->tag))[block_no * NFC_T2T_BLOCK_SIZE]), resp_buffer, 16);
        block_no += 4;
    }
    return 0;

}
