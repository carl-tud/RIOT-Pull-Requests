#include <stdio.h>
#include <string.h>

#include "assert.h"
#include "kernel_defines.h"
#include "ztimer.h"
#include "mutex.h"
#include "pn53x.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "periph/uart.h"
#include "msg.h"
#include "byteorder.h"
#include "architecture.h"
#include "macros/utils.h"
#include "unaligned.h"

#include "log.h"

#include "net/nfc/nfc_error.h"

#include "pn53x.h"
#include "pn532.h"

#define ENABLE_DEBUG CONFIG_PN53_DEBUG
#include "debug.h"


static inline void __debug_hex(const uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i += 1) {
        printf("%02X ", buffer[i]);
    }
}

#define PN53_DEBUG_HEX(bytes, size) \
    do {                                \
      if (ENABLE_DEBUG) {             \
          __debug_hex(bytes, size);   \
      }                               \
    } while (0)

#define PN53_DEBUG_HCI(...) DEBUG("pn53x.hci: " __VA_ARGS__)
#define PN53_DEBUG(command, ...) DEBUG("pn53x." command ": " __VA_ARGS__)
#define PN53_DEBUG_REGISTER(...) PN53_DEBUG("register", __VA_ARGS__)

static uint8_t _tx_rx_framing(nfc_technology_t tech) {
    switch (tech) {
        case NFC_TECHNOLOGY_A: return 0;
        case NFC_TECHNOLOGY_B: return 3;
        case NFC_TECHNOLOGY_F: return 2;
        default:
            assert(false);
            UNREACHABLE();
            return 0;
    }
}

static uint8_t _tx_rx_mode(uint8_t framing, nfc_bitrate_t bitrate) {
    uint8_t mode = 0;
    pn53_bitfield_set(&mode, PN53_REGISTER_TXRX_MODE_BITRATE_INDEX, nfc_bitrate_to_index(bitrate));
    pn53_bitfield_set(&mode, PN53_REGISTER_TXRX_MODE_FRAMING, framing);
    return mode;
}

int pn53_fifo_receive_start_(pn53_dev_t* dev, bool transceive) {
    ssize_t res = 0;
    pn53_register_t reg[] = {
        {
            .address = PN53_REGISTER_FIFO_LEVEL,
            .value = PN53_REGISTER_FIFO_LEVEL_FLAG_FLUSH
        },
        {
            .address = PN53_REGISTER_COMMAND,
            .value = (uint8_t)(transceive ? PN53_CIU_COMMAND_TRANSCEIVE : PN53_CIU_COMMAND_RECEIVE)
        }
    };

    PN53_DEBUG("fifo.rx", "flushing FIFO, CIU_Command=%s\n", transceive ? "Transceive" : "Receive");
    if ((res = pn53_write_registers(dev, reg, ARRAY_SIZE(reg))) < 0) {
        return (int)res;
    }

    return 0;
}

static inline void _copy_register_into(uint8_t** cursor, pn53_register_address_t addr, uint8_t value) {
    memcpy(*cursor, &addr, sizeof(addr));
    *cursor += sizeof(addr);
    **cursor = value;
    *cursor += 1;
}

static void _fifo_timeout(void* keep_polling) {
    *((bool*)keep_polling) = false;
}

