MODULE += nfc

ifneq (,$(filter nfc_llcp,$(USEMODULE)))
  DIRS += llcp
endif
ifneq (,$(filter nfc_mifare,$(USEMODULE)))
  DIRS += mifare
endif
ifneq (,$(filter nfc_ndef,$(USEMODULE)))
  DIRS += ndef
endif
ifneq (,$(filter nfc_t2t,$(USEMODULE)))
  DIRS += t2t
endif
ifneq (,$(filter nfc_t4t,$(USEMODULE)))
  DIRS += t4t
endif
ifneq (,$(filter nfc_generic,$(USEMODULE)))
  DIRS += generic
endif

include $(RIOTBASE)/Makefile.base
