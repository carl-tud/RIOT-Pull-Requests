
#include "net/nfc/generic/generic_rw.h"
#include "assert.h"
#include "log.h"
#include "test_utils/print_stack_usage.h"

static int interact_with_ndef_on_tag(nfc_generic_rw_t *rw, ndef_t *ndef, nfcdev_t *dev, bool read) {
    assert(rw != NULL);
    assert(ndef != NULL);
    assert(dev != NULL);

    rw->dev = dev;
    if (dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("Device not initialized\n");
        return -1;
    }

    nfc_listener_config_t config;
    dev->ops->poll(dev, &config);

    switch (config.technology) {
        case NFC_TECHNOLOGY_A:
            {
                nfc_application_type_t app = nfc_a_get_application_type(config.config.a.sens_res, 
                    config.config.a.sel_res);

                if (app == NFC_APPLICATION_TYPE_T2T || app == NFC_APPLICATION_MIFARE_ULTRALIGHT) {
                    nfc_t2t_rw_t t2t_rw = {
                        .dev = dev,
                    };
                    LOG_DEBUG("Detected T2T compliant NFC tag\n");
                    if (read) {
                        return nfc_t2t_rw_read_ndef(&t2t_rw, ndef, dev);
                    } else {
                        return nfc_t2t_rw_write_ndef(&t2t_rw, ndef, dev);
                    }
                } else if (app == NFC_APPLICATION_TYPE_T4T || app == NFC_APPLICATION_MIFARE_DESFIRE) {
                    nfc_t4t_rw_t t4t_rw = {
                        .dev = dev,
                        .is_tag_selected = true,
                    };
                    LOG_DEBUG("Detected T4T A compliant NFC tag\n");
                    if (read) {
                        return nfc_t4t_rw_read_ndef(&t4t_rw, ndef, dev);
                    } else {
                        return nfc_t4t_rw_write_ndef(&t4t_rw, ndef, dev);
                    }
                } else if (app == NFC_APPLICATION_MIFARE_CLASSIC) {
                    nfc_mifare_classic_rw_t mc_rw = {
                        .dev = dev,
                    };
                    LOG_DEBUG("Detected MFC NFC tag");
                    if (read) {
                        return nfc_mifare_classic_rw_read_ndef(&mc_rw, ndef, dev);
                    } else {
                        return nfc_mifare_classic_rw_write_ndef(&mc_rw, ndef, dev);
                    }
                } else {
                    LOG_ERROR("Unknown or unsupported NFC-A application type\n");
                    return -1;
                }
            }
            break;
        case NFC_TECHNOLOGY_B:
            {
                /* this must be a T4T */
                nfc_t4t_rw_t t4t_rw = {
                    .dev = dev,
                };
                LOG_DEBUG("Detected T4T B compliant NFC tag\n");
                if (read) {
                    return nfc_t4t_rw_read_ndef(&t4t_rw, ndef, dev);
                } else {
                    return nfc_t4t_rw_write_ndef(&t4t_rw, ndef, dev);
                }
            }
            break;
        case NFC_TECHNOLOGY_F:
            return -1; /* Not implemented yet */
        case NFC_TECHNOLOGY_V:
            return -1; /* Not implemented yet */
        default:
            LOG_ERROR("Unknown technology\n");
            return -1;
    }
    return 0;
}

int nfc_generic_rw_read_ndef(nfc_generic_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    return interact_with_ndef_on_tag(rw, ndef, dev, true);
}

int nfc_generic_rw_write_ndef(nfc_generic_rw_t *rw, ndef_t *ndef, nfcdev_t *dev) {
    return interact_with_ndef_on_tag(rw, ndef, dev, false);
}
