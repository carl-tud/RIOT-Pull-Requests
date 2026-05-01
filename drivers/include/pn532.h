#pragma once
#include <stdint.h>
#include "pn53x.h"

typedef pn53_dev_t pn532_dev_t;
typedef pn53_connection_config_t pn532_connection_config_t;
typedef pn53_sam_mode_t pn532_sam_mode_t;

#define PN532_PACKET_LENGTH_MAX  (265)

int pn532_init(pn532_dev_t* dev, const pn532_connection_config_t* config);

static inline int nfcdev_init_pn532(nfcdev_t* nfcdev, pn532_dev_t* dev, const pn532_connection_config_t* config) {
    extern nfcdev_ops_t nfcdev_ops_pn53;
    nfcdev->ops = &nfcdev_ops_pn53;
    nfcdev->dev = dev;
    return pn532_init(dev, config);
}
typedef enum __attribute__((packed)) {
    PN532_COMMAND_DIAGNOSE = 0,
    PN532_COMMAND_GET_FIRMWARE_VERSION = 2,
    PN532_COMMAND_READ_REGISTERS = 6,
    PN532_COMMAND_WRITE_REGISTERS = 8,
    PN532_COMMAND_SET_SERIAL_BAUDRATE = 0x10,
    PN532_COMMAND_SET_PARAMETERS = 0x12,
    PN532_COMMAND_SAM_CONFIGURATION = 0x14,
    PN532_COMMAND_POWER_DOWN = 0x16,
} pn532_command_code_t;

typedef enum __attribute__((packed)) {
    /// Use of the NAD information in case of initiator configuration (ISO-DEP and ISO-DEP initiator)
    PN532_NFC_PARAMETER_INITIATOR_USE_NAD = 1,

    /// Use of the DID information in case of initiator configuration (or CID in case of ISO-DEP initiator configuration)
    PN532_NFC_PARAMETER_INITIATOR_USE_CID = 1 << 1,

    /// Automatic generation of `RATS`
    ///
    /// The `SAK`/Select Acknowledge byte is automatically checked for ISO/IEC14443-4 aka. T=CL support.
    /// If the target supports the T=CL protocol, a `RATS` command is sent to the target, activating the T=CL protocol
    ///
    /// - Note: If ATS is sent by the target upon receiving an `RATS`, the target agrees to use the T=CL protocol aka. ISO/IEC14443 A - 4 protocol further on.
    /// - Note: To use automatic protocol management features (chaining, waiting time extension, error handling),
    /// use `InDataExchange` to send commands to the target.
    PN532_NFC_PARAMETER_INITIATOR_ISO_DEP_AUTO_HANDSHAKE = 1 << 4,

    /// Automatic generation of the `ATR_RES` in target configuration
    PN532_NFC_PARAMETER_TARGET_NFC_DEP_AUTO_HANDSHAKE = 1 << 2,

    /// The emulation of a ISO/IEC14443-4 PICC is enabled
    ///
    /// If enabled, the PN53x automatically sends an `ATS` after receiving an `RATS`.
    /// This option also enables ISO/IEC14443-4 protocol management functionality (S blocks, R blocks, I blocks, chaining, error handling),
    /// thus `TgGetData` and `TgSetData` can be used to retrieve commands sent to the PN53x as target or send responses
    /// without the need to handle ISO/IEC14443-4 protocol features.
    ///
    /// - Note: Only historical bytes in the `ATS` can be customized.
    /// - Note: If ATS is sent by the target upon receiving an `RATS`, the target agrees to use the T=CL protocol aka. ISO/IEC14443 A - 4 protocol further on.
    PN532_NFC_PARAMETER_TARGET_ISO_DEP_AUTO_HANDSHAKE = 1 << 5,

    /// The PN532 does not send Preamble and Postamble
    PN532_NFC_PARAMETER_SUPPRESS_PREAMBLE_POSTAMBLE = 1 << 6,
} pn532_nfc_parameters_t;

