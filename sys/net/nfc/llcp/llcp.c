#include "log.h"
#include "ztimer.h"
#include "architecture.h"
#include "sema.h"

#include "net/nfcdev.h"
#include "net/nfc/llcp.h"
#include "net/nfc/tlv.h"

#define ENABLE_DEBUG CONFIG_LLCP_DEBUG
#include "debug.h"

#define LLCP_DEBUG(...) DEBUG("llcp: " __VA_ARGS__)

#define THREAD_DELAY_MS 10
#define MSG_STOP_CONTROLLER 0x0001
#define MSG_QUEUE_SIZE 8

#define LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED (1)
#define LLCP_SOCKET_STATE_FLAG_CONNECTED           (2)
#define LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_ESTABLISHED (4)
#define LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_CONFIRMED (8)
#define LLCP_SOCKET_STATE_FLAG_CONNECTION_REFUSED (0x10)
#define LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_ABORTED (0x20)

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

static nfc_llcp_socket_t* _socket_with_service_name(nfc_llcp_controller_t* controller, const char* name, size_t length, uint8_t state) {
    nfc_llcp_socket_t* socket = NULL;
    for (size_t i = 0; i < controller->socket_count; i++) {
        if (controller->sockets[i]->state == state &&
            controller->sockets[i]->service_name &&
            strlen(controller->sockets[i]->service_name) == length &&
            memcmp(controller->sockets[i]->service_name, name, length) == 0) {
            socket = controller->sockets[i];
            break;
        }
    }
    return socket;
}

static nfc_llcp_socket_t* _socket_with_sap(nfc_llcp_controller_t* controller, uint8_t local_sap, uint8_t remote_sap, uint8_t state) {
    nfc_llcp_socket_t* socket = NULL;
    for (size_t i = 0; i < controller->socket_count; i++) {
        if (controller->sockets[i]->state == state &&
            (!controller->sockets[i]->ssap || controller->sockets[i]->ssap == local_sap) &&
            (!controller->sockets[i]->dsap || controller->sockets[i]->dsap == remote_sap)) {
            socket = controller->sockets[i];
            break;
        }
    }
    return socket;
}

static ssize_t _get_next_pdu(nfc_llcp_controller_t* controller, uint8_t* tx, nfc_llcp_header_t* tx_header, size_t* current_ix) {
    uint8_t* payload = &tx[sizeof(nfc_llcp_header_t)];
    for (size_t i = 0; i < controller->socket_count; i++) {
        size_t ix = (*current_ix + i) % controller->socket_count;
        nfc_llcp_socket_t* socket = controller->sockets[ix];
        ssize_t res = 0;
        bool found_pdu = false;

        if (socket->state & LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED) {
            if (socket->state & LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_ESTABLISHED) {
                LLCP_DEBUG("establishing connection from socket %" PRIuSIZE "\n", i);
                tx_header->ptype = LLCP_PDU_PTYPE_CONNECT;
                if (socket->dsap == 1 && socket->service_name) {
                    res = strlen(socket->service_name) + 2;
                    *payload++ = LLCP_PARAMETER_TLV_SN;
                    *payload++ = (uint8_t)res;
                    memcpy(payload, socket->service_name, (size_t)(uint8_t)res);
                }
                socket->state = LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED | LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_CONFIRMED;
                // The connect flag will be set when we receive CC
                found_pdu = true;
            }
            else if (socket->state & LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_ABORTED) {
                LLCP_DEBUG("aborting connection from socket %" PRIuSIZE "\n", i);
                tx_header->ptype = LLCP_PDU_PTYPE_DISCONNECT;
                socket->state = LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED;
                found_pdu = true;
                sema_post(&socket->rx_sema);
            }
            else if (socket->state & LLCP_SOCKET_STATE_FLAG_CONNECTED) {
                if ((res = _get_ring_buffer(&socket->tx_buffer, tx + sizeof(nfc_llcp_header_t) + 1, LLCP_MAX_PDU_SIZE - sizeof(nfc_llcp_header_t) - 1)) > 0) {
                    tx_header->ptype = LLCP_PDU_PTYPE_I;
                    *payload = (socket->vs << 4) | (socket->vr & 0x0F);
                    socket->vs = (socket->vs + 1) % 16;
                    res += 1;
                    found_pdu = true;
                }
            } else {
                // Awaiting connection...
            }
        } else {
            if ((res = _get_ring_buffer(&socket->tx_buffer, tx + sizeof(nfc_llcp_header_t), LLCP_MAX_PDU_SIZE - sizeof(nfc_llcp_header_t))) > 0) {
                tx_header->ptype = LLCP_PDU_PTYPE_UI;
                found_pdu = true;
            }
        }

        if (found_pdu) {
            tx_header->dsap = socket->dsap;
            tx_header->ssap = socket->ssap;
            *current_ix = (ix + 1) % controller->socket_count;
            return res;
        }
    }
    return 0;
}

