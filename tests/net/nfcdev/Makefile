BOARD ?= nrf52840dk
include ../Makefile.net_common

# USEMODULE += nrfx_nfct_dev
USEMODULE += pn532_spi

USEMODULE += ztimer_usec

CFLAGS += -Wno-error=unused-function

CFLAGS += -DISR_STACKSIZE=1024*2
CFLAGS += -Wno-error=cast-align
CFLAGS += -Wno-error=unused-variable
CFLAGS += -DLOG_LEVEL=LOG_DEBUG

include $(RIOTBASE)/Makefile.include
