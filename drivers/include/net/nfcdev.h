#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "net/nfc.h"

// might need different strategies: should controller directly respond if technology is polled,
// or callback-based

typedef enum {
    NFCDEV_STATE_UNINITIALIZED = 0,
    NFCDEV_STATE_DISABLED,
    NFCDEV_STATE_IDLE,
    NFCDEV_STATE_POLLING,
    NFCDEV_STATE_LISTENING,
    NFCDEV_STATE_NFC_DEP_CONNECTED,
} nfcdev_state_t;


typedef enum {
    NFCDEV_RX_MULTIPLE = 1,
    NFCDEV_RX_MALFORMED = 1 << 1,
    NFCDEV_NFC_B_WITHOUT_SOF = 1 << 2,
    NFCDEV_NFC_B_WITHOUT_EOF = 1 << 3,
} nfcdev_radio_options_t;

typedef struct {
    nfc_bitrate_t bitrate;

    /// Modulation/demodulation, framing, receiver/transmitter analog presets
    nfc_technology_t technology;

    nfcdev_radio_options_t options;
    bool generate_field : 1;
} nfcdev_radio_config_t;

static inline nfc_field_model_t nfcdev_field_model_from_radio_config(
   const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role
) {
    if (tx->generate_field && !rx->generate_field) {
        return NFC_FIELD_MODEL_PEERS;
    }
    switch (role) {
        case NFC_ROLE_INITIATOR:
            if (tx->generate_field && rx->generate_field) {
                return NFC_FIELD_MODEL_READER_WRITER_TAG;
            }
        case NFC_ROLE_TARGET:
            if (!tx->generate_field && !rx->generate_field) {
                return NFC_FIELD_MODEL_READER_WRITER_TAG;
            }
        default: break;
    }
    assert(false);
    UNREACHABLE();
    return 0;
}

typedef enum __attribute__((packed)) {
    /// Without NFC-A parity bits, otherwise same as @ref NFCDEV_INTERFACE_FRAME
    NFCDEV_INTERFACE_BITS = 1,

    /// Without length, CRC, headers, trailers etc, any protocol unmanaged
    NFCDEV_INTERFACE_FRAME = 1 << 1,

    /// With length, CRC, etc..., any protocol unmanaged
    NFCDEV_INTERFACE_PACKET = 1 << 2,

    /// ISO-DEP payloads, protocol managedd
    NFCDEV_INTERFACE_ISO_DEP = 1 << 3,

    /// NFC-DEP payloads, protocol managed
    NFCDEV_INTERFACE_NFC_DEP = 1 << 4,
} nfcdev_interface_t;

#define NFCDEV_INTERFACE_ANY (0)

typedef uint8_t nfcdev_connection_id_t;

#define NFCDEV_POLLING_RETRIES_INFINITE (0)
#define NFCDEV_POLLING_RETRIES_MAX SIZE_MAX

#define NFCDEV_POLLING_INTERVAL_BUILTIN (0)

typedef struct {
    uint32_t interval;

    size_t retries;

    uint32_t guard_time;
} nfcdev_polling_timing_t;

typedef struct {
    nfc_technology_t technology;

    union {
        nfc_a_tag_polling_config_t a;
        nfc_b_tag_polling_config_t b;
        nfc_f_tag_polling_config_t f;
        nfc_v_tag_polling_config_t v;
    };
} nfcdev_tag_polling_config_t;

typedef struct {
    /// Ignored if @ref nfcdev_polling_loop_t::technology is @ref NFC_TECHNOLOGY_V
    nfc_bitrate_t bitrate;

    nfcdev_polling_timing_t timing;

    /// Passive tag initialization config
    ///
    /// If @ref nfcdev_polling_loop_t::field_model is NFC_FIELD_MODEL_READER_WRITER_TAG
    nfcdev_tag_polling_config_t* tag;

    struct {
        struct {
            size_t atr_length;
            nfc_dep_activation_request_t* atr;
        } nfc_dep;
    } higher_layer;

    nfc_field_model_t field_model : 1;
} nfcdev_polling_loop_t;

