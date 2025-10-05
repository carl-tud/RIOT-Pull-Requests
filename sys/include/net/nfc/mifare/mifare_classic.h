#pragma once

#include <stdint.h>



#define MIFARE_CLASSIC_BLOCK_SIZE 16
#define MIFARE_CLASSIC_SECTOR_SIZE 4
#define MIFARE_CLASSIC_SECTOR_COUNT 16
#define MIFARE_CLASSIC_TOTAL_BLOCKS (MIFARE_CLASSIC_SECTOR_COUNT * MIFARE_CLASSIC_SECTOR_SIZE)

/* MIFARE Application Directory (MAD) Sector */
#define MIFARE_CLASSIC_MAD_SECTOR 0

/* Works for non proprietary sectors */
#define MIFARE_CLASSIC_SECTOR_PUBLIC_KEY_A {0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7}

/* Works for the MIFARE Application Directory (MAD) */
#define MIFARE_CLASSIC_MAD_PUBLIC_KEY_A {0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5}

/* MIFARE commands */
#define MIFARE_CLASSIC_READ 0x30
#define MIFARE_CLASSIC_WRITE 0xA0
#define MIFARE_CLASSIC_AUTH_A 0x60
#define MIFARE_CLASSIC_AUTH_B 0x61


typedef struct {
    uint8_t uid[4];
    uint8_t bcc;
    uint8_t sak;
    uint8_t atqa[2];
    uint8_t manufacturer[8];
} nfc_mifare_classic_block_manufacturer_t;

typedef struct {
    uint8_t data[16];
} nfc_mifare_classic_block_trailer_t;

typedef struct {
    uint8_t key_a[6];
    uint8_t access_bits[4];
    uint8_t key_b[6];
} nfc_mifare_classic_block_trailer_t;

typedef union {
    nfc_mifare_classic_block_manufacturer_t manufacturer;
    nfc_mifare_classic_block_trailer_t trailer;
    nfc_mifare_classic_block_data_t data;
} nfc_mifare_classic_block_t;


typedef struct {
    uint8_t data[16];
} nfc_mifare_classic_block_data_t;

typedef struct {
    nfc_mifare_classic_block_t blocks[4];
} nfc_mifare_classic_sector_t;


typedef struct {
    nfc_mifare_classic_sector_t sectors[16];
} nfc_mifare_classic_tag_t;
