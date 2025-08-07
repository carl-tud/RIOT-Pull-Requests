#include "st25.h"
#include "periph/spi.h"
#include "periph/i2c.h"
#include "periph/gpio.h"
#include "ztimer.h"
#include "memory.h"
#include "log.h"
#include "board.h"
#include "kernel_defines.h"

#define ENABLE_DEBUG 1
#include "debug.h"

/* REMOVE AT SOME POINT */
#define ST25R3916B 1

#define CMD_SET_DEFAULT             (0xC0)
#define CMD_STOP                    (0xC2)
#define CMD_TRANSMIT_WITH_CRC       (0xC4)
#define CMD_TRANSMIT_WITHOUT_CRC    (0xC5)
#define CMD_TRANSMIT_REQA           (0xC6)
#define CMD_TRANSMIT_WUPA           (0xC7)
#define CMD_NFC_INITIAL_FIELD_ON    (0xC8)
#define CMD_NFC_RESPONSE_FIELD_ON   (0xC9)
#define CMD_GO_TO_SENSE             (0xCD)
#define CMD_GO_TO_SLEEP             (0xCE)
#define CMD_MASK_RECEIVE_DATA       (0xD0)
#define CMD_MASK_TRANSMIT_DATA      (0xD1)
#define CMD_CHANGE_AM_AMPLITUDE_STATE (0xD2)
#define CMD_MEASURE_AMPLTIUDE        (0xD3)
#define CMD_RESET_RX_GAIN               (0xD5)
#define CMD_ADJUST_REGULATORS           (0xD6)
#define CMD_CALIBRATE_DRIVER_TIMING       (0xD8)
#define CMD_MEASURE_PHASE                 (0xD9)
#define CMD_CLEAR_RSSI                    (0xDA)
#define CMD_CLEAR_FIFO                    (0xDB)
#define CMD_ENTER_TRANSPARENT_MODE            (0xDC)
#define CMD_MEASURE_POWER_SUPPLY           (0xDF)
#define CMD_START_GENERAL_PURPOSE_TIMER    (0xE0)
#define CMD_START_WAKE_UP_TIMER            (0xE1)
#define CMD_START_MASK_RECEIVE_TIMER       (0xE2)
#define CMD_START_NO_RESPONSE_TIMER        (0xE3)
#define CMD_START_PP_TIMER                 (0xE4)
#define CMD_STOP_NO_RESPONSE_TIMER          (0xE5)
#define CMD_TRIGGER_RC_CALIBRATION         (0xEA)
#define CMD_REGISTER_SPACE_B_ACCESS      (0xFB)
#define CMD_TEST_ACCESS                  (0xFC)

/* TODO: add more commands */


#define SPI_MODE_REGISTER_WRITE          (0x00)
#define SPI_MODE_REGISTER_READ           (0x40)
#define SPI_MODE_FIFO_LOAD               (0x80)
#define SPI_MODE_PT_MEMORY_LOAD_A_CONFIG (0xA0)
#define SPI_MODE_FIFO_READ               (0x9F)
#define SPI_MODE_DIRECT_COMMAND          (0xC0)
/* TODO: add more SPI modes */


/* IO configuration registers */
#define REG_IO_CONF1                              0x00U    /*!< RW IO Configuration Register 1                       */
#define REG_IO_CONF2                              0x01U    /*!< RW IO Configuration Register 2                       */

/* Operation control and mode definition registers */
#define REG_OP_CONTROL                            0x02U    /*!< RW Operation Control Register                        */
#define REG_MODE                                  0x03U    /*!< RW Mode Definition Register                          */
#define REG_BIT_RATE                              0x04U    /*!< RW Bit Rate Definition Register                      */

/* Protocol Configuration registers */
#define REG_ISO14443A_NFC                         0x05U    /*!< RW ISO14443A and NFC 106 kBit/s Settings Register    */
#define REG_EMD_SUP_CONF                 (SPACE_B|0x05U)   /*!< RW EMD Suppression Configuration Register            */
#define REG_ISO14443B_1                           0x06U    /*!< RW ISO14443B Settings Register 1                     */
#define REG_SUBC_START_TIME              (SPACE_B|0x06U)   /*!< RW Subcarrier Start Time Register                    */
#define REG_ISO14443B_2                           0x07U    /*!< RW ISO14443B Settings Register 2                     */
#define REG_PASSIVE_TARGET                        0x08U    /*!< RW Passive Target Definition Register                */
#define REG_STREAM_MODE                           0x09U    /*!< RW Stream Mode Definition Register                   */
#define REG_AUX                                   0x0AU    /*!< RW Auxiliary Definition Register                     */

/* Receiver Configuration registers */
#define REG_RX_CONF1                              0x0BU    /*!< RW Receiver Configuration Register 1                 */
#define REG_RX_CONF2                              0x0CU    /*!< RW Receiver Configuration Register 2                 */
#define REG_RX_CONF3                              0x0DU    /*!< RW Receiver Configuration Register 3                 */
#define REG_RX_CONF4                              0x0EU    /*!< RW Receiver Configuration Register 4                 */
#define REG_P2P_RX_CONF                  (SPACE_B|0x0BU)   /*!< RW P2P Receiver Configuration Register 1             */
#define REG_CORR_CONF1                   (SPACE_B|0x0CU)   /*!< RW Correlator configuration register 1               */
#define REG_CORR_CONF2                   (SPACE_B|0x0DU)   /*!< RW Correlator configuration register 2               */

/* Timer definition registers */
#define REG_MASK_RX_TIMER                         0x0FU    /*!< RW Mask Receive Timer Register                       */
#define REG_NO_RESPONSE_TIMER1                    0x10U    /*!< RW No-response Timer Register 1                      */
#define REG_NO_RESPONSE_TIMER2                    0x11U    /*!< RW No-response Timer Register 2                      */
#define REG_TIMER_EMV_CONTROL                     0x12U    /*!< RW Timer and EMV Control                             */
#define REG_GPT1                                  0x13U    /*!< RW General Purpose Timer Register 1                  */
#define REG_GPT2                                  0x14U    /*!< RW General Purpose Timer Register 2                  */
#define REG_PPON2                                 0x15U    /*!< RW PPON2 Field waiting Timer Register                */
#define REG_SQUELCH_TIMER                (SPACE_B|0x0FU)   /*!< RW Squelch timeout Register                          */
#define REG_FIELD_ON_GT                  (SPACE_B|0x15U)   /*!< RW NFC Field on guard time                           */

/* Interrupt and associated reporting registers */
#define REG_IRQ_MASK_MAIN                         0x16U    /*!< RW Mask Main Interrupt Register                      */
#define REG_IRQ_MASK_TIMER_NFC                    0x17U    /*!< RW Mask Timer and NFC Interrupt Register             */
#define REG_IRQ_MASK_ERROR_WUP                    0x18U    /*!< RW Mask Error and Wake-up Interrupt Register         */
#define REG_IRQ_MASK_TARGET                       0x19U    /*!< RW Mask 3916 Target Interrupt Register               */
#define REG_IRQ_MAIN                              0x1AU    /*!< R  Main Interrupt Register                           */
#define REG_IRQ_TIMER_NFC                         0x1BU    /*!< R  Timer and NFC Interrupt Register                  */
#define REG_IRQ_ERROR_WUP                         0x1CU    /*!< R  Error and Wake-up Interrupt Register              */
#define REG_IRQ_TARGET                            0x1DU    /*!< R  ST25R3916 Target Interrupt Register               */
#define REG_FIFO_STATUS1                          0x1EU    /*!< R  FIFO Status Register 1                            */
#define REG_FIFO_STATUS2                          0x1FU    /*!< R  FIFO Status Register 2                            */
#define REG_COLLISION_STATUS                      0x20U    /*!< R  Collision Display Register                        */
#define REG_PASSIVE_TARGET_STATUS                 0x21U    /*!< R  Passive target state status                       */

/* Definition of number of transmitted bytes */
#define REG_NUM_TX_BYTES1                         0x22U    /*!< RW Number of Transmitted Bytes Register 1            */
#define REG_NUM_TX_BYTES2                         0x23U    /*!< RW Number of Transmitted Bytes Register 2            */

/* NFCIP Bit Rate Display Register */
#define REG_NFCIP1_BIT_RATE                       0x24U    /*!< R  NFCIP Bit Rate Detection Display Register         */

/* A/D Converter Output Register */
#define REG_AD_RESULT                             0x25U    /*!< R  A/D Converter Output Register                     */

/* Antenna tuning registers */
#define REG_ANT_TUNE_A                            0x26U    /*!< RW Antenna Tuning Control (AAT-A) Register 1         */
#define REG_ANT_TUNE_B                            0x27U    /*!< RW Antenna Tuning Control (AAT-B) Register 2         */

/* Antenna Driver and Modulation registers */
#define REG_TX_DRIVER                             0x28U    /*!< RW TX driver register                                */
#define REG_PT_MOD                                0x29U    /*!< RW PT modulation Register                            */
#define REG_AUX_MOD                      (SPACE_B|0x28U)   /*!< RW Aux Modulation setting Register                   */
#define REG_TX_DRIVER_TIMING             (SPACE_B|0x29U)   /*!< RW TX driver timing Register                         */
#define REG_RES_AM_MOD                   (SPACE_B|0x2AU)   /*!< RW Resistive AM modulation register                  */
#define REG_TX_DRIVER_STATUS             (SPACE_B|0x2BU)   /*!< R  TX driver timing readout Register                 */

/* External Field Detector Threshold Registers */
#define REG_FIELD_THRESHOLD_ACTV                  0x2AU    /*!< RW External Field Detector Activation Threshold Reg  */
#define REG_FIELD_THRESHOLD_DEACTV                0x2BU    /*!< RW External Field Detector Deactivation Threshold Reg*/

/* Regulator registers */
#define REG_REGULATOR_CONTROL                     0x2CU    /*!< RW Regulated Voltage Control Register                */
#define REG_REGULATOR_RESULT             (SPACE_B|0x2CU)   /*!< R Regulator Display Register                         */

/* Receiver State Display Register */
#define REG_RSSI_RESULT                           0x2DU    /*!< R RSSI Display Register                              */
#define REG_GAIN_RED_STATE                        0x2EU    /*!< R Gain Reduction State Register                      */
#define REG_CAP_SENSOR_CONTROL                    0x2FU    /*!< RW Capacitive Sensor Control Register                */
#define REG_CAP_SENSOR_RESULT                     0x30U    /*!< R  Capacitive Sensor Display Register                */
#define REG_AUX_DISPLAY                           0x31U    /*!< R Auxiliary Display Register                         */

/* Over/Undershoot Protection Configuration Registers */
#define REG_OVERSHOOT_CONF1              (SPACE_B|0x30U)   /*!< RW  Overshoot Protection Configuration Register 1    */
#define REG_OVERSHOOT_CONF2              (SPACE_B|0x31U)   /*!< RW  Overshoot Protection Configuration Register 2    */
#define REG_UNDERSHOOT_CONF1             (SPACE_B|0x32U)   /*!< RW  Undershoot Protection Configuration Register 1   */
#define REG_UNDERSHOOT_CONF2             (SPACE_B|0x33U)   /*!< RW  Undershoot Protection Configuration Register 2   */

#ifdef ST25R3916B
  /* AWS Configuration Registers */
  #define REG_AWS_CONF1          (SPACE_B|0x2EU)   /*!< RW  AWS Configuration Register 1                     */
  #define REG_AWS_CONF2          (SPACE_B|0x2FU)   /*!< RW  AWS Configuration Register 2                     */
  #define REG_AWS_TIME1          (SPACE_B|0x34U)   /*!< RW  AWS Time Register 1                              */
  #define REG_AWS_TIME2          (SPACE_B|0x35U)   /*!< RW  AWS Time Register 2                              */
  #define REG_AWS_TIME3          (SPACE_B|0x36U)   /*!< RW  AWS Time Register 1                              */
  #define REG_AWS_TIME4          (SPACE_B|0x37U)   /*!< RW  AWS Time Register 2                              */
  #define REG_AWS_TIME5          (SPACE_B|0x38U)   /*!< RW  AWS Time Register 1                              */
  #define REG_AWS_RC_CAL         (SPACE_B|0x39U)   /*!< RW  AWS Time Register 2                              */
#endif /* ST25R3916B */

