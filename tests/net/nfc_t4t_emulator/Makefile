BOARD ?= nrf52840dk
include ../Makefile.net_common

USEMODULE += pn532_spi
USEMODULE += nfc_t4t
USEMODULE += ztimer_usec
USEMODULE += nfc_ndef

CFLAGS += -DISR_STACKSIZE=1024*2
CFLAGS += -Wno-error=cast-align
CFLAGS += -Wno-error=unused-variable
CFLAGS += -DLOG_LEVEL=LOG_DEBUG

include $(RIOTBASE)/Makefile.include
