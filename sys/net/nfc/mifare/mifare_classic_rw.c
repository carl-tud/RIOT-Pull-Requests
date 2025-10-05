#include "mifare_classic_rw.h"

static int mifare_classic_send_authenticate(nfc_mifare_classic_rw_t *rw, bool use_key_a, 
    const uint8_t *key, const nfc_a_nfcid1_t *nfcid1) {
    assert(rw != NULL);
    assert(key != NULL);

    uint8_t cmd_buffer[14];
    uint8_t to_send = 0;
    cmd_buffer[0] = use_key_a ? MIFARE_CLASSIC_AUTH_A : MIFARE_CLASSIC_AUTH_B;
    memcpy(&cmd_buffer[1], key, 6);
    if (nfcid1.len == 4) {
        memcpy(&cmd_buffer[7], nfcid1.nfcid, 4);
        to_send = 11;
    } else {
        memcpy(&cmd_buffer[7], nfcid1.nfcid, 7);
        to_send = 14;
    } else {
        LOG_ERROR("mifare classic: unsupported NFCID1 length %u\n", 
            (unsigned)nfcid1->len);
        return -1;
    }

    uint8_t resp_buffer[16];
    uint8_t resp_len = 0;
    rw->dev->ops->initiator_exchange_data(rw->dev, cmd_buffer, to_send, resp_buffer, &resp_len);

    if (resp_len != 1 || resp_buffer[0] != 0x00) {
        LOG_ERROR("mifare classic: authentication failed\n");
        return -1;
    }

    return 0;
}

int nfc_mifare_classic_rw_read_ndef(nfc_mifare_classic_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(ndef != NULL);

    rw->dev = dev;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T2T RW] Device not initialized\n");
        return -1;
    }

    nfc_a_listener_config_t config = {0};
    rw->dev->ops->poll_a(dev, &config);

    

    ndef_from_buffer(ndef);
    return 0;
}
