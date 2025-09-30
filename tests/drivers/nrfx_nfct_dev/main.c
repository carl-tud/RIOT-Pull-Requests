#include "nrfx_nfct_dev.h"
#include "net/nfcdev.h"
#include "net/nfc/t2t/t2t_emulator.h"

#include "log.h"

static const uint8_t tag_memory[] = {
    '\x04', '\x12', '\x34', '\x56', // Internal
    '\x78', '\x9A', '\xBC', '\xF8', // Serial Number
    '\x4D', '\x00', '\x00', '\x00', // Internal / Lock
    '\xE1', '\x10', '\x06', '\x00', // Capability Container
    '\x03', '\x0C', '\xD1', '\x01', 
    '\x08',                         // TLV for NDEF message (Type, Length, Payload)
    '\x54', '\x02', 'e', 'n',
    'H',    'e',    'l', 'l', 
    'o',    '\xFE', '\x00', '\x00', 
    '\x00', '\x00', '\x00', '\x00', // Data
    '\x00', '\x00', '\x00', '\x00',  // Data
    '\x00', '\x00', '\x00', '\x00',  // Data
    '\x00', '\x00', '\x00', '\x00',  // Data
    '\x00', '\x00', '\x00', '\x00',  // Data
    '\x00', '\x00', '\x00', '\x00',  // Data
    '\x00', '\x00', '\x00', '\x00',  // Data
    '\x00', '\x00', '\x00'
};

int main(void) {

    nfcdev_t nfc_device = NRFX_NFCT_DEV_DEFAULT;

    nfc_t2t_emulator_t t2t_emulator;
    nfc_t2t_t *tag = (nfc_t2t_t *) tag_memory;

    LOG_DEBUG("NRFX NFCT Device Driver Test\n");
    nfc_device.ops->init(&nfc_device, NULL);
    LOG_DEBUG("Device initialized\n");

    nfc_a_nfcid1_t nfcid1 = {
        .len = 4,
        .nfcid = {0x04, 0x12, 0x34, 0x56}
    };
    t2t_emulator_start(&t2t_emulator, &nfc_device, tag, &nfcid1);
    return 0;
}
