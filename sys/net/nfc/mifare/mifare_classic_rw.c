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
        LOG_ERROR("mifare classic: authenticate block %u failed\n", (unsigned)block);
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

    size_t resp_len = 0;
    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, send_buff, sizeof(send_buff), data, &resp_len);
    if (ret != 0) {
        LOG_ERROR("mifare classic: read block %u failed\n", (unsigned)block);
        return -1;
    }

    if (resp_len != MIFARE_CLASSIC_BLOCK_SIZE) {
        LOG_ERROR("mifare classic: read block %u returned wrong length %u\n",
            (unsigned)block, (unsigned)resp_len);
        return -1;
    }

    return 0;
}

// static int mifare_classic_rw_write_block(nfc_mifare_classic_rw_t *rw, uint8_t block,
//     const uint8_t *data) {
//     assert(rw != NULL);
//     assert(data != NULL);

//     uint8_t send_buff[2 + MIFARE_CLASSIC_BLOCK_SIZE];
//     send_buff[0] = MIFARE_CLASSIC_WRITE;                      /* write command */
//     send_buff[1] = block;                                     /* current block */
//     memcpy(&send_buff[2], data, MIFARE_CLASSIC_BLOCK_SIZE);

//     uint8_t resp_buffer[1];
//     size_t resp_len = 0;
//     int ret = rw->dev->ops->initiator_exchange_data(rw->dev, send_buff, sizeof(send_buff), resp_buffer, &resp_len);
//     if (ret != 0) {
//         LOG_ERROR("mifare classic: write block %u failed\n", (unsigned)block);
//         return -1;
//     }

//     if (resp_len != 1 || resp_buffer[0] != MIFARE_CLASSIC_ACK) {
//         LOG_ERROR("mifare classic: write block %u returned error %u\n",
//             (unsigned)block, (unsigned)resp_buffer[0]);
//         return -1;
//     }

//     return 0;
// }

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

// static uint8_t block_to_sector(uint8_t block) {
//     if (block >= MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT * MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR) {
//         /* large sector */
//         return MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT +
//             (block - MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT *
//             MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR) / MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR;
//     } else {
//         /* small sector */
//         return block / MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR;
//     }
// }

/* Returns the first block number of a sector */
static uint8_t sector_to_block(uint8_t sector) {
    if (sector >= MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT) {
        /* large sector */
        return MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT * MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR +
            (sector - MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT) * MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR;
    } else {
        /* small sector */
        return sector * MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR;
    }
}

/* The first 32 sectors contain only 3 data blocks and the last 8 sectors contain 15 blocks */
static inline bool is_small_sector(uint8_t sector) {
    return sector < 32;
}

