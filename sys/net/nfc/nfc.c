#include "net/nfc.h"

uint16_t iso_dep_frame_sizes[] = { 16,24,32,40,48,64,96,128,356,512,1024,2048,4096 };

const nfc_a_polling_command_t wupa = NFC_A_FRAME_CODE_POLLING_ALL;
const nfc_a_polling_frame_t nfc_a_polling_frame_all = {
    .frame = &wupa,
    .length = { .bytes = 1, .trailing_bits = 7 }
};

const nfc_a_polling_command_t reqa = NFC_A_FRAME_CODE_POLLING_ONLY_AWAKE;
const nfc_a_polling_frame_t nfc_a_polling_frame_only_awake = {
    .frame = &reqa,
    .length = { .bytes = 1, .trailing_bits = 7 }
};
