/*
 * Copyright (C) 2016 TriaGnoSys GmbH
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>

#include "kernel_defines.h"
#include "iolist.h"
#include "time_units.h"
#include "mutex.h"
#include "periph/i2c.h"
#include "periph/spi.h"
#include "periph/uart.h"
#include "periph/gpio.h"
#include "net/nfc/mifare/mifare_classic.h"

#include "net/nfc.h"
#include "net/nfcdev.h"

/**
 * @defgroup drivers_pn53x `PN53*` NFC Controller family
 * @ingroup drivers_netdev
 *
 * @brief  Common driver for `PN53*` family of NFC controllers (PN531, PN532, PN533)
 * @{
 */

/**
 * @file
 * @brief Common `PN53*` header
 */


#define PN53_PACKET_LENGTH_MAX  (265)

#define PN53_FRAME_OVERHEAD_MIN (8)
#define PN53_FRAME_OVERHEAD_MIN_COMMAND (PN53_FRAME_OVERHEAD_MIN + 1)

#define PN53_FRAME_OVERHEAD_MAX (11)
#define PN53_FRAME_OVERHEAD_MAX_COMMAND (PN53_FRAME_OVERHEAD_MAX + 1)

/**
 * @brief Frame header length (normal information frame)
 *
 * `00 00 FF LEN LCS TFI`
 */
#define PN35_FRAME_HEADER_NORMAL (6)
#define PN35_FRAME_HEADER_NORMAL_COMMAND (PN35_FRAME_HEADER_NORMAL + 1)
static_assert(PN35_FRAME_HEADER_NORMAL + 2 == PN53_FRAME_OVERHEAD_MIN);

/**
 * @brief Frame header length (extended information frame)
 *
 * `00 00 FF FF FF LEN LEN LCS TFI`
 */
#define PN35_FRAME_HEADER_EXTENDED (9)
#define PN35_FRAME_HEADER_EXTENDED_COMMAND (PN35_FRAME_HEADER_EXTENDED + 1)
static_assert(PN35_FRAME_HEADER_EXTENDED + 2 == PN53_FRAME_OVERHEAD_MAX);


#if !defined(CONFIG_PN53_FRAME_LENGTH_MAX) || defined(DOXYGEN)
#  define CONFIG_PN53_FRAME_LENGTH_MAX (PN53_FRAME_OVERHEAD_MAX + PN53_PACKET_LENGTH_MAX)
#endif

/**
 * @brief Timeout in milliseconds the driver waits until the `PN53*` controller sends an ACK
 * for a packet previously sent to the controller.
 *
 * **Default**: 500 milliseconds
 */
#if !defined(CONFIG_PN53_ACK_TIMEOUT_MS) || defined(DOXYGEN)
#  define CONFIG_PN53_ACK_TIMEOUT_MS 500
#endif

#if !defined(CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS) || defined(DOXYGEN)
#  define CONFIG_PN53_COMMAND_TIMEOUT_DEFAULT_MS (2 * MS_PER_SEC)
#endif

#if !defined(CONFIG_PN53_SELFCHECK) || defined(DOXYGEN)
#  define CONFIG_PN53_SELFCHECK 1
#endif

#if !defined(CONFIG_PN53_DEBUG) || defined(DOXYGEN)
#  define CONFIG_PN53_DEBUG 1
#endif

#if !defined(CONFIG_PN53_DEBUG_REGISTERS) || defined(DOXYGEN)
#  define CONFIG_PN53_DEBUG_REGISTERS 1
#endif

#if !defined(CONFIG_PN53_DEBUG_HCI) || defined(DOXYGEN)
#  define CONFIG_PN53_DEBUG_HCI 1
#endif

#if !defined(CONFIG_PN53_DEBUG_TRANSPORT) || defined(DOXYGEN)
#  define CONFIG_PN53_DEBUG_TRANSPORT 0
#endif

#if !defined(CONFIG_PN53_FIFO_TRANSMIT_REFILL_THRESHOLD) || defined(DOXYGEN)
#  define CONFIG_PN53_FIFO_TRANSMIT_REFILL_THRESHOLD 5
#endif

#if !defined(CONFIG_PN53_FIFO_SIZE) || defined(DOXYGEN)
#  define CONFIG_PN53_FIFO_SIZE 64
#endif

/// @brief Use controller's CIU FIFO instead of `InCommunicateThru` command to send data in
///
/// Depending on the HCI transport (SPI, I²C, or UART) the driver may not be able
/// to refill the FIFO fast enough if the application wants to send more than 64 bytes,
/// which is the size of the CIU's FIFO.
///
/// This applies to @ref nfcdev_transceive and @ref nfcdev_transceive_bytes
///
/// If you enable this, you must pass a reference to an allocated `rx` buffer to
/// @ref nfcdev_transceive and @ref nfcdev_transceive_bytes so that the driver can copy
/// data into this buffer while it reads received bytes from the CIU's FIFO.
#if !defined(CONFIG_PN53_INITIATOR_TRANSCEIVE_USING_FIFO) || defined(DOXYGEN)
#  define CONFIG_PN53_INITIATOR_TRANSCEIVE_USING_FIFO 0
#endif


typedef enum {
    PN53_MODEL_PN531,
    PN53_MODEL_PN532,
    PN53_MODEL_PN533,
    PN53_MODEL_RCS956,
} pn53_model_t;

typedef enum {
    PN53_BUS_I2C = 0,
    PN53_BUS_SPI,
    PN53_BUS_UART
} pn53_bus_kind_t;


typedef struct {
    pn53_bus_kind_t kind;
    union {
        i2c_t i2c;
        spi_t spi;
        uart_t uart;
    };
} pn53_bus_t;

typedef struct {
    pn53_bus_t bus;
    gpio_t reset;
    gpio_t irq;
#if IS_USED(MODULE_PN53X_SPI) || defined(DOXYGEN)
    gpio_t chip_select;
#endif
} pn53_connection_config_t;

typedef struct {
    const pn53_connection_config_t* config;
    mutex_t trap;
#if IS_USED(MODULE_PN53X_UART) || defined(DOXYGEN)
    mutex_t callback;
#endif
    uint8_t backing[CONFIG_PN53_FRAME_LENGTH_MAX];
    size_t max_packet_length;
} pn53_connection_t;

typedef struct {
} pn53_parameters_t;

typedef enum {
    PN53_MANAGED_TRANSPORT_NONE = 0,
    PN53_MANAGED_TRANSPORT_ISO_DEP = NFCDEV_INTERFACE_ISO_DEP,
    PN53_MANAGED_TRANSPORT_NFC_DEP = NFCDEV_INTERFACE_NFC_DEP,
} pn53_managed_target_transport_t;

typedef struct {
    nfc_target_t super;
    pn53_managed_target_transport_t managed_transport;
} pn53_logical_target_t;

typedef struct {
    pn53_connection_t connection;
    const pn53_parameters_t* parameters;
    pn53_model_t model;
    pn53_logical_target_t nfc_targets[2];
    uint32_t command_timeout;
    nfc_role_t nfc_role;
    nfcdev_connection_id_t nfc_current_connection;
    uint8_t nfc_parameters;
    uint8_t bit_framing;
    uint8_t tx_mode;
    uint8_t rx_mode;
    uint8_t manual_receiver;
} pn53_dev_t;

