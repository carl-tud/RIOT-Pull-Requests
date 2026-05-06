/*
 * Copyright (C) 2026 Carl Seifert
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include <stdio.h>
#include <string.h>

#define __ANY_DEBUG IS_ACTIVE(CONFIG_PN53_DEBUG_HCI) || IS_ACTIVE(CONFIG_PN53_DEBUG_TRANSPORT)
#define ENABLE_DEBUG __ANY_DEBUG
#include "debug.h"

#include "assert.h"
#include "kernel_defines.h"
#include "architecture.h"
#include "ztimer.h"
#include "mutex.h"
#include "sema.h"
#include "iolist.h"
#include "periph/gpio.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "periph/uart.h"

#include "pn53x.h"

#define PN53_DEBUG_TRANSPORT(...) \
    do { if(IS_ACTIVE(CONFIG_PN53_DEBUG_TRANSPORT)) { DEBUG("pn53x.hci.transport: " __VA_ARGS__); }} \
    while(0)

#define PN53_DEBUG_HCI(...) \
    do { if(IS_ACTIVE(CONFIG_PN53_DEBUG_HCI)) { DEBUG("pn53x.hci: " __VA_ARGS__); }} \
    while(0)

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

static inline void __debug_iolist(const iolist_t* chunks) {
    while (chunks) {
        PN53_DEBUG_HEX((uint8_t*)chunks->iol_base, chunks->iol_len);
        chunks = chunks->iol_next;
    }
}

#define PN53_DEBUG_CHUNKS(chunks) __debug_iolist(chunks)

#define PN53_HCI_IRQ_SUPPORTED (IS_USED(MODULE_PN53X_I2C) || IS_USED(MODULE_PN53X_SPI))

#define PN53_BUS_I2C_ADDRESS        (0x24)
#define SPI_MODE                    (SPI_MODE_0)
#define SPI_CLK                     (SPI_CLK_1MHZ)
#define UART_BAUDRATE               (115200U)
#define LISTEN_TIMEOUT_SEC          (60U)
#define STANDARD_TIMEOUT_SEC        (2U)

#define SPI_DATA_WRITE              (0x80)
#define SPI_STATUS_READING          (0x40)
#define SPI_DATA_READ               (0xC0)
#define SPI_WRITE_DELAY_US          (2000)

#define PN53_FRAME_IDENTIFIER_HOST_TO_CONTROLLER   (0xd4)
#define PN53_FRAME_IDENTIFIER_CONTROLLER_TO_HOST (0xd5)
#define PN53_FRAME_IDENTIFIER_ERROR                (0x7f)

#define PN53_PREAMBLE_0 0x00
#define PN53_PREAMBLE_1 0x00
#define PN53_PREAMBLE_2 0xff
#define PN53_PREAMBLE   PN53_PREAMBLE_0, PN53_PREAMBLE_1, PN53_PREAMBLE_2
#define PN53_POSTAMBLE  0x00

static uint8_t _ack_frame[] = { PN53_PREAMBLE, 0x00, 0xff, PN53_POSTAMBLE };
static uint8_t _nack_frame[] = { PN53_PREAMBLE, 0xff, 0x00, PN53_POSTAMBLE };

static inline spi_t _spi(const pn53_connection_t* connection) {
    assert(connection->config->bus.kind == PN53_BUS_SPI);
#if IS_USED(MODULE_PN53X_SPI)
    return connection->config->bus.spi;
#else
    (void)connection;
    return 0;
#endif
}

static inline i2c_t _i2c(const pn53_connection_t* connection) {
    assert(connection->config->bus.kind == PN53_BUS_I2C);
#if IS_USED(MODULE_PN53X_I2C)
    return connection->config->bus.i2c;
#else
    (void)connection;
    return 0;
#endif
}

static inline uart_t _uart(const pn53_connection_t* connection) {
    assert(connection->config->bus.kind == PN53_BUS_UART);
#if IS_USED(MODULE_PN53X_UART)
    return connection->config->bus.uart;
#else
    (void)connection;
    return 0;
#endif
}

static void _append_chunk(iolist_t* iolist, iolist_t* chunk) {
    assert(iolist);
    while (iolist->iol_next) {
        iolist = iolist->iol_next;
    }
    iolist->iol_next = chunk;
}

static uint8_t _calculate_packet_checksum(uint8_t* packet, size_t length) {
    uint8_t acc = 0;
    while (length--) {
        acc -= *packet;
        packet += 1;
    }
    return acc;
}

static uint8_t _calculate_packet_checksum_chunked(iolist_t* chunks, uint8_t extra) {
    uint8_t acc = 0;
    while (chunks) {
        acc += _calculate_packet_checksum((uint8_t*)chunks->iol_base, chunks->iol_len);
        chunks = chunks->iol_next;
    }
    acc += _calculate_packet_checksum(&extra, 1);
    return acc;
}

static void _reverse(uint8_t* buffer, size_t length) {
    while (length--) {
        buffer[length] = (buffer[length] & 0xF0) >> 4 | (buffer[length] & 0x0F) << 4;
        buffer[length] = (buffer[length] & 0xCC) >> 2 | (buffer[length] & 0x33) << 2;
        buffer[length] = (buffer[length] & 0xAA) >> 1 | (buffer[length] & 0x55) << 1;
    }
}

static ssize_t _frame_packet(iolist_t* prefix, iolist_t* packet, iolist_t* suffix, size_t max_packet_length) {
    assert(prefix);
    assert(prefix->iol_base);
    assert(prefix->iol_len >= PN53_FRAME_OVERHEAD_MIN);
    assert(packet);
    assert(suffix);

    size_t packet_length = iolist_size(packet) + 1;
    if (packet_length > max_packet_length) {
        PN53_DEBUG_TRANSPORT("packet is %" PRIuSIZE " bytes, longer than max of %" PRIuSIZE "\n",
                             packet_length, max_packet_length);
        return -PN53_ERROR_CONNECTION_PACKET_LENGTH_OUT_OF_RANGE;
    }

    uint8_t* buffer = (uint8_t*)prefix->iol_base;
    uint8_t* cursor = buffer;
    *cursor++ = PN53_PREAMBLE_0;
    *cursor++ = PN53_PREAMBLE_1;
    *cursor++ = PN53_PREAMBLE_2;

    uint8_t lcs = 0;
    if (packet_length < 0xff) {
        /* Normal frame */
        *cursor++= (uint8_t)packet_length;
        lcs -= (uint8_t)packet_length;
    } else {
        /* Extended frame */
        *cursor++ = 0xff;
        *cursor++ = 0xff;
        *cursor++ = (packet_length >> 8) & 0xff;
        *cursor++ = packet_length & 0xff;

        lcs -= (packet_length >> 8) & 0xff;
        lcs -=  packet_length & 0xff;
    }

    *cursor++ = lcs; /* Length checksum (LCS) */
    *cursor++ = PN53_FRAME_IDENTIFIER_HOST_TO_CONTROLLER; /* T? frame indentifier (host to ctrler) */

    prefix->iol_len = (size_t)(ptrdiff_t)(cursor - buffer),
    prefix->iol_next = packet;

    suffix->iol_base = (void*)cursor,
    suffix->iol_len = 2;

    *cursor++ = _calculate_packet_checksum_chunked(packet, PN53_FRAME_IDENTIFIER_HOST_TO_CONTROLLER);
    *cursor++ = 0;

    _append_chunk(packet, suffix);
    return 0;
}

