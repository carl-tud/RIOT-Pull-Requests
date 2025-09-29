#pragma once

/**
 * @file nci_definitions.h
 * @brief NFC Controller Interface (NCI) Group Identifiers (GIDs) and Opcode Identifiers (OIDs)
 * @note Based on NFC Forum NCI Specification (e.g., NCI 2.0)
 */

#define NCI_PAYLOAD_LENGTH_POS  (2u)
#define NCI_PAYLOAD_POS         (3u)
#define NCI_HEADER_SIZE         (3u)

/* Group Identifiers (GIDs) - 4-bit values */
#define NCI_GID_CORE            0x0 /* Core NCI commands and responses */
#define NCI_GID_RF_MGMT         0x1 /* RF Communication Management */
#define NCI_GID_NFCEE_MGMT      0x2 /* NFC Execution Environment Management */
#define NCI_GID_PROPRIETARY     0xF /* Proprietary commands (vendor-specific) */

/* Core Group OIDs (NCI_GID_CORE) */
#define NCI_OID_CORE_RESET              0x00 /* Reset the NFC Controller */
#define NCI_OID_CORE_INIT               0x01 /* Initialize the NFC Controller */
#define NCI_OID_CORE_SET_CONFIG         0x02 /* Set configuration parameters */
#define NCI_OID_CORE_GET_CONFIG         0x03 /* Get configuration parameters */
#define NCI_OID_CORE_CONN_CREATE        0x04 /* Create a logical connection */
#define NCI_OID_CORE_CONN_CLOSE         0x05 /* Close a logical connection */
#define NCI_OID_CORE_CONN_CREDITS       0x06 /* Notify available credits */
#define NCI_OID_CORE_GENERIC_ERROR      0x07 /* Generic error notification */
#define NCI_OID_CORE_INTERFACE_ERROR    0x08 /* Interface-specific error */
#define NCI_OID_CORE_TEST               0x09 /* Test commands (e.g., PRBS) */

/* RF Management Group OIDs (NCI_GID_RF_MGMT) */
#define NCI_OID_RF_DISCOVER_MAP         0x00 /* Map RF interfaces to protocols */
#define NCI_OID_RF_SET_LISTEN_MODE_ROUTING 0x01 /* Set routing for listen mode */
#define NCI_OID_RF_GET_LISTEN_MODE_ROUTING 0x02 /* Get routing for listen mode */
#define NCI_OID_RF_DISCOVER             0x03 /* Start RF discovery */
#define NCI_OID_RF_DISCOVER_SELECT      0x04 /* Select discovered RF target */
#define NCI_OID_RF_INTF_ACTIVATED       0x05 /* RF interface activated notification */
#define NCI_OID_RF_DEACTIVATE           0x06 /* Deactivate RF interface */
#define NCI_OID_RF_FIELD_INFO           0x07 /* Field information notification */
#define NCI_OID_RF_T3T_POLLING          0x08 /* Polling for Type 3 Tag */

/* NFCEE Management Group OIDs (NCI_GID_NFCEE_MGMT) */
#define NCI_OID_NFCEE_DISCOVER          0x00 /* Discover NFCEEs */
#define NCI_OID_NFCEE_MODE_SET          0x01 /* Enable or disable an NFCEE */
#define NCI_OID_NFCEE_STATUS_NTF        0x02 /* NFCEE status notification */
#define NCI_OID_NFCEE_POWER_LINK_CTRL   0x03 /* Power and link control */

/* Proprietary Group (NCI_GID_PROPRIETARY) */
#define NCI_OID_PROPRIETARY_BASE        0x00 /* Vendor-specific, varies by implementation */

/* Message Type Indicators (MT) for NCI Packet Header */
#define NCI_MT_DATA             0x0 /* Data message */
#define NCI_MT_CMD              0x1 /* Command message */
#define NCI_MT_RSP              0x2 /* Response message */
#define NCI_MT_NTF              0x3 /* Notification message */

