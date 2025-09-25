#pragma once

#include "t2t.h"
#include "net/nfcdev.h"

typedef struct {
    nfcdev_t *dev;
    nfc_t2t_t *tag;
} nfc_t2t_rw_t;


int nfc_t2t_rw_read(nfc_t2t_rw_t *rw);
