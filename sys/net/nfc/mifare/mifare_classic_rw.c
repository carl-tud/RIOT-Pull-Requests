#include "net/nfc/mifare/mifare_classic_rw.h"
#include "net/nfc/ndef/ndef.h"

#include <assert.h>
#include <memory.h>
#include "log.h"

static int mifare_classic_rw_send_authenticate(nfc_mifare_classic_rw_t *rw,
    const nfc_a_nfcid1_t *nfcid1, uint8_t block, bool use_key_a, const uint8_t *key) {
    assert(rw != NULL);
    assert(key != NULL);

    int ret = rw->dev->ops->mifare_classic_authenticate(rw->dev, block, nfcid1, use_key_a, key);
    if (ret != 0) {
        LOG_ERROR("MFC: authenticate block %u failed\n", (unsigned)block);
        return NFC_ERR_AUTH;
    }

    return 0;
}

static int mifare_classic_rw_read_block(nfc_mifare_classic_rw_t *rw, uint8_t block, uint8_t *data) {
    assert(rw != NULL);
    assert(data != NULL);

    uint8_t send_buff[2];
    send_buff[0] = MIFARE_CLASSIC_READ; /* current block */
    send_buff[1] = block; /* read command */

    size_t resp_len = MIFARE_CLASSIC_BLOCK_SIZE;
    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, send_buff, sizeof(send_buff), data, &resp_len);
    if (ret != 0) {
        LOG_ERROR("MFC: read block %u failed\n", (unsigned)block);
        return -1;
    }

    if (resp_len != MIFARE_CLASSIC_BLOCK_SIZE) {
        LOG_ERROR("MFC: read block %u returned wrong length %u\n",
            (unsigned)block, (unsigned)resp_len);
        return -1;
    }

    return 0;
}

static int mifare_classic_rw_write_block(nfc_mifare_classic_rw_t *rw, uint8_t block,
    const uint8_t *data) {
    assert(rw != NULL);
    assert(data != NULL);

    LOG_DEBUG("MFC: writing block %u\n", (unsigned)block);

    uint8_t send_buff[2 + MIFARE_CLASSIC_BLOCK_SIZE];
    send_buff[0] = MIFARE_CLASSIC_WRITE;                      /* write command */
    send_buff[1] = block;                                     /* current block */
    memcpy(&send_buff[2], data, MIFARE_CLASSIC_BLOCK_SIZE);

    uint8_t resp_buffer[1];
    size_t resp_len = 1;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, send_buff, sizeof(send_buff), resp_buffer, &resp_len);
    if (ret != 0) {
        LOG_ERROR("MFC: write block %u failed\n", (unsigned)block);
        return -1;
    }

    // if (resp_len != 1 ) {
    //     LOG_ERROR("MFC: write block %u returned wrong length %u\n",
    //         (unsigned)block, (unsigned)resp_len);
    //     return -1;
    // }

    // if ((resp_buffer[0] & 0x0F) != MIFARE_CLASSIC_ACK) {
    //     LOG_ERROR("MFC: write block %u NACK received %02X\n",
    //         (unsigned)block, (unsigned)resp_buffer[0]);
    //     return -1;
        
    // }

    return 0;
}

static bool is_trailer_block(uint8_t block) {
    if (block >= MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT * MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR) {
        /* large sector */
        return (block % MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR) == 15;
    } else {
        /* small sector */
        return (block % MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR) == 3;
    }
}

static inline bool is_mad_sector(uint8_t sector) {
    return (sector == 0x00);
}

static int send_halt(nfc_mifare_classic_rw_t *rw) {
    uint8_t cmd_buffer[1];
    cmd_buffer[0] = MIFARE_CLASSIC_HALT_COMMAND;

    size_t response_len = 0;
    LOG_DEBUG("[MFC RW] Sending HALT command\n");
    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, 1, NULL, 
        &response_len);
    if (ret != 0) {
        LOG_ERROR("[MFC RW] Error during HALT command\n");
        return ret;
    }

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

