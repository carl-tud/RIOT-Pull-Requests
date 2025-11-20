#pragma once

#define NFC_ISO_DEP_PCB_I_BLOCK_FIXED_VALUE       (0x02u)

#define NFC_ISO_DEP_PCB_BLOCK_NUMBER_MASK (0x01u)
#define NFC_ISO_DEP_PCB_NAD_MASK          (0x04u)
#define NFC_ISO_DEP_PCB_DID_MASK          (0x08u)
#define NFC_ISO_DEP_PCB_CHAINING_MASK     (0x10u)

#define NFC_ISO_DEP_PCB_BLOCK_TYPE_MASK         (0xC0u)
#define NFC_ISO_DEP_PCB_BLOCK_TYPE_I_VALUE      (0x00u)
#define NFC_ISO_DEP_PCB_BLOCK_TYPE_R_VALUE      (0x80u)
#define NFC_ISO_DEP_PCB_BLOCK_TYPE_S_VALUE      (0xC0u)

typedef enum {
    NFC_ISO_DEP_BLOCK_TYPE_UNKNOWN = 0,
    NFC_ISO_DEP_BLOCK_TYPE_I,
    NFC_ISO_DEP_BLOCK_TYPE_R,
    NFC_ISO_DEP_BLOCK_TYPE_S,
} nfc_iso_dep_block_type_t;
