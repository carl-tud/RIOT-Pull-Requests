BOARD ?= nrf52840dk
include ../Makefile.net_common

USEMODULE += nfc
USEMODULE += nfc_llcp
USEMODULE += gnrc_pktdump
USEMODULE += gnrc_udp
USEMODULE += gnrc_ipv6
USEMODULE += gnrc_ipv6_nib
USEMODULE += gnrc_ipv6_default
USEMODULE += gnrc_ipv6_nib_6ln
USEMODULE += gnrc_icmpv6_echo
USEMODULE += gnrc_icmpv6_error
USEMODULE += gnrc_sixlowpan_default
USEMODULE += gnrc_sixlowpan_iphc
USEMODULE += gnrc_sixlowpan_iphc
USEMODULE += gnrc_netif
USEMODULE += gnrc_netif_ipv6
#USEMODULE += gnrc_sixlowpan_router_default

ifneq ($(filter esp32-wroom-32,$(BOARD)),)
	# Include packages that pull up and auto-init the link layer.
	# NOTE: 6LoWPAN will be included if IEEE802.15.4 devices are present
	USEMODULE += netdev_default
	USEMODULE += auto_init_gnrc_netif
	# Specify the mandatory networking modules for 6LoWPAN border router
	USEMODULE += gnrc_sixlowpan_border_router_default
    # logic if 'value' is present in $(VARIABLE)
endif

USEMODULE += od
USEMODULE += shell
USEMODULE += shell_cmds_default
USEMODULE += shell_cmd_udp_server
USEMODULE += ps

USEMODULE += pn532_spi
USEMODULE += test_utils_interactive_sync

CFLAGS += -fdata-sections -ffunction-sections -g
CFLAGS += -DLOG_LEVEL=LOG_DEBUG

CFLAGS += -DTHREAD_STACKSIZE_MAIN=4000
CFLAGS += -DCONFIG_GNRC_IPV6_STABLE_PRIVACY=1
CFLAGS += -DCONFIG_NFC_HIGHER_LAYER_ACTIVATION_MESSAGE_CAPACITY=16
CFLAGS += -DCONFIG_LLCP_DEBUG=0
CFLAGS += -DCONFIG_LLCP_THREAD_SIZE=8192
CFLAGS += -DCONFIG_LLCP_DEBUG=0
CFLAGS += -DCONFIG_NFCDEV_DEBUG=0
CFLAGS += -DCONFIG_PN53_DEBUG=0
CFLAGS += -DCONFIG_PN53_DEBUG_REGISTERS=0
CFLAGS += -DCONFIG_PN53_DEBUG_HCI=0
CFLAGS += -DCONFIG_PN53_DEBUG_TRANSPORT=0

include $(RIOTBASE)/Makefile.include
