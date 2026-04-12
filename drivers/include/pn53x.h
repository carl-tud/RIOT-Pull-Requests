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
#define PN53_FRAME_OVERHEAD_MAX (11)

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

typedef struct {
    pn53_connection_t connection;
    const pn53_parameters_t* parameters;
    pn53_model_t model;
    uint8_t nfc_parameters;
    uint32_t command_timeout;
} pn53_dev_t;

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

typedef enum __attribute__((packed)) {
    PN53_COMMAND_DIAGNOSE = 0,
    PN53_COMMAND_GET_FIRMWARE_VERSION = 2,
    PN53_COMMAND_READ_REGISTERS = 6,
    PN53_COMMAND_WRITE_REGISTERS = 8,
    PN53_COMMAND_SAM_CONFIGURATION = 0x14,
    PN53_COMMAND_SET_PARAMETERS = 0x12,
    PN53_COMMAND_POWER_DOWN = 0x16,
    PN53_COMMAND_IN_LIST_PASSIVE_TARGET = 0x4a,
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

ssize_t pn53_read_registers(pn53_dev_t *dev, pn53_register_address_t* registers, uint8_t** values, size_t count);

int pn53_write_registers(pn53_dev_t *dev, pn53_register_t* registers, size_t count);

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
    PN53_TECHNOLOGY_F_212K = 1,
    PN53_TECHNOLOGY_F_424K = 2,
} pn53_technology_baudrate_t;

typedef enum {
    PN53_UART_SPEED_9600_BAUD = 0x00, /* 9.6 kbaud */
    PN53_UART_SPEED_19200_BAUD = 0x01, /* 19.2 kbaud */
    PN53_UART_SPEED_38400_BAUD = 0x02, /* 38.4 kbaud */
    PN53_UART_SPEED_57600_BAUD = 0x03, /* 57.6 kbaud */
    PN53_UART_SPEED_115200_BAUD = 0x04, /* 115.2 kbaud */
    PN53_UART_SPEED_230400_BAUD = 0x05, /* 230.4 kbaud */
    PN53_UART_SPEED_460800_BAUD = 0x06, /* 460.8 kbaud */
    PN53_UART_SPEED_921600_BAUD = 0X07, /* 921.6 kbaud */
    PN53_UART_SPEED_1288_KBAUD = 0x08, /* 1.288 Mbaud */
} pn53_uart_speed_t;

#define PN53_TIMEOUT_FIELD_FROM_MS(ms) (uint8_t)((ms) / 50)
#define PN53_TIMEOUT_FIELD_TO_MS(timeout) ((timeout) * 50)

#define PN532_IC_VERSION(fwver)  ((fwver >> 24) & 0xff)
#define PN532_FW_VERSION(fwver)  ((fwver >> 16) & 0xff)
#define PN532_FW_REVISION(fwver) ((fwver >>  8) & 0xff)
#define PN532_FW_FEATURES(fwver) ((fwver) & 0xff)
/** @} */


#define PN532_PARAM_NAD_USED              (0x01)
#define PN532_PARAM_DID_USED              (0x02)
#define PN532_PARAM_AUTOMATIC_ATR_RES     (0x04)
#define PN532_PARAM_AUTOMATIC_RATS        (0x10)
#define PN532_PARAM_ISO14443A_4_PICC      (0x20)
#define PN532_PARAM_REMOVE_PRE_POST_AMBLE (0x40)

// Register addresses

#define PN532_REGISTER_CONTROL_SWITCH_RNG 0x6106

/// Defines general modes for transmitting and receiving
#define PN532_REGISTER_MODE 0x6301

/// Defines the data rate and framing during transmission.
#define PN532_REGISTER_TX_MODE 0x6302

/// Defines the data rate and framing during reception.
#define PN532_REGISTER_RX_MODE 0x6303

/// Controls the logical behaviour of the antenna driver pins TX1 and TX2
#define PN532_REGISTER_TX_CONTROL 0x6304

