include ../Makefile.drivers_common

USEMODULE += pn7160_spi

# enable debug
CFLAGS += -DLOG_LEVEL=LOG_DEBUG

USEMODULE += nfc_t2t

# increase stack size for main thread
CFLAGS += -DMAIN_STACKSIZE=2048

include $(RIOTBASE)/Makefile.include
