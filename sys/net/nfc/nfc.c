#include "net/nfc.h"

uint16_t iso_dep_frame_sizes[] = { 16,24,32,40,48,64,96,128,356,512,1024,2048,4096 };

const nfc_a_polling_command_t _nfc_a_poll_all = NFC_A_FRAME_CODE_POLLING_ALL;
const nfc_a_polling_frame_t nfc_a_polling_frame_all = {
    .frame = &_nfc_a_poll_all,
    .length = { .trailing_bits = 7 }
};

const nfc_a_polling_command_t _nfc_a_poll_only_awake = NFC_A_FRAME_CODE_POLLING_ONLY_AWAKE;
const nfc_a_polling_frame_t nfc_a_polling_frame_only_awake = {
    .frame = &_nfc_a_poll_only_awake,
    .length = { .trailing_bits = 7 }
};
