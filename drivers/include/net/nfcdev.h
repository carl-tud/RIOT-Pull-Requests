#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "modules.h"
#include "errno.h"
#include "iolist.h"
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

static inline nfc_field_mode_t nfcdev_field_mode_from_radio_config(
   const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role
) {
    if (tx->generate_field && !rx->generate_field) {
        return NFC_FIELD_MODE_PEERS;
    }
    switch (role) {
        case NFC_ROLE_INITIATOR:
            if (tx->generate_field && rx->generate_field) {
                return NFC_FIELD_MODE_READER_WRITER_TAG;
            }
        case NFC_ROLE_TARGET:
            if (!tx->generate_field && !rx->generate_field) {
                return NFC_FIELD_MODE_READER_WRITER_TAG;
            }
        default: break;
    }
    assert(false);
    UNREACHABLE();
    return 0;
}

#define NFCDEV_INTERFACE_ANY (0)

typedef uint8_t nfcdev_connection_id_t;

#define NFCDEV_POLLING_RETRIES_INFINITE (SIZE_MAX)
#define NFCDEV_POLLING_RETRIES_MAX (SIZE_MAX -1)

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

static inline size_t nfcdev_tag_polling_config_frame_count(const nfcdev_tag_polling_config_t* config) {
    return ((struct {
        nfc_technology_t technology;
        struct {
            void* frames;
            size_t frame_count;
        };
    } *)config)->frame_count;
}

static inline bool nfcdev_polling_filter_matches(const nfcdev_tag_polling_config_t* config, const nfc_tag_t* tag) {
    assert(config);
    switch (tag->technology) {
        case NFC_TECHNOLOGY_A: return nfc_a_polling_filter_matches(config->a.filter, &tag->a);
        case NFC_TECHNOLOGY_B: return nfc_b_polling_filter_matches(config->b.filter, &tag->b);
        case NFC_TECHNOLOGY_F: return nfc_f_polling_filter_matches(config->f.filter, &tag->f);
        case NFC_TECHNOLOGY_V: return nfc_v_polling_filter_matches(config->v.filter, &tag->v);
        default: UNREACHABLE();
    }
}

typedef struct {
    /// Ignored if @ref nfcdev_polling_loop_t::technology is @ref NFC_TECHNOLOGY_V
    nfc_bitrate_t bitrate;

    nfcdev_polling_timing_t timing;

    /// Passive tag initialization config
    ///
    /// If @ref nfcdev_polling_loop_t::field_mode is NFC_FIELD_MODE_READER_WRITER_TAG
    const nfcdev_tag_polling_config_t* tag;

    struct {
        struct {
            size_t length;
            const nfc_dep_activation_request_t* atr;
        } nfc_dep;

        nfc_bidirectional_bitrate_selector_t bitrate_selector;
    } higher_layer;

    nfc_field_mode_t field_mode : 1;
} nfcdev_polling_loop_t;

#define NFCDEV_POLLING_REPETIONS_INFINITE (SIZE_MAX)
#define NFCDEV_POLLING_REPETIONS_MAX (SIZE_MAX - 1)

typedef struct {
    size_t loop_count;
    nfcdev_polling_loop_t* loops;
    size_t repetitions;
} nfcdev_polling_config_t;

#if !defined(CONFIG_NFCDEV_POLL_SKIP_UNSUPPORTED_LOOPS) || defined(DOXYGEN)
#  define CONFIG_NFCDEV_POLL_SKIP_UNSUPPORTED_LOOPS 0
#endif

#if !defined(CONFIG_NFCDEV_LISTEN_IGNORE_UNSUPPORTED_CONFIG_ARGUMENTS) || defined(DOXYGEN)
#  define CONFIG_NFCDEV_LISTEN_IGNORE_UNSUPPORTED_CONFIG_ARGUMENTS 1
#endif

#if !defined(CONFIG_NFCDEV_LISTEN_TAG_REQUIRE_ISO_DEP) || defined(DOXYGEN)
#  define CONFIG_NFCDEV_LISTEN_TAG_REQUIRE_ISO_DEP 0
#endif

