BOARD ?= nrf52840dk
include ../Makefile.net_common


USEMODULE += nfc
USEMODULE += nfc_llcp

USEMODULE += pn532_spi
USEMODULE += test_utils_interactive_sync

CFLAGS += -fdata-sections -ffunction-sections -g
CFLAGS += -DLOG_LEVEL=LOG_DEBUG

CFLAGS += -DTHREAD_STACKSIZE_MAIN=4000

include $(RIOTBASE)/Makefile.include
