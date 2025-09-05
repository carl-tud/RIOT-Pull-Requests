include ../Makefile.drivers_common

USEMODULE += ztimer
USEMODULE += ztimer_msec

# select if you want to build the SPI or the I2C version of the driver:
#USEMODULE += pn532_i2c
USEMODULE += pn532_uart
USEMODULE += ztimer_usec

# set default device parameters in case they are undefined
TEST_PN532_I2C ?= I2C_DEV\(0\)
TEST_PN532_RESET ?= GPIO_PIN\(1,1\)
TEST_PN532_IRQ ?= GPIO_PIN\(1,2\)

# alternative SPI configuration
TEST_PN532_SPI ?= SPI_DEV\(0\)
TEST_PN532_NSS ?= GPIO_PIN\(1,3\)

# export parameters
CFLAGS += -DTEST_PN532_I2C=$(TEST_PN532_I2C)
CFLAGS += -DTEST_PN532_RESET=$(TEST_PN532_RESET)
CFLAGS += -DTEST_PN532_IRQ=$(TEST_PN532_IRQ)
CFLAGS += -DTEST_PN532_SPI=$(TEST_PN532_SPI)
CFLAGS += -DTEST_PN532_NSS=$(TEST_PN532_NSS)

CFLAGS += -I$(CURDIR)

CFLAGS += -Wno-unused-function
CFLAGS += -Wno-unused-variable

include $(RIOTBASE)/Makefile.include
