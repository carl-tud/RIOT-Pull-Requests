#pragma once

typedef struct {
    nfcdev_t *dev;
    nfc_t2t_t *tag;
} t2t_rw_t;


void t2t_read(t2t_rw_t *rw);