void llcp_run(void *arg) {
    msg_t msg[MSG_QUEUE_SIZE];
    msg_init_queue(msg, MSG_QUEUE_SIZE);
    msg_t receive_msg;
    nfc_llcp_controller_t* controller = (nfc_llcp_controller_t*)arg;

    uint8_t tx[LLCP_MAX_PDU_SIZE];
    nfc_llcp_header_t tx_header = {};
    nfc_llcp_header_t rx_header = {};
    uint8_t* rx = NULL;
    ssize_t res = 0;
    size_t symmetry_count = 0;
    size_t current_ix = 0;

    bool can_send = nfcdev_role(controller->dev) == NFC_ROLE_INITIATOR;
    if (can_send) {
        res = _get_next_pdu(controller, tx, &tx_header, &current_ix);
        if (tx_header.ptype == 0) {
            // SYMM
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
                tx_header.raw = 0;
                tx_header.ptype = LLCP_PDU_PTYPE_DISCONNECT;
                byteorder_htobebufs(tx, tx_header.raw);
                res = sizeof(nfc_llcp_header_t);
                nfcdev_transceive(controller->dev, tx, (size_t)res, &rx, 0, controller->lto_ms, NFCDEV_INTERFACE_NFC_DEP);
                nfcdev_disconnect(controller->dev, NFCDEV_CONNECTION_ID_CURRENT);
                thread_zombify();
                return;
            }
        }

        if ((size_t)res < sizeof(nfc_llcp_header_t)) {
            LLCP_DEBUG("link disrupted\n");
            goto disconnect;
        }

        mutex_lock(&controller->sockets_mutex);

        tx_header.raw = 0;
        rx_header.raw = byteorder_bebuftohs(rx);
        LLCP_DEBUG("received dsap=0x%02x ptype=0x%02x ssap=0x%02x\n", rx_header.dsap, rx_header.ptype, rx_header.ssap);
        switch (rx_header.ptype) {
            case LLCP_PDU_PTYPE_SYMMETRY:
                symmetry_count += 1;
                break;
            case LLCP_PDU_PTYPE_DISCONNECT: {
                LLCP_DEBUG("pdu=DISC\n");
                if (rx_header.ssap == 0 && rx_header.dsap == 0) {
                    goto disconnect;
                }
                nfc_llcp_socket_t* socket = _socket_with_sap(controller, rx_header.dsap, rx_header.ssap,
                     LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED | LLCP_SOCKET_STATE_FLAG_CONNECTED);
                if (!socket) {
                    LLCP_DEBUG("no matching socket for local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                    break;
                }
                LLCP_DEBUG("disconnected local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                socket->state &= ~LLCP_SOCKET_STATE_FLAG_CONNECTED;
                // Because the to-be-established flag is missing, connection is not going to get
                // re-established either if it was even initiated by us.
                break;
            }
            case LLCP_PDU_PTYPE_UI: {
                LLCP_DEBUG("pdu=UI\n");
                nfc_llcp_socket_t* socket = _socket_with_sap(controller, rx_header.dsap, rx_header.ssap, 0);
                if (!socket) {
                    LLCP_DEBUG("no matching socket for local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                    break;
                }
                iolist_t payload = {
                    .iol_base = (uint8_t*)rx + sizeof(nfc_llcp_header_t),
                    .iol_len = (size_t)res - sizeof(nfc_llcp_header_t)
                };
                _add_ring_buffer(&socket->rx_buffer, &payload);
                sema_post(&socket->rx_sema);
                break;
            }
            case LLCP_PDU_PTYPE_I: {
                LLCP_DEBUG("pdu=I\n");
                nfc_llcp_socket_t* socket = _socket_with_sap(controller, rx_header.dsap, rx_header.ssap,
                     LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED | LLCP_SOCKET_STATE_FLAG_CONNECTED);
                if (!socket) {
                    LLCP_DEBUG("no matching socket for local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                    break;
                }
                if ((size_t)res <= sizeof(nfc_llcp_header_t)) {
                    LLCP_DEBUG("I PDU too short\n");
                    break;
                }
                socket->vr = (socket->vr + 1) % 16;
                iolist_t payload = {
                    .iol_base = (uint8_t*)rx + sizeof(nfc_llcp_header_t) + 1,
                    .iol_len = (size_t)res - sizeof(nfc_llcp_header_t) - 1
                };
                _add_ring_buffer(&socket->rx_buffer, &payload);
                sema_post(&socket->rx_sema);
                break;
            }
            case LLCP_PDU_PTYPE_CONNECT: {
                LLCP_DEBUG("pdu=CONNECT\n");
                nfc_llcp_socket_t* socket;
                if (rx_header.dsap == 1) {
                    res -= sizeof(nfc_llcp_header_t);

                    uint8_t* parameters = (uint8_t*)rx + sizeof(nfc_llcp_header_t);
                    uint8_t type = 0;
                    uint16_t length = 0;
                    char* service_name = NULL;
                    size_t read = 0;
                    while (res > 0 && (read = tlv_parse(parameters, &type, &length, (uint8_t**)&service_name)) > 0) {
                        if (type == LLCP_PARAMETER_TLV_SN && service_name) {
                            break;
                        } else {
                            service_name = NULL;
                        }
                        parameters += read;
                        res -= read;
                    }

                    if (!service_name || length == 0) {
                        LLCP_DEBUG("missing service name\n");
                        tx_header.ptype = LLCP_PDU_PTYPE_DM;
                        tx_header.ssap = 1;
                        tx_header.dsap = rx_header.ssap;
                        tx[sizeof(nfc_llcp_header_t)] = 0x01;
                        res = 1;
                        break;
                    }
                    LLCP_DEBUG("service_name=%.*s\n", length, service_name);

                    socket = _socket_with_service_name(controller, service_name, (size_t)length,
                       LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED);
                    if (!socket) {
                        LLCP_DEBUG("no matching socket for service\n");
                        tx_header.ptype = LLCP_PDU_PTYPE_DM;
                        tx_header.ssap = 1;
                        tx_header.dsap = rx_header.ssap;
                        tx[sizeof(nfc_llcp_header_t)] = 0x02;
                        res = 1;
                        break;
                    }
                } else {
                    socket = _socket_with_sap(controller, rx_header.dsap, rx_header.ssap,
                        LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED);
                    if (!socket) {
                        LLCP_DEBUG("no matching socket for local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                        break;
                    }
                }
                socket->state |= LLCP_SOCKET_STATE_FLAG_CONNECTED;
                socket->ssap = rx_header.dsap;
                socket->dsap = rx_header.ssap;
                LLCP_DEBUG("connection accepted local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                tx_header.ptype = LLCP_PDU_PTYPE_CC;
                tx_header.ssap = socket->ssap;
                tx_header.dsap = socket->dsap;
                res = 0;
                break;
            }
            case LLCP_PDU_PTYPE_DM: {
                LLCP_DEBUG("pdu=DM\n");
                if ((size_t)res <= sizeof(nfc_llcp_header_t)) {
                    LLCP_DEBUG("connection refusal reason missing\n");
                }
                nfc_llcp_socket_t* socket = _socket_with_sap(controller, rx_header.dsap, rx_header.ssap,
                    LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED | LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_CONFIRMED);
                if (!socket) {
                    LLCP_DEBUG("no matching socket for local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                    break;
                }
                socket->state = LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED | LLCP_SOCKET_STATE_FLAG_CONNECTION_REFUSED;
                LLCP_DEBUG("connection refused local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                break;
            }
            case LLCP_PDU_PTYPE_CC: {
                LLCP_DEBUG("pdu=CC\n");
                nfc_llcp_socket_t* socket = _socket_with_sap(controller, rx_header.dsap, rx_header.ssap,
                   LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED | LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_CONFIRMED);
                if (!socket) {
                    LLCP_DEBUG("no matching socket for local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                    break;
                }
                socket->state = LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED | LLCP_SOCKET_STATE_FLAG_CONNECTED;
                LLCP_DEBUG("connection established local=%u remote=%u\n", rx_header.dsap, rx_header.ssap);
                break;
            }
            default:
                LLCP_DEBUG("unsupported PDU type %u\n", rx_header.ptype);
                /* ignore other PDU types */
                break;
        }

        if (!tx_header.ptype) {
            LLCP_DEBUG("collecting message..\n");
            ztimer_sleep(ZTIMER_MSEC, 1);
            // res now tracks the payload length in bytes from the application
            tx_header.ptype = 0;
            res = _get_next_pdu(controller, tx, &tx_header, &current_ix);
            if (res <= 0 && symmetry_count >= 10 && tx_header.ptype == 0) {
                ztimer_sleep(ZTIMER_MSEC, 50);
                res = _get_next_pdu(controller, tx, &tx_header, &current_ix);
            }
            if (tx_header.ptype == 0) {
                tx_header.raw = 0;
                res = 0;
            }
        }

        mutex_unlock(&controller->sockets_mutex);

        byteorder_htobebufs(tx, tx_header.raw);
        res += sizeof(nfc_llcp_header_t);
        if ((res = nfcdev_transceive(controller->dev, tx, (size_t)res, &rx, 0, controller->lto_ms, NFCDEV_INTERFACE_NFC_DEP)) < 0) {
            LLCP_DEBUG("transceiving failed: %" PRIiSIZE "\n", res);
            goto disconnect;
        }
        // res now tracks the received LLCP packet length
    }

disconnect:
    nfcdev_disconnect(controller->dev, NFCDEV_CONNECTION_ID_CURRENT);
    return;
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

    thread_kill_zombie(controller->pid);
    controller->pid = 0;
}

int nfc_llcp_controller_add_socket(nfc_llcp_controller_t *controller, nfc_llcp_socket_t* socket,
                                   nfc_llcp_socket_mode_t mode) {
    assert(socket);
    assert(controller);
    assert(socket->ssap <= 0x3F);
    assert(socket->dsap <= 0x3F);
    assert(!socket->service_name || strlen(socket->service_name) <= UINT8_MAX);

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
    sema_create(&socket->rx_sema, 0);
    socket->state = mode;
    socket->vs = 0;
    socket->vr = 0;
    return 0;
}

void nfc_llcp_controller_disconnect_socket(nfc_llcp_controller_t *controller,
       nfc_llcp_socket_t *socket) {
    mutex_lock(&controller->sockets_mutex);
    if (socket->state == (LLCP_SOCKET_STATE_FLAG_CONNECTION_ORIENTED & LLCP_SOCKET_STATE_FLAG_CONNECTED)) {
        socket->state |= LLCP_SOCKET_STATE_FLAG_CONNECTION_TO_BE_ABORTED;
        // We're now using the semaphore to wait until the LLCP loop has sent the disconnect
        sema_create(&socket->rx_sema, 0);
        sema_wait(&socket->rx_sema);
    }
    mutex_unlock(&controller->sockets_mutex);
}

void nfc_llcp_controller_remove_socket(nfc_llcp_controller_t *controller,
    nfc_llcp_socket_t *socket) {
    assert(socket != NULL);
    assert(controller != NULL);

    mutex_lock(&controller->sockets_mutex);
    for (size_t i = 0; i < controller->socket_count; ++i) {
        if (controller->sockets[i] == socket) {
            sema_destroy(&socket->rx_sema);
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

static void _sema_post(void* sema) {
    sema_post((sema_t*)sema);
}

ssize_t nfc_llcp_socket_receive(nfc_llcp_socket_t* socket, uint8_t* payload, size_t capacity, uint32_t timeout_ms) {
    assert(socket);
    assert(payload);
    assert(capacity > 0);

    if (timeout_ms > 0) {
        ztimer_t timer = { .callback = _sema_post, .arg = &socket->rx_sema };

        if (timeout_ms != LLCP_TIMEOUT_INFINITE) {
            ztimer_set(ZTIMER_MSEC, &timer, timeout_ms);
        }
        sema_wait(&socket->rx_sema);
        if (timeout_ms != LLCP_TIMEOUT_INFINITE) {
            bool triggered = !ztimer_remove(ZTIMER_MSEC, &timer);
            if (triggered) {
                LLCP_DEBUG("app timeout after %" PRIu32 " ms\n", timeout_ms);
                return -ETIMEDOUT;
            }
        }
        LLCP_DEBUG("got signal from LLCP\n");
    }
    return _get_ring_buffer(&socket->rx_buffer, payload, capacity);
}

int nfc_llcp_socket_send_chunks(nfc_llcp_socket_t *socket, const iolist_t* payload) {
    assert(socket);
    assert(payload);
    return _add_ring_buffer(&socket->tx_buffer, payload);
}