/// Controls the settings of the antenna driver
#define PN532_REGISTER_TX_AUTO 0x6305

/// Selects the internal sources for the antenna driver
#define PN532_REGISTER_TX_SELECTOR 0x6306

/// Selects internal receiver settings
#define PN532_REGISTER_RX_SELECTOR 0x6307

/// Selects thresholds for the bit decoder
#define PN532_REGISTER_RX_THRESHOLD 0x6308

/// Defines demodulator settings
#define PN532_REGISTER_DEMODULATOR 0x6309

/// Defines the length of the valid range for the received frame
#define PN532_REGISTER_NFC_F_1 0x630A

/// Defines the length of the valid range for the received frame
#define PN532_REGISTER_NFC_F_2 0x630A

// Controls the communication in NFC-A (+ NFC-A MIFARE) and NFC target mode at 106 kbit/s
#define PN532_REGISTER_NFC_A 0x630C

/// Allows manual fine tuning of the internal receiver.
#define PN532_REGISTER_MANUAL_RECEIVER 0x630D

/// Configure NFC-B
#define PN532_REGISTER_NFC_B 0x630E
// #define PN532_REGISTER_- 0x630F
// #define PN532_REGISTER_- 0x6310

/// Shows the actual MSB values of the CRC calculation
#define PN532_REGISTER_CRC_HIGH 0x6311

/// Shows the actual LSB values of the CRC calculation
#define PN532_REGISTER_CRC_LOW 0x6312

/// Controls the setting of the width of the Miller pause
#define PN532_REGISTER_MILLER_MODULATION_WIDTH 0x6314
#define PN532_REGISTER_ModWidth PN532_REGISTER_MILLER_MODULATION_WIDTH

/// Bit synchronization at 106 kbit/s
#define PN532_REGISTER_TX_BIT_PHASE 0x6315

/// Configures the receiver gain and RF level detector sensitivity.
#define PN532_REGISTER_RF_CONFIG 0x6316

/// Selects the conductance for the N-driver of the antenna driver pins TX1 and TX2 when the driver is switched off.
#define PN532_REGISTER_CONDUCTANCE_N_DRIVER_OFF 0x6313
#define PN532_REGISTER_GsNOFF PN532_REGISTER_CONDUCTANCE_N_DRIVER_OFF

/// Selects the conductance for the N-driver of the antenna driver pins TX1 and TX2 when the driver is switched on.
#define PN532_REGISTER_CONDUCTANCE_N_DRIVER_ON 0x6317
#define PN532_REGISTER_GsNOn PN532_REGISTER_CONDUCTANCE_N_DRIVER_ON

/// Defines the conductance of the P-driver during times of no modulation.
#define PN532_REGISTER_CONDUCTANCE_P_DRIVER_NO_MODULATION 0x6318
#define PN532_REGISTER_CWGsP PN532_REGISTER_CONDUCTANCE_P_DRIVER_NO_MODULATION

/// Defines the driver P-output conductance during modulation.
#define PN532_REGISTER_CONDUCTANCE_P_DRIVER_MODULATION 0x6319
#define PN532_REGISTER_ModGsP PN532_REGISTER_CONDUCTANCE_P_DRIVER_MODULATION

/// Defines settings for the internal timer
#define PN532_REGISTER_TIMER_MODE 0x631A

/// Defines settings for the internal timer
#define PN532_REGISTER_TIMER_PRESCALER 0x631B

/// Describes the MSB of the 16-bit long timer reload value.
#define PN532_REGISTER_TIMER_RELOAD_VALUE_HIGH 0x631C

/// Describes the LSB of the 16-bit long timer reload value.
#define PN532_REGISTER_TIMER_RELOAD_VALUE_LOW 0x631D

/// Describes the 16-bit long timer actual value (Higher 8 bits)
#define PN532_REGISTER_TIMER_COUNTER_VALUE_HIGH 0x631E

