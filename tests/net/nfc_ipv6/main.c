#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "od.h"
#include "byteorder.h"
#include "pn532.h"
#include "net/gnrc.h"
#include "net/gnrc/ipv6.h"
#include "net/gnrc/netif.h"
#include "net/netdev.h"
#include "net/nfcdev.h"
#include "net/nfc/llcp.h"

#define ENABLE_DEBUG 1
#include "debug.h"

#include "shell.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE     (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

int _shell_main(void) {
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
    puts("RIOT network stack example application");

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}

typedef struct {
    gnrc_netif_t super;
    nfc_llcp_socket_t llc;
    const iolist_t* pkt;
} netif_nfc_llcp_t;

extern void gnrc_netif_default_event_callback(netdev_t *dev, netdev_event_t event);

static nfc_llcp_socket_mode_t socket_mode = LLCP_SOCKET_MODE_ACCEPTING;
static nfc_role_t role = NFC_ROLE_TARGET;

static void printbuff(uint8_t* buff, size_t length) {
    while (length) {
        length--;
        printf("%02x ", *buff++);
    }
    puts("");
}

static int _init(gnrc_netif_t* netif) {
    puts("netif init");
    netif_register(&netif->netif);
#if IS_USED(MODULE_GNRC_SIXLOWPAN_FRAG_SFR)
    gnrc_sixlowpan_frag_sfr_init_iface(netif);
#endif
    netif->cur_hl = CONFIG_GNRC_NETIF_DEFAULT_HL;
#ifdef MODULE_GNRC_IPV6_NIB
    gnrc_ipv6_nib_init_iface(netif);
#endif
    netif_nfc_llcp_t* super = container_of(netif, netif_nfc_llcp_t, super);
    nfc_llcp_socket_t* llc = &super->llc;
    netif->l2addr[RFC9428_L2_ADDR_LEN - 1] = llc->ssap;
    netif->l2addr_len = RFC9428_L2_ADDR_LEN;
    if (nfc_llcp_socket_is_connectionless(llc)) {
        // Link is always up, does not need to be connected
        netif->dev->event_callback(netif->dev, NETDEV_EVENT_LINK_UP);
    }
    return 0;
}

