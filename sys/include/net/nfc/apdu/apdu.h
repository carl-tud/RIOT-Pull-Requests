#pragma once

#include <stdint.h>
#include <stddef.h>

#define APDU_CLA_DEFAULT 0x00

#define APDU_INS_POS 1
#define APDU_P1_POS 2
#define APDU_P2_POS 3
#define APDU_LC_POS 4
#define APDU_DATA_POS 5

#define APDU_INS_SELECT 0xA4
#define APDU_INS_READ_BINARY 0xB0
#define APDU_INS_UPDATE_BINARY 0xD6



/* P1 parameters for the SELECT command */
#define APDU_SELECT_P1_SELECT_MF_DF_OR_EF 0x00
#define APDU_SELECT_P1_SELECT_CHILD_DF 0x01
#define APDU_SELECT_P1_SELECT_BY_DF_NAME 0x04
#define APDU_SELECT_P1_SELECT_FROM_MF 0x08

/* P2 parameters for the SELECT command */
#define APDU_SELECT_P2_FIRST_OR_ONLY_OCCURRENCE 0x00
#define APDU_SELECT_P2_LAST_OCCURRENCE 0x01
#define APDU_SELECT_P2_NEXT_OCCURRENCE 0x02
#define APDU_SELECT_P2_PREVIOUS_OCCURRENCE 0x03
#define APDU_SELECT_P2_NO_RESPONSE_DATA 0x0C

/* APDU error codes for SW 1 and SW 2 */
#define APDU_SW1_NO_ERROR 0x90
#define APDU_SW2_NO_ERROR 0x00

#define APDU_SW1_WRONG_LENGTH 0x67
#define APDU_SW1_FUNC_NOT_SUPPORTED 0x68
#define APDU_SW1_COMMAND_NOT_ALLOWED 0x69
#define APDU_SW1_WRONG_PARAMS 0x6A
#define APDU_SW2_FILE_OR_APP_NOT_FOUND 0x82
#define APDU_SW1_INSTRUCTION_NOT_SUPPORTED 0x6D
#define APDU_SW1_CLASS_NOT_SUPPORTED 0x6E
#define APDU_SW1_UNKNOWN 0x6F

uint8_t apdu_get_ins(const uint8_t *apdu, size_t apdu_len);

uint8_t apdu_get_p1(const uint8_t *apdu, size_t apdu_len);

uint8_t apdu_get_p2(const uint8_t *apdu, size_t apdu_len);

uint8_t apdu_get_lc(const uint8_t *apdu, size_t apdu_len);

uint8_t apdu_get_le(const uint8_t *apdu, size_t apdu_len);

const uint8_t *apdu_get_data(const uint8_t *apdu, size_t apdu_len);