/* The first 32 sectors contain only 3 data blocks and the last 8 sectors contain 15 blocks */
static inline bool is_small_sector(uint8_t sector) {
    return sector < 32;
}

static int authenticate_block(nfc_mifare_classic_rw_t *rw, nfc_a_listener_config_t *config,
    uint8_t block_number) {
    uint8_t sector = block_to_sector(block_number);

    /* trys to authenticate the block and uses the correct key */
    LOG_DEBUG("[MFC RW] Authenticating block %u\n", (unsigned)block_number);
    if (is_mad_sector(sector)) {
        /* MAD block, use MAD key */
        if (mifare_classic_rw_send_authenticate(rw, &config->nfcid1,
            block_number, true, MIFARE_CLASSIC_MAD_PUBLIC_KEY_A) < 0) {
            LOG_ERROR("[MFC RW] Authentication failed for MAD block\n");
            return -1;
        }
    } else {
        /* use public key */
        if (mifare_classic_rw_send_authenticate(rw, &config->nfcid1,
            block_number, true, MIFARE_CLASSIC_SECTOR_PUBLIC_KEY_A) < 0) {
            LOG_ERROR("[MFC RW] Authentication failed for sector %u\n",
                (unsigned)sector);
            return -1;
        }
    }

    return 0;
}

int nfc_mifare_classic_rw_read_ndef(nfc_mifare_classic_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    rw->dev = dev;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[MFC RW] Device not initialized\n");
        return -1;
    }

    nfc_a_listener_config_t config = {0};
    rw->dev->ops->poll_a(dev, &config);

    LOG_DEBUG("[MFC RW] Tag found\n");

    const nfc_mifare_classic_size_t size = nfc_mifare_classic_get_size(&config);

    const uint8_t small_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K) ?
    MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT : MIFARE_CLASSIC_1K_SMALL_SECTOR_COUNT;

    const uint8_t large_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K)
    ? MIFARE_CLASSIC_4K_LARGE_SECTOR_COUNT : 0;

    uint8_t block_data[MIFARE_CLASSIC_BLOCK_SIZE];

    /* set this to one so the loop is executed at least once */
    int remaining_ndef_length = 1;

    /* start at block 4 */
    uint8_t block_number = 4;

    /* now read the NDEF message */
    for (uint8_t current_sector = 0; current_sector < small_sector_count + large_sector_count;
        current_sector++) {
        const uint8_t blocks_in_sector = is_small_sector(current_sector) ?
        MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR : MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR;

        if (remaining_ndef_length <= 0) {
            /* we already read the entire NDEF message */
            break;
        }

        if (is_mad_sector(current_sector)) {
            continue; /* skip MAD sector */
        }

        /* we need to authenticate here */
        if (authenticate_block(rw, &config, block_number) < 0) {
            return -1;
        }

        /* iterate over every block in the sector */
        for (uint8_t block = 0; block < blocks_in_sector; block++) {
            if (remaining_ndef_length <= 0) {
                /* we already read the entire NDEF message */
                break;
            }

            /* perform a 16 byte read */
            if (mifare_classic_rw_read_block(rw, block_number,
                block_data) < 0) {
                LOG_ERROR("[MFC RW] Read failed for sector %u block %u\n",
                    (unsigned)current_sector, (unsigned)block);
                return -1;
            }

            /* This block should contain the NDEF TLV */
            if (block_number == 0x04) {
                if (block_data[0] != 0x03) {
                    LOG_ERROR("[MFC RW] No NDEF message TLV found in block 4\n");
                    return -1;
                }

                uint8_t first_byte_position;
                if (block_data[1] == 0xFF) {
                    /* 3 byte length field */
                    remaining_ndef_length = (int) (block_data[2] << 8) | block_data[3];
                    first_byte_position = 4;
                } else {
                    /* 1 byte length field */
                    remaining_ndef_length = (int) block_data[1];
                    first_byte_position = 2;
                }

                const int bytes_in_this_block = MIFARE_CLASSIC_BLOCK_SIZE - first_byte_position;

                const int bytes_to_copy = (remaining_ndef_length < bytes_in_this_block) ?
                    remaining_ndef_length : bytes_in_this_block;

                ndef_put_into_buffer(ndef, &block_data[first_byte_position], bytes_to_copy);
                remaining_ndef_length -= bytes_to_copy;
            } else if (is_trailer_block(block_number)) {
                /* do not read this block as it does not contain NDEF data */
            } else if (remaining_ndef_length > 0) {
                /* continue reading the NDEF message for all blocks > 4*/
                const int bytes_to_copy = (remaining_ndef_length < MIFARE_CLASSIC_BLOCK_SIZE) ?
                    remaining_ndef_length : MIFARE_CLASSIC_BLOCK_SIZE;
                ndef_put_into_buffer(ndef, block_data, bytes_to_copy);
                remaining_ndef_length -= bytes_to_copy;
            }

            block_number += 1;
        }
    }
    /* send HALT command */
    send_halt(rw);

    ndef_from_buffer(ndef);

    LOG_DEBUG("[MFC RW] Read NDEF message\n");
    return 0;
}

