/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ST25R3911B_H_
#define ST25R3911B_H_

#include <zephyr/types.h>

/**
 * @file
 * @defgroup st25r3911b_common ST25R3911B NFC Reader common functionality
 * @{
 *
 * @brief API for the ST25R3911B NFC Reader common functionality.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Max Tx packet length. */
#define ST25R3911B_MAX_TX_LEN 8191

/** No automatic collision resolution. */
#define ST25R3911B_NO_THRESHOLD_ANTICOLLISION 0xFF

/** Max RF Collision Avoidance Threshold */
#define ST25R3911B_MAX_THRESHOLD 0x0F

/** FTD adjustment. */
#define ST25R3911B_FDT_ADJUST 64

/** Mask Receive timer max FTD. */
#define ST25R3911B_MASK_RECEIVER_MAX_FDT 0xFC0

/** Non-Response Timer max value for step 64/fc. */
#define ST25R3911B_NRT_64FC_MAX 0x3FFFC0

/** Non-Response Timer max FTD in fc. */
#define ST25R3911B_NRT_FC_MAX 0x4095BF6A000

/** Fc in 64/fc. */
#define NFC_1FC_IN_64FC 64

/** Fc in 4096/fc. */
#define NFC_1FC_IN_4096FC 4096

/**
 * @defgroup st25r3911b_leds NFC Technology LEDs
 * @{
 * @brief ST25R3911B Leds pin.
 */
#define ST25R3911B_NFCF_LED 4
#define ST25R3911B_NFCB_LED 28
#define ST25R3911B_NFCA_LED 29

/**
 * @}
 */

/** Conversion FC to 64/FC. Timer register can be set
 *  using using the following formula: register value * 64fc.
 */
#define ST25R3911B_FC_TO_64FC(_fc) \
	ceiling_fraction((_fc), NFC_1FC_IN_64FC)

/** Conversion FC to 4096/FC. Timer register can be set
 *  using the following formula: register value * 4096fc.
 */
#define ST25R3911B_FC_TO_4096FC(_fc) \
	ceiling_fraction((_fc), NFC_1FC_IN_4096FC)

/** @brief Initialize NFC Reader
 *
 *  @details NFC Reader initialization common for all
 *           NFC technology.
 *
 *  @return Returns 0 if initialization was successful,
 *          otherwise negative value.
 */
int st25r3911b_init(void);

/** @brief Set NFC Reader Tx packet length.
 *
 *  @details Maximum packet length is @ref ST25R3911B_MAX_TX_LEN.
 *
 *  @param[in] len Tx packet length.
 *
 *  @return Returns 0 if initialization was successful,
 *          otherwise negative value.
 */
int st25r3911b_tx_len_set(u16_t len);

/** @brief Set NFC Reader Non-Response timer.
 *
 *  @details This timer is set to verify whether a tag response is received
 *           within the configured time. Time measurement is started at the
 *           end of the ST25R3911B transmission. According to NFC Forum
 *           Digital 2.0 6.10.1.
 *
 *  @param[in] fc         Wait time defined in 64/fc or 4096/fc unit.
 *  @param[in] long_range Long range mode, if set one FTD time is fc x 4096,
 *                        otherwise fc x 64.
 *  @param[in] emv        Emv mode, if set the timer unconditionally
 *                        produces an IRQ when it expires, it is also not
 *                        stopped by direct command Clear. This means that
 *                        IRQ is independent of the fact whether or not a
 *                        tag reply was detected. In case at the moment of
 *                        timeout a tag reply is being processed no other
 *                        action is taken, in the opposite case, when no tag
 *                        response is being processed additionally the
 *                        signal rx_on is forced to low to stop receive
 *                        process. Otherwise when evm mode is not set the
 *                        IRQ is produced in case the No-Response Timer expires
 *                        before a start of a tag reply is detected and rx_on
 *                        is forced to low to stop receiver process. In the
 *                        opposite case, when start of a tag reply is detected
 *                        before timeout, the timer is stopped, and no IRQ is
 *                        produced.
 *
 *  @return Returns 0 if setting was successful,
 *          otherwise negative value.
 */
int st25r3911b_non_response_timer_set(u16_t fc, bool long_range, bool emv);

/** Set NFC Reader Mask Receive timer.
 *
 *  @details The Mask Receive timer is blocking the receiver and
 *           reception process in framing logic by keeping the Rx
 *           disabled after the end of Tx during the time the tag
 *           reply is not expected. Mask Receive timer is automatically
 *           started at the end of transmission (at the end of EOF).
 *           According to NFC Forum Digital 2.0 6.10.1.
 *
 *  @param[in] fc disable time in 64/fc.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_mask_receive_timer_set(u32_t fc);

/** @brief Initial RF Collision Avoidance and switch on the NFC Reader
 *         field.
 *
 *  @details Perform the RF Collision Avoidance and switch
 *           the field on in case no collision was detected.
 *           If thresholds are set to
 *           @ref ST25R3911B_NO_THRESHOLD_ANTICOLLISION,
 *           then RF Collision Avoidance will not be performed.
 *
 *  @param[in] collision_threshold Collision avoidance Threshold in mV
 *                                peek-to_peek.
 *  @param[in] peer_threshold Peer detection Threshold in mV peek-to_peek.
 *  @param[in] delay Delay after field on in milliseconds.
 *                   According to NFC Forum Digital 2.0 Guard Time
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_field_on(u8_t collision_threshold, u8_t peer_threshold,
			u8_t delay);

/** @brief Disable NFC Reader Receiver and Transceiver.
 *
 *  @return Returns 0 if initialization was successful,
 *          otherwise negative value.
 */
int st25r3911b_rx_tx_disable(void);

/** @brief Get NFC Reader FIFO reload level.
 *
 *  @details The interrupt trigger level for read/write FIFO water level
 *           is configurable so it must be read to read/write FIFO bytes
 *           if necessary. NFC Reader support transfer up to
 *           @ref ST25R3911B_MAX_TX_LEN,
 *           but FIFO is only @ref ST25R3911B_MAX_FIFO_LEN bytes.
 *
 *  @param[out] tx_lvl Number of bytes to write to FIFO in case of
 *                     reload needed.
 *  @param[out] rx_lvl Number of bytes to read to avoid FIFO overflow.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_fifo_reload_lvl_get(u8_t *tx_lvl, u8_t *rx_lvl);

/** @brief Enable/Disable NFC Reader technology led.
 *
 *  @details NFC Reader have a several leds to indicate which
 *           technology is used.
 *
 *  @param[in] led Led to set @ref st25r3911b_leds.
 *  @param[in] on If set, led is on, otherwise led is off.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_technology_led_set(u32_t led, bool on);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ST25R3911B_H_ */