typedef struct {
    size_t loop_count;
    nfcdev_polling_loop_t* loops;
    size_t max_targets;
} nfcdev_polling_config_t;

typedef struct {
    uint32_t duration;

    uint32_t guard_time;
} nfcdev_listening_timing_t;

typedef struct {
    nfcdev_listening_timing_t timing;

    nfc_target_t* target;
} nfcdev_listening_phase_t;

typedef struct {
    size_t loop_count;
    nfcdev_listening_phase_t* phase;
} nfcdev_listening_config_t;

// might need different strategies: should controller directly further connect if technology is is found,
// or callback-based

static inline bool nfc_polling_loop_is_valid(nfcdev_polling_loop_t* loop) {
    assert(loop);
    if (loop->tag) {
        switch (loop->tag->technology) {
            case NFC_TECHNOLOGY_A:
            case NFC_TECHNOLOGY_B:
                return loop->bitrate == NFC_BITRATE_106K;
                break;
            case NFC_TECHNOLOGY_F:
                return (loop->bitrate == NFC_BITRATE_212K) || (loop->bitrate == NFC_BITRATE_424K) == 0;
                break;
            case NFC_TECHNOLOGY_V:
                return loop->bitrate == NFC_BITRATE_UNSET;
            default:
                return false;
        }
    }
}

struct nfcdev_ops;

typedef struct nfcdev {
    void* dev; /**< Pointer to the device specific structure */
    void* config; /**< Pointer to the device specific configuration structure */
    const struct nfcdev_ops* ops; /**< Pointer to the device operations */
    nfcdev_state_t state; /**< Current state of the NFC device */
} nfcdev_t;

#define NFCDEV_POLLING_STOP (-60000)
#define NFCDEV_POLLING_CONTINUE (0)

typedef struct nfcdev_ops {
    int (*init)(nfcdev_t* dev, const void* dev_config);
    int (*deinit)(nfcdev_t* dev);



    int (*configure_radio)(nfcdev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role);

    nfcdev_interface_t (*available_interfaces)(nfcdev_t* dev);
    int (*connect)(nfcdev_t* dev, nfcdev_connection_id_t connection_id);
    int (*disconnect)(nfcdev_t* dev, nfcdev_connection_id_t connection_id);
    
    // nfcdev NFIO (Near Field Input/Output)

    int (*send)(
        nfcdev_t* dev,
        const uint8_t* tx, nfcdev_frame_length_t tx_length,
        nfcdev_interface_t interface);

    ssize_t (*receive)(
        nfcdev_t* dev,
        uint8_t** rx, nfcdev_frame_length_t* rx_length, uint32_t rx_timeout_ms,
        nfcdev_interface_t interface);

    ssize_t (*transceive)(
        nfcdev_t* dev,
        const uint8_t* tx, nfcdev_frame_length_t tx_length,
        uint8_t** rx, nfcdev_frame_length_t* rx_length, uint32_t rx_timeout_ms,
        nfcdev_interface_t interface);

    // --

    int (*poll) (nfcdev_t* dev, const nfcdev_polling_config_t* config, nfc_target_t* targets, size_t max_targets);
    int (*listen) (nfcdev_t* dev, const nfcdev_listening_config_t* config);
} nfcdev_ops_t;

// If you have a driver that only supports a subset of these functions, please create an issue.
// We can make some of the shims below return -ENOTSUP if the relevant operation is not implement,
// i.e., if dev->ops->op_in_question is NULL.

static inline int nfcdev_configure_radio(nfcdev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role) {
    assert(dev);
    assert(dev->ops);
    assert(tx);
    assert(rx);
    return dev->ops->configure_radio ? dev->ops->configure_radio(dev, tx, rx, role) : -ENOTSUP;
}

static inline int nfcdev_configure_radio_passive(nfcdev_t* dev, const nfcdev_radio_config_t* config, nfc_role_t role) {
    return nfcdev_configure_radio(dev, config, config, role);
}

