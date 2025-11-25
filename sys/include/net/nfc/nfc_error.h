#pragma once

/* NFC error code */

#define NFC_SUCCESS              0   /**< Success */
#define NFC_ERR_GENERIC         -1   /**< Generic error */
#define NFC_ERR_NOTSUPP         -2   /**< Operation not supported */
#define NFC_ERR_INVAL           -3   /**< Invalid argument */
#define NFC_ERR_NOMEM           -4   /**< Out of memory */
#define NFC_ERR_TIMEOUT         -5   /**< Operation timed out */
#define NFC_ERR_POLL_NO_TARGET  -6   /**< No tag found */
#define NFC_ERR_COMMUNICATION   -7   /**< Communication error */
#define NFC_ERR_AUTH            -8   /**< Authentication error */
#define NFC_ERR_DESELECTED      -9   /**< Tag deselected */