#ifndef DOXYGEN
static inline pn53_logical_target_t* pn53_current_target(pn53_dev_t* dev) {
    assert(dev->nfc_current_connection < ARRAY_SIZE(dev->nfc_targets));
    return dev->nfc_targets[dev->nfc_current_connection].super.parameters.polling.bitrate == NFC_BITRATE_UNSET
        ? NULL : &dev->nfc_targets[dev->nfc_current_connection];
}
#endif

#ifndef DOXYGEN
#  define PN53_ERRNO(code) (53000 + code)
#  define PN53_ERRNO_STATUS_BASE 53500
#endif

typedef enum {
    PN53_ERROR_CONNECTION_TEST_FAILED                   = PN53_ERRNO(1),
    PN53_ERROR_CONNECTION_CHECKSUM_MISMATCH             = PN53_ERRNO(2),
    PN53_ERROR_CONNECTION_CORRUPTED                     = PN53_ERRNO(3),
    PN53_ERROR_CONNECTION_PACKET_TRUNCATED              = PN53_ERRNO(4),
    PN53_ERROR_CONNECTION_PACKET_LENGTH_OUT_OF_RANGE    = PN53_ERRNO(5),
    PN53_ERROR_CONNECTION_UNEXPECTED_FRAME_DIRECTION    = PN53_ERRNO(6),
    PN53_ERROR_CONNECTION_PACKET_EMPTY                  = PN53_ERRNO(7),
    PN53_ERROR_CONNECTION_RESPONSE_MISSING              = PN53_ERRNO(8),
    PN53_ERROR_CONNECTION_RESPONSE_MISMATCH             = PN53_ERRNO(9),
    PN53_ERROR_CONNECTION_TIMEOUT                       = PN53_ERRNO(10),
    PN53_ERROR_CONNECTION_ACK                           = PN53_ERRNO(11),
    PN53_ERROR_CONNECTION_NACK                          = PN53_ERRNO(12),
    PN53_ERROR_CONNECTION_FRAME_SYNTAX                  = PN53_ERRNO(13),
} pn53_hci_error_t;

ssize_t pn53_hci_transceive(pn53_connection_t* connection, iolist_t* packet,
                            uint8_t** response, uint32_t timeout_ms);

ssize_t pn53_hci_transceive_command(pn53_connection_t* connection, iolist_t* command,
                                    uint8_t** response, uint32_t timeout_ms);

static inline ssize_t pn53_hci_transceive_command2(pn53_connection_t* connection, uint8_t* command, size_t length, uint8_t** response, uint32_t timeout_ms) {
    iolist_t iolist = { .iol_base = (void*)command, .iol_len = length };
    return pn53_hci_transceive_command(connection, &iolist, response, timeout_ms);
}

int pn53_hci_init(pn53_connection_t* connection);
void pn53_hci_reset(const pn53_connection_t* connection);

int pn53_init(pn53_dev_t* dev, const pn53_connection_config_t* config);

typedef enum __attribute__((packed)) {
    PN53_COMMAND_DIAGNOSE                       = 0x00,
    PN53_COMMAND_GET_FIRMWARE_VERSION           = 0x02,
    PN53_COMMAND_GET_GENERAL_STATUS             = 0x04,
    PN53_COMMAND_READ_REGISTERS                 = 0x06,
    PN53_COMMAND_WRITE_REGISTERS                = 0x08,
    PN53_COMMAND_READ_GPIO                      = 0x0C,
    PN53_COMMAND_WRITE_GPIO                     = 0x0E,
    PN53_COMMAND_SET_PARAMETERS                 = 0x12,
    PN53_COMMAND_RF_CONFIGURATION               = 0x32,
    PN53_COMMAND_IN_DATA_EXCHANGE               = 0x40,
    PN53_COMMAND_IN_COMMUNICATE_THRU            = 0x42,
    PN53_COMMAND_IN_DESELECT                    = 0x44,
    PN53_COMMAND_IN_JUMP_FOR_PSL                = 0x46,
    PN53_COMMAND_IN_LIST_PASSIVE_TARGET         = 0x4A,
    PN53_COMMAND_IN_PSL                         = 0x4E,
    PN53_COMMAND_IN_ATR                         = 0x50,
    PN53_COMMAND_IN_RELEASE                     = 0x52,
    PN53_COMMAND_IN_SELECT                      = 0x54,
    PN53_COMMAND_IN_JUMP_FOR_DEP                = 0x56,
    PN53_COMMAND_RF_REGULATION_TEST             = 0x58,
    PN53_COMMAND_IN_AUTO_POLL                   = 0x60,
    PN53_COMMAND_TG_GET_DATA                    = 0x86,
    PN53_COMMAND_TG_GET_INITIATOR_COMMAND       = 0x88,
    PN53_COMMAND_TG_GET_TARGET_STATUS           = 0x8A,
    PN53_COMMAND_TG_INIT_AS_TARGET              = 0x8C,
    PN53_COMMAND_TG_SET_DATA                    = 0x8E,
} pn53_command_code_t;

typedef struct __attribute__((packed)) {
    uint8_t ic;
    uint8_t version;
    uint8_t revision;
    uint8_t support;
} pn53_firmware_version_t;

#ifndef DOXYGEN
#  define _PN53_GET_BIT(field, ix) (((field) & (1 << ix)) >> ix)
#endif

#define PN53_FIRMWARE_SUPPORTS_NFC_A(support) _PN53_GET_BIT(support, 0)
#define PN53_FIRMWARE_SUPPORTS_NFC_B(support) _PN53_GET_BIT(support, 1)
#define PN53_FIRMWARE_SUPPORTS_NFC_DEP(support) _PN53_GET_BIT(support, 2)

int pn53_get_firmware_version(pn53_dev_t* dev, pn53_firmware_version_t** version);

typedef enum {
    PN53_SAM_NORMAL = 1,
    PN53_SAM_VIRTUAL,
    PN53_SAM_WIRED,
    PN53_SAM_DUAL,
} pn53_sam_mode_t;

typedef union __attribute__((packed)) {
    struct {
        /// When bit 0 is set to 1, a full negative pulse has been detected on the CLAD line.
        bool full_negative_pulse_detected : 1;

        /// When bit 1 is set to 1, an external RF field has been detected and switched off during or
        /// after a transaction.
        bool rf_field_off_detected : 1;

        /// When bit 2 is set to 1, a timeout has been detected after SigActIRQ has felt down.
        bool timeout_detected : 1;

        uint8_t _rfu : 4;

        /// When bit 7 is set to 1, the CLAD line is high level whereas when this bit is set to 0, the CLAD line is low level.
        /// _Warning_: When the SAM is not powered, bit 7 is not significant. In other words, for example when the
        /// PN532 is configured in virtual card mode, and if no external RF field is detected, this bit will be read as high,
        /// whatever the real level of the input.
        bool clad_state : 1;
    } __attribute__((packed)) ;
    uint8_t raw;
} pn53_sam_status_t;