/// Describes the 16-bit long timer actual value (Lower 8 bits)
#define PN532_REGISTER_TIMER_COUNTER_VALUE_LOW 0x631F

// #define PN532_REGISTER_- 0x6320

/// General test signals configuration
#define PN532_REGISTER_TEST_SELECTION1 0x6321

/// General test signals configuration and PRBS control
#define PN532_REGISTER_TEST_SELECTION2 0x6322

/// Enables test signals output on pins.
#define PN532_REGISTER_TEST_PIN_ENABLE 0x6323

/// Defines the values for the 8-bit parallel bus when it is used as I/O bus
#define PN532_REGISTER_TEST_PIN_VALUE 0x6324

/// Shows the status of the internal test bus
#define PN532_REGISTER_TEST_BUS 0x6325

/// Controls the digital self-test
#define PN532_REGISTER_TEST_AUTO 0x6326

/// Shows the CIU version
#define PN532_REGISTER_VERSION 0x6327

/// Controls the pins AUX1 and AUX2
#define PN532_REGISTER_TEST_ANALOG 0x6328

/// Defines the test value for the TestDAC1
#define PN532_REGISTER_TEST_DAC1 0x6329

/// Defines the test value for the TestDAC2
#define PN532_REGISTER_TEST_DAC2 0x632A

/// Show the actual value of ADC I and Q
#define PN532_REGISTER_TEST_ADCV 0x632B

// #define PN532_REGISTER_- 0x632C
// #define PN532_REGISTER_- 0x632D
// #define PN532_REGISTER_- 0x632E

/// Power down of the RF level detector
#define PN532_REGISTER_RF_LEVEL_DETECTOR 0x632F

/// Enables the use of secure IC clock on P34 / SIC_CLK
#define PN532_REGISTER_SECURE_IC_CLOCK 0x6330

/// Starts and stops the command execution
#define PN532_REGISTER_COMMAND 0x6331

/// Control bits to enable and disable the passing of interrupt requests
#define PN532_REGISTER_COMMON_INTERRUPT_ENABLE 0x6332

/// Control bits to enable and disable the passing of interrupt requests
#define PN532_REGISTER_DIVERSE_INTERRUPT_ENABLE 0x6333

/// Contains common CIU interrupt request flags
#define PN532_REGISTER_COMMON_IRQ 0x6334

/// Contains miscellaneous interrupt request flags
#define PN532_REGISTER_DIVERSE_IRQ 0x6335

/// Error flags showing the error status of the last command executed
#define PN532_REGISTER_ERROR 0x6336

/// Contains status flags of the CRC, Interrupt Request System and FIFO buffer
#define PN532_REGISTER_STATUS1 0x6337

/// Contain status flags of the receiver, transmitter and Data Mode Detector
#define PN532_REGISTER_STATUS2 0x6338

/// In- and output of 64 byte FIFO buffer
#define PN532_REGISTER_FIFO_DATA 0x6339

/// Indicates the number of bytes stored in the FIFO
#define PN532_REGISTER_FIFO_LEVEL 0x633A

/// Defines the thresholds for FIFO under- and overflow warning
#define PN532_REGISTER_FIFO_WATER_LEVEL 0x633B

/// Contains miscellaneous control bits
#define PN532_REGISTER_CONTROL 0x633C

/// Adjustments for bit oriented frames
#define PN532_REGISTER_BIT_FRAMING 0x633D

/// Defines the first bit collision detected on the RF interface
#define PN532_REGISTER_COLLISION 0x633E

#define PN532_SFR_P3 0xFFB0

#define PN532_SFR_P3CFGA 0xFFFC
#define PN532_SFR_P3CFGB 0xFFFD
#define PN532_SFR_P7CFGA 0xFFF4
#define PN532_SFR_P7CFGB 0xFFF5
#define PN532_SFR_P7 0xFFF7

typedef uint32_t pn53_register_symbol_t;