/* Detection of card presence */
#define REG_WUP_TIMER_CONTROL                     0x32U    /*!< RW Wake-up Timer Control Register                    */
#define REG_AMPLITUDE_MEASURE_CONF                0x33U    /*!< RW Amplitude Measurement Configuration Register      */
#define REG_AMPLITUDE_MEASURE_REF                 0x34U    /*!< RW Amplitude Measurement Reference Register          */
#define REG_AMPLITUDE_MEASURE_AA_RESULT           0x35U    /*!< R  Amplitude Measurement Auto Averaging Display Reg  */
#define REG_AMPLITUDE_MEASURE_RESULT              0x36U    /*!< R  Amplitude Measurement Display Register            */
#define REG_PHASE_MEASURE_CONF                    0x37U    /*!< RW Phase Measurement Configuration Register          */
#define REG_PHASE_MEASURE_REF                     0x38U    /*!< RW Phase Measurement Reference Register              */
#define REG_PHASE_MEASURE_AA_RESULT               0x39U    /*!< R  Phase Measurement Auto Averaging Display Register */
#define REG_PHASE_MEASURE_RESULT                  0x3AU    /*!< R  Phase Measurement Display Register                */

#if defined(ST25R3916)
  #define REG_CAPACITANCE_MEASURE_CONF              0x3BU    /*!< RW Capacitance Measurement Configuration Register    */
  #define REG_CAPACITANCE_MEASURE_REF               0x3CU    /*!< RW Capacitance Measurement Reference Register        */
  #define REG_CAPACITANCE_MEASURE_AA_RESULT         0x3DU    /*!< R  Capacitance Measurement Auto Averaging Display Reg*/
  #define REG_CAPACITANCE_MEASURE_RESULT            0x3EU    /*!< R  Capacitance Measurement Display Register          */
#elif defined(ST25R3916B)
  #define REG_MEAS_TX_DELAY                         0x3BU    /*!< RW Capacitance Measurement Configuration Register    */
#endif /* ST25R3916 */

/* IC identity  */
#define REG_IC_IDENTITY                           0x3FU    /*!< R  Chip Id: 0 for old silicon, v2 silicon: 0x09      */


/*! Register bit definitions  \cond DOXYGEN_SUPPRESS */

#define REG_IO_CONF1_single                         (1U<<7)
#define REG_IO_CONF1_rfo2                           (1U<<6)
#define REG_IO_CONF1_i2c_thd1                       (1U<<5)
#define REG_IO_CONF1_i2c_thd0                       (1U<<4)
#define REG_IO_CONF1_i2c_thd_mask                   (3U<<4)
#define REG_IO_CONF1_i2c_thd_shift                  (4U)
#define REG_IO_CONF1_rfu                            (1U<<3)
#define REG_IO_CONF1_out_cl1                        (1U<<2)
#define REG_IO_CONF1_out_cl0                        (1U<<1)
#define REG_IO_CONF1_out_cl_disabled                (3U<<1)
#define REG_IO_CONF1_out_cl_13_56MHZ                (2U<<1)
#define REG_IO_CONF1_out_cl_4_78MHZ                 (1U<<1)
#define REG_IO_CONF1_out_cl_3_39MHZ                 (0U<<1)
#define REG_IO_CONF1_out_cl_mask                    (3U<<1)
#define REG_IO_CONF1_out_cl_shift                   (1U)
#define REG_IO_CONF1_lf_clk_off                     (1U<<0)
#define REG_IO_CONF1_lf_clk_off_on                  (1U<<0)
#define REG_IO_CONF1_lf_clk_off_off                 (0U<<0)

#define REG_IO_CONF2_sup3V                          (1U<<7)
#define REG_IO_CONF2_sup3V_3V                       (1U<<7)
#define REG_IO_CONF2_sup3V_5V                       (0U<<7)
#define REG_IO_CONF2_vspd_off                       (1U<<6)
#define REG_IO_CONF2_aat_en                         (1U<<5)
#define REG_IO_CONF2_miso_pd2                       (1U<<4)
#define REG_IO_CONF2_miso_pd1                       (1U<<3)
#define REG_IO_CONF2_io_drv_lvl                     (1U<<2)
#if defined(ST25R3916)
  #define REG_IO_CONF2_slow_up                        (1U<<0)
#elif defined(ST25R3916B)
  #define REG_IO_CONF2_act_amsink                     (1U<<0)
#endif /* ST25R3916B */


#define REG_OP_CONTROL_en                           (1U<<7)
#define REG_OP_CONTROL_rx_en                        (1U<<6)
#define REG_OP_CONTROL_rx_chn                       (1U<<5)
#define REG_OP_CONTROL_rx_man                       (1U<<4)
#define REG_OP_CONTROL_tx_en                        (1U<<3)
#define REG_OP_CONTROL_wu                           (1U<<2)
#define REG_OP_CONTROL_en_fd_c1                     (1U<<1)
#define REG_OP_CONTROL_en_fd_c0                     (1U<<0)
#define REG_OP_CONTROL_en_fd_efd_off                (0U<<0)
#define REG_OP_CONTROL_en_fd_manual_efd_ca          (1U<<0)
#define REG_OP_CONTROL_en_fd_manual_efd_pdt         (2U<<0)
#define REG_OP_CONTROL_en_fd_auto_efd               (3U<<0)
#define REG_OP_CONTROL_en_fd_shift                  (0U)
#define REG_OP_CONTROL_en_fd_mask                   (3U<<0)

#define REG_MODE_targ                               (1U<<7)
#define REG_MODE_targ_targ                          (1U<<7)
#define REG_MODE_targ_init                          (0U<<7)
#define REG_MODE_om3                                (1U<<6)
#define REG_MODE_om2                                (1U<<5)
#define REG_MODE_om1                                (1U<<4)
#define REG_MODE_om0                                (1U<<3)
#define REG_MODE_om_bpsk_stream                     (0xfU<<3)
#define REG_MODE_om_subcarrier_stream               (0xeU<<3)
#define REG_MODE_om_topaz                           (0x4U<<3)
#define REG_MODE_om_felica                          (0x3U<<3)
#define REG_MODE_om_iso14443b                       (0x2U<<3)
#define REG_MODE_om_iso14443a                       (0x1U<<3)
#define REG_MODE_om_targ_nfca                       (0x1U<<3)
#define REG_MODE_om_targ_nfcb                       (0x2U<<3)
#define REG_MODE_om_targ_nfcf                       (0x4U<<3)
#define REG_MODE_om_targ_nfcip                      (0x7U<<3)
#define REG_MODE_om_nfc                             (0x0U<<3)
#define REG_MODE_om_mask                            (0xfU<<3)
#define REG_MODE_om_shift                           (3U)
#define REG_MODE_tr_am                              (1U<<2)
#define REG_MODE_tr_am_ook                          (0U<<2)
#define REG_MODE_tr_am_am                           (1U<<2)
#define REG_MODE_nfc_ar1                            (1U<<1)
#define REG_MODE_nfc_ar0                            (1U<<0)
#define REG_MODE_nfc_ar_off                         (0U<<0)
#define REG_MODE_nfc_ar_auto_rx                     (1U<<0)
#define REG_MODE_nfc_ar_eof                         (2U<<0)
#define REG_MODE_nfc_ar_rfu                         (3U<<0)
#define REG_MODE_nfc_ar_mask                        (3U<<0)
#define REG_MODE_nfc_ar_shift                       (0U)

#define REG_BIT_RATE_txrate_106                     (0x0U<<4)
#define REG_BIT_RATE_txrate_212                     (0x1U<<4)
#define REG_BIT_RATE_txrate_424                     (0x2U<<4)
#define REG_BIT_RATE_txrate_848                     (0x3U<<4)
#define REG_BIT_RATE_txrate_mask                    (0x3U<<4)
#define REG_BIT_RATE_txrate_shift                   (4U)
#define REG_BIT_RATE_rxrate_106                     (0x0U<<0)
#define REG_BIT_RATE_rxrate_212                     (0x1U<<0)
#define REG_BIT_RATE_rxrate_424                     (0x2U<<0)
#define REG_BIT_RATE_rxrate_848                     (0x3U<<0)
#define REG_BIT_RATE_rxrate_mask                    (0x3U<<0)
#define REG_BIT_RATE_rxrate_shift                   (0U)

#define REG_ISO14443A_NFC_no_tx_par                 (1U<<7)
#define REG_ISO14443A_NFC_no_tx_par_off             (0U<<7)
#define REG_ISO14443A_NFC_no_rx_par                 (1U<<6)
#define REG_ISO14443A_NFC_no_rx_par_off             (0U<<6)
#define REG_ISO14443A_NFC_nfc_f0                    (1U<<5)
#define REG_ISO14443A_NFC_nfc_f0_off                (0U<<5)
#define REG_ISO14443A_NFC_p_len3                    (1U<<4)
#define REG_ISO14443A_NFC_p_len2                    (1U<<3)
#define REG_ISO14443A_NFC_p_len1                    (1U<<2)
#define REG_ISO14443A_NFC_p_len0                    (1U<<1)
#define REG_ISO14443A_NFC_p_len_mask                (0xfU<<1)
#define REG_ISO14443A_NFC_p_len_shift               (1U)
#define REG_ISO14443A_NFC_antcl                     (1U<<0)

#define REG_EMD_SUP_CONF_emd_emv                    (1U<<7)
#define REG_EMD_SUP_CONF_emd_emv_on                 (1U<<7)
#define REG_EMD_SUP_CONF_emd_emv_off                (0U<<7)
#define REG_EMD_SUP_CONF_rx_start_emv               (1U<<6)
#define REG_EMD_SUP_CONF_rx_start_emv_on            (1U<<6)
#define REG_EMD_SUP_CONF_rx_start_emv_off           (0U<<6)
#define REG_EMD_SUP_CONF_rfu1                       (1U<<5)
#define REG_EMD_SUP_CONF_rfu0                       (1U<<4)
#define REG_EMD_SUP_CONF_emd_thld3                  (1U<<3)
#define REG_EMD_SUP_CONF_emd_thld2                  (1U<<2)
#define REG_EMD_SUP_CONF_emd_thld1                  (1U<<1)
#define REG_EMD_SUP_CONF_emd_thld0                  (1U<<0)
#define REG_EMD_SUP_CONF_emd_thld_mask              (0xfU<<0)
#define REG_EMD_SUP_CONF_emd_thld_shift             (0U)

#define REG_SUBC_START_TIME_rfu2                    (1U<<7)
#define REG_SUBC_START_TIME_rfu1                    (1U<<6)
#define REG_SUBC_START_TIME_rfu0                    (1U<<5)
#define REG_SUBC_START_TIME_sst4                    (1U<<4)
#define REG_SUBC_START_TIME_sst3                    (1U<<3)
#define REG_SUBC_START_TIME_sst2                    (1U<<2)
#define REG_SUBC_START_TIME_sst1                    (1U<<1)
#define REG_SUBC_START_TIME_sst0                    (1U<<0)
#define REG_SUBC_START_TIME_sst_mask                (0x1fU<<0)
#define REG_SUBC_START_TIME_sst_shift               (0U)

#define REG_ISO14443B_1_egt2                        (1U<<7)
#define REG_ISO14443B_1_egt1                        (1U<<6)
#define REG_ISO14443B_1_egt0                        (1U<<5)
#define REG_ISO14443B_1_egt_shift                   (5U)
#define REG_ISO14443B_1_egt_mask                    (7U<<5)
#define REG_ISO14443B_1_sof_1                       (1U<<3)
#define REG_ISO14443B_1_sof_1_3etu                  (1U<<3)
#define REG_ISO14443B_1_sof_1_2etu                  (0U<<3)
#define REG_ISO14443B_1_sof_0                       (1U<<4)
#define REG_ISO14443B_1_sof_0_11etu                 (1U<<4)
#define REG_ISO14443B_1_sof_0_10etu                 (0U<<4)
#define REG_ISO14443B_1_sof_mask                    (3U<<3)
#define REG_ISO14443B_1_eof                         (1U<<2)
#define REG_ISO14443B_1_eof_11etu                   (1U<<2)
#define REG_ISO14443B_1_eof_10etu                   (0U<<2)
#define REG_ISO14443B_1_half                        (1U<<1)
#if defined(ST25R3916)
  #define REG_ISO14443B_1_rx_st_om                    (1U<<0)