int pn53_fifo_transmit_write_(pn53_dev_t* dev, const iolist_t* tx, uint8_t trailing_bit_count,
                               bool transceive, bool transceive_after_receiving
) {
    ssize_t res = 0;
    uint8_t* regs = (uint8_t*)&dev->connection.backing + PN35_FRAME_HEADER_NORMAL_COMMAND;
    size_t length = iolist_size(tx);

    uint8_t* values = NULL;
    pn53_register_address_t addrs[] = {
        PN53_REGISTER_FIFO_LEVEL,
        PN53_REGISTER_COMMON_IRQ,
    };

    uint8_t bit_framing = pn53_bitfield_create(PN53_REGISTER_BIT_FRAMING_MASK_TX_TRAILING_BIT_COUNT, trailing_bit_count);

    size_t max_fifo_bytes_at_once = (dev->connection.max_packet_length - PN53_FRAME_OVERHEAD_MIN_COMMAND) / sizeof(pn53_register_t) - 5;
    PN53_DEBUG("fifo.tx", "can max write %" PRIuSIZE " bytes into FIFO per HCI frame\n", max_fifo_bytes_at_once);

    size_t fifo_length = 0;
    bool transmitting = false;

    bool keep_polling = true;
    ztimer_t timeout = { .callback=_fifo_timeout, .arg=&keep_polling };
    if (dev->command_timeout != PN53_TIMEOUT_NEVER) {
        ztimer_set(ZTIMER_MSEC, &timeout, dev->command_timeout);
    }

    while (length > 0) {
        size_t can_write = MIN(max_fifo_bytes_at_once, MIN(length, CONFIG_PN53_FIFO_SIZE - fifo_length));
        assert(can_write > 0);

        PN53_DEBUG("fifo.tx", "writing %" PRIuSIZE " bytes into FIFO out of %" PRIuSIZE " remaining\n", can_write, length);
        uint8_t* cursor = regs;
        size_t reg_count = 0;

        if (!transmitting) {
            _copy_register_into(&cursor, PN53_REGISTER_COMMAND, PN53_CIU_COMMAND_IDLE);
            _copy_register_into(&cursor, PN53_REGISTER_FIFO_LEVEL, PN53_REGISTER_FIFO_LEVEL_FLAG_FLUSH);
            _copy_register_into(&cursor, PN53_REGISTER_BIT_FRAMING, bit_framing);
            reg_count += 3;
        }

        for (size_t remaining = can_write; remaining > 0; tx = tx->iol_next) {
            assert(tx);
            size_t chunk = MIN(remaining, tx->iol_len);
            for (size_t i = 0; i < chunk; i++) {
                _copy_register_into(&cursor, PN53_REGISTER_FIFO_DATA, ((uint8_t*)tx->iol_base)[i]);
            }
            remaining -= chunk;
        }
        reg_count += can_write;

        if (!transmitting) {
            if (!(transceive && transceive_after_receiving)) {
                _copy_register_into(&cursor, PN53_REGISTER_COMMAND,
                    transceive ? PN53_CIU_COMMAND_TRANSCEIVE : PN53_CIU_COMMAND_TRANSMIT);
                reg_count += 1;
            }
            if (transceive) {
                _copy_register_into( &cursor, PN53_REGISTER_BIT_FRAMING,
                    PN53_REGISTER_BIT_FRAMING_FLAG_START_SEND | bit_framing);
                reg_count += 1;
            }
            transmitting = true;
        }

        if ((res = pn53_write_registers_(dev, regs, reg_count)) < 0) {
            goto _return;
        }
        length -= can_write;
        tx += can_write;

        if (length > 0) {
            uint8_t refill_threshold = fifo_length + can_write - (can_write <= CONFIG_PN53_FIFO_TRANSMIT_REFILL_THRESHOLD ?
                1 : CONFIG_PN53_FIFO_TRANSMIT_REFILL_THRESHOLD);

            while (keep_polling) {
                if ((res = pn53_read_registers(dev, addrs, &values, ARRAY_SIZE(addrs))) < 0) {
                    goto _return;
                }

                if ((values[1] & PN53_REGISTER_COMMON_IRQ_FLAG_TX_FINISHED)) {
                    PN53_DEBUG("fifo.tx", "TX finished, but still need to send %" PRIuSIZE " bytes"
                               " -- controller is transmitting faster than host can refill FIFO\n",
                               length);
                    res = -EFBIG;
                    goto _return;
                }

                // If the controller has sent at least 5 bytes in the meantime, we refill it
                fifo_length = pn53_bitfield_get(values[0], PN53_REGISTER_FIFO_LEVEL_MASK_BYTE_COUNT);
                if (fifo_length <= refill_threshold) {
                    break;
                }
            }
        }
    }

    PN53_DEBUG("fifo.tx", "waiting for TX op to finish\n");
    while (keep_polling) {
        if ((res = pn53_read_registers(dev, addrs, &values, ARRAY_SIZE(addrs))) < 0) {
            goto _return;
        }

        if ((values[1] & PN53_REGISTER_COMMON_IRQ_FLAG_TX_FINISHED) && pn53_bitfield_get(values[0], PN53_REGISTER_FIFO_LEVEL_MASK_BYTE_COUNT) == 0) {
            PN53_DEBUG("fifo.tx", "TX finished\n");
            break;
        }
    }

_return:
    if (dev->command_timeout != PN53_TIMEOUT_NEVER) {
        bool triggered = !ztimer_remove(ZTIMER_MSEC, &timeout);
        if (IS_ACTIVE(ENABLE_DEBUG) && triggered) {
            PN53_DEBUG("fifo.tx", "timed out after %" PRIu32 " ms\n", dev->command_timeout);
        }
    }
    return (int)res;
}

ssize_t pn53_fifo_receive_read_(pn53_dev_t* dev, uint8_t* frame, size_t capacity, uint8_t* trailing_bit_count, uint32_t timeout_ms) {
    ssize_t res = 0;
    uint8_t* addrs = (uint8_t*)&dev->connection.backing + PN35_FRAME_HEADER_NORMAL_COMMAND;
    pn53_register_address_t fifo_data = PN53_REGISTER_FIFO_DATA;

    uint8_t* values = NULL;
    size_t received = 0;
    size_t fifo_byte_count = 0;

    pn53_register_address_t additional_regs[] = {
        PN53_REGISTER_FIFO_LEVEL,
        PN53_REGISTER_CONTROL,
        PN53_REGISTER_COMMON_IRQ,
    };

    size_t max_fifo_bytes_at_once = (dev->connection.max_packet_length - PN53_FRAME_OVERHEAD_MIN_COMMAND) / sizeof(pn53_register_address_t) - ARRAY_SIZE(additional_regs);
    PN53_DEBUG("fifo.rx", "can max read %" PRIuSIZE " bytes from FIFO per HCI frame\n", max_fifo_bytes_at_once);

    bool keep_polling = true;
    ztimer_t timeout = { .callback=_fifo_timeout, .arg=&keep_polling };
    if (timeout_ms != PN53_TIMEOUT_NEVER) {
        ztimer_set(ZTIMER_MSEC, &timeout, timeout_ms);
    }

    while (keep_polling) {
        size_t can_read = MIN(max_fifo_bytes_at_once, fifo_byte_count);
        PN53_DEBUG("fifo.rx", "reading %" PRIuSIZE " bytes out of %" PRIuSIZE " in FIFO\n", can_read, fifo_byte_count);
        for (size_t i = 0; i < can_read * sizeof(pn53_register_address_t); i += sizeof(pn53_register_address_t)) {
            memcpy(addrs + i, &fifo_data, sizeof(fifo_data));
        }
        memcpy(addrs + can_read * sizeof(pn53_register_address_t), additional_regs, sizeof(additional_regs));

        if ((res = pn53_read_registers_(dev, addrs, &values, can_read + ARRAY_SIZE(additional_regs))) < 0) {
            goto _return;
        }

        received += can_read;
        PN53_DEBUG("fifo.rx", "read %" PRIuSIZE ", totalling %" PRIuSIZE " bytes\n", can_read, received);

        if (capacity < can_read) {
            PN53_DEBUG("fifo.rx",
                       "fragment buffer has capacity of %" PRIuSIZE ", but need %" PRIuSIZE " bytes\n",
                       capacity, fifo_byte_count);
            res = -ENOBUFS;
            goto _return;
        } else {
            memcpy(frame, values, can_read);
            frame += can_read;
            capacity -= can_read;
        }

        uint8_t* additional_values = &values[can_read];
        fifo_byte_count = (size_t)pn53_bitfield_get(additional_values[0], PN53_REGISTER_FIFO_LEVEL_MASK_BYTE_COUNT);
        PN53_DEBUG("fifo.rx", "%" PRIuSIZE " bytes still in FIFO\n", fifo_byte_count);
        *trailing_bit_count = pn53_bitfield_get(additional_values[1], PN53_REGISTER_CONTROL_MASK_RX_TRAILING_BIT_COUNT);

        if ((additional_values[2] & PN53_REGISTER_COMMON_IRQ_FLAG_RX_FINISHED) && fifo_byte_count == 0) {
            PN53_DEBUG("fifo.rx", "RX finished\n");
            break;
        }
    }
    res = received;

_return:
    if (timeout_ms != PN53_TIMEOUT_NEVER) {
        bool triggered = !ztimer_remove(ZTIMER_MSEC, &timeout);
        if (IS_ACTIVE(ENABLE_DEBUG) && triggered) {
            PN53_DEBUG("fifo.rx", "timed out after %" PRIu32 " ms\n", timeout_ms);
        }
    }
    return res;
}