static gnrc_pktsnip_t* _recv(gnrc_netif_t* netif) {
    puts("netif recv");
    netif_nfc_llcp_t* super = container_of(netif, netif_nfc_llcp_t, super);

    if (netif->flags & GNRC_NETIF_FLAGS_RAWMODE) {
        DEBUG("gnrc_netif_nfc6: cannot recv in raw mode\n");
        return NULL;
    }

    if (!super->pkt || !super->pkt->iol_next || !super->pkt->iol_next->iol_len) {
        return NULL;
    }
    assert(super->pkt->iol_len >= sizeof(nfc_llcp_header_t));
    assert(super->pkt->iol_base);

    nfc_llcp_header_t header = { .raw = byteorder_bebuftohs(super->pkt->iol_base) };
    uint8_t ssap = header.ssap;
    uint8_t dsap = header.dsap;
    iolist_t* payload = super->pkt->iol_next;

#ifdef MODULE_L2FILTER
    if (!l2filter_pass(dev->filter, &ssap, sizeof(ssap))) {
        DEBUG("gnrc_netif_nfc6: incoming packet filtered by l2filter\n");
        goto safe_out;
    }
#endif

    size_t payload_length = iolist_size(payload);
    gnrc_pktsnip_t* pkt = gnrc_pktbuf_add(NULL, NULL, payload_length, GNRC_NETTYPE_SIXLOWPAN);
    if (!pkt) {
        DEBUG("gnrc_netif_nfc6: cannot allocate pktsnip\n");
        goto out;
    }
    if (iolist_to_buffer(payload, pkt->data, pkt->size) < 0) {
        DEBUG("gnrc_netif_nfc6: cannot copy into pktsnip\n");
        goto safe_out;
    }

#ifdef MODULE_NETSTATS_L2
    netif->stats.rx_count += 1;
    netif->stats.rx_bytes += iolist_size(super->pkt);
#endif

    DEBUG("gnrc_netif_nfc6: received packet from ssap 0x%02x to 0x%02x\n", header.ssap, header.dsap);
#if defined(MODULE_OD) && ENABLE_DEBUG
    od_hex_dump(pkt->data, payload_length, OD_WIDTH_DEFAULT);
#endif

    /* create netif header */
    gnrc_pktsnip_t* netif_header = gnrc_pktbuf_add(NULL, NULL,
                                                   sizeof(gnrc_netif_hdr_t) + RFC9428_L2_ADDR_LEN * 2,
                                                   GNRC_NETTYPE_NETIF);
    if (!netif_header) {
        DEBUG("gnrc_netif_nfc6: no space left in packet buffer\n");
        goto safe_out;
    }

    gnrc_netif_hdr_t* _netif_header = (gnrc_netif_hdr_t*)netif_header->data;
    gnrc_netif_hdr_init(_netif_header, RFC9428_L2_ADDR_LEN, RFC9428_L2_ADDR_LEN);
    memset(((uint8_t*)(_netif_header + 1)), 0, 2 * RFC9428_L2_ADDR_LEN);
    ((uint8_t*)(_netif_header + 1))[RFC9428_L2_ADDR_LEN - 1] = ssap;
    ((uint8_t*)(_netif_header + 1))[RFC9428_L2_ADDR_LEN * 2 - 1] = dsap;
    //gnrc_netif_hdr_set_src_addr(netif_header->data, &ssap, sizeof(ssap));
    //gnrc_netif_hdr_set_dst_addr(netif_header->data, &dsap, sizeof(dsap));
    gnrc_netif_hdr_set_netif(_netif_header, netif);
    // gnrc_netif_hdr_set_timestamp(netif_header->data, <#timestamp#>);

#if defined(MODULE_OD) && ENABLE_DEBUG
    DEBUG("gnrc_netif_nfc6: telling GNRC l2:\n");
    DEBUG(" src: (%u)\n", _netif_header->src_l2addr_len);
    od_hex_dump(gnrc_netif_hdr_get_src_addr(_netif_header), _netif_header->src_l2addr_len, OD_WIDTH_DEFAULT);
    DEBUG("\n  dst: (%u)\n", _netif_header->dst_l2addr_len);
    od_hex_dump(gnrc_netif_hdr_get_dst_addr(_netif_header), _netif_header->dst_l2addr_len, OD_WIDTH_DEFAULT);
    DEBUG("\n");
#endif

    pkt = gnrc_pkt_append(pkt, netif_header);

out:
    return pkt;

safe_out:
    gnrc_pktbuf_release(pkt);
    return NULL;
}

