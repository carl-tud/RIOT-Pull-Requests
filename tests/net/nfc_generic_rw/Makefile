BOARD ?= nrf52840dk
include ../Makefile.net_common


USEMODULE += nfc_generic
USEMODULE += nfc_llcp

USEMODULE += pn532_spi

CFLAGS += -fdata-sections -ffunction-sections -g
CFLAGS += -DLOG_LEVEL=LOG_DEBUG

include $(RIOTBASE)/Makefile.include
