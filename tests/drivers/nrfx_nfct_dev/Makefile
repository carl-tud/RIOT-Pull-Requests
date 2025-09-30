include ../Makefile.drivers_common

USEPKG += nrfx

USEMODULE += nrfx_nfct
USEMODULE += nfc_t2t
USEMODULE += nrfx_nfct_dev

# enable debugging
CFLAGS += -DLOG_LEVEL=LOG_DEBUG
CFLAGS += -DISR_STACKSIZE=1024*2

CFLAGS += -Wno-error=cast-align
CFLAGS += -Wno-error=unused-variable

include $(RIOTBASE)/Makefile.include
