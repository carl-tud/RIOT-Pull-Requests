#include "net/nfc/nfc.h"
#include "log.h"
#include <stdbool.h>

static bool check_for_mifare_classic(nfc_a_sens_res_t sens_res, uint8_t sel_res) {
    /* checks if the sens res and sel res match a MIFARE Classic */
    if ((sel_res == NFC_A_MIFARE_CLASSIC_1K_SEL_RES) && 
        (sens_res.platform_information == NFC_A_MIFARE_CLASSIC_1K_SENS_RES_PLATFORM)) {
            return true;
    } else if ((sel_res == NFC_A_MIFARE_CLASSIC_4K_SEL_RES) &&
        (sens_res.platform_information == NFC_A_MIFARE_CLASSIC_4K_SENS_RES_PLATFORM)) {
            return true;
    }
    return false;
}

static bool check_for_mifare_desfire(nfc_a_sens_res_t sens_res, uint8_t sel_res) {
    /* checks if the sens res and sel res match a MIFARE Desfire */
    if ((sel_res == NFC_A_MIFARE_DESFIRE_SEL_RES) && 
        (sens_res.platform_information == NFC_A_MIFARE_DESFIRE_SENS_RES_PLATFORM) &&
        (sens_res.anticollision_information == NFC_A_MIFARE_DESFIRE_SENS_RES_ANTICOLLISION)) {
            return true;
    }
    return false;
}

static bool check_for_mifare_ultralight(nfc_a_sens_res_t sens_res, uint8_t sel_res) {
    /* checks if the sens res and sel res match a MIFARE Ultralight */
    if ((sel_res == NFC_A_MIFARE_ULTRALIGHT_SEL_RES) && 
        (sens_res.platform_information == NFC_A_MIFARE_ULTRALIGHT_SENS_RES_PLATFORM) &&
        (sens_res.anticollision_information == NFC_A_MIFARE_ULTRALIGHT_SENS_RES_ANTICOLLISION)) {
            return true;
    }
    return false;
}

nfc_application_type_t nfc_a_get_application_type(nfc_a_sens_res_t sens_res, uint8_t sel_res) {
    /* infers the application type by looking at the sel res (SAK) */
    nfc_application_type_t app_type = NFC_APPLICATION_TYPE_UNKNOWN;
    if ((sel_res & NFC_A_SEL_RES_T2T_MASK) == NFC_A_SEL_RES_T2T_VALUE) {
        app_type = NFC_APPLICATION_TYPE_T2T;
        if (check_for_mifare_classic(sens_res, sel_res)) {
            app_type = NFC_APPLICATION_MIFARE_CLASSIC;
        }
        if (check_for_mifare_ultralight(sens_res, sel_res)) {
            app_type = NFC_APPLICATION_MIFARE_ULTRALIGHT;
        }
    } else if ((sel_res & NFC_A_SEL_RES_T4T_MASK) == NFC_A_SEL_RES_T4T_VALUE) {
        app_type = NFC_APPLICATION_TYPE_T4T;
        if (check_for_mifare_desfire(sens_res, sel_res)) {
            app_type = NFC_APPLICATION_MIFARE_DESFIRE;
        }
    } else {
        LOG_ERROR("Unknown NFC-A application type (SEL_RES=0x%02X)\n", sel_res);
    }

    return app_type;
}