#elif defined(ST25R3916B)
  #define REG_ISO14443B_1_rfu0                        (1U<<0)
#endif /* ST25R3916B */

#define REG_ISO14443B_2_tr1_1                       (1U<<7)
#define REG_ISO14443B_2_tr1_0                       (1U<<6)
#define REG_ISO14443B_2_tr1_64fs32fs                (1U<<6)
#define REG_ISO14443B_2_tr1_80fs80fs                (0U<<6)
#define REG_ISO14443B_2_tr1_mask                    (3U<<6)
#define REG_ISO14443B_2_tr1_shift                   (6U)
#define REG_ISO14443B_2_no_sof                      (1U<<5)
#define REG_ISO14443B_2_no_eof                      (1U<<4)
#define REG_ISO14443B_rfu1                          (1U<<3)
#define REG_ISO14443B_rfu0                          (1U<<2)
#define REG_ISO14443B_2_f_p1                        (1U<<1)
#define REG_ISO14443B_2_f_p0                        (1U<<0)
#define REG_ISO14443B_2_f_p_96                      (3U<<0)
#define REG_ISO14443B_2_f_p_80                      (2U<<0)
#define REG_ISO14443B_2_f_p_64                      (1U<<0)
#define REG_ISO14443B_2_f_p_48                      (0U<<0)
#define REG_ISO14443B_2_f_p_mask                    (3U<<0)
#define REG_ISO14443B_2_f_p_shift                   (0U)

#define REG_PASSIVE_TARGET_fdel_3                   (1U<<7)
#define REG_PASSIVE_TARGET_fdel_2                   (1U<<6)
#define REG_PASSIVE_TARGET_fdel_1                   (1U<<5)
#define REG_PASSIVE_TARGET_fdel_0                   (1U<<4)
#define REG_PASSIVE_TARGET_fdel_mask                (0xfU<<4)
#define REG_PASSIVE_TARGET_fdel_shift               (4U)
#define REG_PASSIVE_TARGET_d_ac_ap2p                (1U<<3)
#define REG_PASSIVE_TARGET_d_212_424_1r             (1U<<2)
#define REG_PASSIVE_TARGET_rfu                      (1U<<1)
#define REG_PASSIVE_TARGET_d_106_ac_a               (1U<<0)

#define REG_STREAM_MODE_rfu                         (1U<<7)
#define REG_STREAM_MODE_scf1                        (1U<<6)
#define REG_STREAM_MODE_scf0                        (1U<<5)
#define REG_STREAM_MODE_scf_sc212                   (0U<<5)
#define REG_STREAM_MODE_scf_sc424                   (1U<<5)
#define REG_STREAM_MODE_scf_sc848                   (2U<<5)
#define REG_STREAM_MODE_scf_sc1695                  (3U<<5)
#define REG_STREAM_MODE_scf_bpsk848                 (0U<<5)
#define REG_STREAM_MODE_scf_bpsk1695                (1U<<5)
#define REG_STREAM_MODE_scf_bpsk3390                (2U<<5)
#define REG_STREAM_MODE_scf_bpsk106                 (3U<<5)
#define REG_STREAM_MODE_scf_mask                    (3U<<5)
#define REG_STREAM_MODE_scf_shift                   (5U)
#define REG_STREAM_MODE_scp1                        (1U<<4)
#define REG_STREAM_MODE_scp0                        (1U<<3)
#define REG_STREAM_MODE_scp_1pulse                  (0U<<3)
#define REG_STREAM_MODE_scp_2pulses                 (1U<<3)
#define REG_STREAM_MODE_scp_4pulses                 (2U<<3)
#define REG_STREAM_MODE_scp_8pulses                 (3U<<3)
#define REG_STREAM_MODE_scp_mask                    (3U<<3)
#define REG_STREAM_MODE_scp_shift                   (3U)
#define REG_STREAM_MODE_stx2                        (1U<<2)
#define REG_STREAM_MODE_stx1                        (1U<<1)
#define REG_STREAM_MODE_stx0                        (1U<<0)
#define REG_STREAM_MODE_stx_106                     (0U<<0)
#define REG_STREAM_MODE_stx_212                     (1U<<0)
#define REG_STREAM_MODE_stx_424                     (2U<<0)
#define REG_STREAM_MODE_stx_848                     (3U<<0)
#define REG_STREAM_MODE_stx_mask                    (7U<<0)
#define REG_STREAM_MODE_stx_shift                   (0U)

#define REG_AUX_no_crc_rx                           (1U<<7)
#define REG_AUX_rfu                                 (1U<<6)
#define REG_AUX_nfc_id1                             (1U<<5)
#define REG_AUX_nfc_id0                             (1U<<4)
#define REG_AUX_nfc_id_7bytes                       (1U<<4)
#define REG_AUX_nfc_id_4bytes                       (0U<<4)
#define REG_AUX_nfc_id_mask                         (3U<<4)
#define REG_AUX_nfc_id_shift                        (4U)
#define REG_AUX_mfaz_cl90                           (1U<<3)
#define REG_AUX_dis_corr                            (1U<<2)
#define REG_AUX_dis_corr_coherent                   (1U<<2)
#define REG_AUX_dis_corr_correlator                 (0U<<2)
#define REG_AUX_nfc_n1                              (1U<<1)
#define REG_AUX_nfc_n0                              (1U<<0)
#define REG_AUX_nfc_n_mask                          (3U<<0)
#define REG_AUX_nfc_n_shift                         (0U)

#define REG_RX_CONF1_ch_sel                         (1U<<7)
#define REG_RX_CONF1_ch_sel_PM                      (1U<<7)
#define REG_RX_CONF1_ch_sel_AM                      (0U<<7)
#define REG_RX_CONF1_lp2                            (1U<<6)
#define REG_RX_CONF1_lp1                            (1U<<5)
#define REG_RX_CONF1_lp0                            (1U<<4)
#define REG_RX_CONF1_lp_1200khz                     (0U<<4)
#define REG_RX_CONF1_lp_600khz                      (1U<<4)
#define REG_RX_CONF1_lp_300khz                      (2U<<4)
#define REG_RX_CONF1_lp_2000khz                     (4U<<4)
#define REG_RX_CONF1_lp_7000khz                     (5U<<4)
#define REG_RX_CONF1_lp_mask                        (7U<<4)
#define REG_RX_CONF1_lp_shift                       (4U)
#define REG_RX_CONF1_z600k                          (1U<<3)
#define REG_RX_CONF1_h200                           (1U<<2)
#define REG_RX_CONF1_h80                            (1U<<1)
#define REG_RX_CONF1_z12k                           (1U<<0)
#define REG_RX_CONF1_hz_60_400khz                   (0U<<0)
#define REG_RX_CONF1_hz_60_200khz                   (4U<<0)
#define REG_RX_CONF1_hz_40_80khz                    (2U<<0)
#define REG_RX_CONF1_hz_12_200khz                   (1U<<0)
#define REG_RX_CONF1_hz_12_80khz                    (3U<<0)
#define REG_RX_CONF1_hz_12_200khz_alt               (5U<<0)
#define REG_RX_CONF1_hz_600_400khz                  (8U<<0)
#define REG_RX_CONF1_hz_600_200khz                  (12U<<0)
#define REG_RX_CONF1_hz_mask                        (0xfU<<0)
#define REG_RX_CONF1_hz_shift                       (0U)

#define REG_RX_CONF2_demod_mode                     (1U<<7)
#define REG_RX_CONF2_amd_sel                        (1U<<6)
#define REG_RX_CONF2_amd_sel_mixer                  (1U<<6)
#define REG_RX_CONF2_amd_sel_peak                   (0U<<6)
#define REG_RX_CONF2_sqm_dyn                        (1U<<5)
#define REG_RX_CONF2_pulz_61                        (1U<<4)
#define REG_RX_CONF2_agc_en                         (1U<<3)
#define REG_RX_CONF2_agc_m                          (1U<<2)
#define REG_RX_CONF2_agc_alg                        (1U<<1)
#define REG_RX_CONF2_agc6_3                         (1U<<0)

#define REG_RX_CONF3_rg1_am2                        (1U<<7)
#define REG_RX_CONF3_rg1_am1                        (1U<<6)
#define REG_RX_CONF3_rg1_am0                        (1U<<5)
#define REG_RX_CONF3_rg1_am_mask                    (0x7U<<5)
#define REG_RX_CONF3_rg1_am_shift                   (5U)
#define REG_RX_CONF3_rg1_pm2                        (1U<<4)
#define REG_RX_CONF3_rg1_pm1                        (1U<<3)
#define REG_RX_CONF3_rg1_pm0                        (1U<<2)
#define REG_RX_CONF3_rg1_pm_mask                    (0x7U<<2)
#define REG_RX_CONF3_rg1_pm_shift                   (2U)
#define REG_RX_CONF3_lf_en                          (1U<<1)
#define REG_RX_CONF3_lf_op                          (1U<<0)

#define REG_RX_CONF4_rg2_am3                        (1U<<7)
#define REG_RX_CONF4_rg2_am2                        (1U<<6)
#define REG_RX_CONF4_rg2_am1                        (1U<<5)
#define REG_RX_CONF4_rg2_am0                        (1U<<4)
#define REG_RX_CONF4_rg2_am_mask                    (0xfU<<4)
#define REG_RX_CONF4_rg2_am_shift                   (4U)
#define REG_RX_CONF4_rg2_pm3                        (1U<<3)
#define REG_RX_CONF4_rg2_pm2                        (1U<<2)
#define REG_RX_CONF4_rg2_pm1                        (1U<<1)
#define REG_RX_CONF4_rg2_pm0                        (1U<<0)
#define REG_RX_CONF4_rg2_pm_mask                    (0xfU<<0)
#define REG_RX_CONF4_rg2_pm_shift                   (0U)

#define REG_P2P_RX_CONF_ook_fd                      (1U<<7)
#define REG_P2P_RX_CONF_ook_rc1                     (1U<<6)
#define REG_P2P_RX_CONF_ook_rc0                     (1U<<5)
#define REG_P2P_RX_CONF_ook_thd1                    (1U<<4)
#define REG_P2P_RX_CONF_ook_thd0                    (1U<<3)
#define REG_P2P_RX_CONF_ask_rc1                     (1U<<2)
#define REG_P2P_RX_CONF_ask_rc0                     (1U<<1)
#define REG_P2P_RX_CONF_ask_thd                     (1U<<0)

#define REG_CORR_CONF1_corr_s7                      (1U<<7)
#define REG_CORR_CONF1_corr_s6                      (1U<<6)
#define REG_CORR_CONF1_corr_s5                      (1U<<5)
#define REG_CORR_CONF1_corr_s4                      (1U<<4)
#define REG_CORR_CONF1_corr_s3                      (1U<<3)
#define REG_CORR_CONF1_corr_s2                      (1U<<2)
#define REG_CORR_CONF1_corr_s1                      (1U<<1)
#define REG_CORR_CONF1_corr_s0                      (1U<<0)

#define REG_CORR_CONF2_rfu5                         (1U<<7)
#define REG_CORR_CONF2_rfu4                         (1U<<6)
#define REG_CORR_CONF2_rfu3                         (1U<<5)
#define REG_CORR_CONF2_rfu2                         (1U<<4)
#define REG_CORR_CONF2_rfu1                         (1U<<3)
#define REG_CORR_CONF2_rfu0                         (1U<<2)
#define REG_CORR_CONF2_corr_s9                      (1U<<1)
#define REG_CORR_CONF2_corr_s8                      (1U<<0)