static int _check_radio_config(pn53_dev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role) {
    assert(role == NFC_ROLE_INITIATOR || role == NFC_ROLE_TARGET);
    PN53_DEBUG("radio", "intended config {tx=%c@%u rx=%c@%u role=%s field_mode=%s}\n",
               nfc_string_from_technology(tx->technology), nfc_bitrate_kbps(tx->bitrate),
               nfc_string_from_technology(rx->technology), nfc_bitrate_kbps(rx->bitrate),
               role == NFC_ROLE_INITIATOR ? "initiator" : "target",
               tx->generate_field && !rx->generate_field ? "peers" : "r/w+tag"
   );

    nfc_bitrate_t max_bitrate = MAX(tx->bitrate, rx->bitrate);
    if (tx->generate_field && !rx->generate_field) {
        switch (max_bitrate) {
            case NFC_BITRATE_106K:
            case NFC_BITRATE_212K:
            case NFC_BITRATE_424K:
                break;
            default:
                PN53_DEBUG("radio", "bitrate %u kbps unsupported in active mode\n", nfc_bitrate_kbps(max_bitrate));
                return -ENOTSUP;
        }
    } else {
        switch (max_bitrate) {
            case NFC_BITRATE_106K:
            case NFC_BITRATE_212K:
            case NFC_BITRATE_424K:
            case NFC_BITRATE_848K:
            case NFC_BITRATE_1695K:
                break;
            case NFC_BITRATE_3390K:
                if (dev->model == PN53_MODEL_PN532) {
                    break;
                }
                __attribute__ ((fallthrough));
            default:
                PN53_DEBUG("radio", "bitrate %u kbps unsupported\n", nfc_bitrate_kbps(max_bitrate));
                return -ENOTSUP;
        }
        switch (role) {
            case NFC_ROLE_INITIATOR:
                if (tx->generate_field && rx->generate_field) {
                    break;
                } else {
                    PN53_DEBUG("radio", "gen_field={tx=%i rx=%i} not a standardised field model", tx->generate_field, rx->generate_field);
                    return -ENOTSUP;
                }
            case NFC_ROLE_TARGET:
                if (!tx->generate_field && !rx->generate_field) {
                    break;
                } else {
                    PN53_DEBUG("radio", "gen_field={tx=%i rx=%i} not a standardised field model", tx->generate_field, rx->generate_field);
                    return -ENOTSUP;
                }
            default:
                assert(false);
                UNREACHABLE();
                return -1;
        }
    }
    return 0;
}

int pn53_configure_radio_unchecked(pn53_dev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role) {
    uint8_t tx_mode = 0;
    uint8_t rx_mode = 0;
    if (rx->options & NFCDEV_RX_MULTIPLE) {
        rx_mode |= PN53_REGISTER_RX_MODE_MULTIPLE_FRAMES;
    }

    if (rx->options & NFCDEV_RX_MALFORMED) {
        rx_mode |= PN53_REGISTER_RX_MODE_IGNORE_INVALID;
    }

    uint8_t tx_framing = 0;
    uint8_t rx_framing = 0;
    if (tx->generate_field && !rx->generate_field) {
        tx_framing = 1;
        rx_framing = 1;
    } else {
        tx_framing = _tx_rx_framing(tx->technology);
        rx_framing = _tx_rx_framing(rx->technology);
    }

    tx_mode |= _tx_rx_mode(tx_framing, tx->bitrate);
    rx_mode |= _tx_rx_mode(rx_framing, rx->bitrate);

    uint8_t nfc_b_register = 0;
    if (tx->options & NFCDEV_NFC_B_WITHOUT_SOF) {
        nfc_b_register |= PN53_REGISTER_NFC_B_FLAG_TX_NO_SOF;
    }
    if (tx->options & NFCDEV_NFC_B_WITHOUT_EOF) {
        nfc_b_register |= PN53_REGISTER_NFC_B_FLAG_TX_NO_EOF;
    }

    pn53_register_t regs[] = {
        {
            .address = PN53_REGISTER_CONTROL,
            .value = role == NFC_ROLE_INITIATOR ? PN53_REGISTER_CONTROL_FLAG_INITIATOR : 0
        },
        {
            .address = PN53_REGISTER_TX_AUTO,
            .value = tx->technology == NFC_TECHNOLOGY_A ? PN53_REGISTER_TX_AUTO_FLAG_FORCE_100_PERCENT_ASK : 0
        },
        {
            .address = PN53_REGISTER_TX_MODE,
            .value = tx_mode
        },
        {
            .address = PN53_REGISTER_RX_MODE,
            .value = rx_mode
        },
        {
            .address = PN53_REGISTER_BIT_FRAMING,
            .value = 0 // set PN53_REGISTER_BIT_FRAMING_MASK_TX_TRAILING_BIT_COUNT to 0 (all bits)
        },
        {
            .address = PN53_REGISTER_MANUAL_RECEIVER,
            .value = 0 // disable PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY
        },
        {
            .address = PN53_REGISTER_NFC_B,
            .value = nfc_b_register
        },
        {
            .address = PN53_REGISTER_STATUS2,
            .value = 0 // disable PN53_REGISTER_STATUS2_FLAG_CRYPTO1_ENABLED
        }
    };

    int res = (int)pn53_write_registers(dev, regs, ARRAY_SIZE(regs));
    if (res < 0) {
        return res;
    }
    dev->nfc_role = role;
    dev->tx_mode = tx_mode;
    dev->rx_mode = rx_mode;
    dev->bit_framing = 0;
    dev->manual_receiver = 0;
    return 0;
}