static int _send(gnrc_netif_t* netif, gnrc_pktsnip_t* pkt) {
    (void)netif;
    (void)pkt;
    puts("netif send");
    nfc_llcp_socket_t* llc = &container_of(netif, netif_nfc_llcp_t, super)->llc;
    uint8_t ssap = llc->ssap;
    uint8_t dsap = llc->dsap;
    int res = 0;

    if (pkt == NULL) {
        DEBUG("gnrc_netif_nfc6: pkt was NULL\n");
        return -EINVAL;
    }
    if (pkt->type != GNRC_NETTYPE_NETIF) {
        DEBUG("gnrc_netif_nfc6: first header is not generic netif header\n");
        return -EBADMSG;
    }
    gnrc_netif_hdr_t* netif_header = pkt->data;

    // Could apply special treatment to these scenarios, but should work without, too.
    if (netif_header->flags & GNRC_NETIF_HDR_FLAGS_MORE_DATA) {

    }
    if (netif_header->flags & (GNRC_NETIF_HDR_FLAGS_BROADCAST | GNRC_NETIF_HDR_FLAGS_MULTICAST)) {

    }

#if defined(MODULE_OD) && ENABLE_DEBUG
    DEBUG("gnrc_netif_nfc6: GNRC told us l2:\n");
    DEBUG(" src: (%u)\n", netif_header->src_l2addr_len);
    od_hex_dump(gnrc_netif_hdr_get_src_addr(netif_header), netif_header->src_l2addr_len, OD_WIDTH_DEFAULT);
    DEBUG("\n  dst: (%u)\n", netif_header->dst_l2addr_len);
    od_hex_dump(gnrc_netif_hdr_get_dst_addr(netif_header), netif_header->dst_l2addr_len, OD_WIDTH_DEFAULT);
    DEBUG("\n");
#endif

    if (netif_header->dst_l2addr_len > RFC9428_L2_ADDR_LEN) {
        DEBUG("gnrc_netif_nfc6: dst addr too long\n");
        return -EINVAL;
    };
    if (netif_header->src_l2addr_len > RFC9428_L2_ADDR_LEN) {
        DEBUG("gnrc_netif_nfc6: src addr too long\n");
        return -EINVAL;
    };

    if (netif_header->src_l2addr_len == RFC9428_L2_ADDR_LEN) {
        ssap = gnrc_netif_hdr_get_src_addr(netif_header)[RFC9428_L2_ADDR_LEN - 1];
    }

    if (netif_header->dst_l2addr_len == RFC9428_L2_ADDR_LEN) {
        dsap = gnrc_netif_hdr_get_dst_addr(netif_header)[RFC9428_L2_ADDR_LEN - 1];
    }
    DEBUG("gnrc_netif_nfc6: sending from ssap 0x%02x to dsap 0x%02x\n", ssap, dsap);

#ifdef MODULE_NETSTATS_L2
    if (netif_header->flags & (GNRC_NETIF_HDR_FLAGS_BROADCAST | GNRC_NETIF_HDR_FLAGS_MULTICAST)) {
        netif->stats.tx_mcast_count++;
    } else {
        netif->stats.tx_unicast_count++;
    }
#endif

    res = (int)nfc_llcp_socket_send_chunks(llc, (iolist_t *)pkt->next);

    //if (gnrc_netif_netdev_legacy_api(netif)) {
        /* only for legacy drivers we need to release pkt here */
        gnrc_pktbuf_release(pkt);
    //}
    return res;
}

static int _get(gnrc_netif_t* netif, gnrc_netapi_opt_t* opt) {
    printf("netif get %u\n", opt->opt);
    int res = -ENOTSUP;
    if (true) {
        return res;
    }

    size_t max_len = opt->data_len;
    void* value = opt->data;
    switch (opt->opt) {
        case NETOPT_ADDRESS:
            assert(max_len >= netif->l2addr_len);
            memcpy(value, netif->l2addr, netif->l2addr_len);
            res = netif->l2addr_len;
            break;
        case NETOPT_ADDR_LEN:
        case NETOPT_SRC_LEN:
            assert(max_len == sizeof(uint16_t));
            *((uint16_t*)value) = netif->l2addr_len;
            res = sizeof(uint16_t);
            break;
        case NETOPT_RAWMODE:
//            assert(max_len == sizeof(netopt_enable_t));
//            if (dev->flags & NETDEV_NFC_LLCP_RAW) {
//                *((netopt_enable_t *)value) = NETOPT_ENABLE;
//            }
//            else {
//                *((netopt_enable_t *)value) = NETOPT_DISABLE;
//            }
//            res = sizeof(netopt_enable_t);
            res = -ENOTSUP;
            break;
#ifdef MODULE_GNRC
        case NETOPT_PROTO:
            assert(max_len == sizeof(gnrc_nettype_t));
            *((gnrc_nettype_t *)value) = GNRC_NETTYPE_SIXLOWPAN;
            res = sizeof(gnrc_nettype_t);
            break;
#endif
        case NETOPT_DEVICE_TYPE:
            assert(max_len == sizeof(uint16_t));
            *((uint16_t *)value) = NETDEV_TYPE_NFC6;
            res = sizeof(uint16_t);
            break;
#ifdef MODULE_L2FILTER
        case NETOPT_L2FILTER:
            assert(max_len >= sizeof(l2filter_t **));
            *((l2filter_t **)value) = dev->netdev.filter;
            res = sizeof(l2filter_t **);
            break;
#endif
        case NETOPT_MAX_PDU_SIZE:
            assert(max_len >= sizeof(int16_t));
            *((uint16_t *)value) = IPV6_MIN_MTU;
            res = sizeof(uint16_t);
            break;
        default:
            break;
    }
    return res;
}