#define REG_TIMER_EMV_CONTROL_gptc2                 (1U<<7)
#define REG_TIMER_EMV_CONTROL_gptc1                 (1U<<6)
#define REG_TIMER_EMV_CONTROL_gptc0                 (1U<<5)
#define REG_TIMER_EMV_CONTROL_gptc_no_trigger       (0U<<5)
#define REG_TIMER_EMV_CONTROL_gptc_erx              (1U<<5)
#define REG_TIMER_EMV_CONTROL_gptc_srx              (2U<<5)
#define REG_TIMER_EMV_CONTROL_gptc_etx_nfc          (3U<<5)
#define REG_TIMER_EMV_CONTROL_gptc_mask             (7U<<5)
#define REG_TIMER_EMV_CONTROL_gptc_shift            (5U)
#define REG_TIMER_EMV_CONTROL_rfu                   (1U<<4)
#define REG_TIMER_EMV_CONTROL_mrt_step              (1U<<3)
#define REG_TIMER_EMV_CONTROL_mrt_step_512          (1U<<3)
#define REG_TIMER_EMV_CONTROL_mrt_step_64           (0U<<3)
#define REG_TIMER_EMV_CONTROL_nrt_nfc               (1U<<2)
#define REG_TIMER_EMV_CONTROL_nrt_nfc_on            (1U<<2)
#define REG_TIMER_EMV_CONTROL_nrt_nfc_off           (0U<<2)
#define REG_TIMER_EMV_CONTROL_nrt_emv               (1U<<1)
#define REG_TIMER_EMV_CONTROL_nrt_emv_on            (1U<<1)
#define REG_TIMER_EMV_CONTROL_nrt_emv_off           (0U<<1)
#define REG_TIMER_EMV_CONTROL_nrt_step              (1U<<0)
#define REG_TIMER_EMV_CONTROL_nrt_step_64fc         (0U<<0)
#define REG_TIMER_EMV_CONTROL_nrt_step_4096_fc      (1U<<0)

#define REG_FIFO_STATUS2_fifo_b9                    (1U<<7)
#define REG_FIFO_STATUS2_fifo_b8                    (1U<<6)
#define REG_FIFO_STATUS2_fifo_b_mask                (3U<<6)
#define REG_FIFO_STATUS2_fifo_b_shift               (6U)
#define REG_FIFO_STATUS2_fifo_unf                   (1U<<5)
#define REG_FIFO_STATUS2_fifo_ovr                   (1U<<4)
#define REG_FIFO_STATUS2_fifo_lb2                   (1U<<3)
#define REG_FIFO_STATUS2_fifo_lb1                   (1U<<2)
#define REG_FIFO_STATUS2_fifo_lb0                   (1U<<1)
#define REG_FIFO_STATUS2_fifo_lb_mask               (7U<<1)
#define REG_FIFO_STATUS2_fifo_lb_shift              (1U)
#define REG_FIFO_STATUS2_np_lb                      (1U<<0)

#define REG_COLLISION_STATUS_c_byte3                (1U<<7)
#define REG_COLLISION_STATUS_c_byte2                (1U<<6)
#define REG_COLLISION_STATUS_c_byte1                (1U<<5)
#define REG_COLLISION_STATUS_c_byte0                (1U<<4)
#define REG_COLLISION_STATUS_c_byte_mask            (0xfU<<4)
#define REG_COLLISION_STATUS_c_byte_shift           (4U)
#define REG_COLLISION_STATUS_c_bit2                 (1U<<3)
#define REG_COLLISION_STATUS_c_bit1                 (1U<<2)
#define REG_COLLISION_STATUS_c_bit0                 (1U<<1)
#define REG_COLLISION_STATUS_c_pb                   (1U<<0)
#define REG_COLLISION_STATUS_c_bit_mask             (3U<<1)
#define REG_COLLISION_STATUS_c_bit_shift            (1U)

#define REG_PASSIVE_TARGET_STATUS_rfu               (1U<<7)
#define REG_PASSIVE_TARGET_STATUS_rfu1              (1U<<6)
#define REG_PASSIVE_TARGET_STATUS_rfu2              (1U<<5)
#define REG_PASSIVE_TARGET_STATUS_rfu3              (1U<<4)
#define REG_PASSIVE_TARGET_STATUS_pta_state3        (1U<<3)
#define REG_PASSIVE_TARGET_STATUS_pta_state2        (1U<<2)
#define REG_PASSIVE_TARGET_STATUS_pta_state1        (1U<<1)
#define REG_PASSIVE_TARGET_STATUS_pta_state0        (1U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_power_off  (0x0U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_idle       (0x1U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_ready_l1   (0x2U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_ready_l2   (0x3U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_rfu4       (0x4U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_active     (0x5U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_rfu6       (0x6U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_rfu7       (0x7U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_rfu8       (0x8U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_halt       (0x9U<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_ready_l1_x (0xaU<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_ready_l2_x (0xbU<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_rfu12      (0xcU<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_st_active_x   (0xdU<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_state_mask    (0xfU<<0)
#define REG_PASSIVE_TARGET_STATUS_pta_state_shift   (0U)

#define REG_NUM_TX_BYTES2_ntx4                      (1U<<7)
#define REG_NUM_TX_BYTES2_ntx3                      (1U<<6)
#define REG_NUM_TX_BYTES2_ntx2                      (1U<<5)
#define REG_NUM_TX_BYTES2_ntx1                      (1U<<4)
#define REG_NUM_TX_BYTES2_ntx0                      (1U<<3)
#define REG_NUM_TX_BYTES2_ntx_mask                  (0x1fU<<3)
#define REG_NUM_TX_BYTES2_ntx_shift                 (3U)
#define REG_NUM_TX_BYTES2_nbtx2                     (1U<<2)
#define REG_NUM_TX_BYTES2_nbtx1                     (1U<<1)
#define REG_NUM_TX_BYTES2_nbtx0                     (1U<<0)
#define REG_NUM_TX_BYTES2_nbtx_mask                 (7U<<0)
#define REG_NUM_TX_BYTES2_nbtx_shift                (0U)

#define REG_NFCIP1_BIT_RATE_nfc_rfu1                (1U<<7)
#define REG_NFCIP1_BIT_RATE_nfc_rfu0                (1U<<6)
#define REG_NFCIP1_BIT_RATE_nfc_rate1               (1U<<5)
#define REG_NFCIP1_BIT_RATE_nfc_rate0               (1U<<4)
#define REG_NFCIP1_BIT_RATE_nfc_rate_mask           (0x3U<<4)
#define REG_NFCIP1_BIT_RATE_nfc_rate_shift          (4U)
#define REG_NFCIP1_BIT_RATE_ppt2_on                 (1U<<3)
#define REG_NFCIP1_BIT_RATE_gpt_on                  (1U<<2)
#define REG_NFCIP1_BIT_RATE_nrt_on                  (1U<<1)
#define REG_NFCIP1_BIT_RATE_mrt_on                  (1U<<0)

#define REG_TX_DRIVER_am_mod3                       (1U<<7)
#define REG_TX_DRIVER_am_mod2                       (1U<<6)
#define REG_TX_DRIVER_am_mod1                       (1U<<5)
#define REG_TX_DRIVER_am_mod0                       (1U<<4)
#if defined(ST25R3916)
  #define REG_TX_DRIVER_am_mod_5percent               (0x0U<<4)
  #define REG_TX_DRIVER_am_mod_6percent               (0x1U<<4)
  #define REG_TX_DRIVER_am_mod_7percent               (0x2U<<4)
  #define REG_TX_DRIVER_am_mod_8percent               (0x3U<<4)
  #define REG_TX_DRIVER_am_mod_9percent               (0x4U<<4)
  #define REG_TX_DRIVER_am_mod_10percent              (0x5U<<4)
  #define REG_TX_DRIVER_am_mod_11percent              (0x6U<<4)
  #define REG_TX_DRIVER_am_mod_12percent              (0x7U<<4)
  #define REG_TX_DRIVER_am_mod_13percent              (0x8U<<4)
  #define REG_TX_DRIVER_am_mod_14percent              (0x9U<<4)
  #define REG_TX_DRIVER_am_mod_15percent              (0xaU<<4)
  #define REG_TX_DRIVER_am_mod_17percent              (0xbU<<4)
  #define REG_TX_DRIVER_am_mod_19percent              (0xcU<<4)
  #define REG_TX_DRIVER_am_mod_22percent              (0xdU<<4)
  #define REG_TX_DRIVER_am_mod_26percent              (0xeU<<4)
  #define REG_TX_DRIVER_am_mod_40percent              (0xfU<<4)
#elif defined(ST25R3916B)
  #define REG_TX_DRIVER_am_mod_0percent               (0x0U<<4)
  #define REG_TX_DRIVER_am_mod_8percent               (0x1U<<4)
  #define REG_TX_DRIVER_am_mod_10percent              (0x2U<<4)
  #define REG_TX_DRIVER_am_mod_11percent              (0x3U<<4)
  #define REG_TX_DRIVER_am_mod_12percent              (0x4U<<4)
  #define REG_TX_DRIVER_am_mod_13percent              (0x5U<<4)
  #define REG_TX_DRIVER_am_mod_14percent              (0x6U<<4)
  #define REG_TX_DRIVER_am_mod_15percent              (0x7U<<4)
  #define REG_TX_DRIVER_am_mod_20percent              (0x8U<<4)
  #define REG_TX_DRIVER_am_mod_25percent              (0x9U<<4)
  #define REG_TX_DRIVER_am_mod_30percent              (0xaU<<4)
  #define REG_TX_DRIVER_am_mod_40percent              (0xbU<<4)
  #define REG_TX_DRIVER_am_mod_50percent              (0xcU<<4)
  #define REG_TX_DRIVER_am_mod_60percent              (0xdU<<4)
  #define REG_TX_DRIVER_am_mod_70percent              (0xeU<<4)
  #define REG_TX_DRIVER_am_mod_82percent              (0xfU<<4)
#endif
#define REG_TX_DRIVER_am_mod_mask                   (0xfU<<4)
#define REG_TX_DRIVER_am_mod_shift                  (4U)
#define REG_TX_DRIVER_d_res3                        (1U<<3)
#define REG_TX_DRIVER_d_res2                        (1U<<2)
#define REG_TX_DRIVER_d_res1                        (1U<<1)
#define REG_TX_DRIVER_d_res0                        (1U<<0)
#define REG_TX_DRIVER_d_res_mask                    (0xfU<<0)
#define REG_TX_DRIVER_d_res_shift                   (0U)

#define REG_PT_MOD_ptm_res3                         (1U<<7)
#define REG_PT_MOD_ptm_res2                         (1U<<6)
#define REG_PT_MOD_ptm_res1                         (1U<<5)
#define REG_PT_MOD_ptm_res0                         (1U<<4)
#define REG_PT_MOD_ptm_res_mask                     (0xfU<<4)
#define REG_PT_MOD_ptm_res_shift                    (4U)
#define REG_PT_MOD_pt_res3                          (1U<<3)
#define REG_PT_MOD_pt_res2                          (1U<<2)
#define REG_PT_MOD_pt_res1                          (1U<<1)
#define REG_PT_MOD_pt_res0                          (1U<<0)
#define REG_PT_MOD_pt_res_mask                      (0xfU<<0)
#define REG_PT_MOD_pt_res_shift                     (0U)

#define REG_AUX_MOD_dis_reg_am                      (1U<<7)
#define REG_AUX_MOD_lm_ext_pol                      (1U<<6)
#define REG_AUX_MOD_lm_ext                          (1U<<5)
#define REG_AUX_MOD_lm_dri                          (1U<<4)
#define REG_AUX_MOD_res_am                          (1U<<3)
#if defined(ST25R3916)
  #define REG_AUX_MOD_rfu2                            (1U<<2)
#elif defined(ST25R3916B)
  #define REG_AUX_MOD_rgs_am                          (1U<<2)
#endif /* ST25R3916B */
#define REG_AUX_MOD_rfu1                            (1U<<1)
#define REG_AUX_MOD_rfu0                            (1U<<0)

#define REG_TX_DRIVER_TIMING_d_rat_t3               (1U<<7)
#define REG_TX_DRIVER_TIMING_d_rat_t2               (1U<<6)
#define REG_TX_DRIVER_TIMING_d_rat_t1               (1U<<5)
#define REG_TX_DRIVER_TIMING_d_rat_t0               (1U<<4)
#define REG_TX_DRIVER_TIMING_d_rat_mask             (0xfU<<4)
#define REG_TX_DRIVER_TIMING_d_rat_shift            (4U)
#define REG_TX_DRIVER_TIMING_rfu                    (1U<<3)
#define REG_TX_DRIVER_TIMING_d_tim_m2               (1U<<2)
#define REG_TX_DRIVER_TIMING_d_tim_m1               (1U<<1)
#define REG_TX_DRIVER_TIMING_d_tim_m0               (1U<<0)
#define REG_TX_DRIVER_TIMING_d_tim_m_mask           (0x7U<<0)
#define REG_TX_DRIVER_TIMING_d_tim_m_shift          (0U)