int nfc_mifare_classic_rw_write_ndef(nfc_mifare_classic_rw_t *rw, const ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    rw->dev = dev;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[MFC RW] Device not initialized\n");
        return -1;
    }

    nfc_a_listener_config_t config;
    rw->dev->ops->poll_a(dev, &config);

    LOG_DEBUG("[MFC RW] Card found\n");

    const nfc_mifare_classic_size_t size = nfc_mifare_classic_get_size(&config);

    const uint8_t small_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K) ?
    MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT : MIFARE_CLASSIC_1K_SMALL_SECTOR_COUNT;

    const uint8_t large_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K)
    ? MIFARE_CLASSIC_4K_LARGE_SECTOR_COUNT : 0;

    uint8_t block_data[MIFARE_CLASSIC_BLOCK_SIZE] = {0};

    size_t remaining_ndef_length = ndef_get_size(ndef);

    uint8_t block_number = 4;

    size_t ndef_offset = 0;
    /* now write the NDEF message */
    for (uint8_t current_sector = 1; 
         current_sector < small_sector_count + large_sector_count;
         current_sector++) {
        const uint8_t blocks_in_sector = is_small_sector(current_sector) ?
        MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR : MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR;

        if (remaining_ndef_length <= 0) {
            /* we already wrote the entire NDEF message */
            break;
        }

        /* we need to authenticate here */
        if (authenticate_block(rw, &config, block_number) < 0) {
            return -1;
        }

        /* iterate over every block in the sector */
        uint8_t start_block_number = block_number;
        for (; block_number < (start_block_number + blocks_in_sector); block_number++) {
            if (remaining_ndef_length <= 0) {
                /* we already wrote the entire NDEF message */
                break;
            }

            if (block_number == 0x04) {
                /* first block, write the NDEF TLV */
                block_data[0] = 0x03; /* NDEF message TLV */

                uint8_t first_byte_position;
                if (remaining_ndef_length > 0xFE) {
                    /* 3 byte length field */
                    block_data[1] = 0xFF;
                    block_data[2] = (remaining_ndef_length >> 8) & 0xFF;
                    block_data[3] = remaining_ndef_length & 0xFF;
                    first_byte_position = 4;
                } else {
                    /* 1 byte length field */
                    block_data[1] = (uint8_t) remaining_ndef_length;
                    first_byte_position = 2;
                }

                const size_t bytes_in_this_block = MIFARE_CLASSIC_BLOCK_SIZE - first_byte_position;

                const size_t bytes_to_copy = (remaining_ndef_length < bytes_in_this_block) ?
                    remaining_ndef_length : bytes_in_this_block;

                memcpy(&block_data[first_byte_position], &ndef->buffer.memory[ndef_offset], 
                    bytes_to_copy);

                int ret = mifare_classic_rw_write_block(rw, block_number, block_data);
                if (ret < 0) {
                    return -1;
                }
                ndef_offset += bytes_to_copy;
                if (bytes_to_copy > remaining_ndef_length) {
                    remaining_ndef_length = 0;
                } else {
                    remaining_ndef_length -= bytes_to_copy;
                }
            } else if (is_trailer_block(block_number)) {
                /* do not write this block as it does not contain NDEF data */
                continue;
            } else if (remaining_ndef_length > 0) {
                /* continue writing the NDEF message for all blocks > 4 */
                const size_t bytes_to_copy = (remaining_ndef_length < MIFARE_CLASSIC_BLOCK_SIZE) ?
                    remaining_ndef_length : MIFARE_CLASSIC_BLOCK_SIZE;

                memcpy(block_data, &ndef->buffer.memory[ndef_offset], bytes_to_copy);
                int ret = mifare_classic_rw_write_block(rw, block_number, block_data);
                if (ret < 0) {
                    return -1;
                }
                ndef_offset += bytes_to_copy;
                if (bytes_to_copy > remaining_ndef_length) {
                    remaining_ndef_length = 0;
                } else {
                    remaining_ndef_length -= bytes_to_copy;
                }
            }
        }
    }

    return 0;
}