typedef enum __attribute__((packed)) {
    PN53_STATUS_SUCCESS = 0,

    ///  Time Out, the target has not answered
    PN53_STATUS_ERROR_TARGET_ANSWER_TIMED_OUT = 0x1,

    /// A CRC error has been detected by the CIU
    PN53_STATUS_ERROR_CRC = 0x2,

    /// A Parity error has been detected by the CIU
    PN53_STATUS_ERROR_PARITY = 0x3,

    /// During an anti-collision/select operation (ISO/IEC14443-3 Type A
    /// and ISO/IEC18092 106 kbps passive mode), an erroneous Bit Count
    /// has been detected
    PN53_STATUS_ERROR_BIT_COUNT_INVALID = 0x4,

    /// Framing error during Mifare operation
    PN53_STATUS_ERROR_MIFARE_FRAMING = 0x5,

    /// An abnormal bit-collision has been detected during bit wise anti-collision
    /// at 106 kbps
    PN53_STATUS_ERROR_BIT_COLLISION = 0x6,

    /// Communication buffer size insufficient
    PN53_STATUS_ERROR_COMMS_BUFFER_OVERFLOW = 0x7,

    ///  RF Buffer overflow has been detected by the CIU (bit BufferOvfl of
    ///  the register CIU_Error)
    PN53_STATUS_ERROR_RF_BUFFER_OVERFLOW = 0x9,

    /// In active communication mode, the RF field has not been switched
    /// on in time by the counterpart (as defined in NFCIP-1 standard)
    PN53_STATUS_ERROR_RF_TIMEOUT = 0xa,

    /// RF Protocol error (cf. Error! Reference source not found., description of the
    /// CIU_Error register)
    PN53_STATUS_ERROR_RF_PROTOCOL = 0xb,

    ///  Temperature error: the internal temperature sensor has detected overheating,
    ///  and therefore has automatically switched off the antenna drivers
    PN53_STATUS_ERROR_OVERHEATING = 0xd,

    /// Internal buffer overflow
    PN53_STATUS_ERROR_INTERNAL_BUFFER_OVERFLOW = 0xe,

    /// Invalid parameter (range, format, ...)
    PN53_STATUS_ERROR_INVALID_PARAMETER = 0x10,

    /// DEP Protocol: The PN532 configured in target mode does not support the command
    /// received from the initiator (the command received is not one of the following:
    /// `ATR_REQ, WUP_REQ, PSL_REQ, DEP_REQ, DSL_REQ, RLS_REQ`
    PN53_STATUS_ERROR_NFC_DEP_UNKNOWN_COMMMAND = 0x12,

    /// DEP Protocol, Mifare or ISO/IEC14443-4: The data format does not match to the specification.
    /// Depending on the RF protocol used, it can be:
    /// - BadlengthofRFreceivedframe,
    /// - Incorrect value of PCB or PFB,
    /// - Invalid or unexpected RF received frame,
    /// - NAD or DID incoherence.
    PN53_STATUS_ERROR_INVALID_RX_FRAME = 0x13,

    /// Mifare: Authentication error
    PN53_STATUS_ERROR_MIFARE_AUTH = 0x14,

    PN53_STATUS_ERROR_NFC_SECURE_UNSUPPORTED = 0x18,

    PN53_STATUS_ERROR_I2C_BUSY_TDA = 0x19,

    /// ISO/IEC14443-3: UID Check byte is wrong
    PN53_STATUS_ERROR_UID_CHECK_BYTE_INVALID = 0x23,

    /// DEP Protocol: Invalid device state, the system is in a state which does not allow the operation
    PN53_STATUS_ERROR_NFC_DEP_INVALID_STATE = 0x25,

    /// HCI Operation not allowed in this configuration (host controller interface)
    PN53_STATUS_ERROR_OPERATION_NOT_ALLOWED = 0x26,

    /// This command is not acceptable due to the current context of the PN532
    /// (Initiator vs. Target, unknown target number, Target not in the good state, ...)
    PN53_STATUS_ERROR_COMMAND_NOT_ACCEPTABLE = 0x27,

    /// The PN532 configured as target has been released by its initiator
    PN53_STATUS_ERROR_TARGET_RELEASED = 0x29,

    /// PN532 and ISO/IEC14443-3B only: the ID of the card does not match,
    /// meaning that the expected card has been exchanged with another one.
    PN53_STATUS_ERROR_TARGET_ID_MISMATCH = 0x2a,

    /// PN532 and ISO/IEC14443-3B only: the card previously activated has disappeared.
    PN53_STATUS_ERROR_TARGET_DISAPPEARED = 0x2b,

    /// Mismatch between the NFCID3 initiator and the NFCID3 target in DEP 212/424 kbps passive.
    PN53_STATUS_ERROR_NFC_DEF_NFC_F_ID_MISMATCH = 0x2c,

    /// An over-current event has been detected
    PN53_STATUS_ERROR_OVERCURRENT = 0x2d,

    /// NAD missing in DEP frame
    PN53_STATUS_ERROR_NFC_DEP_MISSING_NAD = 0x2e,
} pn53_status_code_t;

#define PN53_ERRNO_FROM_STATUS_CODE(code) (53700 + (code))
#define PN53_ERRNO_IS_STATUS_CODE(_errno0) ((_errno0) > 53700 && (_errno0) < 53999)
#define PN53_ERRNO_TO_STATUS_CODE(_errno0) (pn53_status_code_t)((_errno0) - 53700)

static inline bool pn53_status_nad_present(uint8_t status) {
    return (status & 0x80) != 0;
}

static inline bool pn53_status_more_information_available(uint8_t status) {
    return (status & 0x40) != 0;
}

static inline pn53_status_code_t pn53_status_code(uint8_t status) {
    return status & 0x3f;
}

typedef uint16_t pn53_register_address_t;

typedef struct __attribute__((packed)) {
    pn53_register_address_t address;
    uint8_t value;
} pn53_register_t;

ssize_t pn53_read_registers_(pn53_dev_t *dev, void* registers, uint8_t** values, size_t count);
static inline ssize_t pn53_read_registers(pn53_dev_t *dev, pn53_register_address_t* registers, uint8_t** values, size_t count) {
    return pn53_read_registers_(dev, registers, values, count);
}

int pn53_write_registers_(pn53_dev_t *dev, void* registers, size_t count);
static inline int pn53_write_registers(pn53_dev_t *dev, pn53_register_t* registers, size_t count) {
    return pn53_write_registers_(dev, registers, count);
}

int pn53_set_parameters(pn53_dev_t* dev, uint8_t parameters);

static inline int pn53_enable_parameters(pn53_dev_t* dev, uint8_t parameters) {
    dev->nfc_parameters |= parameters;
    return pn53_set_parameters(dev, dev->nfc_parameters);
}

static inline int pn53_disable_parameters(pn53_dev_t* dev, uint8_t parameters) {
    dev->nfc_parameters &= ~parameters;
    return pn53_set_parameters(dev, dev->nfc_parameters);
}

typedef enum __attribute__((packed)) {
    PN53_TECHNOLOGY_A_106K = 0,
    /// Topaz/Jewel
    PN53_TECHNOLOGY_A_106K_GEMSTONE = 4,
    PN53_TECHNOLOGY_B_106K = 3,
    PN53_TECHNOLOGY_B_212K = 6,
    PN53_TECHNOLOGY_B_424K = 7,
    PN53_TECHNOLOGY_B_848K = 8,
    PN53_TECHNOLOGY_F_212K = 1,
    PN53_TECHNOLOGY_F_424K = 2,
} pn53_technology_baudrate_t;

