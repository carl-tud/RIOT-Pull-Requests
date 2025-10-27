#include "net/nfc/llcp.h"

#include "log.h"

int nfc_llcp_socket_init(nfc_llcp_socket_t *socket, uint8_t ssap, uint8_t dsap,
    nfc_llcp_socket_mode_t mode) {
    assert(socket != NULL);
    assert(ssap <= 0x3F);
    assert(dsap <= 0x3F);
    
    if (mode != LLCP_SOCKET_MODE_CONNECTIONLESS) {
        LOG_ERROR("[LLCP Socket] Only connectionless mode is supported\n");
        return -1;
    }

    tsrb_init(&socket->rx_buffer, socket->rx_buffer_data, sizeof(socket->rx_buffer_data));
    tsrb_init(&socket->tx_buffer, socket->tx_buffer_data, sizeof(socket->tx_buffer_data));
    socket->ssap = ssap;
    socket->dsap = dsap;

    return 0;
}

int nfc_llcp_socket_receive(nfc_llcp_socket_t *socket, uint8_t *buffer, size_t *buffer_len) {
    assert(socket != NULL);
    assert(buffer != NULL);
    assert(buffer_len != NULL);

    int received_len = tsrb_get_one(&socket->rx_buffer);
    if (received_len <= 0) {
        return -1; /* no data available */
    }

    if (*buffer_len < (size_t)received_len) {
        return -1; /* buffer too small */
    }

    int ret  = tsrb_get(&socket->rx_buffer, buffer, received_len);
    if (ret < 0) {
        return -1; /* error */
    }

    *buffer_len = received_len;

    return 0;
}

int nfc_llcp_socket_send(nfc_llcp_socket_t *socket, const uint8_t *data, uint8_t data_len) {
    assert(socket != NULL);
    assert(data != NULL);
    assert(data_len > 0);
    assert(data_len <= LLCP_MAX_PDU_SIZE - 2); /* 2 bytes for DSAP, SSAP, PTYPE */

    /* first put the length byte into the buffer */
    int ret = tsrb_add_one(&socket->tx_buffer, data_len);
    if (ret < 0) {
        return -1; /* error */
    }

    ret = tsrb_add(&socket->tx_buffer, data, data_len);
    if (ret < 0) {
        return -1; /* error */
    }

    return 0;
}