#define REG_RES_AM_MOD_fa3_f                        (1U<<7)
#define REG_RES_AM_MOD_md_res6                      (1U<<6)
#define REG_RES_AM_MOD_md_res5                      (1U<<5)
#define REG_RES_AM_MOD_md_res4                      (1U<<4)
#define REG_RES_AM_MOD_md_res3                      (1U<<3)
#define REG_RES_AM_MOD_md_res2                      (1U<<2)
#define REG_RES_AM_MOD_md_res1                      (1U<<1)
#define REG_RES_AM_MOD_md_res0                      (1U<<0)
#define REG_RES_AM_MOD_md_res_mask                  (0x7FU<<0)
#define REG_RES_AM_MOD_md_res_shift                 (0U)

#define REG_TX_DRIVER_STATUS_d_rat_r3               (1U<<7)
#define REG_TX_DRIVER_STATUS_d_rat_r2               (1U<<6)
#define REG_TX_DRIVER_STATUS_d_rat_r1               (1U<<5)
#define REG_TX_DRIVER_STATUS_d_rat_r0               (1U<<4)
#define REG_TX_DRIVER_STATUS_d_rat_mask             (0xfU<<4)
#define REG_TX_DRIVER_STATUS_d_rat_shift            (4U)
#define REG_TX_DRIVER_STATUS_rfu                    (1U<<3)
#define REG_TX_DRIVER_STATUS_d_tim_r2               (1U<<2)
#define REG_TX_DRIVER_STATUS_d_tim_r1               (1U<<1)
#define REG_TX_DRIVER_STATUS_d_tim_r0               (1U<<0)
#define REG_TX_DRIVER_STATUS_d_tim_mask             (0x7U<<0)
#define REG_TX_DRIVER_STATUS_d_tim_shift            (0U)

#define REG_FIELD_THRESHOLD_ACTV_trg_l2a            (1U<<6)
#define REG_FIELD_THRESHOLD_ACTV_trg_l1a            (1U<<5)
#define REG_FIELD_THRESHOLD_ACTV_trg_l0a            (1U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_75mV           (0x0U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_105mV          (0x1U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_150mV          (0x2U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_205mV          (0x3U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_290mV          (0x4U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_400mV          (0x5U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_560mV          (0x6U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_800mV          (0x7U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_mask           (7U<<4)
#define REG_FIELD_THRESHOLD_ACTV_trg_shift          (4U)
#define REG_FIELD_THRESHOLD_ACTV_rfe_t3a            (1U<<3)
#define REG_FIELD_THRESHOLD_ACTV_rfe_t2a            (1U<<2)
#define REG_FIELD_THRESHOLD_ACTV_rfe_t1a            (1U<<1)
#define REG_FIELD_THRESHOLD_ACTV_rfe_t0a            (1U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_75mV           (0x0U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_105mV          (0x1U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_150mV          (0x2U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_205mV          (0x3U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_290mV          (0x4U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_400mV          (0x5U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_560mV          (0x6U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_800mV          (0x7U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_25mV           (0x8U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_33mV           (0x9U<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_47mV           (0xAU<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_64mV           (0xBU<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_90mV           (0xCU<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_125mV          (0xDU<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_175mV          (0xEU<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_250mV          (0xFU<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_mask           (0xfU<<0)
#define REG_FIELD_THRESHOLD_ACTV_rfe_shift          (0U)

#define REG_FIELD_THRESHOLD_DEACTV_trg_l2d          (1U<<6)
#define REG_FIELD_THRESHOLD_DEACTV_trg_l1d          (1U<<5)
#define REG_FIELD_THRESHOLD_DEACTV_trg_l0d          (1U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_75mV         (0x0U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_105mV        (0x1U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_150mV        (0x2U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_205mV        (0x3U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_290mV        (0x4U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_400mV        (0x5U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_560mV        (0x6U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_800mV        (0x7U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_mask         (7U<<4)
#define REG_FIELD_THRESHOLD_DEACTV_trg_shift        (4U)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_t3d          (1U<<3)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_t2d          (1U<<2)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_t1d          (1U<<1)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_t0d          (1U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_75mV         (0x0U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_105mV        (0x1U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_150mV        (0x2U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_205mV        (0x3U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_290mV        (0x4U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_400mV        (0x5U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_560mV        (0x6U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_800mV        (0x7U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_25mV         (0x8U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_33mV         (0x9U<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_47mV         (0xAU<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_64mV         (0xBU<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_90mV         (0xCU<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_125mV        (0xDU<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_175mV        (0xEU<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_250mV        (0xFU<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_mask         (0xfU<<0)
#define REG_FIELD_THRESHOLD_DEACTV_rfe_shift        (0U)

#define REG_REGULATOR_CONTROL_reg_s                 (1U<<7)
#define REG_REGULATOR_CONTROL_rege_3                (1U<<6)
#define REG_REGULATOR_CONTROL_rege_2                (1U<<5)
#define REG_REGULATOR_CONTROL_rege_1                (1U<<4)
#define REG_REGULATOR_CONTROL_rege_0                (1U<<3)
#define REG_REGULATOR_CONTROL_rege_mask             (0xfU<<3)
#define REG_REGULATOR_CONTROL_rege_shift            (3U)
#define REG_REGULATOR_CONTROL_mpsv2                 (2U<<2)
#define REG_REGULATOR_CONTROL_mpsv1                 (1U<<1)
#define REG_REGULATOR_CONTROL_mpsv0                 (1U<<0)
#define REG_REGULATOR_CONTROL_mpsv_vdd              (0U)
#define REG_REGULATOR_CONTROL_mpsv_vdd_a            (1U)
#define REG_REGULATOR_CONTROL_mpsv_vdd_d            (2U)
#define REG_REGULATOR_CONTROL_mpsv_vdd_rf           (3U)
#define REG_REGULATOR_CONTROL_mpsv_vdd_am           (4U)
#define REG_REGULATOR_CONTROL_rfu                   (5U)
#define REG_REGULATOR_CONTROL_rfu1                  (6U)
#define REG_REGULATOR_CONTROL_rfu2                  (7U)
#define REG_REGULATOR_CONTROL_mpsv_mask             (7U)
#define REG_REGULATOR_CONTROL_mpsv_shift            (0U)

#define REG_REGULATOR_RESULT_reg_3                  (1U<<7)
#define REG_REGULATOR_RESULT_reg_2                  (1U<<6)
#define REG_REGULATOR_RESULT_reg_1                  (1U<<5)
#define REG_REGULATOR_RESULT_reg_0                  (1U<<4)
#define REG_REGULATOR_RESULT_reg_mask               (0xfU<<4)
#define REG_REGULATOR_RESULT_reg_shift              (4U)
#define REG_REGULATOR_RESULT_i_lim                  (1U<<0)

#define REG_RSSI_RESULT_rssi_am_3                   (1U<<7)
#define REG_RSSI_RESULT_rssi_am_2                   (1U<<6)
#define REG_RSSI_RESULT_rssi_am_1                   (1U<<5)
#define REG_RSSI_RESULT_rssi_am_0                   (1U<<4)
#define REG_RSSI_RESULT_rssi_am_mask                (0xfU<<4)
#define REG_RSSI_RESULT_rssi_am_shift               (4U)
#define REG_RSSI_RESULT_rssi_pm3                    (1U<<3)
#define REG_RSSI_RESULT_rssi_pm2                    (1U<<2)
#define REG_RSSI_RESULT_rssi_pm1                    (1U<<1)
#define REG_RSSI_RESULT_rssi_pm0                    (1U<<0)
#define REG_RSSI_RESULT_rssi_pm_mask                (0xfU<<0)
#define REG_RSSI_RESULT_rssi_pm_shift               (0U)

#define REG_GAIN_RED_STATE_gs_am_3                  (1U<<7)
#define REG_GAIN_RED_STATE_gs_am_2                  (1U<<6)
#define REG_GAIN_RED_STATE_gs_am_1                  (1U<<5)
#define REG_GAIN_RED_STATE_gs_am_0                  (1U<<4)
#define REG_GAIN_RED_STATE_gs_am_mask               (0xfU<<4)
#define REG_GAIN_RED_STATE_gs_am_shift              (4U)
#define REG_GAIN_RED_STATE_gs_pm_3                  (1U<<3)
#define REG_GAIN_RED_STATE_gs_pm_2                  (1U<<2)
#define REG_GAIN_RED_STATE_gs_pm_1                  (1U<<1)
#define REG_GAIN_RED_STATE_gs_pm_0                  (1U<<0)
#define REG_GAIN_RED_STATE_gs_pm_mask               (0xfU<<0)
#define REG_GAIN_RED_STATE_gs_pm_shift              (0U)

#define REG_CAP_SENSOR_CONTROL_cs_mcal4             (1U<<7)
#define REG_CAP_SENSOR_CONTROL_cs_mcal3             (1U<<6)
#define REG_CAP_SENSOR_CONTROL_cs_mcal2             (1U<<5)
#define REG_CAP_SENSOR_CONTROL_cs_mcal1             (1U<<4)
#define REG_CAP_SENSOR_CONTROL_cs_mcal0             (1U<<3)
#define REG_CAP_SENSOR_CONTROL_cs_mcal_mask         (0x1fU<<3)
#define REG_CAP_SENSOR_CONTROL_cs_mcal_shift        (3U)
#define REG_CAP_SENSOR_CONTROL_cs_g2                (1U<<2)
#define REG_CAP_SENSOR_CONTROL_cs_g1                (1U<<1)
#define REG_CAP_SENSOR_CONTROL_cs_g0                (1U<<0)
#define REG_CAP_SENSOR_CONTROL_cs_g_mask            (7U<<0)
#define REG_CAP_SENSOR_CONTROL_cs_g_shift           (0U)

#define REG_CAP_SENSOR_RESULT_cs_cal4               (1U<<7)
#define REG_CAP_SENSOR_RESULT_cs_cal3               (1U<<6)
#define REG_CAP_SENSOR_RESULT_cs_cal2               (1U<<5)
#define REG_CAP_SENSOR_RESULT_cs_cal1               (1U<<4)
#define REG_CAP_SENSOR_RESULT_cs_cal0               (1U<<3)
#define REG_CAP_SENSOR_RESULT_cs_cal_mask           (0x1fU<<3)
#define REG_CAP_SENSOR_RESULT_cs_cal_shift          (3U)
#define REG_CAP_SENSOR_RESULT_cs_cal_end            (1U<<2)
#define REG_CAP_SENSOR_RESULT_cs_cal_err            (1U<<1)

#define REG_AUX_DISPLAY_a_cha                       (1U<<7)
#define REG_AUX_DISPLAY_efd_o                       (1U<<6)
#define REG_AUX_DISPLAY_tx_on                       (1U<<5)
#define REG_AUX_DISPLAY_osc_ok                      (1U<<4)
#define REG_AUX_DISPLAY_rx_on                       (1U<<3)
#define REG_AUX_DISPLAY_rx_act                      (1U<<2)
#define REG_AUX_DISPLAY_en_peer                     (1U<<1)
#define REG_AUX_DISPLAY_en_ac                       (1U<<0)

#define REG_OVERSHOOT_CONF1_ov_tx_mode1             (1U<<7)
#define REG_OVERSHOOT_CONF1_ov_tx_mode0             (1U<<6)
#define REG_OVERSHOOT_CONF1_ov_pattern13            (1U<<5)
#define REG_OVERSHOOT_CONF1_ov_pattern12            (1U<<4)
#define REG_OVERSHOOT_CONF1_ov_pattern11            (1U<<3)
#define REG_OVERSHOOT_CONF1_ov_pattern10            (1U<<2)
#define REG_OVERSHOOT_CONF1_ov_pattern9             (1U<<1)
#define REG_OVERSHOOT_CONF1_ov_pattern8             (1U<<0)

#define REG_OVERSHOOT_CONF2_ov_pattern7             (1U<<7)
#define REG_OVERSHOOT_CONF2_ov_pattern6             (1U<<6)
#define REG_OVERSHOOT_CONF2_ov_pattern5             (1U<<5)
#define REG_OVERSHOOT_CONF2_ov_pattern4             (1U<<4)
#define REG_OVERSHOOT_CONF2_ov_pattern3             (1U<<3)
#define REG_OVERSHOOT_CONF2_ov_pattern2             (1U<<2)
#define REG_OVERSHOOT_CONF2_ov_pattern1             (1U<<1)
#define REG_OVERSHOOT_CONF2_ov_pattern0             (1U<<0)