#if !defined(CONFIG_NFCDEV_LISTEN_TAG_REQUIRE_NFC_DEP) || defined(DOXYGEN)
#  define CONFIG_NFCDEV_LISTEN_TAG_REQUIRE_NFC_DEP 0
#endif

#if IS_ACTIVE(CONFIG_NFCDEV_LISTEN_TAG_REQUIRE_NFC_DEP) && IS_ACTIVE(CONFIG_NFCDEV_LISTEN_TAG_REQUIRE_ISO_DEP)
#  error Must not require both ISO-DEP and NFC-DEP in r/w-tag listen mode
#endif

typedef struct {
    nfc_technology_t technologies;
    nfc_a_tag_t* a;
    nfc_b_tag_t* b;
    nfc_f_tag_t* f;
    nfc_v_tag_t* v;
} nfcdev_tag_listening_config_t;

typedef struct {
    nfcdev_tag_listening_config_t tag;

    struct {
        nfc_dep_target_t* nfc_dep;
        nfc_bidirectional_bitrate_selector_t bitrate_selector;
    } higher_layer;

    struct {
        struct {
            nfc_bitrate_set_t a;
            nfc_bitrate_set_t b;
            nfc_bitrate_set_t f;
        } tag;
        nfc_bitrate_set_t peer;
    } bitrates;
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
    uint8_t trailing_bit_count;
} nfcdev_t;

#define NFCDEV_POLLING_STOP (-60000)
#define NFCDEV_POLLING_CONTINUE (0)

#define NFCDEV_FLAG_REASSEMBLE (1 << 3)
#define NFCDEV_FLAG_SLICE (1 << 4)
#define NFCDEV_FLAG_USE_NODE_ADDRESS (1 << 5)
#define NFCDEV_FLAG_USE_DEVICE_ID (1 << 6)

#define _NFCDEV_MASK_INTERFACE (0b111)
#define _NFCDEV_MASK_TRANSPORT (~0b111)

typedef struct nfcdev_ops {
    int (*configure_radio)(nfcdev_t* dev, const nfcdev_radio_config_t* tx, const nfcdev_radio_config_t* rx, nfc_role_t role);

    void (*inquire)(nfcdev_t* dev, nfcdev_interface_t* available_interfaces, nfc_role_t* role);
    int (*connect)(nfcdev_t* dev, nfcdev_connection_id_t connection_id);
    int (*disconnect)(nfcdev_t* dev, nfcdev_connection_id_t connection_id);

    // nfcdev NFIO (Near Field Input/Output)

    int (*send)(
        nfcdev_t* dev,
        const iolist_t* tx,
        nfcdev_nfio_flags_t flags);

    ssize_t (*receive)(
        nfcdev_t* dev,
        uint8_t** rx, size_t capacity, uint32_t rx_timeout_ms,
        nfcdev_nfio_flags_t flags);

    ssize_t (*transceive)(
        nfcdev_t* dev,
        const iolist_t* tx,
        uint8_t** rx, size_t capacity, uint32_t rx_timeout_ms,
        nfcdev_nfio_flags_t flags);

    // --

    ssize_t (*poll) (nfcdev_t* dev, const nfcdev_polling_config_t* config, nfc_target_t* targets, nfcdev_connection_id_t* connection_ids, size_t max_targets);
    int (*listen) (nfcdev_t* dev, const nfcdev_listening_config_t* config, nfc_target_t* target, uint32_t timeout_ms);
} nfcdev_ops_t;

static inline nfcdev_interface_t nfcdev_available_interfaces(nfcdev_t* dev) {
    assert(dev);
    assert(dev->ops->inquire);
    nfcdev_interface_t interfaces = 0;
    dev->ops->inquire(dev, &interfaces, NULL);
    return interfaces;
}

static inline nfc_role_t nfcdev_role(nfcdev_t* dev) {
    assert(dev);
    assert(dev->ops->inquire);
    nfc_role_t role;
    dev->ops->inquire(dev, NULL, &role);
    return role;
}

static inline ssize_t nfcdev_poll(nfcdev_t* dev, const nfcdev_polling_config_t* config, nfc_target_t* targets, nfcdev_connection_id_t* connection_ids, size_t max_targets) {
    assert(dev);
    assert(config);
    assert(max_targets > 0);
    assert(targets);
    assert(dev->ops->poll);
    return dev->ops->poll(dev, config, targets, connection_ids, max_targets);
}

