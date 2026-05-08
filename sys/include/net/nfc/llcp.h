#pragma once
#include "tsrb.h"
#include "board.h"
#include "mutex.h"
#include "byteorder.h"
#include "iolist.h"
#include "sema.h"
#include "net/nfcdev.h"

#if !defined(CONFIG_LLCP_DEBUG) || defined(DOXYGEN)
#  define CONFIG_LLCP_DEBUG 0
#endif

typedef enum __attribute__((packed)) {
    LLCP_PDU_PTYPE_SYMMETRY      = 0x00,
    LLCP_PDU_PTYPE_PAX           = 0x01,
    LLCP_PDU_PTYPE_AGF           = 0x02,
    LLCP_PDU_PTYPE_UI            = 0x03,
    LLCP_PDU_PTYPE_CONNECT       = 0x04,
    LLCP_PDU_PTYPE_DISCONNECT    = 0x05,
    LLCP_PDU_PTYPE_CC            = 0x06,
    LLCP_PDU_PTYPE_DM            = 0x07,
    LLCP_PDU_PTYPE_FRMR          = 0x08,
    LLCP_PDU_PTYPE_I             = 0x0C, // with sequence number
    LLCP_PDU_PTYPE_RR            = 0x0D, // with sequence number
    LLCP_PDU_PTYPE_RNR           = 0x0E, // with sequence number
} nfc_llcp_pdu_type_t;

/**
 * Connectionless PDU format
 * -----------------------------------------
 * | DSAP    PTYPE  SSAP     Information...|
 * | DDDDDD PP | PP SSSSSS | IIIIIIII | ...|
 * -----------------------------------------
 */

typedef union __attribute__((packed)) {
    struct {
        uint8_t ssap  : 6;
        nfc_llcp_pdu_type_t ptype : 4;
        uint8_t dsap  : 6;
    } __attribute__((packed));
    uint16_t raw;
} nfc_llcp_header_t;

static inline uint8_t nfc_llcp_pdu_get_dsap(const uint8_t *pdu) {
    return (pdu[0] >> 2) & 0x3F;
}

static inline uint8_t nfc_llcp_pdu_get_ssap(const uint8_t *pdu) {
    return pdu[1] & 0x3F;
}

static inline uint8_t nfc_llcp_pdu_get_ptype(const uint8_t *pdu) {
    return ((pdu[0] & 0x03) << 2) | ((pdu[1] >> 6) & 0x03);
}

#define LLCP_PDU_SYMM         0x0000

#define LLCP_LTO_DEFAULT_MS    (100u)
#define LLCP_MIU_DEFAULT       (128u)

#define LLCP_MAX_PDU_SIZE      (LLCP_MIU_DEFAULT)
#define LLCP_SOCKET_RX_BUFFER_SIZE  (LLCP_MAX_PDU_SIZE * 2)
#define LLCP_SOCKET_TX_BUFFER_SIZE  (LLCP_MAX_PDU_SIZE * 2)

#define LLCP_CONTROLLER_MAX_SOCKETS  4

#if !defined(CONFIG_LLCP_THREAD_SIZE) || defined(DOXYGEN)
#  define CONFIG_LLCP_THREAD_SIZE (4000)
#endif

#if !defined(CONFIG_LLCP_RECEIVE_BUSY_FREE_BYTES_THRESHOLD) || defined(DOXYGEN)
#  define CONFIG_LLCP_RECEIVE_BUSY_FREE_BYTES_THRESHOLD (1)
#endif

#define LLCP_BUSY_FREE_BYTES_THRESHOLD \
    (LLCP_MAX_PDU_SIZE - sizeof(nfc_llcp_header_t) - CONFIG_LLCP_RECEIVE_BUSY_FREE_BYTES_THRESHOLD)

typedef enum {
    LLCP_SOCKET_EVENT_CONNECTED,
    LLCP_SOCKET_EVENT_DISCONNECTED,
    LLCP_SOCKET_EVENT_RX,
    LLCP_SOCKET_EVENT_TX,
} nfc_llcp_socket_event_t;

typedef enum {
    LLCP_SOCKET_MODE_UNCONNECTED = 0,
    LLCP_SOCKET_MODE_ACCEPTING = 1,
    LLCP_SOCKET_MODE_CONNECTING = (1 | 4),
} nfc_llcp_socket_mode_t;

struct nfc_llcp_socket;