#define REG_UNDERSHOOT_CONF1_un_tx_mode1            (1U<<7)
#define REG_UNDERSHOOT_CONF1_un_tx_mode0            (1U<<6)
#define REG_UNDERSHOOT_CONF1_un_pattern13           (1U<<5)
#define REG_UNDERSHOOT_CONF1_un_pattern12           (1U<<4)
#define REG_UNDERSHOOT_CONF1_un_pattern11           (1U<<3)
#define REG_UNDERSHOOT_CONF1_un_pattern10           (1U<<2)
#define REG_UNDERSHOOT_CONF1_un_pattern9            (1U<<1)
#define REG_UNDERSHOOT_CONF1_un_pattern8            (1U<<0)

#define REG_UNDERSHOOT_CONF2_un_pattern7            (1U<<7)
#define REG_UNDERSHOOT_CONF2_un_pattern6            (1U<<6)
#define REG_UNDERSHOOT_CONF2_un_pattern5            (1U<<5)
#define REG_UNDERSHOOT_CONF2_un_pattern4            (1U<<4)
#define REG_UNDERSHOOT_CONF2_un_pattern3            (1U<<3)
#define REG_UNDERSHOOT_CONF2_un_pattern2            (1U<<2)
#define REG_UNDERSHOOT_CONF2_un_pattern1            (1U<<1)
#define REG_UNDERSHOOT_CONF2_un_pattern0            (1U<<0)

#ifdef ST25R3916B

  #define REG_AWS_CONF1_rfu3                          (1U<<7)
  #define REG_AWS_CONF1_rfu2                          (1U<<6)
  #define REG_AWS_CONF1_rfu1                          (1U<<5)
  #define REG_AWS_CONF1_rfu0                          (1U<<4)
  #define REG_AWS_CONF1_vddrf_cont                    (1U<<3)
  #define REG_AWS_CONF1_rfam_sep_rx                   (1U<<2)
  #define REG_AWS_CONF1_vddrf_rx_only                 (1U<<1)
  #define REG_AWS_CONF1_rgs_txonoff                   (1U<<0)

  #define REG_AWS_CONF2_rfu1                          (1U<<7)
  #define REG_AWS_CONF2_rfu0                          (1U<<6)
  #define REG_AWS_CONF2_am_sym                        (1U<<5)
  #define REG_AWS_CONF2_en_modsink                    (1U<<4)
  #define REG_AWS_CONF2_am_filt3                      (1U<<3)
  #define REG_AWS_CONF2_am_filt2                      (1U<<2)
  #define REG_AWS_CONF2_am_filt1                      (1U<<1)
  #define REG_AWS_CONF2_am_filt0                      (1U<<0)
  #define REG_AWS_CONF2_am_filt_mask                  (0xfU<<0)
  #define REG_AWS_CONF2_am_filt_shift                 (0U)

  #define REG_AWS_TIME1_tholdx1_3                     (1U<<7)
  #define REG_AWS_TIME1_tholdx1_2                     (1U<<6)
  #define REG_AWS_TIME1_tholdx1_1                     (1U<<5)
  #define REG_AWS_TIME1_tholdx1_0                     (1U<<4)
  #define REG_AWS_TIME1_tmoddx1_mask                  (0xfU<<4)
  #define REG_AWS_TIME1_tmoddx1_shift                 (4U)
  #define REG_AWS_TIME1_tmodsw1_3                     (1U<<3)
  #define REG_AWS_TIME1_tmodsw1_2                     (1U<<2)
  #define REG_AWS_TIME1_tmodsw1_1                     (1U<<1)
  #define REG_AWS_TIME1_tmodsw1_0                     (1U<<0)
  #define REG_AWS_TIME1_tmodsw1_mask                  (0xfU<<0)
  #define REG_AWS_TIME1_tmodsw1_shift                 (0U)

  #define REG_AWS_TIME2_tammod1_3                     (1U<<7)
  #define REG_AWS_TIME2_tammod1_2                     (1U<<6)
  #define REG_AWS_TIME2_tammod1_1                     (1U<<5)
  #define REG_AWS_TIME2_tammod1_0                     (1U<<4)
  #define REG_AWS_TIME2_tammod1_mask                  (0xfU<<4)
  #define REG_AWS_TIME2_tammod1_shift                 (4U)
  #define REG_AWS_TIME2_tdres1_3                      (1U<<3)
  #define REG_AWS_TIME2_tdres1_2                      (1U<<2)
  #define REG_AWS_TIME2_tdres1_1                      (1U<<1)
  #define REG_AWS_TIME2_tdres1_0                      (1U<<0)
  #define REG_AWS_TIME2_tdres1_mask                   (0xfU<<0)
  #define REG_AWS_TIME2_tdres1_shift                  (0U)

  #define REG_AWS_TIME3_tentx1_3                      (1U<<7)
  #define REG_AWS_TIME3_tentx1_2                      (1U<<6)
  #define REG_AWS_TIME3_tentx1_1                      (1U<<5)
  #define REG_AWS_TIME3_tentx1_0                      (1U<<4)
  #define REG_AWS_TIME3_tentx1_mask                   (0xfU<<4)
  #define REG_AWS_TIME3_tentx1_shift                  (4U)
  #define REG_AWS_TIME3_tmods2_3                      (1U<<3)
  #define REG_AWS_TIME3_tmods2_2                      (1U<<2)
  #define REG_AWS_TIME3_tmods2_1                      (1U<<1)
  #define REG_AWS_TIME3_tmods2_0                      (1U<<0)
  #define REG_AWS_TIME3_tmods2_mask                   (0xfU<<0)
  #define REG_AWS_TIME3_tmods2_shift                  (0U)

  #define REG_AWS_TIME4_tholdx2_3                     (1U<<7)
  #define REG_AWS_TIME4_tholdx2_2                     (1U<<6)
  #define REG_AWS_TIME4_tholdx2_1                     (1U<<5)
  #define REG_AWS_TIME4_tholdx2_0                     (1U<<4)
  #define REG_AWS_TIME4_tholdx2_mask                  (0xfU<<4)
  #define REG_AWS_TIME4_tholdx2_shift                 (4U)
  #define REG_AWS_TIME4_tmodsw2_3                     (1U<<3)
  #define REG_AWS_TIME4_tmodsw2_2                     (1U<<2)
  #define REG_AWS_TIME4_tmodsw2_1                     (1U<<1)
  #define REG_AWS_TIME4_tmodsw2_0                     (1U<<0)
  #define REG_AWS_TIME4_tmodsw2_mask                  (0xfU<<0)
  #define REG_AWS_TIME4_tmodsw2_shift                 (0U)

  #define REG_AWS_TIME5_tdres2_3                      (1U<<7)
  #define REG_AWS_TIME5_tdres2_2                      (1U<<6)
  #define REG_AWS_TIME5_tdres2_1                      (1U<<5)
  #define REG_AWS_TIME5_tdres2_0                      (1U<<4)
  #define REG_AWS_TIME5_tdres2_mask                   (0xfU<<4)
  #define REG_AWS_TIME5_tdres2_shift                  (4U)
  #define REG_AWS_TIME5_trez2_3                       (1U<<3)
  #define REG_AWS_TIME5_trez2_2                       (1U<<2)
  #define REG_AWS_TIME5_trez2_1                       (1U<<1)
  #define REG_AWS_TIME5_trez2_0                       (1U<<0)
  #define REG_AWS_TIME5_trez2_mask                    (0xfU<<0)
  #define REG_AWS_TIME5_trez2_shift                   (0U)

  #define REG_AWS_RC_CAL_rfu4                         (1U<<7)
  #define REG_AWS_RC_CAL_rfu3                         (1U<<6)
  #define REG_AWS_RC_CAL_rfu2                         (1U<<5)
  #define REG_AWS_RC_CAL_rfu1                         (1U<<4)
  #define REG_AWS_RC_CAL_rfu0                         (1U<<3)
  #define REG_AWS_RC_CAL_rc_cal_ro_2                  (1U<<2)
  #define REG_AWS_RC_CAL_rc_cal_ro_1                  (1U<<1)
  #define REG_AWS_RC_CAL_rc_cal_ro_0                  (1U<<0)
  #define REG_AWS_RC_CAL_rc_cal_ro_mask               (0x7U<<0)
  #define REG_AWS_RC_CAL_rc_cal_ro_shift              (0U)

#endif /* ST25R3916B */


#define REG_WUP_TIMER_CONTROL_wur                   (1U<<7)
#define REG_WUP_TIMER_CONTROL_wut2                  (1U<<6)
#define REG_WUP_TIMER_CONTROL_wut1                  (1U<<5)
#define REG_WUP_TIMER_CONTROL_wut0                  (1U<<4)
#define REG_WUP_TIMER_CONTROL_wut_mask              (7U<<4)
#define REG_WUP_TIMER_CONTROL_wut_shift             (4U)
#define REG_WUP_TIMER_CONTROL_wto                   (1U<<3)
#define REG_WUP_TIMER_CONTROL_wam                   (1U<<2)
#define REG_WUP_TIMER_CONTROL_wph                   (1U<<1)
#if defined(ST25R3916)
  #define REG_WUP_TIMER_CONTROL_wcap                  (1U<<0)
#elif defined(ST25R3916B)
  #define REG_WUP_TIMER_CONTROL_rfu0                  (1U<<0)
#endif /* ST25R3916 */

#define REG_AMPLITUDE_MEASURE_CONF_am_d3            (1U<<7)
#define REG_AMPLITUDE_MEASURE_CONF_am_d2            (1U<<6)
#define REG_AMPLITUDE_MEASURE_CONF_am_d1            (1U<<5)
#define REG_AMPLITUDE_MEASURE_CONF_am_d0            (1U<<4)
#define REG_AMPLITUDE_MEASURE_CONF_am_d_mask        (0xfU<<4)
#define REG_AMPLITUDE_MEASURE_CONF_am_d_shift       (4U)
#define REG_AMPLITUDE_MEASURE_CONF_am_aam           (1U<<3)
#define REG_AMPLITUDE_MEASURE_CONF_am_aew1          (1U<<2)
#define REG_AMPLITUDE_MEASURE_CONF_am_aew0          (1U<<1)
#define REG_AMPLITUDE_MEASURE_CONF_am_aew_mask      (0x3U<<1)
#define REG_AMPLITUDE_MEASURE_CONF_am_aew_shift     (1U)
#define REG_AMPLITUDE_MEASURE_CONF_am_ae            (1U<<0)

#define REG_PHASE_MEASURE_CONF_pm_d3                (1U<<7)
#define REG_PHASE_MEASURE_CONF_pm_d2                (1U<<6)
#define REG_PHASE_MEASURE_CONF_pm_d1                (1U<<5)
#define REG_PHASE_MEASURE_CONF_pm_d0                (1U<<4)
#define REG_PHASE_MEASURE_CONF_pm_d_mask            (0xfU<<4)
#define REG_PHASE_MEASURE_CONF_pm_d_shift           (4U)
#define REG_PHASE_MEASURE_CONF_pm_aam               (1U<<3)
#define REG_PHASE_MEASURE_CONF_pm_aew1              (1U<<2)
#define REG_PHASE_MEASURE_CONF_pm_aew0              (1U<<1)
#define REG_PHASE_MEASURE_CONF_pm_aew_mask          (0x3U<<1)
#define REG_PHASE_MEASURE_CONF_pm_aew_shift         (1U)
#define REG_PHASE_MEASURE_CONF_pm_ae                (1U<<0)

#if defined(ST25R3916)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_d3          (1U<<7)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_d2          (1U<<6)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_d1          (1U<<5)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_d0          (1U<<4)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_d_mask      (0xfU<<4)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_d_shift     (4U)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_aam         (1U<<3)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_aew1        (1U<<2)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_aew0        (1U<<1)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_aew_mask    (0x3U<<1)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_aew_shift   (1U)
  #define REG_CAPACITANCE_MEASURE_CONF_cm_ae          (1U<<0)
