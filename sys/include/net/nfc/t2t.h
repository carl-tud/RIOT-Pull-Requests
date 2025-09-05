#pragma once

#include "net/nfcdev.h"

typedef struct {
    uint8_t *memory;
    size_t memory_size;
    uint8_t *uid;
    size_t uid_size;

} nfc_t2t_t;

/* CMD: READ 4, MEMORY[4*16]*/
/* CMD: READ 0, UPDATE TAG, */

/* emulates an NFC t2t with an optional callback */
void nfc_t2t_emulate(nfcdev_t *dev, nfc_t2t_t *tag);

/* read a block of 16 bytes */
void nfc_t2t_read_block(nfc_t2t_t *tag, uint8_t *block);

/* write a block of 16 bytes */
void nfc_t2t_write_block(nfc_t2t_t *tag, uint8_t *block);

void nfc_t2t_read(nfcdev_t *dev, nfc_t2t_t *tag);