typedef enum __attribute__((packed)) {
    PN53_UART_SPEED_9600_BAUD = 0x00, /* 9.6 kbaud */
    PN53_UART_SPEED_19200_BAUD = 0x01, /* 19.2 kbaud */
    PN53_UART_SPEED_38400_BAUD = 0x02, /* 38.4 kbaud */
    PN53_UART_SPEED_57600_BAUD = 0x03, /* 57.6 kbaud */
    PN53_UART_SPEED_115200_BAUD = 0x04, /* 115.2 kbaud */
    PN53_UART_SPEED_230400_BAUD = 0x05, /* 230.4 kbaud */
    PN53_UART_SPEED_460800_BAUD = 0x06, /* 460.8 kbaud */
    PN53_UART_SPEED_921600_BAUD = 0x07, /* 921.6 kbaud */
    PN53_UART_SPEED_1288_KBAUD = 0x08, /* 1.288 Mbaud */
} pn53_uart_speed_t;

#define PN53_TIMEOUT_FIELD_FROM_MS(ms) (uint8_t)((ms) / 50)
#define PN53_TIMEOUT_FIELD_TO_MS(timeout) ((timeout) * 50)

// Register addresses

#define PN53_REGISTER(addr) htobe16(addr)

#define PN53_REGISTER_CONTROL_SWITCH_RNG PN53_REGISTER(0x6106)

/// Defines general modes for transmitting and receiving
#define PN53_REGISTER_MODE PN53_REGISTER(0x6301)

/// Defines the data rate and framing during transmission.
#define PN53_REGISTER_TX_MODE PN53_REGISTER(0x6302)

#define PN53_REGISTER_TX_MODE_AUTO_CRC (0x80)
#define PN53_REGISTER_TX_MODE_BITRATE_INDEX (0x70)
#define PN53_REGISTER_TX_MODE_INVERTED (0x08)
#define PN53_REGISTER_TX_MODE_MIX (0x04)
#define PN53_REGISTER_TX_MODE_FRAMING (0x03)

/// Defines the data rate and framing during reception.
#define PN53_REGISTER_RX_MODE PN53_REGISTER(0x6303)

#define PN53_REGISTER_RX_MODE_AUTO_CRC (0x80)
#define PN53_REGISTER_RX_MODE_BITRATE_INDEX (0x70)
#define PN53_REGISTER_RX_MODE_IGNORE_INVALID (0x08)
#define PN53_REGISTER_RX_MODE_MULTIPLE_FRAMES (0x04)
#define PN53_REGISTER_RX_MODE_FRAMING (0x03)

#define PN53_REGISTER_TXRX_MODE_AUTO_CRC (0x80)
#define PN53_REGISTER_TXRX_MODE_BITRATE_INDEX (0x70)
#define PN53_REGISTER_TXRX_MODE_FRAMING (0x03)

/// Controls the logical behaviour of the antenna driver pins TX1 and TX2
#define PN53_REGISTER_TX_CONTROL PN53_REGISTER(0x6304)

/// Controls the settings of the antenna driver
#define PN53_REGISTER_TX_AUTO PN53_REGISTER(0x6305)

#define PN53_REGISTER_TX_AUTO_FLAG_TURN_OFF_FIELD_AFTER_TX (0x80)
#define PN53_REGISTER_TX_AUTO_FLAG_FORCE_100_PERCENT_ASK (0x40)
#define PN53_REGISTER_TX_AUTO_FLAG_WAKE_UP_BY_FIELD (0x20)
#define PN53_REGISTER_TX_AUTO_FLAG_COLLISION_AVOIDANCE (0x08)
#define PN53_REGISTER_TX_AUTO_FLAG_INITIAL_FIELD_ON (0x04)
#define PN53_REGISTER_TX_AUTO_FLAG_TX1_FIELD_ON (0x02)
#define PN53_REGISTER_TX_AUTO_FLAG_TX2_FIELD_ON (0x01)

/// Selects the internal sources for the antenna driver
#define PN53_REGISTER_TX_SELECTOR PN53_REGISTER(0x6306)

/// Selects internal receiver settings
#define PN53_REGISTER_RX_SELECTOR PN53_REGISTER(0x6307)

/// Selects thresholds for the bit decoder
#define PN53_REGISTER_RX_THRESHOLD PN53_REGISTER(0x6308)

/// Defines demodulator settings
#define PN53_REGISTER_DEMODULATOR PN53_REGISTER(0x6309)

/// Defines the length of the valid range for the received frame
#define PN53_REGISTER_NFC_F_1 PN53_REGISTER(0x630A)

/// Defines the length of the valid range for the received frame
#define PN53_REGISTER_NFC_F_2 PN53_REGISTER(0x630B)

// Controls the communication in NFC-A (+ NFC-A MIFARE) and NFC target mode at 106 kbit/s
#define PN53_REGISTER_NFC_A PN53_REGISTER(0x630C)

/// Allows manual fine tuning of the internal receiver.
#define PN53_REGISTER_MANUAL_RECEIVER PN53_REGISTER(0x630D)

#define PN53_REGISTER_MANUAL_RECEIVER_FLAG_TX_RX_MANUAL_PARITY (0x10)

/// Configure NFC-B
#define PN53_REGISTER_NFC_B PN53_REGISTER(0x630E)

#define PN53_REGISTER_NFC_B_FLAG_RX_REQUIRE_SOF (0x80)
#define PN53_REGISTER_NFC_B_FLAG_RX_REQUIRE_EOF (0x40)
#define PN53_REGISTER_NFC_B_FLAG_EOF_SOF_WIDTH_MAX (0x08)
#define PN53_REGISTER_NFC_B_FLAG_TX_NO_SOF (0x02)
#define PN53_REGISTER_NFC_B_FLAG_TX_NO_EOF (0x01)

// #define PN53_REGISTER_- 0x630F
// #define PN53_REGISTER_- 0x6310

/// Shows the actual MSB values of the CRC calculation
#define PN53_REGISTER_CRC_HIGH PN53_REGISTER(0x6311)

/// Shows the actual LSB values of the CRC calculation
#define PN53_REGISTER_CRC_LOW PN53_REGISTER(0x6312)

/// Controls the setting of the width of the Miller pause
#define PN53_REGISTER_MILLER_MODULATION_WIDTH PN53_REGISTER(0x6314)
#define PN53_REGISTER_ModWidth PN53_REGISTER_MILLER_MODULATION_WIDTH

/// Bit synchronization at 106 kbit/s
#define PN53_REGISTER_TX_BIT_PHASE PN53_REGISTER(0x6315)

/// Configures the receiver gain and RF level detector sensitivity.
#define PN53_REGISTER_RF_CONFIG PN53_REGISTER(0x6316)

/// Selects the conductance for the N-driver of the antenna driver pins TX1 and TX2 when the driver is switched off.
#define PN53_REGISTER_CONDUCTANCE_N_DRIVER_OFF PN53_REGISTER(0x6313)
#define PN53_REGISTER_GsNOFF PN53_REGISTER_CONDUCTANCE_N_DRIVER_OFF

/// Selects the conductance for the N-driver of the antenna driver pins TX1 and TX2 when the driver is switched on.
#define PN53_REGISTER_CONDUCTANCE_N_DRIVER_ON PN53_REGISTER(0x6317)
#define PN53_REGISTER_GsNOn PN53_REGISTER_CONDUCTANCE_N_DRIVER_ON

/// Defines the conductance of the P-driver during times of no modulation.
#define PN53_REGISTER_CONDUCTANCE_P_DRIVER_NO_MODULATION PN53_REGISTER(0x6318)
#define PN53_REGISTER_CWGsP PN53_REGISTER_CONDUCTANCE_P_DRIVER_NO_MODULATION