#elif defined(ST25R3916B)
  #define REG_MEAS_TX_DELAY_m_phase_ana               (1U<<7)
  #define REG_MEAS_TX_DELAY_m_amp_ana                 (1U<<6)
  #define REG_MEAS_TX_DELAY_rfu1                      (1U<<5)
  #define REG_MEAS_TX_DELAY_rfu0                      (1U<<4)
  #define REG_MEAS_TX_DELAY_meas_tx_del3              (1U<<3)
  #define REG_MEAS_TX_DELAY_meas_tx_del2              (1U<<2)
  #define REG_MEAS_TX_DELAY_meas_tx_del1              (1U<<1)
  #define REG_MEAS_TX_DELAY_meas_tx_del0              (1U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_0ms           (0U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_0_60ms        (1U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_1_21ms        (2U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_1_81ms        (3U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_2_42ms        (4U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_3_02ms        (5U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_3_62ms        (6U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_4_23ms        (7U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_4_83ms        (8U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_5_44ms        (9U<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_6_44ms        (0xAU<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_7_25ms        (0xBU<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_8_46ms        (0xCU<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_9_67ms        (0xDU<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_10_87ms       (0xEU<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_12_08ms       (0xFU<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_mask          (0xFU<<0)
  #define REG_MEAS_TX_DELAY_meas_tx_del_shift         (0U)
#endif /* ST25R3916 */

#define REG_IC_IDENTITY_ic_type4                    (1U<<7)
#define REG_IC_IDENTITY_ic_type3                    (1U<<6)
#define REG_IC_IDENTITY_ic_type2                    (1U<<5)
#define REG_IC_IDENTITY_ic_type1                    (1U<<4)
#define REG_IC_IDENTITY_ic_type0                    (1U<<3)
#define REG_IC_IDENTITY_ic_type_st25r3916           (5U<<3)
#define REG_IC_IDENTITY_ic_type_st25r3916B          (6U<<3)
#define REG_IC_IDENTITY_ic_type_mask                (0x1fU<<3)
#define REG_IC_IDENTITY_ic_type_shift               (3U)
#define REG_IC_IDENTITY_ic_rev2                     (1U<<2)
#define REG_IC_IDENTITY_ic_rev1                     (1U<<1)
#define REG_IC_IDENTITY_ic_rev0                     (1U<<0)
#define REG_IC_IDENTITY_ic_rev_v0                   (0U<<0)
#define REG_IC_IDENTITY_ic_rev_mask                 (7U<<0)
#define REG_IC_IDENTITY_ic_rev_shift                (0U)


#define SPACE_B (0x40) /* Space B register flag */

#define OPERATION_EN        (0x80)
#define OPERATION_RX_EN     (0x40)
#define OPERATION_RX_CHN    (0x20)
#define OPERATION_TX_MAN    (0x10)
#define OPERATION_TX_EN     (0x08)
#define OPERATION_WU        (0x04)
#define OPERATION_EN_FD_CX  (0x03)


/* Main interrupt register */
#define IRQ_MASK_OSC             (uint32_t)(0x00000080U)   /*!< ST25R3916 oscillator stable interrupt                       */
#define IRQ_MASK_FWL             (uint32_t)(0x00000040U)   /*!< ST25R3916 FIFO water level interrupt                        */
#define IRQ_MASK_RXS             (uint32_t)(0x00000020U)   /*!< ST25R3916 start of receive interrupt                        */
#define IRQ_MASK_RXE             (uint32_t)(0x00000010U)   /*!< ST25R3916 end of receive interrupt                          */
#define IRQ_MASK_TXE             (uint32_t)(0x00000008U)   /*!< ST25R3916 end of transmission interrupt                     */
#define IRQ_MASK_COL             (uint32_t)(0x00000004U)   /*!< ST25R3916 bit collision interrupt                           */
#define IRQ_MASK_RX_REST         (uint32_t)(0x00000002U)   /*!< ST25R3916 automatic reception restart interrupt             */
#define IRQ_MASK_RFU             (uint32_t)(0x00000001U)   /*!< ST25R3916 RFU interrupt                                     */

/* Timer and NFC interrupt register */
#define IRQ_MASK_DCT             (uint32_t)(0x00008000U)   /*!< ST25R3916 termination of direct command interrupt.          */
#define IRQ_MASK_NRE             (uint32_t)(0x00004000U)   /*!< ST25R3916 no-response timer expired interrupt               */
#define IRQ_MASK_GPE             (uint32_t)(0x00002000U)   /*!< ST25R3916 general purpose timer expired interrupt           */
#define IRQ_MASK_EON             (uint32_t)(0x00001000U)   /*!< ST25R3916 external field on interrupt                       */
#define IRQ_MASK_EOF             (uint32_t)(0x00000800U)   /*!< ST25R3916 external field off interrupt                      */
#define IRQ_MASK_CAC             (uint32_t)(0x00000400U)   /*!< ST25R3916 collision during RF collision avoidance interrupt */
#define IRQ_MASK_CAT             (uint32_t)(0x00000200U)   /*!< ST25R3916 minimum guard time expired interrupt              */
#define IRQ_MASK_NFCT            (uint32_t)(0x00000100U)   /*!< ST25R3916 initiator bit rate recognised interrupt           */

/* Error and wake-up interrupt register */
#define IRQ_MASK_CRC             (uint32_t)(0x00800000U)   /*!< ST25R3916 CRC error interrupt                               */
#define IRQ_MASK_PAR             (uint32_t)(0x00400000U)   /*!< ST25R3916 parity error interrupt                            */
#define IRQ_MASK_ERR2            (uint32_t)(0x00200000U)   /*!< ST25R3916 soft framing error interrupt                      */
#define IRQ_MASK_ERR1            (uint32_t)(0x00100000U)   /*!< ST25R3916 hard framing error interrupt                      */
#define IRQ_MASK_WT              (uint32_t)(0x00080000U)   /*!< ST25R3916 wake-up interrupt                                 */
#define IRQ_MASK_WAM             (uint32_t)(0x00040000U)   /*!< ST25R3916 wake-up due to amplitude interrupt                */
#define IRQ_MASK_WPH             (uint32_t)(0x00020000U)   /*!< ST25R3916 wake-up due to phase interrupt                    */
#define IRQ_MASK_WCAP  IRQ_MASK_NONE   /*!< ST25R3916B disable capacitive WU                            */

/* Passive Target Interrupt Register */
#define IRQ_MASK_PPON2           (uint32_t)(0x80000000U)   /*!< ST25R3916 PPON2 Field on waiting Timer interrupt            */
#define IRQ_MASK_SL_WL           (uint32_t)(0x40000000U)   /*!< ST25R3916 Passive target slot number water level interrupt  */
#define IRQ_MASK_APON            (uint32_t)(0x20000000U)   /*!< ST25R3916 Anticollision done and Field On interrupt         */
#define IRQ_MASK_RXE_PTA         (uint32_t)(0x10000000U)   /*!< ST25R3916 RXE with an automatic response interrupt          */
#define IRQ_MASK_WU_F            (uint32_t)(0x08000000U)   /*!< ST25R3916 212/424b/s Passive target interrupt: Active       */
#define IRQ_MASK_RFU2            (uint32_t)(0x04000000U)   /*!< ST25R3916 RFU2 interrupt                                    */
#define IRQ_MASK_WU_A_X          (uint32_t)(0x02000000U)   /*!< ST25R3916 106kb/s Passive target state interrupt: Active*   */
#define IRQ_MASK_WU_A            (uint32_t)(0x01000000U)   /*!< ST25R3916 106kb/s Passive target state interrupt: Active    */

#define SPI_WRITE_DELAY_US (2000)

#define I2C_ADDRESS        (0x50)

#define NFC_A_CONFIG_SIZE   (15U)

/* Buffer length for SPI transfers */
#define BUFFER_LENGTH (128)

#if IS_ACTIVE(ENABLE_DEBUG)
#define PRINTBUFF printbuff
static void printbuff(uint8_t *buff, unsigned len)
{
    while (len) {
        len--;
        printf("%02x ", *buff++);
    }
    printf("\n");
}
#else
#define PRINTBUFF(...)
#endif

static void wait_ready(st25_t *dev)
{
    mutex_lock(&(dev->trap));
}

static int _write(const st25_t *dev, uint8_t *buff, unsigned len)
{
    assert(dev != NULL);
    assert(buff != NULL);
    assert(len > 0);

    if (dev->conf->mode == ST25_SPI) {

        spi_acquire(dev->conf->spi, dev->conf->nss, SPI_MODE_1, SPI_CLK_1MHZ);
        gpio_clear(dev->conf->nss);

        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, buff, NULL, len);

        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);

    } else {
        /* i2c communication */
        i2c_acquire(dev->conf->i2c);
        i2c_write_bytes(dev->conf->i2c, I2C_ADDRESS, buff, len, 0);
        i2c_release(dev->conf->i2c);
    }

    DEBUG("st25 -> ");
    PRINTBUFF(buff, len);

    return 0;
}

static int _write_and_read(const st25_t *dev, uint8_t *read, unsigned len, 
    uint8_t *to_write, unsigned to_write_len)
{
    assert(dev != NULL);
    assert(to_write != NULL);
    assert(read != NULL);
    assert(len > 0);

    if (dev->conf->mode == ST25_SPI) {
        spi_acquire(dev->conf->spi, dev->conf->nss, SPI_MODE_1, SPI_CLK_1MHZ);
        gpio_clear(dev->conf->nss);

        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true,
                            to_write, NULL, to_write_len);
        spi_transfer_bytes(dev->conf->spi, SPI_CS_UNDEF, true, NULL, read, len);

        gpio_set(dev->conf->nss);
        spi_release(dev->conf->spi);
    } else {
        i2c_acquire(dev->conf->i2c);

        i2c_write_bytes(dev->conf->i2c, I2C_ADDRESS, to_write, to_write_len, 0);
        i2c_read_bytes(dev->conf->i2c, I2C_ADDRESS, read, len, 0);

        i2c_release(dev->conf->i2c);
    }

    DEBUG("st25 -> ");
    PRINTBUFF(to_write, to_write_len);

    DEBUG("st25 <- ");
    PRINTBUFF(read, len);

    return len;
}

static int _write_multiple_reg(const st25_t *dev, uint8_t reg, const uint8_t *data, unsigned len)
{
    assert(dev != NULL);
    assert(data != NULL);
    assert(reg <= 0x3F);

    uint8_t buff[BUFFER_LENGTH];
    if ((reg & SPACE_B) != 0U) {
        /* we are in register space B */
        buff[0] = CMD_REGISTER_SPACE_B_ACCESS;
        buff[1] = SPI_MODE_REGISTER_WRITE | reg;
        memcpy(&buff[2], data, len);
    } else {
        buff[0] = SPI_MODE_REGISTER_WRITE | reg;
        memcpy(&buff[1], data, len);
    }

    return _write(dev, buff, len + 1);
}

static int _write_reg(const st25_t *dev, uint8_t reg, uint8_t data)
{
    assert(dev != NULL);
    assert(reg <= 0x3F);

    uint8_t buff[BUFFER_LENGTH];
    if ((reg & SPACE_B) != 0U) {
        /* we are in register space B */
        buff[0] = CMD_REGISTER_SPACE_B_ACCESS;
        buff[1] = SPI_MODE_REGISTER_WRITE | reg;
        buff[2] = data;
        return _write(dev, buff, 3);
    } else {
        buff[0] = SPI_MODE_REGISTER_WRITE | reg;
        buff[1] = data;
        return _write(dev, buff, 2);
    }
}

static int _read_reg(const st25_t *dev, uint8_t reg, uint8_t *data)
{
    assert(dev != NULL);
    assert(data != NULL);
    assert(reg <= 0x3F);

    *data = 0;

    uint8_t buff[BUFFER_LENGTH];
    uint8_t to_send = 0;
    if ((reg & SPACE_B) != 0U) {
        /* we are in register space B */
        buff[0] = CMD_REGISTER_SPACE_B_ACCESS;
        reg = reg & ~SPACE_B; // clear the SPACE_B bit
        buff[1] = SPI_MODE_REGISTER_READ | reg;
        to_send = 2;
    } else {
        buff[0] = SPI_MODE_REGISTER_READ | reg;
        to_send = 1;
    }

    int ret = _write_and_read(dev, data, 1, buff, to_send);
    return ret;
}