int nfcdev_configure_radio_pn53(nfcdev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role) {
    int res = 0;
    if ((res = _check_radio_config(dev->dev, tx, rx, role)) < 0) {
        return res;
    }
    return pn53_configure_radio_unchecked(dev->dev, tx, rx, role);
}

#define PN53_INTERFACE_OP_TX (0b01)
#define PN53_INTERFACE_OP_RX (0b10)

static int _configure_rx_tx(pn53_dev_t* dev, uint8_t ops, uint8_t trailing_tx_bits, uint8_t tx_flags, uint8_t rx_flags, uint8_t manual_recv_flags) {
    assert((ops & PN53_INTERFACE_OP_TX) || (ops & PN53_INTERFACE_OP_RX));
    tx_flags &= PN53_REGISTER_TX_MODE_AUTO_CRC;
    rx_flags &= PN53_REGISTER_RX_MODE_AUTO_CRC;
    manual_recv_flags &= PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY;

    if (IS_ACTIVE(ENABLE_DEBUG)) {
        PN53_DEBUG("nfio.config", "[^] requested crc={tx=%u rx=%u} trailing_bits=%u\n", tx_flags != 0, rx_flags != 0, trailing_tx_bits <= 7 ? trailing_tx_bits : 8);
    }

    pn53_register_t regs[4] = {};
    uint8_t* persisted[ARRAY_SIZE(regs)] = {};
    size_t reg_count = 0;

    if ((dev->manual_receiver & PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY) != manual_recv_flags
        && (pn53_bitfield_get(dev->tx_mode, PN53_REGISTER_TX_MODE_FRAMING) == _tx_rx_framing(NFC_TECHNOLOGY_A) ||
            pn53_bitfield_get(dev->rx_mode, PN53_REGISTER_RX_MODE_FRAMING) == _tx_rx_framing(NFC_TECHNOLOGY_A)
    )) {
        PN53_DEBUG("nfio.config", "[-] ManualRCV dirty (NFC-A parity)\n");
        persisted[reg_count] = &dev->manual_receiver;
        regs[reg_count++] = (pn53_register_t) {
            .address = PN53_REGISTER_MANUAL_RECEIVER,
            .value = (dev->manual_receiver &~ PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY) | manual_recv_flags
        };
    }

    // When TxMode is already as desired, we don't want to issue an additonal WriteRegister HCI
    // command when FIFO transmit is used as the transmit function write to BitFraming anyway
    // and thus can set the trailing bit count.
    if ((ops & PN53_INTERFACE_OP_TX) && (
            (dev->tx_mode & PN53_REGISTER_TX_MODE_AUTO_CRC) != tx_flags ||
            ((trailing_tx_bits <= 7)
                && pn53_bitfield_get(dev->bit_framing, PN53_REGISTER_BIT_FRAMING_MASK_TX_TRAILING_BIT_COUNT) != trailing_tx_bits))
    ) {

        if (trailing_tx_bits <= 7) {
            PN53_DEBUG("nfio.config", "[-] BitFraming dirty\n");
            uint8_t bit_framing = dev->bit_framing;
            pn53_bitfield_set(&bit_framing, PN53_REGISTER_CONTROL_MASK_RX_TRAILING_BIT_COUNT, trailing_tx_bits);

            persisted[reg_count] = &dev->bit_framing;
            regs[reg_count++] = (pn53_register_t) {
                .address = PN53_REGISTER_BIT_FRAMING,
                .value = bit_framing
            };
        }

        PN53_DEBUG("nfio.config", "[-] TxMode dirty\n");
        persisted[reg_count] = &dev->tx_mode;
        regs[reg_count++] = (pn53_register_t) {
            .address = PN53_REGISTER_TX_MODE,
            .value = (dev->tx_mode &~ PN53_REGISTER_TX_MODE_AUTO_CRC) | tx_flags
        };
    }

    if ((ops & PN53_INTERFACE_OP_RX) && (dev->rx_mode & PN53_REGISTER_RX_MODE_AUTO_CRC) != rx_flags) {
        PN53_DEBUG("nfio.config", "[-] RxMode dirty\n");

        persisted[reg_count] = &dev->rx_mode;
        regs[reg_count++] = (pn53_register_t) {
            .address = PN53_REGISTER_RX_MODE,
            .value = (dev->rx_mode & ~PN53_REGISTER_RX_MODE_AUTO_CRC) | rx_flags
        };
    }

    assert(reg_count <= ARRAY_SIZE(regs));
    if (reg_count > 0) {
        PN53_DEBUG("nfio.config", "[-] applying...\n");
        int res = 0;
        if ((res = (int)pn53_write_registers(dev, regs, reg_count)) < 0) {
            return res;
        }

        for (size_t i = 0; i < reg_count; i += 1) {
            assert(persisted[i]);
            *persisted[i] = regs[i].value;
        }
        PN53_DEBUG("nfio.config", "[$] configured for TX/RX\n");
    } else {
        PN53_DEBUG("nfio.config", "[$] nothing to configure for TX/RX\n");
    }
    return 0;
}

