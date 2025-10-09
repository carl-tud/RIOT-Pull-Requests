BOARD ?= nrf52840dk
include ../Makefile.net_common

# USEMODULE += nrfx_nfct_dev
USEMODULE += pn532_spi

USEMODULE += nfc_t2t

# suppress errors
CFLAGS += -Wno-error=unused-function
CFLAGS += -Wno-error=unused-variable

CFLAGS += -DLOG_LEVEL=LOG_DEBUG

include $(RIOTBASE)/Makefile.include
