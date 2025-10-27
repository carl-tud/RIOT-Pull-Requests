#include "net/nfc/llcp.h"

#include "log.h"
#include "ztimer.h"

#define THREAD_DELAY_MS 10

#define MSG_STOP_CONTROLLER 0x0001

#define MSG_QUEUE_SIZE 8

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


    nfc_llcp_controller_t *controller = (nfc_llcp_controller_t *) arg;

    while (1) {

        if (msg_try_receive(&receive_msg) == 0) {
            llcp_process_message(receive_msg.type);
        } 

        mutex_lock(&controller->sockets_mutex);
        /* check all sockets and their respective ring buffers */
        for (size_t i = 0; i < controller->socket_count; ++i) {
            nfc_llcp_socket_t *socket = &controller->sockets[i];
            size_t send_data_len;
            size_t receive_data_len = LLCP_MAX_PDU_SIZE;
            uint8_t send_buff[LLCP_MAX_PDU_SIZE];
            uint8_t receive_buff[LLCP_MAX_PDU_SIZE];
    
            /* check if there is data to send in the tx_buffer */
            if ((send_data_len = tsrb_get_one(&socket->tx_buffer)) > 0) {
                /* we need to read the rest of the data */
                tsrb_get(&socket->tx_buffer, send_buff + 2, send_data_len);

                /* create the LLCP PDU */
                send_buff[0] = (socket->dsap << 2) | (LLCP_PDU_PTYPE_I >> 2);
                send_buff[1] = ((LLCP_PDU_PTYPE_I & 0x03) << 6) | (socket->ssap & 0x3F);
            } else {
                /* if there is nothing to send, send SYMM as the LLCP expects this */
                send_buff[0] = LLCP_PDU_SYMM & 0xFF00;
                send_buff[1] = LLCP_PDU_SYMM & 0x00FF;
                send_data_len = 2;
            }

            if (controller->mode == NFCDEV_MODE_INITIATOR) {
                controller->dev->ops->initiator_exchange_data(controller->dev, send_buff, 
                    send_data_len + 2, receive_buff, &receive_data_len);
            } else {
                controller->dev->ops->target_send_data(controller->dev, send_buff, send_data_len);
                controller->dev->ops->target_receive_data(controller->dev, receive_buff, 
                    &receive_data_len);
            }

            if (receive_data_len <= 2) {
                LOG_WARNING("[LLCP Controller] No meaningful data received\n");
                continue; /* no meaningful data received */
            }

            uint8_t pdu_type = nfc_llcp_pdu_get_ptype(receive_buff);

            switch (pdu_type) {
                case LLCP_PDU_PTYPE_UI:
                    /* store the data in the rx_buffer */
                    tsrb_add(&socket->rx_buffer, receive_buff + 2, receive_data_len - 2);
                    break;
                case LLCP_PDU_PTYPE_I:
                    /* NOT SUPPORTED */
                    LOG_ERROR("[LLCP Controller] I PDU not supported\n");
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

int nfc_llcp_controller_init(nfc_llcp_controller_t *controller, nfcdev_t *dev, nfcdev_mode_t mode) {
    assert(controller != NULL);
    assert(dev != NULL);
    mutex_init(&controller->sockets_mutex);

    controller->socket_count = 0;
    controller->dev = dev;
    controller->mode = mode;

    if (controller->dev->state == NFCDEV_STATE_UNINITIALIZED) {
        /* the NFC-DEP link must be established before starting the LLCP controller */
        LOG_ERROR("[LLCP Controller] Device not initialized\n");
        return -1;
    }

    if (controller->mode == NFCDEV_MODE_INITIATOR) {
        controller->dev->ops->poll_dep(controller->dev, NFC_BAUDRATE_106K);
    } else {
        controller->dev->ops->listen_dep(controller->dev, NFC_BAUDRATE_106K);
    }

    assert(controller->pid == 0);
    controller->pid = thread_create(controller->thread_stack, sizeof(controller->thread_stack),
        THREAD_PRIORITY_MAIN - 1, 0, (void *) &llcp_run, controller, "LLCP Controller");

    return 0;
}

void nfc_llcp_controller_stop(nfc_llcp_controller_t *controller) {
    assert(controller != NULL);

    msg_t stop_msg = {
        .type = MSG_STOP_CONTROLLER,
    };

    msg_send(&stop_msg, controller->pid);
    controller->pid = 0;

    while(thread_getstatus(controller->pid) != STATUS_ZOMBIE) {
        ztimer_sleep(ZTIMER_MSEC, 10);
    }

    thread_kill_zombie(controller->pid);
}

void nfc_llcp_controller_add_socket(nfc_llcp_controller_t *controller, nfc_llcp_socket_t *socket) {
    assert(socket != NULL);
    assert(controller != NULL);

    mutex_lock(&controller->sockets_mutex);
    if (controller->socket_count >= LLCP_CONTROLLER_MAX_SOCKETS) {
        mutex_unlock(&controller->sockets_mutex);
        LOG_ERROR("[LLCP Controller] Maximum number of sockets reached\n");
        return;
    }

    controller->sockets[controller->socket_count++] = *socket;
    mutex_unlock(&controller->sockets_mutex);
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