#define NFCDEV_CONNECTION_ID_CURRENT (0)

static inline int nfcdev_connect(nfcdev_t* dev, nfcdev_connection_id_t connection_id) {
    assert(dev);
    return dev->ops->connect ? dev->ops->connect(dev, connection_id) : -ENOTSUP;
}

static inline int nfcdev_disconnect(nfcdev_t* dev, nfcdev_connection_id_t connection_id) {
    assert(dev);
    return dev->ops->disconnect ? dev->ops->disconnect(dev, connection_id) : -ENOTSUP;
}

#define _NFCDEV_DISCONNECT_ALL (0xff)

static inline int nfcdev_disconnect_all(nfcdev_t* dev) {
    assert(dev);
    return dev->ops->disconnect ? dev->ops->disconnect(dev, _NFCDEV_DISCONNECT_ALL) : -ENOTSUP;
}

static inline int nfcdev_listen(nfcdev_t* dev, const nfcdev_listening_config_t* config, nfc_target_t* target, uint32_t timeout_ms) {
    assert(dev);
    assert(config);
    assert(dev->ops->listen);
    return dev->ops->listen(dev, config, target, timeout_ms);
}

static inline nfcdev_frame_length_t __frame_length_size(size_t length) {
    assert(length <= NFCDEV_FRAME_LENGTH_BYTES_MAX);
    return (nfcdev_frame_length_t) { .bytes = length };
}

static inline nfcdev_frame_length_t __frame_length_typed(nfcdev_frame_length_t length) {
    return length;
}

static inline size_t __size_from_frame_length(nfcdev_frame_length_t length) {
    return length.bytes;
}

static inline size_t __size_from_size(size_t length) {
    return length;
}

static inline uint8_t __trailing_bits_from_frame_length(nfcdev_frame_length_t length) {
    return length.bytes;
}

static inline uint8_t __trailing_bits_from_size(size_t length) {
    (void)length;
    return 0;
}

#define __as_frame_length(length) _Generic((length), \
    nfcdev_frame_length_t: __frame_length_typed, \
    size_t:                __frame_length_size, \
    int:                   __frame_length_size \
)(length)

#define __to_frame_length_bytes(length) _Generic((length), \
    nfcdev_frame_length_t: __size_from_frame_length, \
    size_t:                __size_from_size, \
    int:                   __size_from_size \
)(length)

#define __to_frame_length_bits(length) _Generic((length), \
    nfcdev_frame_length_t: __trailing_bits_from_frame_length, \
    size_t:                __trailing_bits_from_size, \
    int:                   __trailing_bits_from_size \
)(length)

#define __as_frame_length_ref(length) _Generic((length), \
    nfcdev_frame_length_t*: (length), \
    void*:                  (nfcdev_frame_length_t*)(length), \
    default:                &(nfcdev_frame_length_t){ .bytes = (size_t)(length) } \
)

#define __as_buffer_ref(ptr) _Generic((ptr), \
    uint8_t**: (ptr), \
    uint8_t*:  &(uint8_t*){ (uint8_t*)(uintptr_t)(ptr) }, \
    default:   (uint8_t**)(uintptr_t)(ptr) \
)

#define __typecheck_interface(interface) _Generic((interface), \
    nfcdev_interface_t: (nfcdev_interface_t)interface, \
    int: (nfcdev_interface_t)interface \
)


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

static inline ssize_t __nfcdev_send__impl__(nfcdev_t* dev, const iolist_t* tx, nfcdev_nfio_flags_t flags) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->send);
    assert(tx);
    assert(tx->iol_len > 0);

    return dev->ops->send(dev, tx, flags);
}

#define nfcdev_send_chunks(dev, tx, flags) \
    __nfcdev_send__impl__(dev, \
        tx, \
        ((nfcdev_nfio_flags_t) { \
            .interface = (flags) & _NFCDEV_MASK_INTERFACE, \
            .use_nad = (((flags) & NFCDEV_FLAG_USE_NODE_ADDRESS) != 0), \
            .use_did = (((flags) & NFCDEV_FLAG_USE_DEVICE_ID) != 0), \
            .slice = (((flags) & NFCDEV_FLAG_SLICE) != 0) \
        }) \
    )