static ssize_t _parse_packet_begin(uint8_t** cursor, size_t length) {
    uint8_t* frame = *cursor;
    size_t packet_length = 0;
    uint8_t length_checksum = 0;

    /* Read header (entire frame must at least be 8 bytes: 00 00 FF LEN LCS TFI DCS 00 */
    if (length < PN53_FRAME_OVERHEAD_MIN ||
         (frame[0] != 0x00) || (frame[1] != 0x00) || (frame[2] != 0xff)) {
        PN53_DEBUG_TRANSPORT("invalid frame header\n");
        return -PN53_ERROR_CONNECTION_CORRUPTED;
    }
    frame += 3;

    if (frame[0] == 0xff && frame[1] == 0xff) {
        /* Extended frame
         * FF FF LEN LEN ...
         * The extended frame header including LCS luckily fits into PN53_HCI_FRAME_OVERHEAD_MIN=8,
         * so we don't need to check again. */
        packet_length = (frame[2] << 8) | frame[3];
        /* Okay if overflows */
        length_checksum -= frame[2] + frame[3];
        frame += 4;
    } else {
        if (frame[0] == 0xff) {
            PN53_DEBUG_TRANSPORT("rolling with unexpected frame length 0xff\n");
        }
        /* Normal frame
         * LEN ... */
        packet_length = (size_t)*frame;
        length_checksum -= *frame;
        frame += 1;
    }

    uint8_t lcs = *frame; /* Length checksum (LCS) */
    frame += 1;
    if (length_checksum != lcs) {
        PN53_DEBUG_TRANSPORT("LCS mismatch\n");
        return -PN53_ERROR_CONNECTION_CHECKSUM_MISMATCH;
    }

    if (packet_length > PN53_PACKET_LENGTH_MAX) {
        PN53_DEBUG_TRANSPORT("packet is %" PRIuSIZE " bytes, longer than max of %" PRIuSIZE "\n",
                             packet_length, PN53_PACKET_LENGTH_MAX);
        return -PN53_ERROR_CONNECTION_PACKET_LENGTH_OUT_OF_RANGE;
    }

    if (packet_length == 0) {
        PN53_DEBUG_TRANSPORT("packet is empty");
        return -PN53_ERROR_CONNECTION_PACKET_EMPTY;
    }

    *cursor = frame;
    return packet_length;
}

