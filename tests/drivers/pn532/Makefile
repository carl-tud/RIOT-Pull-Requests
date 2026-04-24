include ../Makefile.drivers_common

USEMODULE += ztimer
USEMODULE += ztimer_msec

# select if you want to build the SPI or the I2C version of the driver:
#USEMODULE += pn532_i2c
USEMODULE += pn532_spi
USEMODULE += nfc
USEMODULE += ztimer_usec
USEMODULE += test_utils_interactive_sync

# set default device parameters in case they are undefined
TEST_PN53_BUS_I2C ?= I2C_DEV\(0\)
TEST_PN532_RESET ?= GPIO_PIN\(1,1\)
TEST_PN532_IRQ ?= GPIO_PIN\(1,2\)

# alternative SPI configuration
TEST_PN53_BUS_SPI ?= SPI_DEV\(0\)
TEST_PN532_NSS ?= GPIO_PIN\(1,3\)

# export parameters
CFLAGS += -DTEST_PN53_BUS_I2C=$(TEST_PN53_BUS_I2C)
CFLAGS += -DTEST_PN532_RESET=$(TEST_PN532_RESET)
CFLAGS += -DTEST_PN532_IRQ=$(TEST_PN532_IRQ)
CFLAGS += -DTEST_PN53_BUS_SPI=$(TEST_PN53_BUS_SPI)
CFLAGS += -DTEST_PN532_NSS=$(TEST_PN532_NSS)

CFLAGS += -I$(CURDIR)

CFLAGS += -Wno-unused-function
CFLAGS += -Wno-unused-variable

CFLAGS += -DTHREAD_STACKSIZE_MAIN=4000

include $(RIOTBASE)/Makefile.include