static int _set(gnrc_netif_t *netif, const gnrc_netapi_opt_t* opt) {
    (void)netif;
    puts("netif set");
    int res = -ENOTSUP;

    size_t len = opt->data_len;
    void* value = opt->data;
    switch (opt->opt) {
        case NETOPT_ADDRESS:
            puts("setting l2addr?");
            res = -ENOTSUP;
            break;
        case NETOPT_RAWMODE:
//            if ((*(bool *)value)) {
//                dev->flags |= NETDEV_NFC_LLCP_RAW;
//            }
//            else {
//                dev->flags &= ~NETDEV_NFC_LLCP_RAW;
//            }
//            res = sizeof(uint16_t);
            res = -ENOTSUP;
            break;
#ifdef MODULE_GNRC
        case NETOPT_PROTO:
            assert(len == sizeof(gnrc_nettype_t));
            res = *((gnrc_nettype_t *)value) == GNRC_NETTYPE_SIXLOWPAN ? (int)sizeof(gnrc_nettype_t) : -EINVAL;
            break;
#endif
#ifdef MODULE_L2FILTER
        case NETOPT_L2FILTER:
            res = l2filter_add(dev->netdev.filter, value, len);
            break;
        case NETOPT_L2FILTER_RM:
            res = l2filter_rm(dev->netdev.filter, value, len);
            break;
#endif
        default:
            break;
    }
    return res;
}

static const gnrc_netif_ops_t netif_ops = {
    .init = _init,
    .send = _send,
    .recv = _recv,
    .get = gnrc_netif_get_from_netdev,
    .set = gnrc_netif_set_from_netdev,
};

static int _netdev_send(netdev_t *dev, const iolist_t *iolist) {
    puts("netdev send");
    (void)dev;
    (void)iolist;
    return -ENOTSUP;
}

static int _netdev_recv(netdev_t *dev, void *buf, size_t len, void *info) {
    puts("netdev receive");
    (void)dev;
    (void)buf;
    (void)len;
    (void)info;
    return -ENOTSUP;
}

static int _netdev_init(netdev_t *dev) {
    puts("netdev init");
    (void)dev;
    return -ENOTSUP;
}

static void _netdev_isr(netdev_t *dev) {
    (void)dev;
}

static int _netdev_get(netdev_t *dev, netopt_t opt, void *value, size_t max_len) {
    puts("netdev get");
    (void)dev;
    (void)opt;
    (void)value;
    (void)max_len;
    gnrc_netapi_opt_t _opt = { .opt = opt, .data = (void*)value, .data_len = max_len };
    return _get(dev->context, &_opt);
}

static int _netdev_set(netdev_t *dev, netopt_t opt, const void *value, size_t value_len) {
    puts("netdev set");
    gnrc_netapi_opt_t _opt = { .opt = opt, .data = (void*)value, .data_len = value_len };
    return _set(dev->context, &_opt);
}

const netdev_driver_t netdev_driver = {
    .send = _netdev_send,
    .recv = _netdev_recv,
    .init = _netdev_init,
    .isr = _netdev_isr,
    .get = _netdev_get,
    .set = _netdev_set,
};

static void _llc_event(nfc_llcp_socket_t* socket, nfc_llcp_socket_event_t llc_event, const iolist_t* payload) {
    netif_nfc_llcp_t* netif = container_of(socket, netif_nfc_llcp_t, llc);
    netdev_t* netdev = netif->super.dev;
    switch (llc_event) {
        case LLCP_SOCKET_EVENT_RX:
            netif->pkt = payload;
            // This event callback is supposed to call netif->recv
            netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
            netif->pkt = NULL;
            break;
        case LLCP_SOCKET_EVENT_TX:
            netif->pkt = payload;
            // This event callback is supposed to call netif->recv
            netdev->event_callback(netdev, NETDEV_EVENT_TX_COMPLETE);
            netif->pkt = NULL;
            break;
        case LLCP_SOCKET_EVENT_CONNECTED:
            printf("gnrc_netif_nfc6: assigning l2addr 0x%02x\n", socket->ssap);
            netif->super.l2addr[RFC9428_L2_ADDR_LEN - 1] = socket->ssap;
            netif->super.l2addr_len = RFC9428_L2_ADDR_LEN;
            netdev->event_callback(netdev, NETDEV_EVENT_LINK_UP);
            break;
        case LLCP_SOCKET_EVENT_DISCONNECTED:
            netdev->event_callback(netdev, NETDEV_EVENT_LINK_DOWN);
            break;
        break;
    }

}