static ssize_t _parse_packet_end(uint8_t** cursor, size_t packet_length) {
    assert(packet_length > 0);
    uint8_t* packet = *cursor;
    uint8_t tfi = *packet; /* T? frame identifier (TFI) */

    /* TFI+packet checksum in DCS, check DCS */
    uint8_t dcs = packet[packet_length];
    if (_calculate_packet_checksum(packet, packet_length) != dcs) {
        PN53_DEBUG_TRANSPORT("DCS mismatch\n");
        return -PN53_ERROR_CONNECTION_CHECKSUM_MISMATCH;
    }

    uint8_t postamble = packet[packet_length + 1];
    if (postamble != 0) {
        PN53_DEBUG_TRANSPORT("preposterous postamble %02x, expected 00\n", postamble);
        return -PN53_ERROR_CONNECTION_CORRUPTED;
    }

    if (tfi == PN53_FRAME_IDENTIFIER_ERROR) {
        PN53_DEBUG_HCI("packet type: error (TFI=0x7d)\n");
        return -PN53_ERROR_CONNECTION_FRAME_SYNTAX;
    }
    if (tfi != PN53_FRAME_IDENTIFIER_CONTROLLER_TO_HOST) {
        PN53_DEBUG_HCI("illegal TFI %02X from controller, expected 0xd5\n", tfi);
        return -PN53_ERROR_CONNECTION_UNEXPECTED_FRAME_DIRECTION;
    }
    PN53_DEBUG_TRANSPORT("packet type: response (TFI=0xd5, length=%" PRIuSIZE " bytes)\n", packet_length);

    *cursor = packet + 1;
    return packet_length - 1;
}

static int _parse_ack(uint8_t* frame, size_t length) {
    if (length != sizeof(_ack_frame) || memcmp(frame, _ack_frame, sizeof(_ack_frame)) != 0) {
        PN53_DEBUG_TRANSPORT("corrupted ACK frame\n");
        return -PN53_ERROR_CONNECTION_CORRUPTED;
    }
    return 0;
}

static void _hci_event(void* connection) {
    // PN53_DEBUG_TRANSPORT("HCI event\n");
    sema_post(&((pn53_connection_t*)connection)->trap);
}

#define PN53_RESET_TOGGLE_SLEEP_MS       (400)
#define PN53_RESET_BACKOFF_MS            (10)

