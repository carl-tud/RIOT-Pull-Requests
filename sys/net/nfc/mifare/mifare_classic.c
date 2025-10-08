#include "net/nfc/mifare/mifare_classic.h"

nfc_mifare_classic_size_t nfc_mifare_classic_get_size(const nfc_a_listener_config_t *config) {
    if ((config->sel_res & 0x18) == 0x18) {
        return NFC_MIFARE_CLASSIC_SIZE_4K;
    } else {
        return NFC_MIFARE_CLASSIC_SIZE_1K;
    }
}