static inline nfcdev_interface_t nfcdev_available_interface(nfcdev_t* dev) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->available_interfaces);
    return dev->ops->available_interfaces(dev);
}

static inline ssize_t nfcdev_send(nfcdev_t* dev,
    const uint8_t* tx, nfcdev_frame_length_t tx_length,
    nfcdev_interface_t interface
) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->send);
    assert(tx);
    assert(tx_length.bytes > 0);

    return dev->ops->send(dev,
        tx, tx_length,
        interface
    );
}

static inline ssize_t nfcdev_send_bytes(nfcdev_t* dev,
    const uint8_t* tx, size_t tx_length,
    nfcdev_interface_t interface
) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->send);
    assert(tx);
    assert(tx_length > 0);
    assert(tx_length <= NFCDEV_FRAME_LENGTH_BYTES_MAX);

    return nfcdev_send(dev,
        tx, (nfcdev_frame_length_t) { .bytes = tx_length },
        interface
    );
}

static inline ssize_t nfcdev_receive(nfcdev_t* dev,
    uint8_t** rx, nfcdev_frame_length_t* rx_length, uint32_t rx_timeout_ms,
    nfcdev_interface_t interface
) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->transceive);
    // Either you give me a buffer and a capacity OR no buffer and capacity of zero.
    assert((rx_length && rx_length->bytes > 0 && rx && *rx) || ((!rx_length || rx_length->bytes == 0) && (!rx || !*rx)));

    return dev->ops->receive(dev,
        rx, rx_length, rx_timeout_ms,
        interface
    );
}

static inline ssize_t nfcdev_receive_bytes(nfcdev_t* dev,
    uint8_t** rx, size_t rx_capacity, uint32_t rx_timeout_ms,
    nfcdev_interface_t interface
) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->transceive);
    assert(rx_capacity <= NFCDEV_FRAME_LENGTH_BYTES_MAX);
    assert((rx_capacity > 0 && rx && *rx) || (rx_capacity == 0 && (!rx || !*rx)));

    nfcdev_frame_length_t rx_length = { .bytes = rx_capacity };
    return nfcdev_receive(dev,
        rx, &rx_length, rx_timeout_ms,
        interface
    );
}

static inline ssize_t nfcdev_transceive(nfcdev_t* dev,
    const uint8_t* tx, nfcdev_frame_length_t tx_length,
    uint8_t** rx, nfcdev_frame_length_t* rx_length, uint32_t rx_timeout_ms,
    nfcdev_interface_t interface
) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->transceive);
    assert(tx);
    assert(tx_length.bytes > 0);
    // Either you give me a buffer and a capacity OR no buffer and capacity of zero.
    assert((rx_length && rx_length->bytes > 0 && rx && *rx) || ((!rx_length || rx_length->bytes == 0) && (!rx || !*rx)));

    return dev->ops->transceive(dev,
        tx, tx_length,
        rx, rx_length, rx_timeout_ms,
        interface
    );
}

static inline ssize_t nfcdev_transceive_bytes(nfcdev_t* dev,
    const uint8_t* tx, size_t tx_length,
    uint8_t** rx, size_t rx_capacity, uint32_t rx_timeout_ms,
    nfcdev_interface_t interface
) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->transceive);
    assert(tx);
    assert(tx_length > 0);
    assert(tx_length <= NFCDEV_FRAME_LENGTH_BYTES_MAX);
    assert(rx_capacity <= NFCDEV_FRAME_LENGTH_BYTES_MAX);
    assert((rx_capacity > 0 && rx && *rx) || (rx_capacity == 0 && (!rx || !*rx)));

    nfcdev_frame_length_t rx_length = { .bytes = rx_capacity };
    return nfcdev_transceive(dev,
        tx, (nfcdev_frame_length_t) { .bytes = tx_length },
        rx, &rx_length, rx_timeout_ms,
        interface
    );
}


