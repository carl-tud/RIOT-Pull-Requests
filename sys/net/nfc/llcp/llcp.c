#include "log.h"
#include "ztimer.h"
#include "architecture.h"

#include "net/nfcdev.h"
#include "net/nfc/llcp.h"

#define ENABLE_DEBUG CONFIG_LLCP_DEBUG
#include "debug.h"

#define LLCP_DEBUG(...) DEBUG("llcp: " __VA_ARGS__)

#define THREAD_DELAY_MS 10
#define MSG_STOP_CONTROLLER 0x0001
#define MSG_QUEUE_SIZE 8

static ssize_t _get_ring_buffer(tsrb_t* rb, uint8_t* payload, size_t capacity) {
    assert(rb);
    assert(payload);
    assert(capacity > 0);

    nfc_llcp_controller_buffer_header_t header = {};
    if (tsrb_peek(rb, (uint8_t*)&header, sizeof(header)) != sizeof(nfc_llcp_controller_buffer_header_t)) {
        return -ENOMSG;
    }
    if (header.length > capacity) {
        LLCP_DEBUG("need capacity of %" PRIuSIZE ", have only %" PRIuSIZE "\n",
                   header.length, capacity);
        return -ENOBUFS;
    }

    tsrb_drop(rb, sizeof(nfc_llcp_controller_buffer_header_t));

    // Need to wait until the caller has written their payload.
    // They already have written their header so can't be that long until all payload is in.
    ssize_t res = 0;
    while (header.length != 0) {
        if ((res = tsrb_get(rb, payload, header.length)) < 0) {
            // This function actually never fails, but implementation detail
            return res;
        }
        header.length -= (size_t)res;
        payload += (size_t)res;
    }
    return (ssize_t)header.length;
}

static int _add_ring_buffer(tsrb_t* rb, const iolist_t* payload) {
    assert(rb);
    assert(payload);

    nfc_llcp_controller_buffer_header_t header = {
        .length = iolist_size(payload)
    };
    assert(header.length > 0);
    if (header.length > (LLCP_MAX_PDU_SIZE - sizeof(nfc_llcp_header_t))) {
        return -ENOBUFS;
    }

    if ((sizeof(header) + header.length) > tsrb_free(rb)) {
        return -ENOBUFS;
    }

    int res = 0;
    if ((res = tsrb_add(rb, (uint8_t*)&header, sizeof(header))) < 0) {
        return res;
    }
    for (; payload; payload = payload->iol_next) {
        if ((res = tsrb_add(rb, payload->iol_base, payload->iol_len)) < 0) {
            return res;
        }
    }
    return 0;
}

static void llcp_process_message(uint16_t message_type) {
    switch (message_type) {
        case MSG_STOP_CONTROLLER:
            thread_zombify();
            break;
        default:
            break;
    }
}

static void llcp_run(void *arg) {
    msg_t msg[MSG_QUEUE_SIZE];
    msg_init_queue(msg, MSG_QUEUE_SIZE);
    msg_t receive_msg;
    nfc_llcp_controller_t* controller = (nfc_llcp_controller_t*)arg;

    uint8_t tx[LLCP_MAX_PDU_SIZE];
    nfc_llcp_header_t* tx_header = (nfc_llcp_header_t*)tx;
    uint8_t rx[LLCP_MAX_PDU_SIZE];
    nfc_llcp_header_t* rx_header = (nfc_llcp_header_t*)rx;
    ssize_t res = 0;

    while (1) {
        if (msg_try_receive(&receive_msg) == 1) {
            llcp_process_message(receive_msg.type);
        }
        mutex_lock(&controller->sockets_mutex);
        /* check all sockets and their respective ring buffers */
        for (size_t i = 0; i < controller->socket_count; i += 1) {
            nfc_llcp_socket_t* socket = &controller->sockets[i];

            if ((res = _get_ring_buffer(&socket->tx_buffer, (uint8_t*)tx + sizeof(nfc_llcp_header_t), sizeof(tx) - sizeof(nfc_llcp_header_t))) > 0)  {
                tx_header->dsap = socket->dsap;
                tx_header->ssap = socket->ssap;
                tx_header->ptype = LLCP_PDU_PTYPE_UI;
            } else {
                /* if there is nothing to send, send SYMM as the LLCP expects this */
                tx_header->raw[0] = 0;
                tx_header->raw[1] = 0;
                res = 2;
            }

            if ((res = nfcdev_transceive(controller->dev, tx, (size_t)res, rx, sizeof(rx), THREAD_DELAY_MS, NFCDEV_INTERFACE_NFC_DEP)) < 0) {
                LLCP_DEBUG("transceive failed: %" PRIiSIZE "\n", res);
                continue;
            }

            if ((size_t)res <= sizeof(nfc_llcp_header_t)) {
                LLCP_DEBUG("no meaningful data received?\n");
                continue;
            }

            switch (rx_header->ptype) {
                case LLCP_PDU_PTYPE_UI:
                    LLCP_DEBUG("UI PDU\n");
                    iolist_t payload = {
                        .iol_base = (uint8_t*)rx + sizeof(nfc_llcp_header_t),
                        .iol_len = (size_t)res - sizeof(nfc_llcp_header_t)
                    };
                    _add_ring_buffer(&socket->rx_buffer, &payload);
                    break;
                case LLCP_PDU_PTYPE_I:
                    /* NOT SUPPORTED */
                    LLCP_DEBUG("I PDU not supported\n");
                    break;
                case LLCP_PDU_PTYPE_DISCONNECT:
                    LLCP_DEBUG("disconnect\n");
                    /* NOT SUPPORTED */
                    LLCP_DEBUG("I PDU not supported\n");
                    break;
                default:
                    /* ignore other PDU types */
                    break;
            }
        }
        mutex_unlock(&controller->sockets_mutex);

        /* LLCP runs every 10 ms (must be smaller than the LTO)*/
        ztimer_sleep(ZTIMER_MSEC, THREAD_DELAY_MS); 
    }
}

