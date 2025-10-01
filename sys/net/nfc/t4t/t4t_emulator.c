#include "net/nfc/t4t/t4t_emulator.h"

#include <assert.h>
#include "log.h"
#include <stdbool.h>
#include <string.h>
#include "net/nfc/apdu/apdu.h"

/* T4T communicate with APDUs. There are two types of APDUs:
command APDU (C-APDU) and response APDU (R-APDU). APDUs have a format */

static int send_success_response(nfc_t4t_emulator_t *emulator) {
    const uint8_t response[] = {APDU_SW1_NO_ERROR, APDU_SW2_NO_ERROR};
    return emulator->dev->ops->target_send_data(emulator->dev, response, sizeof(response));
}

static int send_not_found_response(nfc_t4t_emulator_t *emulator) {
    const uint8_t response[] = {APDU_SW1_WRONG_PARAMS, APDU_SW2_FILE_OR_APP_NOT_FOUND};
    return emulator->dev->ops->target_send_data(emulator->dev, response, sizeof(response));
}

static int process_select_command(nfc_t4t_emulator_t *emulator, const uint8_t *cmd, uint8_t cmd_len) {
    (void) cmd_len;
    if (!emulator->tag->selected_ndef_application) {
        /* NDEF Application is not selected and we need to check whether 
        the command is for the selection of the NDEF application */
        if (memcmp(cmd, T4T_APDU_NDEF_TAG_APPLICATION_SELECT, 
            sizeof(T4T_APDU_NDEF_TAG_APPLICATION_SELECT)) == 0) {
            LOG_DEBUG("[T4T Emulator] Received NDEF Tag Application SELECT command\n");
            emulator->tag->selected_ndef_application = true;

            /* Send response: 90 00 (Success) */
            return send_success_response(emulator);
        } else {
            LOG_DEBUG("[T4T Emulator] SELECT command received but NDEF application not selected\n");
            return NFC_ERR_GENERIC;
        }
    }

    /* the NDEF application DF is selected */
    if (memcmp(cmd, T4T_APDU_CC_FILE_SELECT, sizeof(T4T_APDU_CC_FILE_SELECT)) == 0) {
        LOG_DEBUG("[T4T Emulator] Received CC file SELECT command\n");
        emulator->tag->selected_cc_file = true;

        /* Send response: 90 00 (Success) */
        return send_success_response(emulator);
    } else {
        LOG_DEBUG("[T4T Emulator] SELECT command not recognized\n");
        return send_not_found_response(emulator);
    }

    /* TODO: work with T4T_APDU_NDEF_FILE_SELECT */
}

static int process_t4t_command(nfc_t4t_emulator_t *emulator, const uint8_t *cmd, uint8_t cmd_len) {
    (void) emulator;

    if (cmd_len == 0) {
        LOG_DEBUG("[T4T Emulator] Received empty data\n");
        return 0;
    } else if (cmd_len == 1) {
        LOG_DEBUG("[T4T Emulator] Received command byte only\n");
        return 0;
    }

    LOG_DEBUG("[T4T Emulator] Data size: %d\n", cmd_len);


    switch (cmd[APDU_INS_POS]) {
        case APDU_INS_SELECT:
            return process_select_command(emulator, cmd, cmd_len);
            
        case APDU_INS_READ_BINARY:
            
            return NFC_ERR_GENERIC;

        case APDU_INS_UPDATE_BINARY:
            
            return NFC_ERR_GENERIC;
        default:
            
            return NFC_ERR_GENERIC;
    }

    if (cmd_len == NFC_A_RATS_LENGTH && cmd[0] == NFC_A_RATS_MAGIC_NUMBER) {
        LOG_DEBUG("[T4T Emulator] Received RATS command\n");
        /* this shouldn't happen, as RATS is handled by the underlying driver */
        return NFC_ERR_GENERIC;
    } 

    if (emulator->tag->selected_ndef_application) {
        /* we are selected, so we can process the commands */
        switch (cmd[APDU_INS_POS]) {
            case APDU_INS_SELECT:
                LOG_DEBUG("[T4T Emulator] Received SELECT command\n");
                /* Process the SELECT command */
                return send_success_response(emulator);
        }
    } else {
        /* we are not selected and need to check whether the command is for the selection of 
        the NDEF application */
        if (memcmp(cmd, T4T_APDU_NDEF_TAG_APPLICATION_SELECT, sizeof(T4T_APDU_NDEF_TAG_APPLICATION_SELECT)) == 0) {
            LOG_DEBUG("[T4T Emulator] Received NDEF Tag Application Select command\n");
            emulator->tag->selected_ndef_application = true;

            /* Send response: 90 00 (Success) */
            return send_success_response(emulator);
        }
    }

    LOG_DEBUG("[T4T Emulator] Command not recognized or not implemented\n");
    return NFC_ERR_GENERIC;
}

void t4t_emulator_start(nfc_t4t_emulator_t *emulator, nfcdev_t *dev, 
    nfc_t4t_t *tag, nfc_a_nfcid1_t *nfcid1) {
    assert (emulator != NULL);
    assert (dev != NULL);
    assert (tag != NULL);
    assert (nfcid1 != NULL);

    emulator->dev = dev;
    emulator->tag = tag;
    emulator->dev->state = NFCDEV_STATE_DISABLED;


    nfc_a_listener_config_t config = {
        .sel_res = NFC_A_SEL_RES_T4T_VALUE,
        .sens_res = {
            .anticollision_information = 0x01,
            .platform_information = 0x00
        }
    };
    memcpy(&(config.nfcid1), nfcid1, sizeof(nfc_a_nfcid1_t));


    LOG_DEBUG("[T4T Emulator] Starting emulation\n");
    emulator->dev->ops->listen_a(emulator->dev, &config);
    // emulator->dev->state = NFCDEV_STATE_ENABLED;

    uint8_t rx_buffer[128];
    size_t rx_len = 0;

    /* infinite loop */
    while (true) {
        /* receive data */
        int ret = emulator->dev->ops->target_receive_data(emulator->dev, rx_buffer, &rx_len);
        if (ret < 0) {
            LOG_DEBUG("[T4T Emulator] Error receiving data\n");
            return;
        }

        LOG_DEBUG("[T4T Emulator] Received %u bytes\n", rx_len);
        ret = process_t4t_command(emulator, rx_buffer, rx_len);
        if (ret < 0) {
            LOG_DEBUG("[T4T Emulator] Error processing command\n");
            return;
        }
    }

    return;
}
