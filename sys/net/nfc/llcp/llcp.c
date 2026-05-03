#include "log.h"
#include "ztimer.h"
#include "architecture.h"

#include "net/nfcdev.h"
#include "net/nfc/llcp.h"
#include "net/nfc/tlv.h"

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
    if (tsrb_peek(rb, (uint8_t*)&header, sizeof(header)) != sizeof(header)) {
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
    size_t yet_to_receive = header.length;
    while (yet_to_receive > 0) {
        if ((res = tsrb_get(rb, payload, yet_to_receive)) < 0) {
            // This function actually never fails, but implementation detail
            return res;
        }
        yet_to_receive -= (size_t)res;
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

static void llcp_run(void *arg) {
    msg_t msg[MSG_QUEUE_SIZE];
    msg_init_queue(msg, MSG_QUEUE_SIZE);
    msg_t receive_msg;
    nfc_llcp_controller_t* controller = (nfc_llcp_controller_t*)arg;

    uint8_t tx[LLCP_MAX_PDU_SIZE];
    nfc_llcp_header_t tx_header = {};
    uint8_t* rx = NULL;
    ssize_t res = 0;
    size_t symmetry_count = 0;

    bool can_send = nfcdev_role(controller->dev) == NFC_ROLE_INITIATOR;
    if (can_send) {
        for (size_t i = 0; i < controller->socket_count; i += 1) {
            nfc_llcp_socket_t* socket = controller->sockets[i];
            if ((res = _get_ring_buffer(&socket->tx_buffer,
                (uint8_t*)tx + sizeof(nfc_llcp_header_t),
                sizeof(tx) - sizeof(nfc_llcp_header_t)
            )) > 0)  {
                tx_header.dsap = socket->dsap;
                tx_header.ssap = socket->ssap;
                tx_header.ptype = LLCP_PDU_PTYPE_UI;
                break;
            }
        }

        // Found a pdu to send
        if (tx_header.ptype) {
            assert(res > 0);
        } else {
            tx_header.raw = 0;
            res = 0;
        }
        byteorder_htobebufs(tx, tx_header.raw);
        res += sizeof(nfc_llcp_header_t);
        if ((res = nfcdev_transceive(controller->dev, tx, (size_t)res, &rx, 0, controller->lto_ms, NFCDEV_INTERFACE_NFC_DEP)) < 0) {
            LLCP_DEBUG("transceiving failed: %" PRIiSIZE "\n", res);
            nfcdev_disconnect(controller->dev, NFCDEV_CONNECTION_ID_CURRENT);
            return;
        }
    } else {
        if ((res = nfcdev_receive(controller->dev, &rx, 0, controller->lto_ms, NFCDEV_INTERFACE_NFC_DEP)) < 0) {
            LLCP_DEBUG("receiving failed: %" PRIiSIZE "\n", res);
            nfcdev_disconnect(controller->dev, NFCDEV_CONNECTION_ID_CURRENT);
            return;
        }
        can_send = true;
        LLCP_DEBUG("first target receive done\n");
    }

    // res now tracks the received LLCP packet length

    while (true) {
        if (msg_try_receive(&receive_msg) == 1) {
            if (receive_msg.type == MSG_STOP_CONTROLLER) {
                nfcdev_disconnect(controller->dev, NFCDEV_CONNECTION_ID_CURRENT);
                thread_zombify();
                return;
            }
        }

        mutex_lock(&controller->sockets_mutex);
        /* check all sockets and their respective ring buffers */
        for (size_t i = 0; i < controller->socket_count; i += 1) {
            nfc_llcp_socket_t* socket = controller->sockets[i];

            // res still tracks the received LLCP packet length

            if ((size_t)res < sizeof(nfc_llcp_header_t)) {
                LLCP_DEBUG("link disrupted\n");
                nfcdev_disconnect(controller->dev, NFCDEV_CONNECTION_ID_CURRENT);
                return;
            }

            nfc_llcp_header_t rx_header = { .raw = byteorder_bebuftohs(rx) };
            LLCP_DEBUG("received dsap=0x%02x ptype=0x%02x ssap=0x%02x\n", rx_header.dsap, rx_header.ptype, rx_header.ssap);
            switch (rx_header.ptype) {
                case LLCP_PDU_PTYPE_SYMMETRY:
                    symmetry_count += 1;
                    break;
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
                    nfcdev_disconnect(controller->dev, NFCDEV_CONNECTION_ID_CURRENT);
                    return;
                default:
                    /* ignore other PDU types */
                    break;
            }

            LLCP_DEBUG("collecting message..\n");
            ztimer_sleep(ZTIMER_MSEC, 1);
            // res now tracks the payload length in bytes from the application

            if ((res = _get_ring_buffer(&socket->tx_buffer, (uint8_t*)tx + sizeof(nfc_llcp_header_t), sizeof(tx) - sizeof(nfc_llcp_header_t))) > 0)  {
                tx_header.dsap = socket->dsap;
                tx_header.ssap = socket->ssap;
                tx_header.ptype = LLCP_PDU_PTYPE_UI;
                LLCP_DEBUG("sending PDU\n");
            }

            if (res <= 0 && symmetry_count >= 10) {
                ztimer_sleep(ZTIMER_MSEC, 50);
                if ((res = _get_ring_buffer(&socket->tx_buffer, (uint8_t*)tx + sizeof(nfc_llcp_header_t), sizeof(tx) - sizeof(nfc_llcp_header_t))) > 0)  {
                    tx_header.dsap = socket->dsap;
                    tx_header.ssap = socket->ssap;
                    tx_header.ptype = LLCP_PDU_PTYPE_UI;
                    LLCP_DEBUG("sending PDU\n");
                }
            }

            if (res <= 0) {
                LLCP_DEBUG("nothing send, SYMM\n");
                // Symmetry PDU
                tx_header.raw = 0;
                res = 0;
            }
            byteorder_htobebufs(tx, tx_header.raw);
            res += sizeof(nfc_llcp_header_t);
            if ((res = nfcdev_transceive(controller->dev, tx, (size_t)res, &rx, 0, controller->lto_ms, NFCDEV_INTERFACE_NFC_DEP)) < 0) {
                LLCP_DEBUG("transceiving failed: %" PRIiSIZE "\n", res);
                nfcdev_disconnect(controller->dev, NFCDEV_CONNECTION_ID_CURRENT);
                return;
            }

            // res now tracks the received LLCP packet length
        }
        mutex_unlock(&controller->sockets_mutex);
    }
}

int nfc_llcp_controller_init(nfc_llcp_controller_t* controller, nfcdev_t* dev, uint8_t* parameters, size_t parameter_length) {
    assert(controller);
    assert(dev);
    mutex_init(&controller->sockets_mutex);
    controller->lto_ms = 100;

    if (parameter_length > 0) {
        uint8_t type = 0;
        uint16_t length = 0;
        uint8_t* value = NULL;
        size_t read = 0;
        while (parameter_length > 0 && (read = tlv_parse(parameters, &type, &length, &value)) > 0) {
            if (type == LLCP_PARAMETER_TLV_LTO && length == 1 && value) {
                if (*value) {
                    controller->lto_ms = *value * 10;
                    LLCP_DEBUG("got peer LTO of %" PRIu32 "ms\n", controller->lto_ms);
                }
                break;
            }
            parameters += read;
            parameter_length -= read;
        }
    }

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

int nfc_llcp_controller_add_socket(nfc_llcp_controller_t *controller, nfc_llcp_socket_t* socket,
                                   uint8_t ssap, uint8_t dsap, nfc_llcp_socket_mode_t mode) {
    assert(socket);
    assert(controller);
    assert(ssap <= 0x3F);
    assert(dsap <= 0x3F);

    mutex_lock(&controller->sockets_mutex);
    if (controller->socket_count >= LLCP_CONTROLLER_MAX_SOCKETS) {
        mutex_unlock(&controller->sockets_mutex);
        LLCP_DEBUG("maximum number of sockets reached\n");
        return -ENOBUFS;
    }

    controller->sockets[controller->socket_count++] = socket;
    mutex_unlock(&controller->sockets_mutex);

    tsrb_init(&socket->rx_buffer, socket->rx_buffer_data, sizeof(socket->rx_buffer_data));
    tsrb_init(&socket->tx_buffer, socket->tx_buffer_data, sizeof(socket->tx_buffer_data));
    socket->ssap = ssap;
    socket->dsap = dsap;
    socket->mode = mode;
    return 0;
}

void nfc_llcp_controller_remove_socket(nfc_llcp_controller_t *controller, 
    nfc_llcp_socket_t *socket) {
    assert(socket != NULL);
    assert(controller != NULL);

    mutex_lock(&controller->sockets_mutex);
    for (size_t i = 0; i < controller->socket_count; ++i) {
        if (controller->sockets[i] == socket) {
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
