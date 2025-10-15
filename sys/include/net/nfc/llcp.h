#pragma once
#include "net/nfcdev.h"
#include "sys/tsrb.h"
#include "event_queue.h"

#define LLCP_PDU_PTYPE_SYMM 0x00
#define LLCP_PDU_PTYPE_PAX  0x01
#define LLCP_PDU_PTYPE_AGF  0x02
#define LLCP_PDU_PTYPE_UI   0x03
#define LLCP_PDU_PTYPE_CONNECT 0x04
#define LLCP_PDU_PTYPE_DISC    0x05
#define LLCP_PDU_PTYPE_CC     0x06
#define LLCP_PDU_PTYPE_DM     0x07
#define LLCP_PDU_PTYPE_FRMR   0x08
#define LLCP_PDU_PTYPE_I      0x0C
#define LLCP_PDU_PTYPE_RR     0x0D
#define LLCP_PDU_PTYPE_RNR    0x0E

#define LLCP_PDU_SYMM         0x0000

#define LLCP_LTO_DEFAULT_MS    (100u)
#define LLCP_MIU_DEFAULT       (128u)

#define LLCP_CONTROLLER_MAX_SOCKETS  4

typedef enum {
    LLCP_SOCKET_MODE_CONNECTIONLESS = 0,
    LLCP_SOCKET_MODE_CONNECTION_ORIENTED,
} nfc_llcp_socket_mode_t;

typedef struct {
    tsrb_t rx_buffer;
    tsrb_t tx_buffer;
    uint8_t rx_buffer_data[LLCP_MAX_PDU_SIZE * 2];
    uint8_t tx_buffer_data[LLCP_MAX_PDU_SIZE * 2];
    uint8_t ssap;
    uint8_t dsap;
} nfc_llcp_socket_t;

typedef struct {
    uint8_t thread_stack[THREAD_STACKSIZE_DEFAULT];
    kernel_pid_t pid;

    event_queue_t *evq;

    mutex_t sockets_mutex; /* protects the sockets array */
    llcp_socket_t sockets[LLCP_CONTROLLER_MAX_SOCKETS];
    size_t socket_count;

    nfcdev_t *dev;
    nfcdev_mode_t mode;
} nfc_llcp_controller_t;

/**
 * Connectionless PDU format
 * -----------------------------------------
 * | DSAP    PTYPE  SSAP     Information...|
 * | DDDDDD PP | PP SSSSSS | IIIIIIII | ...|
 * -----------------------------------------
 */

 /* LLCP PDU Accessors */
uint8_t nfc_llcp_pdu_get_dsap(const uint8_t *pdu);

uint8_t nfc_llcp_pdu_get_ssap(const uint8_t *pdu);

uint8_t nfc_llcp_pdu_get_ptype(const uint8_t *pdu);

/* LLCP Controller */
int nfc_llcp_controller_init(nfc_llcp_controller_t *controller, nfcdev_t *dev, nfcdev_mode_t mode);

void nfc_llcp_controller_stop(nfc_llcp_controller_t *controller)

void nfc_llcp_controller_add_socket(nfc_llcp_controller_t *controller, nfc_llcp_socket_t *socket)

void nfc_llcp_controller_remove_socket(nfc_llcp_controller_t *controller, nfc_llcp_socket_t *socket)

/* LLCP Socket */
int nfc_llcp_socket_init(llcp_socket_t *socket, uint8_t ssap, uint8_t dsap,
    llcp_socket_mode_t mode);

int nfc_llcp_socket_receive(llcp_socket_t *socket, uint8_t *buffer, size_t *buffer_len);

int nfc_llcp_socket_send(llcp_socket_t *socket, const uint8_t *data, uint8_t data_len);
