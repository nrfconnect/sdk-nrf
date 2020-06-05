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

/** Maximum TX packet length. */
#define ST25R3911B_MAX_TX_LEN 8191

/** No automatic collision resolution. */
#define ST25R3911B_NO_THRESHOLD_ANTICOLLISION 0xFF

/** Maximum collision resolution threshold. */
#define ST25R3911B_MAX_THRESHOLD 0x0F

/** Frame delay time (FDT) adjustment. */
#define ST25R3911B_FDT_ADJUST 64

/** Maximum FDT for the Mask Receive timer. */
#define ST25R3911B_MASK_RECEIVER_MAX_FDT 0xFC0

/** No-Response timer maximum value for step 64/fc. */
#define ST25R3911B_NRT_64FC_MAX 0x3FFFC0

/** Maximum FDT for the No-Response timer (in fc). */
#define ST25R3911B_NRT_FC_MAX 0x4095BF6A000

/** FC in 64/fc. */
#define NFC_1FC_IN_64FC 64

/** FC in 4096/fc. */
#define NFC_1FC_IN_4096FC 4096

/** ST25R3911B on shield the NFC technology indication LEDs.
 */
enum st25r3911b_leds {
	/** ST25R3911B NFCA LED ID. */
	ST25R3911B_NFCA_LED,

	/** ST25R3911B NFCB LED ID. */
	ST25R3911B_NFCB_LED,

	/** ST25R3911B NFCF LED ID. */
	ST25R3911B_NFCF_LED
};

/** Conversion fc to 64/fc. The timer register can be set
 *  using the following formula: <tt>register value * 64fc</tt>
 */
#define ST25R3911B_FC_TO_64FC(_fc) \
	ceiling_fraction((_fc), NFC_1FC_IN_64FC)

/** Conversion fc to 4096/fc. The timer register can be set
 *  using the following formula: <tt>register value * 4096fc</tt>
 */
#define ST25R3911B_FC_TO_4096FC(_fc) \
	ceiling_fraction((_fc), NFC_1FC_IN_4096FC)

/** @brief Initialize the NFC reader.
 *
 *  @details The NFC reader initialization is common for all
 *  NFC technology.
 *
 *  @retval 0  If the operation was successful.
 *             Otherwise, a (negative) error code is returned.
 */
int st25r3911b_init(void);

/** @brief Set the NFC Reader TX packet length.
 *
 *  @details The maximum packet length is @ref ST25R3911B_MAX_TX_LEN.
 *
 *  @param[in] len TX packet length.
 *
 *  @retval 0  If the operation was successful.
 *             Otherwise, a (negative) error code is returned.
 */
int st25r3911b_tx_len_set(u16_t len);

/** @brief Set NFC Reader No-Response timer.
 *
 *  @details This timer is set to verify whether a tag response is received
 *           within the configured time. Time measurement is started at the
 *           end of the ST25R3911B transmission. According to NFC Forum
 *           Digital 2.0 6.10.1.
 *
 *  @param[in] fc         Wait time defined in 64/fc or 4096/fc unit.
 *  @param[in] long_range Long range mode. If this parameter is set, one
 *                        FDT time is fc * 4096, otherwise fc * 64.
 *  @param[in] emv        Emv mode. If set, the timer unconditionally
 *                        produces an IRQ when it expires and is not
 *                        stopped by a direct command Clear. This means that
 *                        IRQ is independent of whether a tag reply was
 *                        detected. If a tag reply is being processed at the
 *                        moment of time-out, no other action is taken. If no
 *                        tag response is being processed, the signal rx_on
 *                        is forced to low to stop the receive process.
 *                        If emv mode is not set, the
 *                        IRQ is produced if the No-Response timer expires
 *                        before a start of a tag reply is detected, and rx_on
 *                        is forced to low to stop the receiver process. If
 *                        the start of a tag reply is detected
 *                        before time-out, the timer is stopped, and no IRQ is
 *                        produced.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_non_response_timer_set(u16_t fc, bool long_range, bool emv);

/** Set NFC Reader Mask Receive timer.
 *
 *  @details The Mask Receive timer blocks the receiver and
 *           reception process in framing logic by keeping the RX
 *           disabled after the end of TX during the time the tag
 *           reply is not expected. The Mask Receive timer is automatically
 *           started at the end of transmission (at the end of EOF).
 *           According to NFC Forum Digital 2.0 6.10.1.
 *
 *  @param[in] fc Disable time in 64/fc.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_mask_receive_timer_set(u32_t fc);

/** @brief Perform automatic collision resolution and switch on the NFC Reader
 *         field.
 *
 *  @details Perform automatic collision resolution and switch
 *           the field on if no collision was detected.
 *           If thresholds are set to
 *           @ref ST25R3911B_NO_THRESHOLD_ANTICOLLISION,
 *           then RF Collision Avoidance will not be performed.
 *
 *  @param[in] collision_threshold Collision resolution threshold in mV
 *                                peek-to-peek.
 *  @param[in] peer_threshold Peer detection threshold in mV peek-to-peek.
 *  @param[in] delay Delay after field on in milliseconds,
 *                   according to NFC Forum Digital 2.0 Guard Time.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
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
 *           is configurable, so it must be read to read/write FIFO bytes
 *           if necessary. The NFC Reader supports transfer up to
 *           @ref ST25R3911B_MAX_TX_LEN,
 *           but FIFO is only @ref ST25R3911B_MAX_FIFO_LEN bytes.
 *
 *  @param[out] tx_lvl Number of bytes to write to FIFO if a
 *                     reload is needed.
 *  @param[out] rx_lvl Number of bytes to read to avoid FIFO overflow.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_fifo_reload_lvl_get(u8_t *tx_lvl, u8_t *rx_lvl);

/** @brief Enable/disable NFC Reader technology LED.
 *
 *  @details The NFC Reader has several LEDs to indicate which
 *           technology is used.
 *
 *  @param[in] led LED to set @ref st25r3911b_leds.
 *  @param[in] on If set, LED is on, otherwise LED is off.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_technology_led_set(enum st25r3911b_leds led, bool on);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ST25R3911B_H_ */