static void _shift_iolist(const iolist_t* chunks, iolist_t* offset_chunks, size_t offset) {
    while (chunks->iol_len <= offset) {
        offset -= chunks->iol_len;
        chunks = chunks->iol_next;
    }
    if (chunks) {
        *offset_chunks = *chunks;
        offset_chunks->iol_len -= offset;
        offset_chunks->iol_base += offset;
    }
}

int nfcdev_send_pn53(nfcdev_t* nfcdev, const iolist_t* tx, nfcdev_nfio_flags_t flags) {
    pn53_dev_t* dev = nfcdev->dev;
    size_t _length = iolist_size(tx);
    assert(tx);
    PN53_DEBUG("nfio", "[***] sending %" PRIuSIZE " bytes (%u trailing bits)\n",
               _length, flags.trailing_bits ? flags.trailing_bits : 8);
    assert(_length > 0);

    int res = 0;
    uint8_t trailing_bits;
    uint8_t tx_flags = 0;
    uint8_t manual_recv_flags = 0;

    if (dev->nfc_role == NFC_ROLE_TARGET && dev->nfc_target_need_to_send_atr_res) {
        PN53_DEBUG("nfio", "need to TgSetGeneralBytes, expecting ATR_RES\n");
        iolist_t general_bytes = {};
        size_t length = iolist_size(tx);

        // This requirement is a bit lazy because we could manually remove prefix and CRC
        // and then send.
        switch (flags.interface) {
            case NFCDEV_INTERFACE_PACKET:
                assert(tx->iol_len >= sizeof(nfc_dep_header_t));
                bool is_atr = length >= (sizeof(nfc_dep_header_t) + sizeof(nfc_dep_activation_response_t))
                    && ((nfc_dep_header_t*)tx->iol_base)->direction == NFC_DEP_CMD0_RESPONSE
                    && ((nfc_dep_header_t*)tx->iol_base)->code == NFC_DEP_PDU_CODE_ACTIVATION_REQUEST
                    && (size_t)((nfc_dep_header_t*)tx->iol_base)->length == length;

                if (!is_atr) {
                    PN53_DEBUG("nfio", "need ATR_RES\n");
                    return -EINVAL;
                }
                // Keep track of offset in iolist: in PACKET interface case, there's a header
                // to remove
                res = sizeof(nfc_dep_header_t);
                __attribute__((fallthrough));

            case NFCDEV_INTERFACE_NFC_DEP:
                if (length < sizeof(nfc_dep_activation_response_t)) {
                    PN53_DEBUG("nfio", "need ATR_RES\n");
                    return -EINVAL;
                }
                // Keep track of offset in iolist: in PACKET/NFC_DEP interface cases,
                // there's always the ATR_RES body to skip over to reach the general bytes.
                // General bytes may end up being empty, but that's fine.
                res += sizeof(nfc_dep_activation_response_t);
                _shift_iolist(tx, &general_bytes, (size_t)res);
                if ((res = pn53_tg_set_general_bytes(dev, &general_bytes)) < 0) {
                    return res;
                }
                if ((res = iolist_to_buffer(&general_bytes,
                    pn53_emulated_target(dev)->super.higher_layer.nfc_dep.general,
                    sizeof(pn53_emulated_target(dev)->super.higher_layer.nfc_dep.general))) < 0) {
                    PN53_DEBUG("listen", "general bytes buf too small\n");
                }
                pn53_emulated_target(dev)->super.higher_layer.nfc_dep.length =
                    sizeof(nfc_dep_activation_response_t) + general_bytes.iol_len;

                pn53_emulated_target(dev)->super.higher_layer.nfc_dep.atr.general_bytes_available =
                    general_bytes.iol_len > 0;

                return 0;
            default:
                // We actually need to run TgSetGeneralBytes, and if we don't, the controller
                // loses track of NFC-DEP state (?)
                PN53_DEBUG("nfio", "sending ATR_RES over other interface disables NFC-DEP interface\n");
                pn53_emulated_target(dev)->managed_transport = PN53_MANAGED_TRANSPORT_NONE;
                // Skip TgSetGeneralBytes.
        }
    }

    switch (flags.interface) {
        case NFCDEV_INTERFACE_BITS:
            manual_recv_flags = PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY;
            __attribute__ ((fallthrough));
            // Rest is same as FRAME interface
        case NFCDEV_INTERFACE_FRAME:
            // Will be set by FIFO transceive function, circumvent another RegisterWrite
            trailing_bits = IS_ACTIVE(CONFIG_PN53_INITIATOR_TRANSCEIVE_USING_FIFO)
                ? -1 : flags.trailing_bits;
            break;

        case NFCDEV_INTERFACE_PACKET:
            if (flags.trailing_bits != 0) {
                assert(false);
                PN53_DEBUG("nfio", "only full-byte boundaries supported on interface\n");
            }
            trailing_bits = NFCDEV_TRAILING_BITS_ALL;
            tx_flags = PN53_REGISTER_TX_MODE_AUTO_CRC;
            break;

        case NFCDEV_INTERFACE_ISO_DEP:
        case NFCDEV_INTERFACE_NFC_DEP:
            switch (dev->nfc_role) {
                case NFC_ROLE_INITIATOR:
                    PN53_DEBUG("nfio.dep", "use `transceive` instead as initiator for ISO-DEP/NFC-DEP\n");
                    return -ENOTSUP;
                case NFC_ROLE_TARGET:
                    if (pn53_emulated_target(dev)->managed_transport != (pn53_managed_target_transport_t)flags.interface) {
                        PN53_DEBUG("nfio.dep", "controller did not activate interface\n");
                        return -ENOTCONN;
                    }
                    return pn53_tg_set_data(dev, tx);
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }
        default:
            PN53_DEBUG("nfio", "interface not supported\n");
            return -ENOTSUP;
    }

    if (flags.use_nad || flags.use_did) {
        PN53_DEBUG("nfio", "DID/CID, NAD not supported on interface\n");
        return -ENOTSUP;
    }

    // TODO: target.
    // BITS, FRAME, PACKET interfaces
    if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_TX,
        trailing_bits, tx_flags, 0, manual_recv_flags)) < 0) {
        PN53_DEBUG("nfio", "failed to configure radio\n");
        return res;
    }

    switch (dev->nfc_role) {
        case NFC_ROLE_TARGET:
            if (!IS_ACTIVE(CONFIG_PN53_TARGET_SEND_USING_FIFO)) {
                PN53_DEBUG("nfio", "using TgResponseToInitiator\n");
                return pn53_tg_response_to_initiator(dev, tx);
            }
            break;
        default: break;
    }

    PN53_DEBUG("nfio", "using FIFO\n");
    return pn53_fifo_transmit(dev, tx, flags.trailing_bits);
}