uint8_t *get_pointer_to_block(nfc_mifare_classic_tag_t *tag, uint8_t block) {
    assert(tag != NULL);

    const nfc_mifare_classic_size_t size = tag->size;

    const uint8_t small_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K) ?
    MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT : MIFARE_CLASSIC_1K_SMALL_SECTOR_COUNT;

    const uint8_t large_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K)
    ? MIFARE_CLASSIC_4K_LARGE_SECTOR_COUNT : 0;

    const uint8_t total_block_count = small_sector_count * MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR +
        large_sector_count * MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR;

    if (block >= total_block_count) {
        LOG_ERROR("MFC: block %u out of range (max %u)\n",
            (unsigned)block, (unsigned)(total_block_count - 1));
        return NULL;
    }

    /* the tag data is contiguous, interpret it as a byte array */
    return &(((uint8_t *) &(tag->data))[block * MIFARE_CLASSIC_BLOCK_SIZE]);
}

int nfc_mifare_classic_rw_read(nfc_mifare_classic_rw_t *rw, nfc_mifare_classic_tag_t *tag,
    nfcdev_t *dev) {
    assert(rw != NULL);
    assert(tag != NULL);
    assert(dev != NULL);

    rw->dev = dev;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T2T RW] Device not initialized\n");
        return -1;
    }

    nfc_a_listener_config_t config;
    rw->dev->ops->poll_a(dev, &config);

    nfc_mifare_classic_size_t size = nfc_mifare_classic_get_size(&config);
    tag->size = size;

    LOG_DEBUG("[MFC RW] Card found");

    const uint8_t small_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K) ?
    MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT : MIFARE_CLASSIC_1K_SMALL_SECTOR_COUNT;

    const uint8_t large_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K)
    ? MIFARE_CLASSIC_4K_LARGE_SECTOR_COUNT : 0;

    uint8_t block_number = 0;

    for (uint8_t current_sector = 0; 
        current_sector < small_sector_count + large_sector_count;
        current_sector++) {
        const uint8_t blocks_in_sector = is_small_sector(current_sector) ?
        MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR : MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR;

        /* we need to authenticate for every sector */
        if (authenticate_block(rw, &config, block_number) < 0) {
            return NFC_ERR_AUTH;
        }

        /* iterate over every block in the sector */
        for (uint8_t block = 0; block < blocks_in_sector; block++) {
            uint8_t *block_ptr = get_pointer_to_block(tag, block_number);
            if (block_ptr == NULL) {
                return -1;
            }

            /* perform a 16 byte read */
            if (mifare_classic_rw_read_block(rw, block_number, block_ptr) < 0) {
                LOG_ERROR("[MFC RW] Read failed for sector %u block %u\n",
                    (unsigned)current_sector, (unsigned)block);
                return -1;
            }

            block_number += 1;
        }
    }
    
    return 0;
}
