#include "net/nfc/llcp.h"

 /* LLCP PDU Accessors */
uint8_t nfc_llcp_pdu_get_dsap(const uint8_t *pdu) {
    return (pdu[0] >> 2) & 0x3F;
}

uint8_t nfc_llcp_pdu_get_ssap(const uint8_t *pdu) {
    return pdu[1] & 0x3F;
}

uint8_t nfc_llcp_pdu_get_ptype(const uint8_t *pdu) {
    return (pdu[0] & 0x03) << 2 | (pdu[1] >> 6) & 0x03;
}