ssize_t nfcdev_receive_pn53(nfcdev_t* nfcdev,
    uint8_t** rx, size_t capacity,
    uint32_t rx_timeout_ms, nfcdev_nfio_flags_t flags
) {
    pn53_dev_t* dev = nfcdev->dev;
    PN53_DEBUG("nfio", "[***] receiving\n");

    ssize_t res = 0;
    uint8_t* internal;
    uint8_t rx_flags = 0;
    uint8_t manual_recv_flags = 0;

    if (dev->nfc_role == NFC_ROLE_TARGET && dev->nfc_target_first_rx_length > 0) {
        bool response_has_been_sent = (dev->model == PN53_MODEL_PN532
            && dev->nfc_parameters & PN532_NFC_PARAMETER_TARGET_ISO_DEP_AUTO_HANDSHAKE
            && pn53_emulated_target(dev)->managed_transport == PN53_MANAGED_TRANSPORT_ISO_DEP)
            || (dev->nfc_parameters & PN53_NFC_PARAMETER_TARGET_NFC_DEP_AUTO_HANDSHAKE
                && pn53_emulated_target(dev)->managed_transport == PN53_MANAGED_TRANSPORT_NFC_DEP);
        // If the ATS or ATR_RES has been automatically sent, we do not want to
        // to let the application use `receive` a command because that'd imply
        // it could also `send` a respone -- a response that has, in fact, already been
        // sent by the controller -- namely the automatic ATS or ATR_RES.
        // In that case, the application can use this special ability of PN53s (or PN532)
        // to retain and retrieve the request that elicited the automatic response
        // by calling pn53_listen_get_rats() or pn53_listen_get_atr_request().
        if (!response_has_been_sent) {
            PN53_DEBUG("nfio", "returning retained 1st cmd\n");
            res = dev->nfc_target_first_rx_length;
            uint8_t* command = dev->nfc_target_first_rx;
            // This is either a proprietary command (or ATR_REQ if auto ATR_RES is disabled)
            // So ISO-DEP cannot be managed, otherwise this would RATS that came out of
            // TgInitAsTarget
            assert(dev->nfc_emulated_transport != PN53_MANAGED_TRANSPORT_ISO_DEP);
            // And NFC-DEP can only be managed if the command was an ATR_REQ
            nfc_dep_activation_request_t* _atr;
            assert(dev->nfc_emulated_transport == PN53_MANAGED_TRANSPORT_NFC_DEP
                   || pn53_listen_get_atr_request(dev, &_atr) <= 0);

            switch (flags.interface) {
                case NFCDEV_INTERFACE_PACKET: break;
                case NFCDEV_INTERFACE_NFC_DEP:
                    if ((res = pn53_listen_get_atr_request(dev, (nfc_dep_activation_request_t**)&command)) <= 0) {
                        PN53_DEBUG("nfio", "cannot use NFC_DEP interface on 1st cmd, not ATR_REQ\n");
                        return -ENOMSG;
                    }
                    break;
                default:
                    PN53_DEBUG("nfio", "cannot use interface on 1st cmd\n");
                    return -EINVAL;
            }
            dev->nfc_target_first_rx = NULL;
            dev->nfc_target_first_rx_length = 0;

            if (rx) {
                if (*rx && capacity > 0) {
                    if (capacity < (size_t)res) {
                        return -ENOBUFS;
                    }
                    memcpy(*rx, command, (size_t)res);
                } else {
                    *rx = command;
                }
            }
            return res;
        }
    }

    switch (flags.interface) {
        case NFCDEV_INTERFACE_BITS:
            manual_recv_flags = PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY;
            __attribute__ ((fallthrough));
            // Rest is same as FRAME interface
        case NFCDEV_INTERFACE_FRAME:
            break;

        case NFCDEV_INTERFACE_PACKET:
            rx_flags = PN53_REGISTER_RX_MODE_AUTO_CRC;
            break;

        case NFCDEV_INTERFACE_ISO_DEP:
        case NFCDEV_INTERFACE_NFC_DEP:
            switch (dev->nfc_role) {
                case NFC_ROLE_INITIATOR:
                    PN53_DEBUG("nfio.dep", "use `transceive` as initiator for ISO-DEP/NFC-DEP\n");
                    return -ENOTSUP;
                case NFC_ROLE_TARGET:
                    if (pn53_emulated_target(dev)->managed_transport != (pn53_managed_target_transport_t)flags.interface) {
                        PN53_DEBUG("nfio.dep", "controller did not activate interface\n");
                        return -ENOTCONN;
                    }
                    uint8_t* command = NULL;
                    res = pn53_tg_get_data(dev, &command, rx_timeout_ms);
                    if (res > 0 && rx) {
                        if (*rx && capacity > 0) {
                            if (capacity < (size_t)res) {
                                return -ENOBUFS;
                            }
                            memcpy(*rx, command, (size_t)res);
                        } else {
                            *rx = command;
                        }
                    }
                    return res;
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }
        default:
            PN53_DEBUG("nfio", "interface not supported\n");
            return -ENOTSUP;
    }

    if (flags.use_nad || flags.use_did) {
        PN53_DEBUG("nfio", "DID/CID, NAD not supported on interface\n");
        return -ENOTSUP;
    }

    // BITS, FRAME, PACKET interfaces
    if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_RX,
                                -1, 0, rx_flags, manual_recv_flags)) < 0) {
        PN53_DEBUG("nfio", "failed to configure radio\n");
        return res;
    }

    switch (dev->nfc_role) {
        case NFC_ROLE_TARGET:
            if (pn53_emulated_target(dev)->managed_transport == PN53_MANAGED_TRANSPORT_ISO_DEP) {
                PN53_DEBUG("nfio", "can only use ISO_DEP interface, managed by controller\n");
                return -ENOTSUP;
            }
            if (!IS_ACTIVE(CONFIG_PN53_TARGET_RECEIVE_USING_FIFO)) {
                PN53_DEBUG("nfio", "using TgGetInitiatorCommand\n");
                uint8_t* command = NULL;
                res = pn53_tg_get_initiator_command(dev, &command, rx_timeout_ms);
                if (res > 0 && rx) {
                    if (*rx && capacity > 0) {
                        if (capacity < (size_t)res) {
                            return -ENOBUFS;
                        }
                        memcpy(*rx, command, (size_t)res);
                    } else {
                        *rx = command;
                    }
                }
                return res;
            }
            break;
        default: break;
    }

    PN53_DEBUG("nfio", "using FIFO\n");
    if (capacity == 0 || !rx || !*rx) {
        PN53_DEBUG("nfio", "need buffer to receive from FIFO\n");
        return -ENOBUFS;
    }

    uint8_t trailing_bits = NFCDEV_TRAILING_BITS_ALL;
    if ((res = pn53_fifo_receive(dev, *rx, capacity, &trailing_bits, rx_timeout_ms)) < 0) {
        return res;
    }
    PN53_DEBUG("nfio", "trailing bits: %u\n",
               trailing_bits == NFCDEV_TRAILING_BITS_ALL ? 8 : trailing_bits);
    // We get bit count basically for free from FIFO function, we do that
    // to avoid another HCI round trip here just to get the bit count.
    // 2 additional bytes to retrieve the bit count in the FIFO function are bearable...
    nfcdev->trailing_bit_count = trailing_bits;
    return res;
}

