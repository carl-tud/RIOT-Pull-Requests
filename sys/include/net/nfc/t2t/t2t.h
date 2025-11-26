/*
 * Copyright (C) 2024 Technische Universität Dresden
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

#pragma once

/**
 * @ingroup     sys/net/nfc
 * @{
 *
 * @file
 * @brief       Typedefs and function definitions for NFC Forum Type 2 Tags.
 *
 * Typedefs and function definitions for NFC Forum Type 2 Tags.
 *
 * @author     Max Stolze   <max_karl.stolze@mailbox.tu-dresden.de>
 * @author     Nico Behrens <nifrabe@outlook.de>
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include "net/nfc/tlv.h"
#include "net/nfc/ndef/ndef.h"
#include "net/nfc/nfc_a.h"

/* commands for T2t */
#define NFC_T2T_ACK 0x0A
#define NFC_T2T_NACK 0x0B

#define NFC_T2T_READ_COMMAND 0x30
#define NFC_T2T_WRITE_COMMAND 0xA2
#define NFC_T2T_SECTOR_SELECT_COMMAND 0xC2
#define NFC_T2T_HALT_COMMAND 0x50

/* T2T specific constants */
#define NFC_T2T_VERSION_1_1 0x11
#define NFC_T2T_CC_MAGIC_NUMBER 0xE1
#define NFC_T2T_CC_READ_WRITE 0x00
#define NFC_T2T_CC_READ_ONLY 0x0F
#define NFC_T2T_LOCK_BYTES_READ_ONLY 0xFF
#define NFC_T2T_LOCK_BYTES_READ_WRITE 0x00

/* T2T sizes */
#define NFC_T2T_BLOCK_SIZE 4
#define NFC_T2T_BLOCKS_PER_SECTOR 256
#define NFC_T2T_STATIC_MEMORY_SIZE 64
#define NFC_T2T_STATIC_MEMORY_DATA_AREA_SIZE 64
#define NFC_T2T_STATIC_BLOCKS 16

#define NFC_T2T_DEFAULT_MEM_SIZE NFC_T2T_STATIC_MEMORY_SIZE

#define NFC_T2T_INTERNAL_SIZE   10
#define NFC_T2T_CC_SIZE         4
#define NFC_T2T_LOCK_BYTES_SIZE 2

#define NFC_T2T_RESERVED_SIZE (NFC_T2T_INTERNAL_SIZE + NFC_T2T_LOCK_BYTES_SIZE + NFC_T2T_CC_SIZE)

/* common tlv types */
#define NFC_T2T_NDEF_TLV_TYPE 0x03
#define NFC_T2T_TERMINATOR_TLV_TYPE 0xFE


/* the memory size is issued via kconfig, otherwise, use static memory size */
#ifndef CONFIG_NFC_T2T_MEMORY_SIZE
    #define NFC_T2T_MEMORY_SIZE NFC_T2T_STATIC_MEMORY_SIZE
#else
    #define NFC_T2T_MEMORY_SIZE CONFIG_NFC_T2T_MEMORY_SIZE
#endif

/**
 * @brief Struct for Lock Bytes
 * 
 * T2T Lock bytes for static memory layout.
 */
typedef struct {
    uint8_t lock_byte1;
    uint8_t lock_byte2;
} t2t_static_lock_bytes_t;

/**
 * @brief Capability Container struct.
 * 
 * Capability Container struct holds four uint8_t for each byte of the CC
 * as defined in T2T Op 6.1. Magic number, T2T version, data area size 
 * divided by 8 and read_write settings.
 */
typedef struct {
    uint8_t magic_number; //EH1 for NDEF
    uint8_t version_number; // msb bit major, lsb bit minor -> 0x11
    uint8_t memory_size; // data area size / 8
    uint8_t read_write_access; // 4 bit read, 4 bit write,
} t2t_cc_t;

/**
 * @brief Type 2 Tag struct
 * 
 * @note Type 2 Tag container holding the entire memory layout.
 * 
 */
typedef struct {
    uint8_t internal[NFC_T2T_INTERNAL_SIZE];
    uint8_t lock_bytes[NFC_T2T_LOCK_BYTES_SIZE];
    t2t_cc_t cc;
    uint8_t data_array[NFC_T2T_MEMORY_SIZE - NFC_T2T_INTERNAL_SIZE - 
        NFC_T2T_LOCK_BYTES_SIZE - NFC_T2T_CC_SIZE];
} nfc_t2t_t;

int t2t_read_blocks(const nfc_t2t_t *tag, uint8_t block_no, uint8_t *blocks, uint8_t sector);

int t2t_write_block(nfc_t2t_t *tag, uint8_t block_no, const uint8_t *block, uint8_t sector);

uint16_t t2t_get_memory_size(const nfc_t2t_t *tag);

bool t2t_is_read_only(const nfc_t2t_t *tag);

void t2t_set_read_only(nfc_t2t_t *tag);

int t2t_get_ndef(const nfc_t2t_t *tag, ndef_t *ndef);

int t2t_init_with_ndef(nfc_t2t_t *tag, ndef_t *ndef, const nfc_a_nfcid1_t *nfcid1);

void t2t_get_nfcid1(const nfc_t2t_t *tag, nfc_a_nfcid1_t *nfcid1, nfc_a_nfcid1_len_t len);

void t2t_print(nfc_t2t_t *tag);