#define nfcdev_send(dev, tx, tx_length, flags) \
    __nfcdev_send__impl__(dev, \
        (&(iolist_t){ \
            .iol_base = (void*)(tx), \
            .iol_len = (size_t)(__to_frame_length_bytes(tx_length)) \
        }), \
        ((nfcdev_nfio_flags_t) { \
            .interface = (flags) & _NFCDEV_MASK_INTERFACE, \
            .slice = (((flags) & NFCDEV_FLAG_SLICE) != 0), \
            .use_nad = (((flags) & NFCDEV_FLAG_USE_NODE_ADDRESS) != 0), \
            .use_did = (((flags) & NFCDEV_FLAG_USE_DEVICE_ID) != 0), \
            .trailing_bits = __to_frame_length_bits(tx_length) \
        }) \
    )

static inline ssize_t __nfcdev_receive__impl(nfcdev_t* dev,
    uint8_t** rx, size_t capacity, uint32_t rx_timeout_ms,
    nfcdev_nfio_flags_t flags
) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->receive);
    // Either you give me a buffer and a capacity OR no buffer and capacity of zero.
    assert(((capacity > 0) && rx && *rx) || capacity == 0);

    return dev->ops->receive(dev, rx, capacity, rx_timeout_ms, flags);
}

#define nfcdev_receive(dev, rx, capacity, rx_timeout_ms, flags) \
    __nfcdev_receive__impl(dev, \
        __as_buffer_ref(rx), capacity, rx_timeout_ms, \
        ((nfcdev_nfio_flags_t) { \
            .interface = (flags) & _NFCDEV_MASK_INTERFACE, \
            .use_nad = (((flags) & NFCDEV_FLAG_USE_NODE_ADDRESS) != 0), \
            .use_did = (((flags) & NFCDEV_FLAG_USE_DEVICE_ID) != 0), \
            .reassemble = (((flags) & NFCDEV_FLAG_REASSEMBLE) != 0) \
        }) \
    )

#include <stdio.h>
#include "pn53x.h"
#include "architecture.h"

static inline ssize_t __nfcdev_transceive__impl__(nfcdev_t* dev,
    const iolist_t* tx,
    uint8_t** rx, size_t capacity, uint32_t rx_timeout_ms,
    nfcdev_nfio_flags_t flags
) {
    assert(dev);
    assert(dev->ops);
    assert(dev->ops->transceive);
    // Either you give me a buffer and a capacity OR no buffer and capacity of zero.
    assert(((capacity > 0) && rx && *rx) || capacity == 0);
    return dev->ops->transceive(dev, tx, rx, capacity, rx_timeout_ms, flags);
}

#define nfcdev_transceive_chunks(dev, tx, rx, capacity, rx_timeout_ms, flags) \
    __nfcdev_transceive__impl__(dev, \
        tx, \
        __as_buffer_ref(rx), capacity, rx_timeout_ms, \
        ((nfcdev_nfio_flags_t) { \
            .interface = (_flags) & _NFCDEV_MASK_INTERFACE, \
            .reassemble = ((_flags) & NFCDEV_FLAG_REASSEMBLE != 0), \
            .slice = ((_flags) & NFCDEV_FLAG_SLICE != 0) \
        }) \
    )

#define nfcdev_transceive(dev, tx, tx_length, rx, rx_capacity, rx_timeout_ms, flags) \
    __nfcdev_transceive__impl__(dev, \
        (&(iolist_t){ \
            .iol_base = (void*)(tx), \
            .iol_len = (size_t)(__to_frame_length_bytes(tx_length)) \
        }), \
        __as_buffer_ref(rx), rx_capacity, rx_timeout_ms, \
        ((nfcdev_nfio_flags_t) { \
            .interface = (flags) & _NFCDEV_MASK_INTERFACE, \
            .reassemble = (((flags) & NFCDEV_FLAG_REASSEMBLE) != 0), \
            .slice = (((flags) & NFCDEV_FLAG_SLICE) != 0), \
            .trailing_bits = __to_frame_length_bits(tx_length) \
        }) \
    )