/* RF Technologies */
#define NCI_NFC_RF_TECHNOLOGY_A     0x00 /* NFC-A (ISO/IEC 14443 Type A) */
#define NCI_NFC_RF_TECHNOLOGY_B     0x01 /* NFC-B (ISO/IEC 14443 Type B) */
#define NCI_NFC_RF_TECHNOLOGY_F     0x02 /* NFC-F (FeliCa) */
#define NCI_NFC_RF_TECHNOLOGY_V     0x03 /* NFC-V (ISO/IEC 15693) */

/* RF Technology and Mode */
#define NCI_NFC_A_PASSIVE_POLL_MODE    0x00 /* NFC-A Passive Polling */
#define NCI_NFC_B_PASSIVE_POLL_MODE    0x01 /* NFC-B Passive Polling */
#define NCI_NFC_F_PASSIVE_POLL_MODE    0x02 /* NFC-F Passive Polling */
#define NCI_NFC_ACTIVE_POLL_MODE       0x03 /* NFC Active Polling */
#define NCI_NFC_V_PASSIVE_POLL_MODE    0x06 /* NFC-V Passive Polling */
#define NCI_NFC_A_PASSIVE_LISTEN_MODE  0x80 /* NFC-A Passive Listening */
#define NCI_NFC_B_PASSIVE_LISTEN_MODE  0x81 /* NFC-B Passive Listening */
#define NCI_NFC_F_PASSIVE_LISTEN_MODE  0x82 /* NFC-F Passive Listening */
#define NCI_NFC_ACTIVE_LISTEN_MODE     0x83 /* NFC Active Listening */

/* NCI Status */
#define NCI_STATUS_OK              0x00 /* Operation successful */
#define NCI_STATUS_REJECTED        0x01 /* Operation rejected */
#define NCI_STATUS_FAILED          0x03 /* Operation failed */
#define NCI_STATUS_NOT_INITIALIZED 0x04 /* NFC Controller not initialized */
#define NCI_STATUS_SYNTAX_ERROR    0x05 /* Syntax error in command */
#define NCI_STATUS_SEMANTIC_ERROR  0x06 /* Semantic error in command */
#define NCI_STATUS_INVALID_PARAM   0x09 /* Invalid parameter */
#define NCI_STATUS_MESSAGE_SIZE_EXCEEDED 0x0A /* Message size exceeded */

#define NCI_RF_FRAME_CORRUPTED  0x02 /* RF frame corrupted */
#define NCI_RF_TRANSMISSION_EXCEPTION 0xB0 /* RF transmission exception */
#define NCI_RF_PROTOCOL_EXCEPTION   0xB1 /* RF protocol error */
#define NCI_RF_TIMEOUT_EXCEPTION    0xB2 /* RF timeout error */
#define NCI_RF_UNEXPECTED_DATA    0xB3 /* Unexpected RF data */

/**
 * @brief Macro to construct NCI Packet Control Byte
 * @param mt Message Type (NCI_MT_xxx)
 * @param pbf Packet Boundary Flag (0 or 1)
 * @param gid Group Identifier (NCI_GID_xxx)
 * @return Control byte for NCI packet header
 */
#define NCI_CONTROL_BYTE(mt, pbf, gid) (((mt & 0x7) << 5) | ((pbf & 0x1) << 4) | (gid & 0xF))

#define NCI_GET_MT(b)  (((b) >> 5) & 0x7)
#define NCI_GET_PBF(b) (((b) >> 4) & 0x1)
#define NCI_GET_GID(b) ((b) & 0xF)
#define NCI_GET_OID(b) ((b) & 0x3F)

/**
 * @brief Macro to construct NCI Packet Opcode
 * @param oid Opcode Identifier (NCI_OID_xxx)
 * @return Opcode byte for NCI packet
 */
#define NCI_OPCODE(oid) (oid & 0x3F)
