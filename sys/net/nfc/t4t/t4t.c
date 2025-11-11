#include "net/nfc/t4t/t4t.h"
#include "net/nfc/ndef/ndef.h"

#include <memory.h>
#include <assert.h>
#include <stdio.h>

#define T4T_NDEF_FILE_ID 0xE104

int t4t_from_ndef(nfc_t4t_t *tag, const ndef_t *ndef_msg) {
    assert(tag != NULL);
    assert(ndef_msg != NULL);

    if (ndef_get_size(ndef_msg) + 2 > tag->cc_file.ndef_file_control_tlv.max_ndef_size) {
        return -1;
    } else {
        memcpy(tag->ndef_file + 2, ndef_msg->buffer.memory, ndef_get_size(ndef_msg));
        return 0;
    }
}

bool t4t_is_writable(const nfc_t4t_t *tag) {
    assert(tag != NULL);

    if (tag->cc_file.ndef_file_control_tlv.write_access == 0x00) {
        return true; /* writable without any security */
    } else if (tag->cc_file.ndef_file_control_tlv.write_access == 0xFF) {
        return false; /* not writable */
    } else {
        return false; /* write access with some security, not implemented yet */
    }
}

static int t4t_init_cc_file(nfc_t4t_t *tag, uint16_t max_apdu_size, size_t max_ndef_file_size) {
    if (tag == NULL) {
        return -1;
    }

    tag->cc_file.cc_len = sizeof(t4t_cc_file_t);
    tag->cc_file.mapping_version = 0x20; /* version 2.0 */
    tag->cc_file.mle = max_apdu_size;
    tag->cc_file.mlc = max_apdu_size;

    tag->cc_file.ndef_file_control_tlv.type = T4T_NDEF_FILE_TLV_TYPE;
    tag->cc_file.ndef_file_control_tlv.length = T4T_NDEF_FILE_TLV_LEN;
    tag->cc_file.ndef_file_control_tlv.file_id = T4T_NDEF_FILE_ID; /* NDEF file ID */
    tag->cc_file.ndef_file_control_tlv.max_ndef_size = max_ndef_file_size;
    tag->cc_file.ndef_file_control_tlv.read_access = 0x00;  /* read access without any security */
    tag->cc_file.ndef_file_control_tlv.write_access = 0x00; /* write access without any security */

    return 0;
}

int t4t_init(nfc_t4t_t *tag, uint16_t max_capdu_size, uint8_t *ndef_file, 
    size_t max_ndef_file_size) {
    if (tag == NULL || ndef_file == NULL || max_ndef_file_size == 0) {
        return -1;
    }

    tag->ndef_file = ndef_file;
    tag->selected_ndef_application = false;
    tag->selected_cc_file = false;
    tag->selected_ndef_file = false;
    int ret = t4t_init_cc_file(tag, max_capdu_size, max_ndef_file_size);
    if (ret != 0) {
        return ret;
    }

    return 0;
}

void t4t_cc_file_print(const t4t_cc_file_t *cc) {
    printf("T4T CC File:\n");
    printf("  CC Length: %u\n", cc->cc_len);
    printf("  Mapping Version: 0x%02X\n", cc->mapping_version);
    printf("  MLE: %u\n", cc->mle);
    printf("  MLC: %u\n", cc->mlc);
    printf("  NDEF File Control TLV:\n");
    printf("    Type: 0x%02X\n", cc->ndef_file_control_tlv.type);
    printf("    Length: %u\n", cc->ndef_file_control_tlv.length);
    printf("    File ID: 0x%04X\n", cc->ndef_file_control_tlv.file_id);
    printf("    Max NDEF Size: %u\n", cc->ndef_file_control_tlv.max_ndef_size);
    printf("    Read Access: 0x%02X\n", cc->ndef_file_control_tlv.read_access);
    printf("    Write Access: 0x%02X\n", cc->ndef_file_control_tlv.write_access);
}