/// Defines the driver P-output conductance during modulation.
#define PN53_REGISTER_CONDUCTANCE_P_DRIVER_MODULATION PN53_REGISTER(0x6319)
#define PN53_REGISTER_ModGsP PN53_REGISTER_CONDUCTANCE_P_DRIVER_MODULATION

/// Defines settings for the internal timer
#define PN53_REGISTER_TIMER_MODE PN53_REGISTER(0x631A)

/// Defines settings for the internal timer
#define PN53_REGISTER_TIMER_PRESCALER PN53_REGISTER(0x631B)

/// Describes the MSB of the 16-bit long timer reload value.
#define PN53_REGISTER_TIMER_RELOAD_VALUE_HIGH PN53_REGISTER(0x631C)

/// Describes the LSB of the 16-bit long timer reload value.
#define PN53_REGISTER_TIMER_RELOAD_VALUE_LOW PN53_REGISTER(0x631D)

/// Describes the 16-bit long timer actual value (Higher 8 bits)
#define PN53_REGISTER_TIMER_COUNTER_VALUE_HIGH PN53_REGISTER(0x631E)

/// Describes the 16-bit long timer actual value (Lower 8 bits)
#define PN53_REGISTER_TIMER_COUNTER_VALUE_LOW PN53_REGISTER(0x631F)

// #define PN53_REGISTER_- 0x6320

/// General test signals configuration
#define PN53_REGISTER_TEST_SELECTION1 PN53_REGISTER(0x6321)

/// General test signals configuration and PRBS control
#define PN53_REGISTER_TEST_SELECTION2 PN53_REGISTER(0x6322)

/// Enables test signals output on pins.
#define PN53_REGISTER_TEST_PIN_ENABLE PN53_REGISTER(0x6323)

/// Defines the values for the 8-bit parallel bus when it is used as I/O bus
#define PN53_REGISTER_TEST_PIN_VALUE PN53_REGISTER(0x6324)

/// Shows the status of the internal test bus
#define PN53_REGISTER_TEST_BUS PN53_REGISTER(0x6325)

/// Controls the digital self-test
#define PN53_REGISTER_TEST_AUTO PN53_REGISTER(0x6326)

/// Shows the CIU version
#define PN53_REGISTER_VERSION PN53_REGISTER(0x6327)

/// Controls the pins AUX1 and AUX2
#define PN53_REGISTER_TEST_ANALOG PN53_REGISTER(0x6328)

/// Defines the test value for the TestDAC1
#define PN53_REGISTER_TEST_DAC1 PN53_REGISTER(0x6329)

/// Defines the test value for the TestDAC2
#define PN53_REGISTER_TEST_DAC2 PN53_REGISTER(0x632A)

/// Show the actual value of ADC I and Q
#define PN53_REGISTER_TEST_ADCV PN53_REGISTER(0x632B)

// #define PN53_REGISTER_- 0x632C
// #define PN53_REGISTER_- 0x632D
// #define PN53_REGISTER_- 0x632E

/// Power down of the RF level detector
#define PN53_REGISTER_RF_LEVEL_DETECTOR PN53_REGISTER(0x632F)

/// Enables the use of secure IC clock on P34 / SIC_CLK
#define PN53_REGISTER_SECURE_IC_CLOCK PN53_REGISTER(0x6330)

/// Starts and stops the command execution
#define PN53_REGISTER_COMMAND PN53_REGISTER(0x6331)

#define PN53_REGISTER_COMMAND_MASK_COMMAND (0x0f)
#define PN53_REGISTER_COMMAND_FLAG_POWER_DOWN (0x10)
#define PN53_REGISTER_COMMAND_FLAG_RECV_OFF (0x20)

/// Control bits to enable and disable the passing of interrupt requests
#define PN53_REGISTER_COMMON_INTERRUPT_ENABLE PN53_REGISTER(0x6332)

/// Control bits to enable and disable the passing of interrupt requests
#define PN53_REGISTER_DIVERSE_INTERRUPT_ENABLE PN53_REGISTER(0x6333)

/// Contains common CIU interrupt request flags
#define PN53_REGISTER_COMMON_IRQ PN53_REGISTER(0x6334)

#define PN53_REGISTER_COMMON_IRQ_FLAG_TX_FINISHED (0x40)
#define PN53_REGISTER_COMMON_IRQ_FLAG_RX_FINISHED (0x20)
#define PN53_REGISTER_COMMON_IRQ_FLAG_IDLE        (0x20)
#define PN53_REGISTER_COMMON_IRQ_FLAG_HI_ALERT    (0x08)
#define PN53_REGISTER_COMMON_IRQ_FLAG_LO_ALERT    (0x04)
#define PN53_REGISTER_COMMON_IRQ_FLAG_ERROR       (0x02)
#define PN53_REGISTER_COMMON_IRQ_FLAG_TIMER       (0x01)

/// Contains miscellaneous interrupt request flags
#define PN53_REGISTER_DIVERSE_IRQ PN53_REGISTER(0x6335)

/// Error flags showing the error status of the last command executed
#define PN53_REGISTER_ERROR PN53_REGISTER(0x6336)

/// Contains status flags of the CRC, Interrupt Request System and FIFO buffer
#define PN53_REGISTER_STATUS1 PN53_REGISTER(0x6337)

/// Contain status flags of the receiver, transmitter and Data Mode Detector
#define PN53_REGISTER_STATUS2 PN53_REGISTER(0x6338)

#define PN53_REGISTER_STATUS2_FLAG_CRYPTO1_ENABLED (0x08)

/// In- and output of 64 byte FIFO buffer
#define PN53_REGISTER_FIFO_DATA PN53_REGISTER(0x6339)

/// Indicates the number of bytes stored in the FIFO
#define PN53_REGISTER_FIFO_LEVEL PN53_REGISTER(0x633A)

#define PN53_REGISTER_FIFO_LEVEL_MASK_BYTE_COUNT (0x7f)
#define PN53_REGISTER_FIFO_LEVEL_FLAG_FLUSH (0x80)

/// Defines the thresholds for FIFO under- and overflow warning
#define PN53_REGISTER_FIFO_WATER_LEVEL PN53_REGISTER(0x633B)

/// Contains miscellaneous control bits
#define PN53_REGISTER_CONTROL PN53_REGISTER(0x633C)

#define PN53_REGISTER_CONTROL_FLAG_TIMER_STOP (0x80)
#define PN53_REGISTER_CONTROL_FLAG_TIMER_START (0x40)
#define PN53_REGISTER_CONTROL_FLAG_COPY_NFC_DEP_ID_TO_FIFO (0x20)
#define PN53_REGISTER_CONTROL_FLAG_INITIATOR (0x10)
#define PN53_REGISTER_CONTROL_MASK_RX_TRAILING_BIT_COUNT (0x07)

/// Adjustments for bit oriented frames
#define PN53_REGISTER_BIT_FRAMING PN53_REGISTER(0x633D)

#define PN53_REGISTER_BIT_FRAMING_FLAG_START_SEND (0x80)
#define PN53_REGISTER_BIT_FRAMING_MASK_RX_BIT_OFFSET (0x70)
#define PN53_REGISTER_BIT_FRAMING_MASK_TX_TRAILING_BIT_COUNT (0x07)

