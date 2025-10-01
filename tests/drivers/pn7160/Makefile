include ../Makefile.drivers_common

USEMODULE += pn7160_spi

CFLAGS += -DDEVELHELP
CFLAGS += -DLOG_LEVEL=LOG_DEBUG

USEMODULE += nfc_t4t
USEMODULE += nfc_t2t


# increase stack size for main thread
CFLAGS += -DMAIN_STACKSIZE=2048

include $(RIOTBASE)/Makefile.include