typedef struct nfc_llcp_socket {
    const char* service_name;
    void (*event_callback)(struct nfc_llcp_socket* socket, nfc_llcp_socket_event_t event, const iolist_t* payload);
    tsrb_t tx_buffer;
    uint8_t tx_buffer_data[LLCP_SOCKET_TX_BUFFER_SIZE];
    uint8_t ssap;
    uint8_t dsap;
    uint8_t state;
    uint8_t vs : 4;
    uint8_t vr : 4;
    uint8_t vsa : 4;
    uint8_t vra : 4;
    bool local_busy : 1;
    bool local_ack : 1;
    bool remote_busy : 1;
    uint8_t remote_receive_window;
} nfc_llcp_socket_t;

static inline bool nfc_llcp_socket_is_connectionless(const nfc_llcp_socket_t* socket) {
    return (socket->state & 1) == 0;
}

typedef struct {
    nfc_llcp_socket_t super;
    tsrb_t rx_buffer;
    uint8_t rx_buffer_data[LLCP_SOCKET_RX_BUFFER_SIZE];
    sema_t rx_sema;
    bool wait_disconnect;
} nfc_llcp_socket_sync_t;

typedef struct {
    char thread_stack[CONFIG_LLCP_THREAD_SIZE];
    kernel_pid_t pid;

    mutex_t sockets_mutex; /* protects the sockets array */
    nfc_llcp_socket_t* sockets[LLCP_CONTROLLER_MAX_SOCKETS];
    size_t socket_count;

    nfcdev_t* dev;
    nfc_role_t mode;
    uint32_t lto_ms;
    uint8_t remote_receive_window;
} nfc_llcp_controller_t;

typedef struct __attribute__((packed)) {
    size_t length;
} nfc_llcp_controller_buffer_header_t;

/* LLCP Controller */
int nfc_llcp_controller_init(nfc_llcp_controller_t* controller, nfcdev_t* dev, uint8_t* parameters, size_t parameter_length);

void nfc_llcp_controller_stop(nfc_llcp_controller_t* controller);

int nfc_llcp_controller_add_socket(nfc_llcp_controller_t* controller, nfc_llcp_socket_t* socket,
                                   nfc_llcp_socket_mode_t mode);

int nfc_llcp_controller_add_socket_sync(nfc_llcp_controller_t* controller, nfc_llcp_socket_sync_t* socket,
                                   nfc_llcp_socket_mode_t mode);

void nfc_llcp_controller_remove_socket(nfc_llcp_controller_t* controller, nfc_llcp_socket_t* socket);

static inline void nfc_llcp_controller_remove_socket_sync(nfc_llcp_controller_t* controller, nfc_llcp_socket_sync_t* socket) {
    return nfc_llcp_controller_remove_socket(controller, &socket->super);
    sema_destroy(&socket->rx_sema);
}

void nfc_llcp_controller_disconnect_socket(nfc_llcp_controller_t* controller, nfc_llcp_socket_t *socket);

void nfc_llcp_controller_disconnect_socket_sync(nfc_llcp_controller_t* controller, nfc_llcp_socket_sync_t* socket);

#define LLCP_TIMEOUT_INFINITE (uint32_t)(-1)

ssize_t nfc_llcp_socket_receive_sync(nfc_llcp_socket_sync_t* socket, uint8_t* payload, size_t capacity, uint32_t timeout_ms);

int nfc_llcp_socket_send_chunks(nfc_llcp_socket_t* socket, const iolist_t* payload);

static inline int nfc_llcp_socket_send_chunks_sync(nfc_llcp_socket_sync_t* socket, const iolist_t* payload) {
    return nfc_llcp_socket_send_chunks(&socket->super, payload);
}

static inline int nfc_llcp_socket_send(nfc_llcp_socket_t* socket, const uint8_t* payload, size_t length) {
    iolist_t _payload = {
        .iol_base = (void*)payload,
        .iol_len = length
    };
    return nfc_llcp_socket_send_chunks(socket, &_payload);
}

static inline int nfc_llcp_socket_send_sync(nfc_llcp_socket_sync_t* socket, const uint8_t* payload, size_t length) {
    return nfc_llcp_socket_send(&socket->super, payload, length);
}

#define LLCP_PARAMETER_TLV_VERSION (1)
#define LLCP_PARAMETER_TLV_MIUX    (2)
#define LLCP_PARAMETER_TLV_WKS     (3)
#define LLCP_PARAMETER_TLV_LTO     (4)
#define LLCP_PARAMETER_TLV_RW      (5)
#define LLCP_PARAMETER_TLV_SN      (6)
#define LLCP_PARAMETER_TLV_OPT     (7)