void pn53_hci_reset(const pn53_connection_t* connection) {
    assert(connection);
    gpio_clear(connection->config->reset);
    ztimer_sleep(ZTIMER_MSEC, PN53_RESET_TOGGLE_SLEEP_MS);
    gpio_set(connection->config->reset);
    ztimer_sleep(ZTIMER_MSEC, PN53_RESET_BACKOFF_MS);
}

int pn53_hci_init(pn53_connection_t* connection) {
#if PN53_HCI_IRQ_SUPPORTED
    PN53_DEBUG_TRANSPORT("using HCI IRQ\n");
    sema_create(&connection->trap, 0);
    gpio_init_int(connection->config->irq, GPIO_IN_PU, GPIO_FALLING, _hci_event, (void*)connection);
#endif

    gpio_init(connection->config->reset, GPIO_OUT);
    gpio_set(connection->config->reset);

    if (connection->config->bus.kind == PN53_BUS_SPI) {
        PN53_DEBUG_TRANSPORT("initializing SPI\n");
#if IS_USED(MODULE_PN53X_SPI)
        /* we handle the CS line manually... */
        gpio_init(connection->config->chip_select, GPIO_OUT);
        gpio_set(connection->config->chip_select);
#else
        return -ENOTSUP;
#endif
    } else if (connection->config->bus.kind) {
#if IS_USED(MODULE_PN53X_UART)
        PN53_DEBUG_TRANSPORT("initializing UART\n");
        mutex_init(&connection->callback);
        mutex_lock(&connection->callback);
        int ret = uart_init(dev->conf->uart, UART_BAUDRATE, uart_rx_cb, (void *) dev);
        if (ret < 0) {
            PN53_DEBUG_TRANSPORT("uart_init failed with %d\n", ret);
            return ret;
        }
        ztimer_sleep(ZTIMER_MSEC, 1000);
        // uart_mode(dev->conf->uart, UART_DATA_BITS_8, UART_PARITY_NONE, UART_STOP_BITS_1);
        // uart_poweron(dev->conf->uart);
#else
        return -ENOTSUP;
#endif
    }

    return 0;
}