#include "stdio_base.h"

static int _read_toggle(const char* prompt,
                        char char_a, int value_a,
                        char char_b, int value_b,
                        char char_default
) {
    char c = '\0';
    do {
        if (c == '.') {
            c = char_default;
            break;
        }
        else if (c != '\n' && c != '\r') {
            printf("Enter %s (default: %c) [%c|%c|.]:\n", prompt, char_default, char_a, char_b);
        }
        c = getchar();
    } while (c != char_a && c!= char_b);

    if (c == '.') {
        c = char_default;
    }

    printf("(got '%c')\n", c);
    if (c == char_a) {
        return value_a;
    } else if (c == char_b) {
        return value_b;
    } else {
        printf("Invalid input!\n");
        return -EINVAL;
    }
}

int main(void) {
    static const pn532_connection_config_t config = {
        .bus = {
            .kind = PN53_BUS_SPI,
            .spi = SPI_DEV(0),
        },
#if IS_USED(MODULE_PN532_SPI)
            //.chip_select = GPIO_PIN(1, 8),
            .chip_select = GPIO_PIN(0, 5),
#endif
            //.reset = GPIO_PIN(0, 7),
            .reset = GPIO_PIN(0, 4),
            .irq = GPIO_PIN(0, 26),
    };
    static pn532_dev_t pn532 = {};
    static nfcdev_t dev = {};

    ssize_t res = 0;

    role = _read_toggle("NFC role, (t)arget or (i)nitiator?",
                        't', NFC_ROLE_TARGET, 'i', NFC_ROLE_INITIATOR, 't');
    socket_mode = _read_toggle("LLCP socket mode, (c)onnect or (a)ccept",
                               'c', LLCP_SOCKET_MODE_CONNECTING,
                               'a', LLCP_SOCKET_MODE_ACCEPTING, 'a');

    if ((res = nfcdev_init_pn532(&dev, &pn532, &config)) < 0) {
        printf("init: error %" PRIdSIZE " \n", res);
        return (int)res;
    }

    puts("device initiliazed");


    static nfcdev_tag_polling_config_t polling_config_a = {
        .technology = NFC_TECHNOLOGY_A
    };


    static nfcdev_tag_polling_config_t polling_config_f = {
        .technology = NFC_TECHNOLOGY_F,
    };

#define GENERAL_BYTES_LENGTH (3 + 3 + 3)

    static const nfc_dep_activation_request_t atr_req = {
        .id = { 1,2,3,4,5,6,7,8,9,10 },
        .device_id = 0x42,
        .general_bytes_available = true,
        .payload_reduction = NFC_DEP_PAYLOAD_LIMIT_252,
        .general_bytes = {
            NFC_DEP_GENERAL_BYTE0_LLCP,
            NFC_DEP_GENERAL_BYTE1_LLCP,
            NFC_DEP_GENERAL_BYTE2_LLCP,

            LLCP_PARAMETER_TLV_VERSION,
                0x01,
                0x10,
            LLCP_PARAMETER_TLV_LTO,
                0x01,
                0x32,
        }
    };

    static nfc_dep_target_t nfc_dep_target =  {
        .length = sizeof(nfc_dep_activation_response_t) + GENERAL_BYTES_LENGTH,
        .atr = {
            .id = { 1,2,3,4,5,6,7,8,9,10 },
            .device_id = 0x42,
            .general_bytes_available = true,
        },
        .general = {
            NFC_DEP_GENERAL_BYTE0_LLCP,
            NFC_DEP_GENERAL_BYTE1_LLCP,
            NFC_DEP_GENERAL_BYTE2_LLCP,

            LLCP_PARAMETER_TLV_VERSION,
                0x01,
                0x10,
            LLCP_PARAMETER_TLV_LTO,
                0x01,
                0x32
        }
    };

    static nfcdev_polling_loop_t polling_loops[] = {
        {
            .bitrate = NFC_BITRATE_106K,
            .timing = {
                .interval = NFCDEV_POLLING_INTERVAL_BUILTIN /* ms */,
                .retries = 5,
                .guard_time = 10,
            },
            .tag = &polling_config_a,
            .higher_layer.nfc_dep = {
                .length = sizeof(nfc_dep_activation_request_t) + GENERAL_BYTES_LENGTH,
                .atr = &atr_req
            },
            .higher_layer.bitrate_selector = NFC_SELECT_FASTEST_UP_TO(NFC_BITRATE_848K),
            .field_mode = NFC_FIELD_MODE_READER_WRITER_TAG,
        },
        {
            .bitrate = NFC_BITRATE_212K,
            .timing = {
                .interval = NFCDEV_POLLING_INTERVAL_BUILTIN /* ms */,
                .retries = 5,
                .guard_time = 10,
            },
            .tag = &polling_config_f,
            .higher_layer.nfc_dep = {
                .length = sizeof(nfc_dep_activation_request_t) + GENERAL_BYTES_LENGTH,
                .atr = &atr_req
            },
            .higher_layer.bitrate_selector = NFC_SELECT_FASTEST_UP_TO(NFC_BITRATE_848K),
            .field_mode = NFC_FIELD_MODE_READER_WRITER_TAG,
        },
        {
            .bitrate = NFC_BITRATE_212K,
            .timing = {
                .interval = NFCDEV_POLLING_INTERVAL_BUILTIN /* ms */,
                .retries = 5,
                .guard_time = 10,
            },
            .higher_layer.nfc_dep = {
                .length = sizeof(nfc_dep_activation_request_t) + GENERAL_BYTES_LENGTH,
                .atr = &atr_req
            },
            .higher_layer.bitrate_selector = NFC_SELECT_FASTEST_UP_TO(NFC_BITRATE_848K),
            .field_mode = NFC_FIELD_MODE_PEERS,
        },
    };

    static nfcdev_polling_config_t polling_config = {
        .loops = polling_loops,
        .loop_count = ARRAY_SIZE(polling_loops),
        .repetitions = NFCDEV_POLLING_REPETIONS_INFINITE
    };

    static nfc_a_tag_t tag_a = {
        .id = {
            .length = 4,
            .uid = { 0x80, 0x42, 0x00, 0x66 }
        },
        .polling_response = {
            .uid_size_indicator = 0,
            .bit_frame_anticollision = 4,
        },
        .select_response = NFC_A_SELECT_RESPONSE_FLAG_NFC_DEP,
    };
    tag_a.ats = pn532_builtin_ats;

    static nfc_f_tag_t tag_f = {
        .id.raw = { 1,0xFE,3,4,5,6,7,8 },
        .pmm.raw = {},
        .system_code = NFC_F_SYSTEM_CODE_NDEF
    };

    static nfcdev_listening_config_t listening_config = {
        .tag = {
            .technologies = NFC_TECHNOLOGY_A | NFC_TECHNOLOGY_F,
            .a = &tag_a,
            .f = &tag_f
        },
        .higher_layer.nfc_dep = &nfc_dep_target,
        .bitrates = {
            .tag.a = NFC_BITRATE_106K,
            .tag.f = NFC_BITRATE_212K | NFC_BITRATE_424K,
            .peer = NFC_BITRATE_106K | NFC_BITRATE_212K | NFC_BITRATE_424K,
        }
    };

    nfc_target_t target;

    puts("");

    uint8_t* general_bytes = NULL;
    size_t general_length = 0;

    switch (role) {
        case NFC_ROLE_INITIATOR: {
            puts("Polling");

            if ((res = nfcdev_poll(&dev, &polling_config, &target, NULL, 1)) < 0) {
                printf("poll: error %" PRIdSIZE " \n", res);
                return res;
            }
            if (target.higher_layer.nfc_dep.length < sizeof(nfc_dep_activation_response_t)) {
                puts("poll: ATR_RES missing");
                return -1;
            }
            general_bytes = target.higher_layer.nfc_dep.atr.general_bytes;
            general_length = nfc_dep_atr_response_general_length(&target.higher_layer.nfc_dep);
            break;
        }
        case NFC_ROLE_TARGET: {
            puts("Listening");

            if ((res = nfcdev_listen(&dev, &listening_config, &target, PN53_TIMEOUT_NEVER)) < 0) {
                printf("listen: error %" PRIdSIZE " \n", res);
                return res;
            }
            nfc_dep_activation_request_t* atr = NULL;
            if ((res = pn53_listen_get_atr_request(&pn532, &atr)) < 0 || (size_t)res < sizeof(nfc_dep_activation_request_t)) {
                puts("listen: ATR_REQ missing");
                return -1;
            }

            general_bytes = atr->general_bytes;
            general_length = _nfc_dep_atr_request_general_length((size_t)res);
            break;
        }
        default:
            puts("role invalid");
            return -1;
    }

    puts("");
    printf("target: ");
    nfc_print_target(&target);
    printf("\n");

    static uint8_t prefix[] = {
        NFC_DEP_GENERAL_BYTE0_LLCP,
        NFC_DEP_GENERAL_BYTE1_LLCP,
        NFC_DEP_GENERAL_BYTE2_LLCP,
    };
    if (general_length < 3 || memcmp(general_bytes, prefix, sizeof(prefix)) != 0) {
        puts("missing Ffm general bytes prefix");
        return -1;
    }

    puts("general bytes:");
    printbuff(general_bytes, general_length);
    puts("");

    static nfc_llcp_controller_t controller = {};
    if (nfc_llcp_controller_init(&controller, &dev, general_bytes + sizeof(prefix), general_length - sizeof(prefix)) < 0) {
        puts("failed to init controller");
        return -1;
    }

    puts("LLCP running");

    static netdev_t netdev = {
        .event_callback = gnrc_netif_default_event_callback
    };
    static netif_nfc_llcp_t netif = {
        .super = {
            .dev = NULL,
            .device_type = NETDEV_TYPE_NFC6,
            .l2addr = { 0 },
            .l2addr_len = 0,
            .ipv6 = {
                .mtu = IPV6_MIN_MTU,
            },
            .flags = GNRC_NETIF_FLAGS_HAS_L2ADDR
                | GNRC_NETIF_FLAGS_6LO
                | GNRC_NETIF_FLAGS_6LO_HC
                | GNRC_NETIF_FLAGS_6LN
                | GNRC_NETIF_FLAGS_IPV6_STABLE_PRIVACY,
        },
        .llc = {
            // the address values between 0x(0)2 (?) and 0x3f can be used for generating IPv6 IIDs
            // If we are supposed to accept a connection, leave this empty, the other
            // peer will set this to the link they open (which will be SSAP=0x24, DSAP=0x22)
            .ssap = 0,
            .dsap = 0,
            // .service_name = "urn:nfc:xsn:6lo",
            .event_callback = _llc_event
        }
    };

    if (socket_mode == LLCP_SOCKET_MODE_CONNECTING) {
        netif.llc.ssap = 0x22;
        netif.llc.dsap = 0x24;
    }

    if (nfc_llcp_controller_add_socket(&controller, &netif.llc, socket_mode) < 0) {
        puts("failed to add LLCP socket");
        return -1;
    }
    puts("LLCP socket registered");

    netdev.driver = &netdev_driver;
    netdev_register(&netdev, NETDEV_EXPERIMENTAL_NFC_LLCP, 0);

    static char netif_stack[4000];
    if ((res = gnrc_netif_create(&netif.super, netif_stack, sizeof(netif_stack), GNRC_NETIF_PRIO, "nfcllc0", &netdev, &netif_ops)) < 0) {
        printf("netif registration: error %" PRIdSIZE " \n", res);
        return res;
    }
    netdev.context = &netif;

    _shell_main();
}
