#include "net/nfc/t2t.h"


void nfc_t2t_emulate(nfcdev_t *dev, nfc_t2t_t *tag) {
    listen_a(dev, (nfc_a_config_t) {
        .uid = tag->uid,
        .uid_size = tag->uid_size,
    });
}