/// Defines the first bit collision detected on the RF interface
#define PN53_REGISTER_COLLISION PN53_REGISTER(0x633E)

#define PN53_REGISTER_SFR_P3 PN53_REGISTER(0xFFB0)
#define PN53_REGISTER_SFR_P3CFGA PN53_REGISTER(0xFFFC)
#define PN53_REGISTER_SFR_P3CFGB PN53_REGISTER(0xFFFD)
#define PN53_REGISTER_SFR_P7CFGA PN53_REGISTER(0xFFF4)
#define PN53_REGISTER_SFR_P7CFGB PN53_REGISTER(0xFFF5)
#define PN53_REGISTER_SFR_P7 PN53_REGISTER(0xFFF7)

typedef uint32_t pn53_register_symbol_t;


#ifndef DOXYGEN

#  define PN53_REGISTER_SYMBOL_(register, mask) (((mask) << 16) | (register))

#  define PN53_REGISTER_SYMBOL_MASK(symbol) ((symbol) >> 16)
#  define PN53_REGISTER_SYMBOL_REGISTER(symbol) ((symbol) & 0xffff)

#  define PN53_REGISTER_SYMBOL(register_name, mask) \
    PN53_REGISTER_SYMBOL_(PN53_REGISTER_ ## register_name, mask)

#  define PN53_REGISTER_SYMBOL_REGISTER_CAPACITY (PN53_REGISTER_COLLISION - PN53_REGISTER_MODE)
#  define PN53_SYMBOLS_START_REGISTER PN53_REGISTER_MODE

#endif

typedef struct {
    uint8_t register_values[PN53_REGISTER_SYMBOL_REGISTER_CAPACITY];
    uint8_t change_masks[PN53_REGISTER_SYMBOL_REGISTER_CAPACITY];
} pn53_register_symbols_t;

int pn53_register_symbol_set(pn53_register_symbols_t* symbols, pn53_register_symbol_t symbol, uint8_t value);

int16_t pn53_register_symbol_get(pn53_register_symbols_t* symbols, pn53_register_symbol_t symbol);

int pn53_register_symbols_write(pn53_dev_t* dev, pn53_register_symbols_t* symbols);

static inline void pn53_bitfield_set(uint8_t* bitfield, uint8_t mask, uint8_t value) {
    *bitfield &= ~mask;
    uint8_t bit_offset = __builtin_ctz(mask);
    *bitfield |= (value << bit_offset) & mask;
}

static inline uint8_t pn53_bitfield_create(uint8_t mask, uint8_t value) {
    uint8_t bit_offset = __builtin_ctz(mask);
    return (value << bit_offset) & mask;
}

static inline uint8_t pn53_bitfield_get(uint8_t bitfield, uint8_t mask) {
    uint8_t bit_offset = __builtin_ctz(mask);
    return (bitfield & mask) >> bit_offset;
}

/// A register mask for `TxLastBits` in the `CIU_BitFraming` register containing
/// the number of bits of the last byte that shall be transmitted. Set to 0 to indicate all bits
/// shall be sent.
#define PN53_SYMBOL_TX_BIT_COUNT PN53_REGISTER_SYMBOL(BIT_FRAMING, 0x07)

/// A register mask for `TxFraming` in the `CIU_TxMode` register indicating the
/// framing type used during transmission.
#define PN53_SYMBOL_TX_FRAMING PN53_REGISTER_SYMBOL(TX_MODE, 0x03)

/// A register mask for `TxSpeed` in the `CIU_TxMode` register indicating the
/// bit rate used during transmission.
#define PN53_SYMBOL_RX_FRAMING PN53_REGISTER_SYMBOL(RX_MODE, 0x03)

/// A register mask for `RxFraming` in the `CIU_TxMode` register indicating the
/// framing type used during reception.
#define PN53_SYMBOL_TX_BITRATE PN53_REGISTER_SYMBOL(TX_MODE, 0x70)

/// A register mask for `RxSpeed` in the `CIU_TxMode` register indicating the
/// bit rate used during reception.
#define PN53_SYMBOL_RX_BITRATE PN53_REGISTER_SYMBOL(RX_MODE, 0x70)

/// A register mask for `RxNoErr` in the `CIU_RxMode` register which determines whether
/// the PN53x accepts invalid frames (less than 4 bits are ignored and the receiver stays active).
#define PN53_SYMBOL_ACCEPT_INVALID_FRAMES PN53_REGISTER_SYMBOL(RX_MODE, 0x08)

/// A register mask for `RxMultiple` in the `CIU_RxMode` register
#define PN53_SYMBOL_ACCEPT_MULTIPLE_FRAMES PN53_REGISTER_SYMBOL(RX_MODE, 0x04)

/// A register mask for `MFCrypto1On` in the `CIU_Status2` register
#define PN53_SYMBOL_ENABLE_MIFARE_CRYPTO1 PN53_REGISTER_SYMBOL(STATUS2, 0x08)

/// A register mask wrapping `ParityDisable` in the `CIU_ManualRCV` register which indicates
/// whether parity bits are automatically added to frames, checked in received frames and
/// removed from them.
#define PN53_SYMBOL_AUTO_PARITY PN53_REGISTER_SYMBOL(MANUAL_RECEIVER, 0x10)

/// A register mask for `TxCRCEn` in the `CIU_TxMode` register which determines whether a
/// Cyclic Redundancy Code is automatically added to frames by the PN53x.
#define PN53_SYMBOL_TX_AUTO_CRC PN53_REGISTER_SYMBOL(TX_MODE, 0x80)

/// A register mask for `RxCRCEn` in the `CIU_RxMode` register which determines whether the
/// Cyclic Redundancy Code is automatically checked and removed from received frames
/// by the PN53x.
#define PN53_SYMBOL_RX_AUTO_CRC PN53_REGISTER_SYMBOL(RX_MODE, 0x80)

/// A register mask for `Initiator` in the `CIU_Control` register determining the PN53x's transceiver behavior.
#define PN53_SYMBOL_IS_INITIATOR PN53_REGISTER_SYMBOL(CONTROL, 0x10)

/// A register mask for `InitialRFOn` in the `CIU_TxAuto` register.
#define PN53_SYMBOL_INITIAL_FIELD_ON PN53_REGISTER_SYMBOL(TX_MODE, 0x04)

/// A register mask for `Force100ASK` in the `CIU_TxAuto` register.
#define PN53_SYMBOL_TX_FORCE_100_PERCENT_ASK PN53_REGISTER_SYMBOL(TX_AUTO, 0x40)

/// A register mask for `MFHalted` in the `CIU_MifNFC` register.
#define PN53_SYMBOL_MIFARE_IS_HALTED PN53_REGISTER_SYMBOL(NFC_A, 0x04)

typedef enum __attribute__((packed)) {
    PN53_CIU_COMMAND_IDLE                                = 0,
    PN53_CIU_COMMAND_CONFIG                          = 1,
    PN53_CIU_COMMAND_GENERATE_RANDOM_ID              = 2,
    PN53_CIU_COMMAND_CALCULATE_CRC                   = 3,
    PN53_CIU_COMMAND_TRANSMIT                        = 4,
    PN53_CIU_COMMAND_CIU_REGISTER_MODIFY             = 7,
    PN53_CIU_COMMAND_RECEIVE                         = 8,
    PN53_CIU_COMMAND_SELF_TEST                       = 9,
    PN53_CIU_COMMAND_TRANSCEIVE                      = 12,
    PN53_CIU_COMMAND_TARGET_EMULATION_AUTO_COLLISION = 13,
    PN53_CIU_COMMAND_AUTHENTICATE_MIFARE_CLASSIC     = 14,
    PN53_CIU_COMMAND_SOFT_RESET                      = 15,
} pn53_ciu_command_t;

typedef union __attribute__((packed)) {
    struct {
        bool p30 : 1;
        bool p31 : 1;
        bool p32 : 1;
        bool p33 : 1;
        bool p34 : 1;
        bool p35 : 1;
        uint8_t _rfu : 1;
        bool write : 1;
    } __attribute__((packed));

    uint8_t raw;
} pn53_gpio_p3_t;

typedef union __attribute__((packed)) {
    struct {
        uint8_t _rfu_0 : 1;
        bool p71 : 1;
        bool p72 : 1;
        uint8_t _rfu_1 : 4;
        bool write : 1;
    } __attribute__((packed));

    uint8_t raw;
} pn53_gpio_p7_t;

typedef union __attribute__((packed)) {
    struct {
        bool i0 : 1;
        bool i1 : 1;
        uint8_t _rfu : 6;
    } __attribute__((packed));

    uint8_t raw;
} pn53_gpio_i0i1_t;

typedef struct __attribute__((packed)) {
    pn53_gpio_p3_t p3;
    pn53_gpio_p7_t p7;
    pn53_gpio_i0i1_t i0i1;
} pn53_read_gpio_payload_t;

ssize_t pn53_read_gpio(pn53_dev_t* dev, pn53_read_gpio_payload_t** response);

typedef struct __attribute__((packed)) {
    pn53_gpio_p3_t p3;
    pn53_gpio_p7_t p7;
} pn53_write_gpio_payload_t;

int pn53_write_gpio(pn53_dev_t* dev, pn53_write_gpio_payload_t payload);

typedef struct {
    pn53_status_code_t last_command_status;
    bool field_detected;
    uint8_t target_count;

    struct {
        struct {
            nfc_technology_t technology;
            nfc_bitrate_t bitrate;
        } tx;

        struct {
            nfc_technology_t technology;
            nfc_bitrate_t bitrate;
        } rx;

        nfc_field_model_t mode : 1;
        bool nfc_a_gemstone : 1;
    } targets[2];

    pn53_sam_status_t sam_status;
} pn53_general_status_t;

int pn53_get_general_status(pn53_dev_t* dev, pn53_general_status_t* status);

typedef enum __attribute__((packed)) {
    PN53_RF_CONFIGURATION_ITEM_RF_FIELD                 = 0x01,
    PN53_RF_CONFIGURATION_ITEM_VARIOUS_TIMINGS          = 0x02,
    PN53_RF_CONFIGURATION_ITEM_MAXIMUM_RETRIES_FRAME    = 0x04,
    PN53_RF_CONFIGURATION_ITEM_MAXIMUM_RETRIES          = 0x05,
    PN53_RF_CONFIGURATION_ITEM_ANALOG_NFC_A             = 0x0A,
    PN53_RF_CONFIGURATION_ITEM_ANALOG_NFC_B             = 0x0B,
    PN53_RF_CONFIGURATION_ITEM_ANALOG_NFC_F             = 0x0C,

    /// `ISO-DEP` at 212, 424, 848 kbit/s
    PN53_RF_CONFIGURATION_ITEM_ANALOG_FAST_ISO_DEP      = 0x0D,
} pn53_rf_configuration_item_t;

typedef struct __attribute__((packed)) {
    pn53_rf_configuration_item_t item;

    union {
        union __attribute__((packed)) {
            struct {
                bool automatic_rf_collision_avoidance : 1;
                bool rf_on : 1;
                uint8_t _rfu : 6;
            } __attribute__((packed));

            uint8_t raw;
        } rf_field;

        struct __attribute__((packed)) {
            uint8_t _rfu;
            uint8_t atr_response_timeout;
            uint8_t non_dep_timeout;
        } various_timings;

        struct __attribute__((packed)) {
            struct __attribute__((packed))  {
                uint8_t activation;
                uint8_t parameter_selection;
            } nfc_dep;
            uint8_t passive_activation;
        } max_retries_higher_layer;

        uint8_t max_retries_frame;

        struct __attribute__((packed)) {
            uint8_t rf_configuration;
            uint8_t conductance_n_driver_on;
            uint8_t conductance_p_driver_no_modulation;
            uint8_t conductance_p_driver_modulation;
            uint8_t demodulator_when_rf_on;
            uint8_t rx_threshold;
            uint8_t demodulator_when_rf_off;
            uint8_t conductance_n_driver_off;
            uint8_t miller_modulation_width;
            uint8_t nfc_a;
            uint8_t tx_bit_phase;
        } registers_when_nfc_a;

        struct __attribute__((packed)) {
            uint8_t conductance_n_driver_on;
            uint8_t conductance_p_driver_modulation;
            uint8_t rx_threshold;
        } registers_when_nfc_b;

        struct __attribute__((packed)) {
            uint8_t rf_configuration;
            uint8_t conductance_n_driver_on;
            uint8_t conductance_p_driver_no_modulation;
            uint8_t conductance_p_driver_modulation;
            uint8_t demodulator_when_rf_on;
            uint8_t rx_threshold;
            uint8_t demodulator_when_rf_off;
            uint8_t conductance_n_driver_off;
        } registers_when_nfc_f;

        struct __attribute__((packed)) {
            struct __attribute__((packed)) {
                uint8_t rx_threshold;
                uint8_t miller_modulation_width;
                uint8_t nfc_a;
            } at_212;

            struct __attribute__((packed)) {
                uint8_t rx_threshold;
                uint8_t miller_modulation_width;
                uint8_t nfc_a;
            } at_424;

            struct __attribute__((packed)) {
                uint8_t rx_threshold;
                uint8_t miller_modulation_width;
                uint8_t nfc_a;
            } at_848;
        } registers_when_fast_iso_dep;
    } __attribute__((packed)) ;
} pn53_rf_configuration_payload_t;

int pn53_rf_configuration(pn53_dev_t* dev, const pn53_rf_configuration_payload_t* rf_config);

int pn53_set_field_enablement(pn53_dev_t* dev, bool intent_to_enable, bool collision_avoidance);

ssize_t pn53_in_list_passive_targets_a(pn53_dev_t* dev,
    uint8_t max_targets, nfc_a_id_t* id, nfc_a_tag_t* tags, uint32_t timeout_ms);

ssize_t pn53_in_list_passive_targets_b(pn53_dev_t* dev,
    uint8_t max_targets, nfc_bitrate_t bitrate, uint8_t application_family,
    nfc_b_polling_method_t method, nfc_b_tag_t* tags, uint32_t timeout_ms);

ssize_t pn53_in_list_passive_targets_f(pn53_dev_t* dev, uint8_t max_targets, nfc_bitrate_t bitrate,
    nfc_f_system_code_t system_code, nfc_f_polling_additional_request_t additional_request,
    uint8_t timeslots, nfc_f_tag_t* tags, uint32_t timeout_ms);

#ifndef DOXYGEN
int pn53_deselect_reselect_release(pn53_dev_t *dev, uint8_t tg, pn53_command_code_t code);
#endif

static inline int pn53_deselect(pn53_dev_t *dev, nfcdev_connection_id_t connection_id) {
    return pn53_deselect_reselect_release(dev, connection_id + 1, PN53_COMMAND_IN_DESELECT);
}

static inline int pn53_deselect_all(pn53_dev_t *dev) {
    return pn53_deselect_reselect_release(dev, 0, PN53_COMMAND_IN_DESELECT);
}

static inline int pn53_reselect(pn53_dev_t *dev, nfcdev_connection_id_t connection_id) {
    return pn53_deselect_reselect_release(dev, connection_id + 1, PN53_COMMAND_IN_SELECT);
}

static inline int pn53_release(pn53_dev_t *dev, nfcdev_connection_id_t connection_id) {
    return pn53_deselect_reselect_release(dev, connection_id + 1, PN53_COMMAND_IN_RELEASE);
}

static inline int pn53_release_all(pn53_dev_t *dev) {
    return pn53_deselect_reselect_release(dev, 0, PN53_COMMAND_IN_RELEASE);
}

#ifndef DOXYGEN
ssize_t pn53_in_communicate_thru_data_exchange(pn53_dev_t* dev, const uint8_t* command, size_t length, uint8_t** response, uint32_t timeout_ms, pn53_command_code_t code);
#endif

static inline ssize_t pn53_in_communicate_thru(pn53_dev_t* dev, const uint8_t* command, size_t length, uint8_t** response, uint32_t timeout_ms) {
    return pn53_in_communicate_thru_data_exchange(dev, command, length, response, timeout_ms, PN53_COMMAND_IN_COMMUNICATE_THRU);
}

static inline ssize_t pn53_in_data_exchange(pn53_dev_t* dev, const uint8_t* command, size_t length, uint8_t** response, uint32_t timeout_ms) {
    return pn53_in_communicate_thru_data_exchange(dev, command, length, response, timeout_ms, PN53_COMMAND_IN_DATA_EXCHANGE);
}

#define PN53_FIFO_TIMEOUT_NEVER (0)

int nfcdev_configure_radio_pn53(nfcdev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role);

int pn53_fifo_receive_start_(pn53_dev_t* dev, bool transceive);

ssize_t pn53_fifo_receive_read_(pn53_dev_t* dev,
                                uint8_t* rx, size_t capacity, uint8_t* rx_trailing_bit_count, uint32_t timeout_ms);

int pn53_fifo_transmit_write_(pn53_dev_t* dev,
    const uint8_t* tx, size_t length, uint8_t tx_trailing_bit_count,
    bool transceive, bool transceive_after_receiving
);

static inline int pn53_fifo_transmit(pn53_dev_t* dev,
    const uint8_t* tx, size_t length, uint8_t tx_trailing_bit_count
) {
    return pn53_fifo_transmit_write_(dev, tx, length, tx_trailing_bit_count, false, false);
}

static inline ssize_t pn53_fifo_receive(pn53_dev_t* dev,
    uint8_t* rx, size_t capacity, uint8_t* rx_trailing_bit_count, uint32_t timeout_ms
) {
    int res = pn53_fifo_receive_start_(dev, false);
    if (res < 0) {
        return res;
    }
    return pn53_fifo_receive_read_(dev, rx, capacity, rx_trailing_bit_count, timeout_ms);
}

static inline ssize_t pn53_fifo_transceive_target_receive(pn53_dev_t* dev,
    uint8_t* rx, size_t rx_capacity, uint8_t* rx_trailing_bit_count, uint32_t timeout_ms
) {
    int res = pn53_fifo_receive_start_(dev, true);
    if (res < 0) {
        return res;
    }
    return pn53_fifo_receive_read_(dev, rx, rx_capacity, rx_trailing_bit_count, timeout_ms);
}

static inline int pn53_fifo_transceive_target_transmit(pn53_dev_t* dev,
    const uint8_t* tx, size_t tx_length, uint8_t tx_trailing_bit_count
) {
    return pn53_fifo_transmit_write_(dev, tx, tx_length, tx_trailing_bit_count, true, true);
}

static inline ssize_t pn53_fifo_transceive_initiator(pn53_dev_t* dev,
    const uint8_t* tx, size_t tx_length, uint8_t tx_trailing_bit_count,
    uint8_t* rx, size_t rx_capacity, uint8_t* rx_trailing_bit_count, uint32_t timeout_ms
) {
    int res = 0;
    if ((res = pn53_fifo_transmit_write_(dev, tx, tx_length, tx_trailing_bit_count, true, false)) < 0) {
        return res;
    }
    if ((res = pn53_fifo_receive_start_(dev, false)) < 0) {
        return res;
    }
    return pn53_fifo_receive_read_(dev, rx, rx_capacity, rx_trailing_bit_count, timeout_ms);
}

/**
 * @brief   PN532 supported targets
 */
//typedef enum {
//    PN532_BR_106_ISO_14443_A = 0,
//    PN532_BR_212_FELICA,
//    PN532_BR_424_FELICA,
//    PN532_BR_106_ISO_14443_B,
//    PN532_BR_106_JEWEL
//} pn532_target_t;
//
///**
// * @brief   ISO14443A Card types
// */
//typedef enum {
//    ISO14443A_UNKNOWN,
//    ISO14443A_MIFARE,
//    ISO14443A_TYPE4
//} nfc_iso14443a_type_t;
//
///**
// * @brief   ISO14443A tag description
// */
//typedef struct {
//    uint8_t target;                /**< Target */
//    uint8_t auth;                  /**< Card has been authenticated. Do not modify manually */
//    uint8_t id_len;                /**< Length of the ID field */
//    uint8_t acknowledgement;               /**< SEL_RES */
//    unsigned sns_res;           /**< SNS_RES */
//    nfc_iso14443a_type_t type;  /**< Type of ISO14443A card */
//    uint8_t id[8];                 /**< Card ID (length given by id_len) */
//} nfc_iso14443a_t;
//
///**
// * @brief   Mifare keys
// */
//typedef enum {
//    PN532_MIFARE_KEY_A = 0x60,
//    PN532_MIFARE_KEY_B = 0x61
//} pn532_mifare_key_t;
//
//typedef union {
//    uint8_t mifare_params[6];
//    uint8_t felica_params[18];
//    uint8_t nfcid3t[10];
//} pn532_target_params_t;
//
///**
// * @brief   Obtain Tag 4 data length from buffer
// *
// * This is useful in case the length has been read and one intents to read the
// * data.
// */
//#define PN532_ISO14443A_4_LEN_FROM_BUFFER(b) ((b[0] << 8) | b[1])
//
///**
// * @brief   Hard reset the chipset
// *
// * The chipset is reset by toggling the reset pins
// *
// * @param[in]  dev          target device
// *
// */
//void pn532_reset(const pn53_dev_t* dev);
//
///**
// * @brief   Initialize the module and peripherals
// *
// * This is the first method to be called in order to interact with the pn532.
// * It configures the GPIOs and the i2c/spi interface (depending on @p mode).
// *
// *  @param[in]  dev         target device
// *  @param[in]  params      configuration parameters
// *  @param[in]  mode        initialization mode
// *
// * @return                  0 on success
// * @return                  <0 i2c/spi/uart initialization error, the value is given
// *                          by the i2c/spi/uart init method.
// */


#ifdef __cplusplus
}
#endif

/** @} */