static int _fifo_load(const st25_t *dev, const uint8_t *data, unsigned len)
{
    assert(dev != NULL);
    assert(data != NULL);

    uint8_t buff[BUFFER_LENGTH];
    buff[0] = SPI_MODE_FIFO_LOAD;
    memcpy(&buff[1], data, len);

    return _write(dev, buff, len + 1);
}

static int _read_interrupts(st25_t *dev, uint32_t *interrupts) {
    assert(dev != NULL);
    assert(interrupts != NULL);

    uint8_t reg_content = 0;
    *interrupts = 0;

    for (int i = 0; i < 4; i++) {
        int ret = _read_reg(dev, REG_IRQ_MAIN + i, &reg_content);
        if (ret < 0) {
            return ret;
        }
        *interrupts |= ((uint32_t) reg_content << (8 * i));
    }

    return 0;
}

static int _fifo_status(const st25_t *dev, uint16_t *byte_size, uint8_t *bit_size) {
    assert(dev != NULL);

    uint8_t status_1 = 0;
    uint8_t status_2 = 0;

    _read_reg(dev, REG_FIFO_STATUS1, &status_1);
    _read_reg(dev, REG_FIFO_STATUS2, &status_2);

    if (status_2 & 0x10 || status_2 & 0x20) {
        DEBUG("st25: FIFO underflow/overflow\n");
        return -1;
    }

    *byte_size = (status_1) | (((uint16_t) (status_2 & 0xC0)) << 2);
    *bit_size  = (status_2 & 0x0E) >> 1;

    return 0;
}

static int _fifo_read(const st25_t *dev, uint8_t *data, uint16_t *bytes, uint8_t *bits)
{
    assert(dev != NULL);
    assert(data != NULL);

    /* get the amount of bytes in the FIFO */
    _fifo_status(dev, bytes, bits);

    if (*bytes == 0 && *bits == 0) {
        /* no data in FIFO */
        DEBUG("st25: FIFO is empty\n");
        return -1;
    }

    if (*bytes > BUFFER_LENGTH - 1) {
        DEBUG("st25: FIFO size %u is too large, max is %u\n", *bytes, BUFFER_LENGTH - 1);
        return -1;
    }

    DEBUG("st25: Reading %u bytes and %u bits from FIFO\n", *bytes, *bits);

    uint8_t operation_mode = SPI_MODE_FIFO_READ;

    /* read the bytes from the FIFO */
    int ret = _write_and_read(dev, data, *bits > 0 ? (unsigned) *bytes + 1 : (unsigned) *bytes, 
                              &operation_mode, 1);
    return ret;
}

/* 1 is disabled, 0 is active */
static int _write_interrupt_mask(st25_t *dev, uint32_t mask) 
{
    assert(dev != NULL);

    for (int i = 0; i < 4; i++) {
        uint8_t reg_content = (mask >> (8 * i)) & 0xFF;
        int ret = _write_reg(dev, REG_IRQ_MASK_MAIN + i, reg_content);
        if (ret < 0) {
            return ret;
        }
    }

    return 0;
}

static int _disable_all_interrupts(st25_t *dev)
{
    assert(dev != NULL);

    /* Disable all interrupts */
    uint32_t mask = 0xFFFFFFFFU;
    return _write_interrupt_mask(dev, mask);
}

static int _enable_all_interrupts(st25_t *dev)
{
    assert(dev != NULL);

    /* Enable all interrupts */
    uint32_t mask = 0x00000000U; // Enable all interrupts
    return _write_interrupt_mask(dev, mask);
}

static int _send_cmd(st25_t *dev, uint8_t cmd)
{
    assert(dev != NULL);

    uint8_t operation_mode = SPI_MODE_DIRECT_COMMAND | cmd;

    return _write(dev, &operation_mode, 1);
}

static void _nfc_event(void *dev)
{
    mutex_unlock(&(((st25_t *)dev)->trap));
}

/* waits until any one of the IRQs defined by mask are triggered */
static void wait_for(st25_t *dev, uint32_t mask) {
    assert(dev != NULL);

    do {
        DEBUG("st25 wait for mask: %lu\n", mask);
        mutex_lock(&(dev->trap));
        DEBUG("st25: NFC event triggered\n");
        _read_interrupts(dev, &(((st25_t *)dev)->irq_status));
        DEBUG("st25 irq: %08" PRIx32 "\n", dev->irq_status);
    } while((dev->irq_status & mask) == 0);

    DEBUG("st25: Continuing...\n");
}

static int _write_test_reg(st25_t *dev, uint8_t byte) {
    assert(dev != NULL);

    uint8_t buff[3]  = {0};

    buff[0] = 0xFC;
    buff[1] = SPI_MODE_REGISTER_WRITE | 0x01;
    buff[2] = byte;

    int ret = _write(dev, buff, 3);
    if (ret < 0) {
        DEBUG("st25: Error writing test register\n");
    }

    return ret;

}

static void _set_general_purpose_timer(st25_t *dev, uint16_t time) {
    assert(dev != NULL);
    /* the timer is defined in steps of 8 / fc (590 ns) */

    /* Set the general purpose timer */
    uint8_t reg_content = (time >> 8) & 0xFF;
    _write_reg(dev, REG_GPT1, reg_content); /* contains the msb */

    reg_content = time & 0xFF;
    _write_reg(dev, REG_GPT2, reg_content);
}

static void _busy_wait_for(st25_t *dev, uint32_t mask) {
    assert(dev != NULL);

    DEBUG("st25: Busy waiting for mask: %lu\n", mask);
    while(true) {
        uint32_t irq_status = 0;
        _read_interrupts(dev, &irq_status);

        if ((irq_status & mask) != 0) {
            DEBUG("st25: Mask %lu triggered\n", mask);
            break;
        }
        ztimer_sleep(ZTIMER_USEC, 100); /* sleep for 0.1 ms */
    }
}

static void _send_sens_req(st25_t *dev) {
    uint8_t reg_content = 0;
    // uint8_t sens_req = 0x26;

    // _disable_all_interrupts(dev);

    _write_interrupt_mask(dev, ~(IRQ_MASK_TXE | IRQ_MASK_RXE | IRQ_MASK_RXS | IRQ_MASK_COL));

    reg_content = OPERATION_EN | OPERATION_RX_EN | OPERATION_TX_EN;
    _write_reg(dev, REG_OP_CONTROL, reg_content);

    _read_reg(dev, REG_OP_CONTROL, &reg_content);
    assert((reg_content & OPERATION_EN) != 0);

    /* mode configuratin */
    _write_reg(dev, REG_MODE, REG_MODE_om_iso14443a);

    /* prepare transmission */
    _send_cmd(dev, CMD_STOP);
    _send_cmd(dev, CMD_RESET_RX_GAIN);

/*     reg_content = REG_ISO14443A_NFC_no_tx_par | REG_ISO14443A_NFC_no_rx_par | 
    REG_ISO14443A_NFC_nfc_f0;
    _write_reg(dev, REG_ISO14443A_NFC, reg_content); */

    _write_reg(dev, REG_AUX, REG_AUX_no_crc_rx);

    // _enable_all_interrupts(dev);

    // _write_interrupt_mask(dev, ~(IRQ_MASK_GPE)); /* disable all but */
    _write_reg(dev, REG_NUM_TX_BYTES2, 0x00);


    // _set_general_purpose_timer(dev, 0xFFFA);

    /* check if the timer is on */
    // assert((reg_content & REG_NFCIP1_BIT_RATE_gpt_on) != 0);

    //_busy_wait_for(dev, IRQ_MASK_TXE | IRQ_MASK_RXE | IRQ_MASK_RXS | IRQ_MASK_COL | IRQ_MASK_GPE);


    DEBUG("st25: Sending SENS_REQ\n");
    _send_cmd(dev, CMD_TRANSMIT_REQA);
    wait_for(dev, IRQ_MASK_TXE);

    ztimer_sleep(ZTIMER_USEC, 100);

    DEBUG("st25: SENS_REQ sent, waiting for response...\n");

    /* TODO: we only get 4 bits here. why? */
    uint8_t fifo_buffer[BUFFER_LENGTH] = {0};
    uint16_t bytes = 0;
    uint8_t bits = 0;
    _fifo_read(dev, fifo_buffer, &bytes, &bits);

    DEBUG("st25: SENS_RES received with byte 1: 0x%02x and byte 2: 0x%02x\n",
          fifo_buffer[0], fifo_buffer[1]);
}

/* changes only certain bits and leaves the rest unchanged */
static void _change_bits_reg(st25_t *dev, uint8_t reg, uint8_t mask, uint8_t value) {
    assert(dev != NULL);
    assert(reg <= 0x3F);

    uint8_t reg_content = 0;
    _read_reg(dev, reg, &reg_content);

    reg_content &= ~mask; // clear the bits defined by mask
    reg_content |= (value & mask); // set the bits defined by value

    _write_reg(dev, reg, reg_content);
}


static void _configure_device(st25_t *dev) {
    assert(dev != NULL);

    _send_cmd(dev, CMD_SET_DEFAULT);

    // set to 3V
    // _write_reg(dev, REG_IO_CONF2, REG_IO_CONF2_sup3V_3V);
}

static void _load_pt_memory_a_config(st25_t *dev, const uint8_t *nfc_a_config) {
    assert(dev != NULL);

    uint8_t buff[NFC_A_CONFIG_SIZE + 1];
    buff[0] = SPI_MODE_PT_MEMORY_LOAD_A_CONFIG;
    memcpy(&buff[1], nfc_a_config, NFC_A_CONFIG_SIZE);
    int ret = _write(dev, buff, NFC_A_CONFIG_SIZE + 1);
    if (ret < 0) {
        DEBUG("st25: Error loading NFC-A configuration\n");
    }

    return;

}

static void _clear_interrupts(st25_t *dev) {
    assert(dev != NULL);

    /* Clear all interrupts */
    uint32_t reg_content = 0;
    _read_interrupts(dev, &reg_content);
}   

int st25_poll_nfc_a(st25_t *dev) {
    DEBUG("st25: Polling for NFC-A...\n");

    _enable_all_interrupts(dev);

    _write_reg(dev, REG_BIT_RATE, 0x00);

    _send_sens_req(dev);

    return 0;
}

int st25_listen_nfc_a(st25_t *dev) {
    assert(dev != NULL);

    DEBUG("st25: Listening for NFC-A...\n");

    uint8_t buff[] = {
        0x74, 0x34, 0x48, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* NFCID1 */
        0x00, 0x00, /* SENS_RES */
        0x20, /* SAK L1 */
        0x20, /* SAK L2 */
        0x00, /* SAK L3 RFU */
    };
    
    _load_pt_memory_a_config(dev, buff);

    _write_reg(dev, REG_BIT_RATE, 0x00);
    _write_reg(dev, REG_MODE, REG_MODE_om_targ_nfca | REG_MODE_targ | 0x01);
    

    _write_reg(dev, REG_OP_CONTROL, OPERATION_EN | 
                    OPERATION_RX_EN | OPERATION_TX_EN);


    _clear_interrupts(dev);
    _write_interrupt_mask(dev, ~(IRQ_MASK_NFCT | IRQ_MASK_WU_A | IRQ_MASK_WU_A_X));

    _change_bits_reg(dev, REG_PASSIVE_TARGET, REG_PASSIVE_TARGET_d_106_ac_a, 0x00);


    _send_cmd(dev, CMD_GO_TO_SENSE);
    
    uint8_t target_state = 0;
    _read_reg(dev, REG_PASSIVE_TARGET, &target_state);
    DEBUG("st25: Passive target state: 0x%02x\n", target_state);

    wait_for(dev, IRQ_MASK_NFCT);
    return 0;
}



int st25_init(st25_t *dev, const st25_params_t *params)
{
    assert(dev != NULL);
    assert(params != NULL);

    dev->conf = params;

    /* Initialize SPI */

    if (dev->conf->mode == ST25_SPI) {
        /* we handle the CS line manually... */
        gpio_init(dev->conf->nss, GPIO_OUT);
        gpio_set(dev->conf->nss);
    }

    dev->irq_status = 0;

    gpio_init_int(dev->conf->irq, GPIO_IN_PD, GPIO_RISING,
            _nfc_event, (void *)dev);

    mutex_init(&(dev->trap));
    mutex_lock(&(dev->trap));

    _configure_device(dev);
    _disable_all_interrupts(dev);
    
    DEBUG("st25: Initialized ST25 device\n");

    return 0;
}