int nfc_llcp_controller_init(nfc_llcp_controller_t* controller, nfcdev_t* dev) {
    assert(controller);
    assert(dev);
    mutex_init(&controller->sockets_mutex);

    controller->socket_count = 0;
    controller->dev = dev;

    assert(controller->pid == KERNEL_PID_UNDEF);
    controller->pid = thread_create(controller->thread_stack, sizeof(controller->thread_stack),
        THREAD_PRIORITY_MAIN - 1, 0, (void *) &llcp_run, controller, "llcp");
    return 0;
}

void nfc_llcp_controller_stop(nfc_llcp_controller_t *controller) {
    assert(controller);

    msg_t stop_msg = {
        .type = MSG_STOP_CONTROLLER,
    };

    msg_send(&stop_msg, controller->pid);
    while(thread_getstatus(controller->pid) != STATUS_ZOMBIE) {
        ztimer_sleep(ZTIMER_MSEC, 10);
    }

    controller->pid = 0;

    thread_kill_zombie(controller->pid);
}

int nfc_llcp_controller_add_socket(nfc_llcp_controller_t *controller, nfc_llcp_socket_t *socket) {
    assert(socket);
    assert(controller);

    mutex_lock(&controller->sockets_mutex);
    if (controller->socket_count >= LLCP_CONTROLLER_MAX_SOCKETS) {
        mutex_unlock(&controller->sockets_mutex);
        LLCP_DEBUG("maximum number of sockets reached\n");
        return -ENOBUFS;
    }

    controller->sockets[controller->socket_count++] = *socket;
    mutex_unlock(&controller->sockets_mutex);
    return 0;
}

void nfc_llcp_controller_remove_socket(nfc_llcp_controller_t *controller, 
    nfc_llcp_socket_t *socket) {
    assert(socket != NULL);
    assert(controller != NULL);

    mutex_lock(&controller->sockets_mutex);
    for (size_t i = 0; i < controller->socket_count; ++i) {
        if (&controller->sockets[i] == socket) {
            /* found the socket, remove it by shifting the rest */
            for (size_t j = i; j < controller->socket_count - 1; ++j) {
                controller->sockets[j] = controller->sockets[j + 1];
            }
            controller->socket_count--;
            break;
        }
    }
    mutex_unlock(&controller->sockets_mutex);
}

int nfc_llcp_socket_init(nfc_llcp_socket_t *socket, uint8_t ssap, uint8_t dsap,
    nfc_llcp_socket_mode_t mode) {
    assert(socket != NULL);
    assert(ssap <= 0x3F);
    assert(dsap <= 0x3F);

    if (mode != LLCP_SOCKET_MODE_CONNECTIONLESS) {
        return -1;
    }

    tsrb_init(&socket->rx_buffer, socket->rx_buffer_data, sizeof(socket->rx_buffer_data));
    tsrb_init(&socket->tx_buffer, socket->tx_buffer_data, sizeof(socket->tx_buffer_data));
    socket->ssap = ssap;
    socket->dsap = dsap;

    return 0;
}

ssize_t nfc_llcp_socket_receive(nfc_llcp_socket_t* socket, uint8_t* payload, size_t capacity) {
    assert(socket);
    assert(payload);
    assert(capacity > 0);
    return _get_ring_buffer(&socket->rx_buffer, payload, capacity);
}

int nfc_llcp_socket_send_chunks(nfc_llcp_socket_t *socket, const iolist_t* payload) {
    assert(socket);
    assert(payload);
    return _add_ring_buffer(&socket->tx_buffer, payload);
}
