#pragma once

#include <stdint.h>
#include <net/nfc/nfc_a.h>



#define MIFARE_CLASSIC_BLOCK_SIZE 16

/* MIFARE Application Directory (MAD) Sector */
#define MIFARE_CLASSIC_MAD_SECTOR 0

/* 1K and 4K MIFARE Classic differ in their sector counts */
/**
 * @brief  MIFARE Classic 1K tags have 16 sectors with 4 blocks each
 * 
 */
#define MIFARE_CLASSIC_1K_SMALL_SECTOR_COUNT 16

/**
 * @brief MIFARE Classic 4K tags have 40 sectors:
 * - 32 small sectors with 4 blocks each
 * - 8 large sectors with 16 blocks each
 */
#define MIFARE_CLASSIC_4K_SECTOR_COUNT 40
#define MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT 32
#define MIFARE_CLASSIC_4K_LARGE_SECTOR_COUNT 8

#define MIFARE_CLASSIC_BLOCKS_IN_SMALL_SECTOR 4
#define MIFARE_CLASSIC_BLOCKS_IN_LARGE_SECTOR 16

/* Works for non proprietary sectors */
#define MIFARE_CLASSIC_SECTOR_PUBLIC_KEY_A ((uint8_t[]){0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7})

/* Works for the MIFARE Application Directory (MAD), usually Sector 0 */
#define MIFARE_CLASSIC_MAD_PUBLIC_KEY_A ((uint8_t[]){0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5})

/* MIFARE commands */

/**
 * @brief Reads 16 bytes (1 block)
 *
 */
#define MIFARE_CLASSIC_READ 0x30

/**
 * @brief Writes 16 bytes (1 block)
 * 
 */
#define MIFARE_CLASSIC_WRITE 0xA0

#define MIFARE_CLASSIC_AUTH_A 0x60
#define MIFARE_CLASSIC_AUTH_B 0x61

#define MIFARE_CLASSIC_ACK 0x0A

#define MIFARE_CLASSIC_HALT_COMMAND 0x50

typedef enum {
    NFC_MIFARE_CLASSIC_SIZE_1K = 0,
    NFC_MIFARE_CLASSIC_SIZE_4K,
} nfc_mifare_classic_size_t;


typedef struct {
    uint8_t uid[4];
    uint8_t bcc;
    uint8_t sak;
    uint8_t atqa[2];
    uint8_t manufacturer[8];
} nfc_mifare_classic_block_manufacturer_t;

typedef struct {
    uint8_t key_a[6];
    uint8_t access_bits[4];
    uint8_t key_b[6];
} nfc_mifare_classic_block_trailer_t;

typedef struct {
    uint8_t data[16];
} nfc_mifare_classic_block_data_t;

typedef union {
    nfc_mifare_classic_block_manufacturer_t manufacturer;
    nfc_mifare_classic_block_trailer_t trailer;
    nfc_mifare_classic_block_data_t data;
} nfc_mifare_classic_block_t;

typedef struct {
    nfc_mifare_classic_block_t blocks[4];
} nfc_mifare_classic_sector_small_t;

typedef struct {
    nfc_mifare_classic_block_t blocks[16];
} nfc_mifare_classic_sector_large_t;

typedef struct {
    nfc_mifare_classic_sector_small_t sectors[MIFARE_CLASSIC_1K_SMALL_SECTOR_COUNT];
} nfc_mifare_classic_1k_t;

typedef struct {
    nfc_mifare_classic_sector_small_t sectors_small[MIFARE_CLASSIC_4K_SMALL_SECTOR_COUNT];
    nfc_mifare_classic_sector_large_t sectors_large[MIFARE_CLASSIC_4K_LARGE_SECTOR_COUNT];
} nfc_mifare_classic_4k_t;

typedef struct {
    nfc_mifare_classic_size_t size;
    union {
        nfc_mifare_classic_1k_t mifare_1k;
        nfc_mifare_classic_4k_t mifare_4k;
    } data;
} nfc_mifare_classic_tag_t;

nfc_mifare_classic_size_t nfc_mifare_classic_get_size(const nfc_a_listener_config_t *config);