ssize_t nfcdev_transceive_pn53(nfcdev_t* nfcdev,
    const iolist_t* tx,
    uint8_t** rx, size_t capacity,
    uint32_t rx_timeout_ms, nfcdev_nfio_flags_t flags
) {
    pn53_dev_t* dev = nfcdev->dev;
    assert(tx);
    assert(!flags.reassemble || (rx && capacity > 0));
    size_t _length = iolist_size(tx);
    PN53_DEBUG("nfio", "[***] transceiving %" PRIuSIZE " bytes (%u trailing bits)\n",
               _length, flags.trailing_bits ? flags.trailing_bits : 8);
    assert(_length > 0);
    ssize_t res = 0;
    uint8_t* internal;

    uint8_t trailing_bits;
    uint8_t tx_flags = 0;
    uint8_t rx_flags = 0;
    uint8_t manual_recv_flags = 0;

    switch (flags.interface) {
        case NFCDEV_INTERFACE_BITS:
            manual_recv_flags = PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY;
            __attribute__ ((fallthrough));
            // Rest is same as FRAME interface
        case NFCDEV_INTERFACE_FRAME:
            // Will be set by FIFO transceive function, circumvent another RegisterWrite
            trailing_bits = IS_ACTIVE(CONFIG_PN53_INITIATOR_TRANSCEIVE_USING_FIFO)
                ? -1 : flags.trailing_bits;
            break;

        case NFCDEV_INTERFACE_PACKET:
            if (flags.trailing_bits != 0) {
                assert(false);
                PN53_DEBUG("nfio", "only full-byte boundaries supported on interface\n");
            }
            trailing_bits = NFCDEV_TRAILING_BITS_ALL;
            tx_flags = PN53_REGISTER_TX_MODE_AUTO_CRC;
            rx_flags = PN53_REGISTER_RX_MODE_AUTO_CRC;
            break;

        case NFCDEV_INTERFACE_ISO_DEP:
        case NFCDEV_INTERFACE_NFC_DEP:
            if (flags.trailing_bits != 0) {
                assert(false);
                PN53_DEBUG("nfio", "only full-byte boundaries supported on interface\n");
            }
            switch (dev->nfc_role) {
                case NFC_ROLE_INITIATOR: {
                    pn53_logical_target_t* target = pn53_current_target(dev);
                    if (!target && target->managed_transport != (pn53_managed_target_transport_t)flags.interface) {
                        PN53_DEBUG("nfio.dep", "controller did not activate interface, consider nfcdev_hostnfc\n");
                        return -ENOTCONN;
                    }

                    uint8_t params = dev->nfc_parameters;
                    pn53_bitfield_set(&params, PN53_NFC_PARAMETER_INITIATOR_USE_CID, flags.use_did);
                    pn53_bitfield_set(&params, PN53_NFC_PARAMETER_INITIATOR_USE_NAD, flags.use_nad);
                    if (params != dev->nfc_parameters) {
                        PN53_DEBUG("nfio", "need to set params for CID/DID/NAD\n");
                        if ((res = pn53_set_parameters(dev, params)) < 0) {
                            return res;
                        }
                    }

                    if (flags.slice) {
                        return 0;
//                        size_t max_payload = pn53_max_exchange_payload_length(dev);
//                        uint8_t* slice = (uint8_t*)&dev->connection.backing + PN35_FRAME_HEADER_NORMAL_COMMAND;
//                        const iolist_t* chunk = tx;
//                        size_t to_be_sent = iolist_size(tx);
//                        while (to_be_sent > 0) {
//                            size_t avail = max_payload;
//                            while (chunk && to_be_sent > 0 && avail > 0) {
//                                size_t portion = MIN(chunk->iol_len, avail);
//                                memcpy(slice, chunk->iol_base, <#size_t n#>)
//
//                                slice += portion;
//                                avail -= portion;
//                                to_be_sent -= portion;
//                            }
//
//                            ssize_t res = pn53_in_data_exchange(dev, pn53_current_connection_id(dev), &status, &slice, rx, rx_timeout_ms);
//                            if (res < 0) {
//                                return res;
//                            }
//                        }

                    } else {
                        return pn53_in_data_exchange(dev, pn53_current_connection_id(dev), NULL, tx, rx, rx_timeout_ms);
                    }
                }
                case NFC_ROLE_TARGET:
                    if ((res = nfcdev_send_pn53(nfcdev, tx, flags)) < 0) {
                        return res;
                    }
                    return nfcdev_receive_pn53(nfcdev, rx, capacity, rx_timeout_ms, flags);
                default:
                    assert(false);
                    UNREACHABLE();
                    return -1;
            }
        default:
            PN53_DEBUG("nfio", "interface not supported\n");
            return -ENOTSUP;
    }

    if (flags.use_nad || flags.use_did) {
        PN53_DEBUG("nfio", "DID/CID, NAD not supported on interface\n");
        return -ENOTSUP;
    }

    // BITS, FRAME, PACKET interfaces
    if ((res = _configure_rx_tx(dev, PN53_INTERFACE_OP_TX | PN53_INTERFACE_OP_RX,
        trailing_bits, tx_flags, rx_flags, manual_recv_flags)) < 0) {
        PN53_DEBUG("nfio", "failed to configure radio\n");
        return res;
    }
    switch (dev->nfc_role) {
        case NFC_ROLE_INITIATOR:
            if (IS_ACTIVE(CONFIG_PN53_INITIATOR_TRANSCEIVE_USING_FIFO)) {
                PN53_DEBUG("nfio", "using FIFO\n");
                if (capacity == 0 || !rx || !*rx) {
                    PN53_DEBUG("nfio", "need buffer to receive from FIFO\n");
                    return -ENOBUFS;
                }

                uint8_t trailing_bits = NFCDEV_TRAILING_BITS_ALL;
                if ((res = pn53_fifo_transceive_initiator(dev,
                    tx, flags.trailing_bits,
                    *rx, capacity, &trailing_bits, rx_timeout_ms
                )) < 0) {
                    return res;
                }
                PN53_DEBUG("nfio", "trailing bits: %u\n",
                           trailing_bits == NFCDEV_TRAILING_BITS_ALL ? 8 : trailing_bits);
                // We get bit count basically for free from FIFO function, we do that
                // to avoid another HCI round trip here just to get the bit count.
                // 2 additional bytes to retrieve the bit count in the FIFO function are bearable...
                nfcdev->trailing_bit_count = trailing_bits;
                return res;
            } else {
                PN53_DEBUG("nfio", "using InCommunicateThru\n");
                res = pn53_in_communicate_thru(dev, tx, &internal, rx_timeout_ms);
                if (res < 0) {
                    return res;
                }

                if (rx) {
                    // Interested in response
                    if (*rx && capacity > 0) {
                        // Wants copy
                        PN53_DEBUG("nfio", "response copy requested\n");
                        if (capacity < (size_t)res) {
                            PN53_DEBUG("nfio", "cannot copy response, need %"
                                       PRIuSIZE ", buffer has only %" PRIuSIZE " bytes\n",
                                       (size_t)res, capacity);
                            return -ENOBUFS;
                        }
                        memcpy(*rx, internal, (size_t)res);
                    } else {
                        // Wants ref to internal temporary buf
                        *rx = internal;
                    }
                }

                if (pn53_bitfield_get(dev->rx_mode, PN53_REGISTER_RX_MODE_FRAMING) == _tx_rx_framing(NFC_TECHNOLOGY_A)) {
                    // Need to update response trailing bits bits

                    if (flags.interface == NFCDEV_INTERFACE_BITS || flags.interface == NFCDEV_INTERFACE_FRAME) {
                        // Need to retrieve trailing bit count
                        PN53_DEBUG("nfio", "need to retrieve RX bit count\n");
                        pn53_register_address_t addr = PN53_REGISTER_CONTROL;
                        ssize_t res2 = pn53_read_registers(dev, &addr, &internal, 1);
                        if (res2 < 0) {
                            return res;
                        }
                        nfcdev->trailing_bit_count =
                            pn53_bitfield_get(*internal, PN53_REGISTER_CONTROL_MASK_RX_TRAILING_BIT_COUNT);

                        PN53_DEBUG("nfio", "trailing bits: %u\n",
                                   nfcdev->trailing_bit_count == NFCDEV_TRAILING_BITS_ALL ? 8 : nfcdev->trailing_bit_count);
                    }
                }
                return res;
            }
        case NFC_ROLE_TARGET:
            if ((res = nfcdev_send_pn53(nfcdev, tx, flags)) < 0) {
                return res;
            }
            return nfcdev_receive_pn53(nfcdev, rx, capacity, rx_timeout_ms, flags);
        default:
            assert(false);
            UNREACHABLE();
            return -1;
    }
}
