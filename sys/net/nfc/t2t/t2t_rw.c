/* Reader write of the T2T */

#include "t2t_rw.h"
#include "t2t.h"

#include "log.h"

/* read the entire t2t into the t2t struct*/
int t2t_rw_read(t2t_rw_t *rw) {
    if(rw == NULL || rw->dev == NULL || rw->tag == NULL){
        LOG_ERROR("t2t_rw_read: null pointer\n");
        return -1;
    }
    nfc_t2t_t *tag = rw->tag;
    nfcdev_t *dev = rw->dev;

    uint8_t cmd_buffer[5];
    uint8_t resp_buffer[16];
    size_t resp_len = 0;

    for(uint8_t block = 0; block < (tag->usable_memory / NFC_T2T_BLOCK_SIZE); block+=4){
        cmd_buffer[0] = NFC_T2T_READ_COMMAND;
        cmd_buffer[1] = block;
        resp_len = sizeof(resp_buffer);
        if(dev->ops->initiator_exchange_data(dev, cmd_buffer, 2, resp_buffer, &resp_len) != 0){
            LOG_ERROR("Error reading block %d\n", block);
            return -1;
        }
        if(resp_len != 16){
            LOG_ERROR("Unexpected response length %ld when reading block %d\n", resp_len, block);
            return -1;
        }
        memcpy(&tag->memory[block * NFC_T2T_BLOCK_SIZE], resp_buffer, 16);
    }
    return 0;

}
