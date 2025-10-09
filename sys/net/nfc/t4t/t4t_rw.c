
#include "net/nfc/t4t/t4t.h"

int nfc_t4t_select_ndef_application(nfc_t4t_rw_t *rw) {
    assert(rw != NULL);

    uint8_t response[256];
    size_t response_len = 0;

    int ret = rw->dev->ops->initiator_exchange_data(rw->dev, select_ndef_apdu,
        sizeof(select_ndef_apdu), response, &response_len);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Failed to send SELECT NDEF application APDU\n");
        return -1;
    }

    if (response_len < 2 || response[response_len - 2] != 0x90 || response[response_len - 1] != 0x00) {
        LOG_ERROR("[T4T RW] SELECT NDEF application failed with status %02X%02X\n",
            response[response_len - 2], response[response_len - 1]);
        return -1;
    }

    LOG_DEBUG("[T4T RW] SELECT NDEF application successful\n");
    return 0;
}

int nfc_t4t_rw_read_tag(nfc_t4t_rw_t *rw, nfc_t4t_t *tag, nfcdev_t *dev) {
    assert(rw != NULL);
    assert(tag != NULL);

    nfc_listener_config_t config;
    rw->dev = dev;
    rw->tag = tag;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T4T RW] Device not initialized\n");
        return -1;
    }

    int ret = rw->dev->ops->poll_a(dev, &config);
    if (ret != 0) {
        LOG_ERROR("[T4T RW] Polling failed\n");
        return -1;
    }

    nfc_t4t_select_ndef_application(rw);

    return 0;
}
