#include "net/nfcdev.h"

#include "net/nfc/t2t/t2t_emulator.h"
#include "log.h"

/* Generic NFC Forum T2T emulator making use of nfcdev */
static int send_ack_nack(nfc_t2t_emulator_t *emulator, bool ack) {
    uint8_t tx_buffer[1];
    if (ack) {
        LOG_DEBUG("[T2T Emulator] Sending ACK");
        tx_buffer[0] = NFC_T2T_ACK;
    }
    else {
        LOG_DEBUG("[T2T Emulator] Sending NACK");
        tx_buffer[0] = NFC_T2T_NACK;
    }

    emulator->dev->ops->target_exchange_data(emulator->dev, tx_buffer, 1, 
        NULL, NULL);
    return 0;
}

static size_t process_t2t_command(nfc_t2t_emulator_t *emulator, const uint8_t *cmd, uint8_t cmd_len) {
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
        emulator->dev->ops->target_exchange_data(emulator->dev, data_buffer, 16, NULL, NULL);
        return 0;
    }
    else if (command == NFC_T2T_WRITE_COMMAND) {
        if (data_size != 5) {
            LOG_WARNING("[T2T Emulator] Write command does not contain the correct amount of bytes");
            return -1;
        }
        t2t_write_block(emulator->tag, *data_buffer, data_buffer + 1);
        send_ack_nack(emulator, true);
        // process_write_command(*data_buffer, data_buffer + 1);
    }
    else if (command == NFC_T2T_SECTOR_SELECT_COMMAND) {
        LOG_DEBUG("[T2T Emulator] Sector select command received\n");
        send_ack_nack(emulator, false);
    } else {
        LOG_ERROR("[T2T Emulator] Unknown command received: 0x%02X\n", command);
        send_ack_nack(emulator, false);
    }

    return 0;
}

void t2t_emulator_start(nfc_t2t_emulator_t *emulator, nfcdev_t *dev, nfc_t2t_t *tag) {
    assert (emulator != NULL);
    assert (dev != NULL);
    assert (tag != NULL);

    emulator->dev = dev;
    emulator->tag = tag;
    emulator->dev->state = NFCDEV_STATE_DISABLED;

    emulator->dev->ops->init(emulator->dev, NULL);

    nfc_a_listener_config_t config = {
        .nfcid1 = {
            .len = NFC_A_NFCID1_LEN4,
            .nfcid = {0xDE, 0xAD, 0xBE, 0xEF}
        },
        .sel_res = NFC_A_SEL_RES_T2T_VALUE
    };
    emulator->dev->ops->listen_a(emulator->dev, &config);
    emulator->dev->state = NFCDEV_STATE_LISTENING;

    uint8_t rx_buffer[32];
    size_t rx_len = 0;

    /* infinite loop */
    while (true) {
        /* receive data */
        emulator->dev->ops->target_exchange_data(emulator->dev, NULL, 0, rx_buffer, &rx_len);

        /* process data and send response */
        process_t2t_command(emulator, rx_buffer, rx_len);
    }

    return;
}
