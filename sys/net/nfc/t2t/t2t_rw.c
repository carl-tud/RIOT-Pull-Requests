/* Reader write of the T2T */

#include "net/nfc/t2t/t2t_rw.h"
#include "net/nfc/t2t/t2t.h"

#include <memory.h>

#include "log.h"

/* read the entire t2t into the t2t struct*/
int nfc_t2t_rw_read(nfc_t2t_rw_t *rw) {
    if(rw == NULL || rw->dev == NULL || rw->tag == NULL){
        LOG_ERROR("t2t_rw_read: null pointer\n");
        return -1;
    }

    uint8_t cmd_buffer[5];
    uint8_t resp_buffer[16];
    size_t resp_len = 0;

    /* read until the end of the tag is reached 
     the end of the tag is */

    /* Poll for card presence */
    if (NFC_ERR_POLL_NO_TARGET == rw->dev->ops->poll_a(rw->dev)) {
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