#define PN532_NFC_PARAMETER_MASK ( \
    PN532_NFC_PARAMETER_INITIATOR_USE_NAD | \
    PN532_NFC_PARAMETER_INITIATOR_USE_CID | \
    PN532_NFC_PARAMETER_INITIATOR_ISO_DEP_AUTO_HANDSHAKE | \
    PN532_NFC_PARAMETER_TARGET_NFC_DEP_AUTO_HANDSHAKE | \
    PN532_NFC_PARAMETER_TARGET_ISO_DEP_AUTO_HANDSHAKE | \
    PN532_NFC_PARAMETER_SUPPRESS_PREAMBLE_POSTAMBLE \
)

static inline int pn532_set_parameters(pn532_dev_t* dev, pn532_nfc_parameters_t parameters) {
    assert((parameters & ~PN532_NFC_PARAMETER_MASK) == 0);
    return pn53_set_parameters((pn53_dev_t*)dev, (uint8_t)parameters);
}

static inline int pn532_enable_parameters(pn532_dev_t* dev, pn532_nfc_parameters_t parameters) {
    assert((parameters & ~PN532_NFC_PARAMETER_MASK) == 0);
    return pn53_enable_parameters((pn53_dev_t*)dev, (uint8_t)parameters);
}

static inline int pn532_set_parameters_enablement(pn532_dev_t* dev, pn532_nfc_parameters_t parameters, bool enablement) {
    assert((parameters & ~PN532_NFC_PARAMETER_MASK) == 0);
    return pn53_set_parameters_enablement((pn53_dev_t*)dev, (uint8_t)parameters, enablement);
}

static inline int pn532_disable_parameters(pn532_dev_t* dev, pn532_nfc_parameters_t parameters) {
    assert((parameters & ~PN532_NFC_PARAMETER_MASK) == 0);
    return pn53_disable_parameters(dev, (uint8_t)parameters);
}

ssize_t pn532_sam_configuration(pn532_dev_t* dev, pn532_sam_mode_t mode, uint8_t timeout, bool use_irq);

ssize_t pn532_set_uart_speed(pn532_dev_t* dev, pn53_uart_speed_t speed);

typedef enum __attribute__((packed)) {
    PN532_WAKEUP_SOURCE_INT0 = 1,
    PN532_WAKEUP_SOURCE_INT1 = 1 << 1,
    PN532_WAKEUP_SOURCE_RF = 1 << 3,
    PN532_WAKEUP_SOURCE_UART = 1 << 4,
    PN532_WAKEUP_SOURCE_SPI = 1 << 5,
    PN532_WAKEUP_SOURCE_GPIO = 1 << 6,
    PN532_WAKEUP_SOURCE_I2C = 1 << 7,
} pn532_wakeup_sources_t;

int pn532_power_down(pn532_dev_t* dev, pn532_wakeup_sources_t wakeup_sources, bool generate_irq);

#define PN532_TARGETS_MAX (2)

typedef enum __attribute__((packed)) {
    PN532_IN_AUTO_POLL_CONSTRAINT_ONLY_A_OR_F = 1,
    PN532_IN_AUTO_POLL_CONSTRAINT_ONLY_ISO_DEP = 1 << 1,
    PN532_IN_AUTO_POLL_CONSTRAINT_ONLY_NFC_DEP = 1 << 2,
    PN532_IN_AUTO_POLL_CONSTRAINT_ONLY_NFC_DEP_IN_PEER_FIELD_MODEL = 1 << 3,
} pn532_in_auto_poll_constraint_t;

typedef struct __attribute__((packed)) {
    pn53_technology_baudrate_t brty : 4;
    pn532_in_auto_poll_constraint_t constraint : 4;
} pn532_in_auto_poll_type_t;

extern const nfc_a_ats_t pn532_builtin_ats;
