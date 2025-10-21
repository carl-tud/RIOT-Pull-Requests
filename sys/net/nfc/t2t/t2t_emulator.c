#include "net/nfcdev.h"

#include "net/nfc/t2t/t2t_emulator.h"
#include "log.h"
#include "assert.h"

#define T2T_NACK_ERROR -1
#define T2T_EMULATOR_HALTED -2

/* Generic NFC Forum T2T emulator making use of nfcdev */

static int send_ack_nack(nfc_t2t_emulator_t *emulator, bool ack) {
    uint8_t tx_buffer[1];
    if (ack) {
        LOG_DEBUG("[T2T Emulator] Sending ACK\n");
        tx_buffer[0] = NFC_T2T_ACK;
    }
    else {
        LOG_DEBUG("[T2T Emulator] Sending NACK\n");
        tx_buffer[0] = NFC_T2T_NACK;
    }

    emulator->dev->ops->target_send_data(emulator->dev, tx_buffer, 1);
    return 0;
}

static int process_t2t_command(nfc_t2t_emulator_t *emulator, const uint8_t *cmd, uint8_t cmd_len) {
    if (cmd_len == 0) {
        LOG_DEBUG("[T2T Emulator] Received empty data\n");
        return 0;
    }
    else if (cmd_len == 1) {
        LOG_DEBUG("[T2T Emulator] Received command byte only\n");
        return 0;
    }

    uint8_t command = cmd[0];
    const uint8_t *data_buffer = cmd + 1;
    uint8_t data_size = cmd_len - 1;

    if (command == NFC_T2T_READ_COMMAND) {
        uint8_t block_address = data_buffer[0];
        uint8_t data_buffer[16];
        LOG_DEBUG("[T2T Emulator] Read command received for block %d\n", block_address);
        t2t_read_blocks(emulator->tag, block_address, data_buffer);
        emulator->dev->ops->target_send_data(emulator->dev, data_buffer, 16);
        return 0;
    }
    else if (command == NFC_T2T_WRITE_COMMAND) {
        if (data_size != 5) {
            LOG_WARNING("[T2T Emulator] Write command does not contain the correct "
                "amount of bytes\n");
            return -1;
        }

        if (emulator->tag->cc->read_write_access == NFC_T2T_CC_READ_ONLY) {
            LOG_WARNING("[T2T Emulator] Tag is read-only, cannot process write command\n");
            return send_ack_nack(emulator, false);
        }

        t2t_write_block(emulator->tag, *data_buffer, data_buffer + 1);
        return send_ack_nack(emulator, true);
        // process_write_command(*data_buffer, data_buffer + 1);
    }
    else if (command == NFC_T2T_SECTOR_SELECT_COMMAND) {
        LOG_DEBUG("[T2T Emulator] Sector select command received\n");
        return send_ack_nack(emulator, false);
    } else if (command == NFC_T2T_HALT_COMMAND) {
        LOG_DEBUG("[T2T Emulator] Halt command received\n");
        emulator->state = NFC_T2T_STATE_SLEEPING;
        send_ack_nack(emulator, true);
        return T2T_EMULATOR_HALTED;
    } else {
        LOG_ERROR("[T2T Emulator] Unknown command received: 0x%02X\n", command);
        emulator->state = NFC_T2T_STATE_IDLE;
        return send_ack_nack(emulator, false);
    }

    return 0;
}

void t2t_emulator_start(nfc_t2t_emulator_t *emulator, nfcdev_t *dev, nfc_t2t_t *tag,
    nfc_a_nfcid1_t *nfcid1) {
    assert (emulator != NULL);
    assert (dev != NULL);
    assert (tag != NULL);

    emulator->dev = dev;
    emulator->tag = tag;

    if (emulator->dev->state == NFCDEV_STATE_UNINITIALIZED) {
        LOG_ERROR("[T2T Emulator] Device not initialized\n");
        return;
    }

    nfc_a_listener_config_t config = {
        .sel_res = NFC_A_SEL_RES_T2T_VALUE,
        .nfcid1 = *nfcid1
    };

    emulator->dev->ops->listen_a(emulator->dev, &config);
    LOG_DEBUG("[T2T Emulator] FINISHED LISTEN_A\n");

    uint8_t rx_buffer[32];
    size_t rx_len = 0;

    /* infinite loop */
    while (true) {
        /* receive data */
        LOG_DEBUG("[T2T Emulator] Waiting for data...\n");
        int ret = emulator->dev->ops->target_receive_data(emulator->dev, rx_buffer, &rx_len);
        if (ret < 0) {
            LOG_ERROR("[T2T Emulator] Error receiving data: %d\n", ret);
            return;
        }

        if (emulator->state == NFC_T2T_STATE_SLEEPING) {
            LOG_DEBUG("[T2T Emulator] In sleep mode, ignoring received data\n");
            return;
        }

        /* process data and send response */
        ret = process_t2t_command(emulator, rx_buffer, rx_len);
        if (ret == T2T_EMULATOR_HALTED) {
            LOG_DEBUG("[T2T Emulator] Emulator halted\n");
            return;
        } else if (ret < 0) {
            LOG_ERROR("[T2T Emulator] Error processing command\n");
            return;
        }
        LOG_DEBUG("[T2T Emulator] Sent data...\n");
    }

    return;
}