int _read_status(const pn53_connection_t* connection, uint8_t* status) {
    (void)connection;
    (void)status;

#if IS_USED(MODULE_PN53X_SPI)
    if (connection->config->bus.kind == PN53_BUS_SPI) {
        spi_acquire(_spi(connection), SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
        gpio_clear(connection->config->chip_select);

        spi_transfer_byte(_spi(connection), SPI_CS_UNDEF, true, SPI_STATUS_READING);
        spi_transfer_bytes(_spi(connection), SPI_CS_UNDEF, true, NULL, status, 1);

        gpio_set(connection->config->chip_select);
        spi_release(_spi(connection));
        return 0;
    }
#endif
    return -1;
}


static ssize_t _write(pn53_connection_t* connection, iolist_t* chunks) {
    (void)chunks;
    if (IS_ACTIVE(CONFIG_PN53_DEBUG_TRANSPORT)) {
        PN53_DEBUG_TRANSPORT("[->] ");
        PN53_DEBUG_CHUNKS(chunks);
        DEBUG("\n");
    }

    ssize_t res = -1;
    switch (connection->config->bus.kind) {
#if IS_USED(MODULE_PN53X_I2C)
    case PN53_BUS_I2C:
        i2c_acquire(_i2c(connection));
        res = i2c_write_bytes(_i2c(connection), PN53_BUS_I2C_ADDRESS, buffer, length, 0);
        if (res == 0) {
            res = (ssize_t)length;
        }
        i2c_release(_i2c(connection));
        break;
#endif
#if IS_USED(MODULE_PN53X_SPI)
    case PN53_BUS_SPI:
        if ((res = iolist_to_buffer(chunks, connection->backing, sizeof(connection->backing))) < 0) {
            PN53_DEBUG_TRANSPORT("failed to copy chunks to backing buffer\n");
            return res;
        }
        _reverse(connection->backing, (size_t)res);

        spi_acquire(_spi(connection), SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
        gpio_clear(connection->config->chip_select);

        ztimer_sleep(ZTIMER_USEC, SPI_WRITE_DELAY_US);
        spi_transfer_byte(_spi(connection), SPI_CS_UNDEF, true, SPI_DATA_WRITE);

        /* Problem: need to reverse bits, so need to copy to temporary buffer -- otherwise
         * we inadvertently change application buffers... */

        spi_transfer_bytes(_spi(connection), SPI_CS_UNDEF, true, connection->backing, NULL, (size_t)res);

        /*
        while (chunks) {
            _reverse((uint8_t*)chunks->iol_base, chunks->iol_len);
            spi_transfer_bytes(_spi(connection), SPI_CS_UNDEF, true,
                               (uint8_t*)chunks->iol_base, NULL, chunks->iol_len);
            chunks = chunks->iol_next;
        }
         res = (ssize_t)iolist_size(chunks);
        */

        gpio_set(connection->config->chip_select);
        spi_release(_spi(connection));
        break;
#endif
#if IS_USED(MODULE_PN53X_UART)
    case PN53_BUS_UART:
        // reverse(buff, len);
        uart_write(_uart(connection), buffer, length);
        res = (ssize_t)length;
        break;
#endif
    default:
        PN53_DEBUG_TRANSPORT("unsupported bus kind\n");
        assert(false);
        return -1;
    }

    /* TODO: Why wait here? */
    ztimer_sleep(ZTIMER_USEC, 1000);
    return res;
}

#define PN53_READ_INITIAL (0b01)
#define PN53_READ_FINAL   (0b10)

static ssize_t _read(const pn53_connection_t* connection, uint8_t* buffer, size_t length, uint8_t flags) {
    ssize_t res = -1;

    switch (connection->config->bus.kind) {
#if IS_USED(MODULE_PN53X_I2C)
    case PN53_BUS_I2C:
        i2c_acquire(_i2c(connection));
        /* length+1 for RDY after read is accepted */
        ret = i2c_read_bytes(_i2c(connection), PN53_BUS_I2C_ADDRESS, buffer, length + 1, 0);
        if (ret == 0) {
            ret = (int)len + 1;
        }
        i2c_release(_i2c(connection));
        break;
#endif
#if IS_USED(MODULE_PN53X_SPI)
    case PN53_BUS_SPI:
        if (flags & PN53_READ_INITIAL) {
            spi_acquire(_spi(connection), SPI_CS_UNDEF, SPI_MODE, SPI_CLK);
            gpio_clear(connection->config->chip_select);

            spi_transfer_byte(_spi(connection), SPI_CS_UNDEF, true, SPI_DATA_READ);
        }

        spi_transfer_bytes(_spi(connection), SPI_CS_UNDEF, true, NULL, buffer, length);

        if (flags & PN53_READ_FINAL) {
            gpio_set(connection->config->chip_select);
            spi_release(_spi(connection));
        }

        /* this byte is inserted to keep SPI in line with I2C */
        res = (ssize_t)length;
        _reverse(buffer, length);
        break;
#endif
    /* TODO: Where's UART? */
    default:
        PN53_DEBUG_TRANSPORT("unsupported bus kind\n");
        assert(false);
        return -1;
    }

    /* wait for a while */
    ztimer_sleep(ZTIMER_USEC, SPI_WRITE_DELAY_US);

    if (IS_ACTIVE(CONFIG_PN53_DEBUG_TRANSPORT) && res >= 0) {
        PN53_DEBUG_TRANSPORT("[<-] ");
        PN53_DEBUG_HEX(buffer, (size_t)res);
        DEBUG("\n");
    }
    return res;
}

static ssize_t _send_ack(pn53_connection_t* connection) {
    assert(connection);
    static iolist_t ack = { .iol_base = (void*)_ack_frame, .iol_len = sizeof(_ack_frame) };
    return _write(connection, &ack);
}

static ssize_t _recv_ack(const pn53_connection_t* connection) {
    PN53_DEBUG_TRANSPORT("recv ack..\n");
    assert(connection);
    ssize_t res = 0;
    uint8_t frame[sizeof(_ack_frame)];
    if ((res = _read(connection, frame, sizeof(frame), PN53_READ_INITIAL | PN53_READ_FINAL)) < 0) {
        return res;
    }
    assert((size_t)res <= sizeof(frame));
    if (res != sizeof(_ack_frame)) {
        if (IS_ACTIVE(CONFIG_PN53_DEBUG_TRANSPORT)) {
            PN53_DEBUG_TRANSPORT("expected ACK, received corrupted frame: ");
            PN53_DEBUG_HEX(frame, res);
        }
        return -PN53_ERROR_CONNECTION_CORRUPTED;
    }
    return _parse_ack(frame, res);
}

static ssize_t _send_nack(pn53_connection_t* connection) {
    assert(connection);
    static iolist_t nack = { .iol_base = (void*)_nack_frame, .iol_len = sizeof(_nack_frame) };
    return _write(connection, &nack);
}

/* We never receive NACKs, so don't need _recv_nack. */

static ssize_t _send_packet(pn53_connection_t* connection, iolist_t* packet) {
    (void)_send_nack;
    assert(connection);
    if (IS_ACTIVE(CONFIG_PN53_DEBUG_HCI)) {
        PN53_DEBUG_HCI("[->] ");
        PN53_DEBUG_CHUNKS(packet);
        DEBUG("\n");
    }

    ssize_t res = 0;
    uint8_t overhead[PN53_FRAME_OVERHEAD_MAX];
    iolist_t iolist = {
        .iol_base = (void*)overhead,
        .iol_len = PN53_FRAME_OVERHEAD_MAX
    };
    static iolist_t suffix = {};

    if ((res = _frame_packet(&iolist, packet, &suffix, connection->max_packet_length)) < 0) {
        return res;
    }

    res = _write(connection, &iolist);
    while (packet->iol_next) {
        if (packet->iol_next == &suffix) {
            packet->iol_next = NULL;
            break;
        }
        packet = packet->iol_next;
    }
    return res;
}

static ssize_t _recv_packet(pn53_connection_t* connection, uint8_t** packet) {
    assert(connection);
    ssize_t res = 0;
    if ((res = _read(connection, connection->backing, PN53_FRAME_OVERHEAD_MIN, PN53_READ_INITIAL)) < 0) {
        return res;
    }
    /* We may read more than than just the header... */
    size_t already_read = res;

    uint8_t* cursor = connection->backing;
    if ((res = _parse_packet_begin(&cursor, res)) < 0) {
        return res;
    }
    /* Length of packet */
    size_t packet_length = res;
    /* This is the actual length of the frame header, must not exceed number of bytes read above. */
    size_t header_length = (size_t)(ptrdiff_t)(cursor - connection->backing);
    assert(header_length <= already_read);

    /* Still need to read packet and DCS 00. From that chunk we have already read a few bytes
     * during the _read call above. If the command in the packet is empty (which happens precisely
     * when we receive an error frame without a command and the TFI 0x7D as the only packet byte),
     * we have already read the complete frame. Because packet_length will always be >= 1 (at least
     * TFI) which is checked by _parse_packet_begin, this difference will always be 0 or greater.
     * This also ensures that _parse_packet_end will never read out of bounds when it reads
     * packet_length + 2 bytes. */
    size_t need_to_read = packet_length + 2 - (already_read - header_length);
    if (need_to_read > 0) {
        if ((res = _read(connection, connection->backing + already_read, need_to_read, PN53_READ_FINAL)) < 0) {
            return res;
        }
    }

    if ((res = _parse_packet_end(&cursor, packet_length)) < 0) {
        return res;
    }
    if (IS_ACTIVE(CONFIG_PN53_DEBUG_HCI)) {
        PN53_DEBUG_HCI("[<-] ");
        PN53_DEBUG_HEX(cursor, (size_t)res);
        DEBUG("\n");
    }

    if (packet) {
        *packet = cursor;
    }
    return res;
}

static ssize_t _block_with_timeout(pn53_connection_t* connection, uint32_t timeout_ms) {
    if (timeout_ms == PN53_TIMEOUT_NEVER) {
        sema_wait(&connection->trap);
        return 0;
    } else {
        assert(timeout_ms != 0);
        bool triggered = sema_wait_timed_ztimer(&connection->trap, ZTIMER_MSEC, timeout_ms) == -ETIMEDOUT;
        if (triggered) {
            PN53_DEBUG_HCI("timeout after %" PRIu32 " ms, aborting with ACK\n", timeout_ms);
            // Best effort, i.e., discard result
            ssize_t res = _send_ack(connection);
            if (res < 0) {
                PN53_DEBUG_HCI("ACK to abort failed with %i\n", (int)res);
            }
            return -ETIMEDOUT;
        } else {
            PN53_DEBUG_HCI("timeout not fired\n");
            return 0;
        }
    }
}

#if IS_USED(MODULE_PN53_UART)
static uint8_t _uart_buffer[128] = {0};
static uint8_t _uart_index = 0;

static void _uart_rx_cb(void* connection, uint8_t byte) {
    (void) connection;
    PN53_DEBUG_TRANSPORT("UART IRQ triggered with byte %02X\n", byte);
    _uart_buffer[_uart_index++] = byte;
    mutex_unlock(&((pn53_connection_t*)connection)->callback);
}
#endif

ssize_t pn53_hci_transceive(pn53_connection_t* connection, iolist_t* packet,
                            uint8_t** response, uint32_t timeout_ms) {
    ssize_t res = 0;
    uint32_t timestamp = 0;
    if (IS_ACTIVE(CONFIG_PN53_DEBUG_HCI_TIMING)) {
        timestamp = ztimer_now(ZTIMER_MSEC);
    }
    /* First, send packet. */
    if ((res = _send_packet(connection, packet)) < 0) {
        PN53_DEBUG_TRANSPORT("sending packet failed\n");
        return res;
    }
    if (IS_ACTIVE(CONFIG_PN53_DEBUG_HCI_TIMING)) {
        PN53_DEBUG_HCI("[->] %" PRIu32 " ms\n", ztimer_now(ZTIMER_MSEC) - timestamp);
        timestamp = ztimer_now(ZTIMER_MSEC);
    }

    /* Wait until data is available. */
    if ((res = _block_with_timeout(connection, CONFIG_PN53_ACK_TIMEOUT_MS)) < 0) {
        PN53_DEBUG_TRANSPORT("ACK timeout expired\n");
        return res;
    }

    if (IS_ACTIVE(CONFIG_PN53_DEBUG_HCI_TIMING)) {
        PN53_DEBUG_HCI("[<- IRQ] %" PRIu32 " ms\n", ztimer_now(ZTIMER_MSEC) - timestamp);
        timestamp = ztimer_now(ZTIMER_MSEC);
    }

    /* We expect an ACK from the controller. */
    if ((res = _recv_ack(connection)) < 0) {
        PN53_DEBUG_TRANSPORT("receiving ACK for packet failed\n");
        return res;
    }

    if (IS_ACTIVE(CONFIG_PN53_DEBUG_HCI_TIMING)) {
        PN53_DEBUG_HCI("[<- ACK] %" PRIu32 " ms\n", ztimer_now(ZTIMER_MSEC) - timestamp);
        timestamp = ztimer_now(ZTIMER_MSEC);
    }

    PN53_DEBUG_TRANSPORT("[<-] ACK\n");

    /* Wait until the response is available. */
    if ((res = _block_with_timeout(connection, timeout_ms)) < 0) {
        PN53_DEBUG_HCI("response timeout expired\n");
        return res;
    }

    if (IS_ACTIVE(CONFIG_PN53_DEBUG_HCI_TIMING)) {
        PN53_DEBUG_HCI("[<- IRQ] %" PRIu32 " ms\n", ztimer_now(ZTIMER_MSEC) - timestamp);
        timestamp = ztimer_now(ZTIMER_MSEC);
    }

    /* We expect an ACK from the controller. */
    if ((res = _recv_packet(connection, response)) < 0) {
        PN53_DEBUG_TRANSPORT("receiving response failed\n");
        return res;
    }

    if (IS_ACTIVE(CONFIG_PN53_DEBUG_HCI_TIMING)) {
        PN53_DEBUG_HCI("[<-] %" PRIu32 " ms\n", ztimer_now(ZTIMER_MSEC) - timestamp);
    }

    PN53_DEBUG_TRANSPORT("round trip complete\n");
    return res;
}