#ifndef DOXYGEN

#  define PN53_REGISTER_SYMBOL_(register, mask) (((mask) << 16) | (register))

#  define PN53_REGISTER_SYMBOL_MASK(symbol) ((symbol) >> 16)
#  define PN53_REGISTER_SYMBOL_REGISTER(symbol) ((symbol) & 0xffff)

#  define PN53_REGISTER_SYMBOL(register_name, mask) \
    PN53_REGISTER_SYMBOL_(PN53_REGISTER_ ## register_name, mask)

#  define PN53_REGISTER_SYMBOL_REGISTER_CAPACITY (PN532_REGISTER_COLLISION - PN532_REGISTER_MODE)
#  define PN53_SYMBOLS_START_REGISTER PN532_REGISTER_MODE

#endif

typedef struct {
    uint8_t register_values[PN53_REGISTER_SYMBOL_REGISTER_CAPACITY];
    uint8_t change_masks[PN53_REGISTER_SYMBOL_REGISTER_CAPACITY];
} pn53_register_symbols_t;

int pn53_register_symbol_set(pn53_register_symbols_t* symbols, pn53_register_symbol_t symbol, uint8_t value);

int16_t pn53_register_symbol_get(pn53_register_symbols_t* symbols, pn53_register_symbol_t symbol);

int pn53_register_symbols_write(pn53_dev_t* dev, pn53_register_symbols_t* symbols);

/// A register mask for `TxLastBits` in the `CIU_BitFraming` register containing
/// the number of bits of the last byte that shall be transmitted. Set to 0 to indicate all bits
/// shall be sent.
#define pn53_register_symbol_tX_BIT_COUNT PN53_REGISTER_SYMBOL(BIT_FRAMING, 0x07)

/// A register mask for `TxFraming` in the `CIU_TxMode` register indicating the
/// framing type used during transmission.
#define pn53_register_symbol_tX_FRAMING PN53_REGISTER_SYMBOL(TX_MODE, 0x03)

/// A register mask for `TxSpeed` in the `CIU_TxMode` register indicating the
/// bit rate used during transmission.
#define PN53_SYMBOL_RX_FRAMING PN53_REGISTER_SYMBOL(RX_MODE, 0x03)

/// A register mask for `RxFraming` in the `CIU_TxMode` register indicating the
/// framing type used during reception.
#define pn53_register_symbol_tX_BITRATE PN53_REGISTER_SYMBOL(TX_MODE, 0x70)

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
#define pn53_register_symbol_tX_AUTO_CRC PN53_REGISTER_SYMBOL(TX_MODE, 0x80)

/// A register mask for `RxCRCEn` in the `CIU_RxMode` register which determines whether the
/// Cyclic Redundancy Code is automatically checked and removed from received frames
/// by the PN53x.
#define PN53_SYMBOL_RX_AUTO_CRC PN53_REGISTER_SYMBOL(RX_MODE, 0x80)

/// A register mask for `Initiator` in the `CIU_Control` register determining the PN53x's transceiver behavior.
#define PN53_SYMBOL_IS_INITIATOR PN53_REGISTER_SYMBOL(CONTROL, 0x10)

/// A register mask for `InitialRFOn` in the `CIU_TxAuto` register.
#define PN53_SYMBOL_INITIAL_FIELD_ON PN53_REGISTER_SYMBOL(TX_MODE, 0x04)

/// A register mask for `Force100ASK` in the `CIU_TxAuto` register.
#define pn53_register_symbol_tX_FORCE_100_PERCENT_ASK PN53_REGISTER_SYMBOL(TX_AUTO, 0x40)

/// A register mask for `MFHalted` in the `CIU_MifNFC` register.
#define PN53_SYMBOL_MIFARE_IS_HALTED PN53_REGISTER_SYMBOL(NFC_A, 0x04)

/**
 * @brief   Possible SAM configurations
 */
typedef enum {
    PN53_SAM_NORMAL = 1,
    PN53_SAM_VIRTUAL,
    PN53_SAM_WIRED,
    PN53_SAM_DUAL,
} pn53_sam_mode_t;

/**
 * @brief   PN532 supported targets
 */
typedef enum {
    PN532_BR_106_ISO_14443_A = 0,
    PN532_BR_212_FELICA,
    PN532_BR_424_FELICA,
    PN532_BR_106_ISO_14443_B,
    PN532_BR_106_JEWEL
} pn532_target_t;

/**
 * @brief   ISO14443A Card types
 */
typedef enum {
    ISO14443A_UNKNOWN,
    ISO14443A_MIFARE,
    ISO14443A_TYPE4
} nfc_iso14443a_type_t;

/**
 * @brief   ISO14443A tag description
 */
typedef struct {
    uint8_t target;                /**< Target */
    uint8_t auth;                  /**< Card has been authenticated. Do not modify manually */
    uint8_t id_len;                /**< Length of the ID field */
    uint8_t acknowledgement;               /**< SEL_RES */
    unsigned sns_res;           /**< SNS_RES */
    nfc_iso14443a_type_t type;  /**< Type of ISO14443A card */
    uint8_t id[8];                 /**< Card ID (length given by id_len) */
} nfc_iso14443a_t;

/**
 * @brief   Mifare keys
 */
typedef enum {
    PN532_MIFARE_KEY_A = 0x60,
    PN532_MIFARE_KEY_B = 0x61
} pn532_mifare_key_t;

typedef union {
    uint8_t mifare_params[6];
    uint8_t felica_params[18];
    uint8_t nfcid3t[10];
} pn532_target_params_t;

/**
 * @brief   Obtain Tag 4 data length from buffer
 *
 * This is useful in case the length has been read and one intents to read the
 * data.
 */
#define PN532_ISO14443A_4_LEN_FROM_BUFFER(b) ((b[0] << 8) | b[1])

/**
 * @brief   Hard reset the chipset
 *
 * The chipset is reset by toggling the reset pins
 *
 * @param[in]  dev          target device
 *
 */
void pn532_reset(const pn53_dev_t* dev);

/**
 * @brief   Initialize the module and peripherals
 *
 * This is the first method to be called in order to interact with the pn532.
 * It configures the GPIOs and the i2c/spi interface (depending on @p mode).
 *
 *  @param[in]  dev         target device
 *  @param[in]  params      configuration parameters
 *  @param[in]  mode        initialization mode
 *
 * @return                  0 on success
 * @return                  <0 i2c/spi/uart initialization error, the value is given
 *                          by the i2c/spi/uart init method.
 */
int pn53_init(pn53_dev_t* dev, const pn53_connection_config_t* config);

int pn532_init(nfcdev_t *nfcdev, const void *dev_config);


#if IS_USED(MODULE_PN53X_I2C) || DOXYGEN
/**
 * @brief   Initialization of PN532 using i2c
 *
 * @see pn532_init for parameter and return value details
 * @note Use `pn532_i2c` module to use this function.
 */
static inline int pn532_init_i2c(pn53_dev_t* dev, const pn532_params_t *params)
{
    return pn532_init(dev, params, PN53_BUS_I2C);
}
#endif

#if IS_USED(MODULE_PN53_SPI) || DOXYGEN
/**
 * @brief   Initialization of PN532 using spi
 *
 * @see pn532_init for parameter and return value details
 * @note Use `pn532_spi` module to use this function.
 */
static inline int pn532_init_spi(pn53_dev_t* dev, const pn532_params_t *params)
{
    return _pn532_init(dev, params, PN53_BUS_SPI);
}
#endif

#if IS_USED(MODULE_PN53X_UART) || DOXYGEN
static inline int pn532_init_uart(pn53_dev_t* dev, const pn532_params_t *params)
{
    return pn532_init(dev, params, PN53_BUS_UART);
}
#endif

/**
 * @brief   Get the firmware version of the pn532
 *
 * The firmware version returned is a 4 byte long value:
 *  - ic version,
 *  - fw version,
 *  - fw revision
 *  - feature support
 *
 * @param[in]  dev          target device
 * @param[out] fw_ver       encoded firmware version
 *
 * @return                  0 on success
 */
int pn532_fw_version(pn53_dev_t* dev, uint32_t *fw_ver);

/**
 * @brief   Read register of the pn532
 *
 * Refer to the datasheet for a comprehensive list of registers and meanings.
 * For SFR registers the high byte must be set to 0xff.
 *
 * http://www.nxp.com/documents/user_manual/141520.pdf
 *
 * @param[in]  dev          target device
 * @param[out] out          value of the register
 * @param[in]  addr         address of the register to read
 *
 * @return                  0 on success
 */
int pn532_read_reg(pn53_dev_t* dev, uint8_t *out, unsigned addr);

/**
 * @brief   Write register of the pn532
 *
 * Refer to the datasheet for a comprehensive list of registers and meanings.
 *
 * http://www.nxp.com/documents/user_manual/141520.pdf
 *
 * @param[in]  dev          target device
 * @param[in]  addr         address of the register to read
 * @param[in]  val          value to write in the register
 *
 * @return                  0 on success
 */
int pn532_write_reg(pn53_dev_t* dev, unsigned addr, uint8_t val);

int pn532_update_reg(pn53_dev_t* dev, unsigned addr, uint8_t val, uint8_t mask);

/**
 * @brief   Set new settings for the Security Access Module
 *
 * @param[in]  dev          target device
 * @param[in]  mode         new mode for the SAM
 * @param[in]  timeout      timeout for Virtual Card mode with LSB of 50ms.
 *                          (0 = infinite and 0xFF = 12.75s)
 *
 * @return                  0 on success
 */
ssize_t pn53_sam_configuration(pn53_dev_t* dev, pn53_sam_mode_t mode, uint8_t timeout, bool use_irq);

/**
 * @brief   Get one ISO14443-A passive target
 *
 * This method blocks until a target is detected.
 *
 * @param[in]  dev          target device
 * @param[out] out          target to be stored
 * @param[in]  max_retries  maximum number of attempts to activate a target
 *                          (0xff blocks indefinitely)
 *
 * @return                  0 on success
 * @return                  -1 when no card detected (if non blocking)
 */
int pn532_get_passive_iso14443a(pn53_dev_t* dev, nfc_iso14443a_t *out, unsigned max_retries);

/**
 * @brief   Authenticate a Mifare classic card
 *
 * This operation must be done before reading or writing the segment.
 *
 * @param[in]  dev          target device
 * @param[in]  card         card to use
 * @param[in]  keyid        which key to use
 * @param[in]  key          buffer containing the key
 * @param[in]  block        which block to authenticate
 *
 * @return                  0 on success
 */
// int pn532_mifareclassic_authenticate(pn53_dev_t* dev, nfc_iso14443a_t *card,
//                                     pn532_mifare_key_t keyid, uint8_t *key, unsigned block);

/**
 * @brief   Read a block of a Mifare classic card
 *
 * The block size is 16 bytes and it must be authenticated before read.
 *
 * @param[in]  dev          target device
 * @param[out] odata        buffer where to store the data
 * @param[in]  card         card to use
 * @param[in]  block        which block to read
 *
 * @return                  0 on success
 */
int pn532_mifareclassic_read(pn53_dev_t* dev, uint8_t *odata, nfc_iso14443a_t *card, unsigned block);

/**
 * @brief   Write a block of a Mifare classic card
 *
 * The block size is 16 bytes and it must be authenticated before written.
 *
 * @param[in]  dev          target device
 * @param[in]  idata        buffer containing the data to write
 * @param[in]  card         card to use
 * @param[in]  block        which block to write to
 *
 * @return                  0 on success
 */
int pn532_mifareclassic_write(pn53_dev_t* dev, uint8_t *idata, nfc_iso14443a_t *card, unsigned block);

/**
 * @brief   Read a block of a Mifare Ultralight card
 *
 * The block size is 32 bytes and it must be authenticated before read.
 *
 * @param[in]  dev          target device
 * @param[out] odata        buffer where to store the data
 * @param[in]  card         card to use
 * @param[in]  page         which block to read
 *
 * @return                  0 on success
 */
int pn532_mifareulight_read(pn53_dev_t* dev, uint8_t *odata, nfc_iso14443a_t *card, unsigned page);

/**
 * @brief   Activate the NDEF file of a ISO14443-A Type 4 tag
 *
 * @param[in]  dev          target device
 * @param[in]  card         card to activate
 *
 * @return                  0 on success
 */
int pn532_iso14443a_4_activate(pn53_dev_t* dev, nfc_iso14443a_t *card);

/**
 * @brief   Read data from the NDEF file of a ISO14443-A Type 4 tag
 *
 * The first two bytes of an NDEF file are the length of the data. Afterwards,
 * at offset 0x02 starts the data itself. If one tries to read further than the
 * end of the data no data is returned.
 *
 * @param[in]  dev          target device
 * @param[out] odata        buffer where to store the data
 * @param[in]  card         card to activate
 * @param[in]  offset       offset where to start reading
 * @param[in]  len          length to read
 *
 * @return                  0 on success
 */
int pn532_iso14443a_4_read(pn53_dev_t* dev, uint8_t *odata, nfc_iso14443a_t *card, unsigned offset,
                           uint8_t len);

/**
 * @brief   Deselect a previously selected passive card
 *
 * @param[in]  dev          target device
 * @param[in] target_id     id of the target to deselect (0x00 for all)
 */
void pn532_deselect_passive(pn53_dev_t* dev);

/**
 * @brief   Release an active passive card
 *
 * @param[in]  dev          target device
 * @param[in] target_id     id of the target to release (0x00 for all)
 */
void pn532_release_passive(pn53_dev_t* dev);

int pn53_set_parameters(pn53_dev_t* dev, uint8_t parameters);

int pn532_poll(nfcdev_t *nfcdev, nfc_listener_config_t *config);

int pn532_poll_a(nfcdev_t *nfcdev, nfc_a_listen_config_t *config);

int pn532_poll_b(nfcdev_t *nfcdev, nfc_b_listener_config_t *config);

int pn532_poll_f(nfcdev_t *nfcdev, nfc_f_listener_config_t *config);

int pn532_initiator_exchange_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len,
                                  uint8_t *rcv, size_t *receive_len);

int pn532_target_exchange_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len,
                               uint8_t *rcv, size_t *receive_len);

int pn532_target_receive_data(nfcdev_t *nfcdev, uint8_t *rcv, size_t *receive_len);

int pn532_target_send_data(nfcdev_t *nfcdev, const uint8_t *send, size_t send_len);

int pn532_listen_a(nfcdev_t *nfcdev, const nfc_a_listen_config_t *config);

int pn532_mifare_classic_authenticate(nfcdev_t *nfcdev, uint8_t block_number, 
    const nfc_a_id_t *uid, bool is_key_a, const uint8_t *key);

static const nfcdev_ops_t pn532_ops = {
    .init = pn532_init,
    .poll_a = pn532_poll_a,
    .poll_b = pn532_poll_b,
    .poll_f = pn532_poll_f,
    .poll = pn532_poll,
    .listen_a = pn532_listen_a,
    .target_send_data = pn532_target_send_data,
    .target_receive_data = pn532_target_receive_data,
    .initiator_exchange_data = pn532_initiator_exchange_data,
    .mifare_classic_authenticate = pn532_mifare_classic_authenticate,
};

#ifdef __cplusplus
}
#endif

/** @} */
