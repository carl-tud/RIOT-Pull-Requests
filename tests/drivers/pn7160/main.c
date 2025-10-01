#include "pn7160.h"
#include "net/nfcdev.h"
#include "net/nfc/t2t/t2t.h"
#include "net/nfc/t2t/t2t_rw.h"
#include "net/nfc/t4t/t4t_emulator.h"
#include "net/nfc/nfc_a.h"

#include "log.h"

#define MAX_NDEF_RECORDS 8

// static const uint8_t tag_memory[] = {
//     '\x04', '\x12', '\x34', '\x56', // Internal
//     '\x78', '\x9A', '\xBC', '\xF8', // Serial Number
//     '\x4D', '\x00', '\x00', '\x00', // Internal / Lock
//     '\xE1', '\x10', '\x06', '\x00', // Capability Container
//     '\x03', '\x0C', '\xD1', '\x01', 
//     '\x08',                         // TLV for NDEF message (Type, Length, Payload)
//     '\x54', '\x02', 'e', 'n',
//     'H',    'e',    'l', 'l', 
//     'o',    '\xFE', '\x00', '\x00', 
//     '\x00', '\x00', '\x00', '\x00', // Data
//     '\x00', '\x00', '\x00', '\x00',  // Data
//     '\x00', '\x00', '\x00', '\x00',  // Data
//     '\x00', '\x00', '\x00', '\x00',  // Data
//     '\x00', '\x00', '\x00', '\x00',  // Data
//     '\x00', '\x00', '\x00', '\x00',  // Data
//     '\x00', '\x00', '\x00', '\x00',  // Data
//     '\x00', '\x00', '\x00'
// };

static uint8_t ndef_buf[256];

static nfc_t4t_t t4t;

static void _test_rw(nfc_t2t_rw_t *rw, nfcdev_t *dev) {
    ndef_t ndef;
    ndef_init(&ndef, ndef_buf, sizeof(ndef_buf));

    nfc_t2t_rw_read_ndef(rw, &ndef, dev);

    ndef_record_desc_t records[MAX_NDEF_RECORDS];
    ndef_parse(&ndef, records, MAX_NDEF_RECORDS);
    ndef_pretty_print(records, ndef.record_count);
    LOG_DEBUG("pn7160 rw test done\n");
}

/* the PN7160 does not support T2T because it can only communicatewith the ISO-DEP interface */
static void _test_emulator(nfc_t4t_emulator_t *emulator, nfcdev_t *dev) {
    ndef_t ndef;

    ndef_init(&ndef, ndef_buf, sizeof(ndef_buf));
    ndef_record_add_text(&ndef, "Hello from PN7160 T4T Emulator", 20, "en", 2, UTF8);
    t4t_init(&t4t, 64, ndef_buf, sizeof(ndef_buf));
    nfc_a_nfcid1_t nfcid = {
        .nfcid = {0x08, 0x12, 0x34, 0x56},
        .len = 4
    };
    t4t_emulator_start(emulator, dev, &t4t, &nfcid);
}

int main(void) {
    LOG_DEBUG("pn7160 test\n");

    pn7160_t dev;
    pn7160_params_t params = {
        .spi = SPI_DEV(0),
        .reset = GPIO_PIN(1, 6),
        .irq = GPIO_PIN(1, 7),
        .nss = GPIO_PIN(1, 8),
        .mode = PN7160_SPI
    };

    nfcdev_t nfc_device = {
        .dev = &dev,
        .config = (void *) &params,
        .ops = &pn7160_ops,
        .state = NFCDEV_STATE_DISABLED
    };

    nfc_t2t_rw_t t2t_rw;
    nfc_t4t_emulator_t t4t_emulator;
    nfc_device.ops->init(&nfc_device, &params);

    _test_rw(&t2t_rw, &nfc_device);

    _test_emulator(&t4t_emulator, &nfc_device);


    return 0;
}
