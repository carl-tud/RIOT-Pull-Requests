#include <stdbool.h>
#include "log.h"
#include "net/nfc/constants.h"

static bool check_for_mifare_classic(nfc_a_polling_response_t polling_response, uint8_t acknowledgement) {
    /* checks if the sens res and sel res match a MIFARE Classic */
    if ((acknowledgement == NFC_A_MIFARE_CLASSIC_1K_SEL_RES) && 
        (polling_response.platform_information == NFC_A_MIFARE_CLASSIC_1K_SENS_RES_PLATFORM)) {
            return true;
    } else if ((acknowledgement == NFC_A_MIFARE_CLASSIC_4K_SEL_RES) &&
        (polling_response.platform_information == NFC_A_MIFARE_CLASSIC_4K_SENS_RES_PLATFORM)) {
            return true;
    }
    return false;
}

static bool check_for_mifare_desfire(nfc_a_polling_response_t polling_response, uint8_t acknowledgement) {
    /* checks if the sens res and sel res match a MIFARE Desfire */
    if ((acknowledgement == NFC_A_MIFARE_DESFIRE_SEL_RES) && 
        (polling_response.platform_information == NFC_A_MIFARE_DESFIRE_SENS_RES_PLATFORM) &&
        (polling_response.anticollision_information == NFC_A_MIFARE_DESFIRE_SENS_RES_ANTICOLLISION)) {
            return true;
    }
    return false;
}

static bool check_for_mifare_ultralight(nfc_a_polling_response_t polling_response, uint8_t acknowledgement) {
    /* checks if the sens res and sel res match a MIFARE Ultralight */
    if ((acknowledgement == NFC_A_MIFARE_ULTRALIGHT_SEL_RES) && 
        (polling_response.platform_information == NFC_A_MIFARE_ULTRALIGHT_SENS_RES_PLATFORM) &&
        (polling_response.anticollision_information == NFC_A_MIFARE_ULTRALIGHT_SENS_RES_ANTICOLLISION)) {
            return true;
    }
    return false;
}

nfc_application_type_t nfc_a_determine_application_type(nfc_a_polling_response_t polling_response, uint8_t acknowledgement) {
    /* infers the application type by looking at the sel res (SAK) */
    nfc_application_type_t app_type = NFC_APPLICATION_TYPE_UNKNOWN;
    if ((acknowledgement & NFC_A_SEL_RES_T2T_MASK) == NFC_A_SEL_RES_T2T_VALUE) {
        app_type = NFC_APPLICATION_TYPE_T2T;
        if (check_for_mifare_classic(polling_response, acknowledgement)) {
            app_type = NFC_APPLICATION_MIFARE_CLASSIC;
        }
        if (check_for_mifare_ultralight(polling_response, acknowledgement)) {
            app_type = NFC_APPLICATION_MIFARE_ULTRALIGHT;
        }
    } else if ((acknowledgement & NFC_A_SEL_RES_T4T_MASK) == NFC_A_SEL_RES_T4T_VALUE) {
        app_type = NFC_APPLICATION_TYPE_T4T;
        if (check_for_mifare_desfire(polling_response, acknowledgement)) {
            app_type = NFC_APPLICATION_MIFARE_DESFIRE;
        }
    } else {
        LOG_ERROR("Unknown NFC-A application type (SEL_RES=0x%02X)\n", acknowledgement);
    }

    return app_type;
}