int nfc_mifare_classic_rw_read_ndef(nfc_mifare_classic_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    rw->dev = dev;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[MIFARE CLASSIC RW] Device not initialized\n");
        return -1;
    }

    nfc_a_listener_config_t config = {0};
    rw->dev->ops->poll_a(dev, &config);

    LOG_DEBUG("[MIFARE CLASSIC RW] Card found\n");

    const nfc_mifare_classic_size_t size = nfc_mifare_classic_get_size(&config);

    const uint8_t small_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K) ?
    MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT : MIFARE_CLASSIC_1K_SMALL_SECTOR_COUNT;

    const uint8_t large_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K)
    ? MIFARE_CLASSIC_4K_LARGE_SECTOR_COUNT : 0;

    /* every sector needs to be authenticated */
    bool is_authenticated = false;

    uint8_t block_data[MIFARE_CLASSIC_BLOCK_SIZE];

    /* set this to one so the loop is executed at least once */
    int remaining_ndef_length = 1;

    /* now read the NDEF message */
    for (uint8_t current_sector = 0; current_sector < small_sector_count + large_sector_count;
        current_sector++) {
        const uint8_t blocks_in_sector = is_small_sector(current_sector) ?
        MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR : MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR;

        /* iterate over every block in the sector */
        for (uint8_t block = 0; block < blocks_in_sector; block++) {
            if (remaining_ndef_length <= 0) {
                /* we already read the entire NDEF message */
                break;
            }

            if (current_sector == 0x00) {
                continue; /* skip MAD sector */
            }

            const uint8_t block_number = sector_to_block(current_sector) + block;

            if (!is_authenticated) {
                /* Authenticate the sector before reading */
                LOG_DEBUG("[MIFARE CLASSIC RW] Authenticating block %u\n", (unsigned)block_number);
                if (is_mad_sector(current_sector)) {
                    /* MAD block, use MAD key */
                    if (mifare_classic_rw_send_authenticate(rw, &config.nfcid1,
                        block_number, true, MIFARE_CLASSIC_MAD_PUBLIC_KEY_A) < 0) {
                        LOG_ERROR("[MIFARE CLASSIC RW] Authentication failed for MAD block\n");
                        return -1;
                    }
                } else {
                    /* use public key*/
                    if (mifare_classic_rw_send_authenticate(rw, &config.nfcid1,
                        block_number, true, MIFARE_CLASSIC_SECTOR_PUBLIC_KEY_A) < 0) {
                        LOG_ERROR("[MIFARE CLASSIC RW] Authentication failed for sector %u\n",
                            (unsigned)current_sector);
                        return -1;
                    }
                }
                is_authenticated = true;
            }

            if (is_trailer_block(block_number)) {
                /* 
                 * Don't read the trailer block as it only contains access modifiers and the
                 * two keys. We need to authenticate afterwards.
                 */
                is_authenticated = false;
                continue;
            }

            /* perform a 16 byte read */
            if (mifare_classic_rw_read_block(rw, block_number,
                block_data) < 0) {
                LOG_ERROR("[MIFARE CLASSIC RW] Read failed for sector %u block %u\n",
                    (unsigned)current_sector, (unsigned)block);
                return -1;
            }

            /* This block should contain the NDEF TLV */
            if (block_number == 0x04) {
                if (block_data[0] != 0x03) {
                    LOG_ERROR("[MIFARE CLASSIC RW] No NDEF message TLV found in block 2\n");
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
            } else if (remaining_ndef_length > 0) {
                /* continue reading the NDEF message for all blocks > 2*/
                const int bytes_to_copy = (remaining_ndef_length < MIFARE_CLASSIC_BLOCK_SIZE) ?
                    remaining_ndef_length : MIFARE_CLASSIC_BLOCK_SIZE;
                ndef_put_into_buffer(ndef, block_data, bytes_to_copy);
                remaining_ndef_length -= bytes_to_copy;
            }
        }
    }

    ndef_from_buffer(ndef);

    LOG_DEBUG("[MIFARE CLASSIC RW] Read NDEF message\n");
    return 0;
}

/* Reads the entire tag */
int nfc_mifare_classic_rw_read(nfc_mifare_classic_rw_t *rw, nfc_mifare_classic_tag_t *tag,
    nfcdev_t *dev) {
    assert(rw != NULL);
    assert(tag != NULL);
    assert(dev != NULL);

    // rw->dev = dev;
    // if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
    //     LOG_ERROR("[T2T RW] Device not initialized\n");
    //     return -1;
    // }

    // nfc_a_listener_config_t config = {0};
    // rw->dev->ops->poll_a(dev, &config);

    // LOG_DEBUG("[MIFARE CLASSIC RW] Card found");

    // /* Now we authenticate the first block */
    // if (mifare_classic_rw_send_authenticate(rw, &config.nfcid1, 0, true,
    //     MIFARE_CLASSIC_SECTOR_PUBLIC_KEY_A) < 0) {
    //     LOG_ERROR("[MIFARE CLASSIC RW] Authentication failed\n");
    //     return -1;
    // }

    // const nfc_mifare_classic_size_t size = nfc_mifare_classic_get_size(&config);
    // const uint8_t small_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K) ?
    // MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT : MIFARE_CLASSIC_1K_SMALL_SECTOR_COUNT;
    // const uint8_t large_sector_count = (size == NFC_MIFARE_CLASSIC_SIZE_4K)
    // ? MIFARE_CLASSIC_4K_LARGE_SECTOR_COUNT : 0;

    // /* now read look for the NDEF message */
    // for (int uint8_t read)

    // ndef_from_buffer(ndef);
    return 0;
}
