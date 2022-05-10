/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ST25R3911B_NFCA_H_
#define ST25R3911B_NFCA_H_

#include <zephyr/kernel.h>
#include <zephyr/types.h>

/**
 * @file
 * @defgroup st25r3911b_nfca ST25R3911B NFC Reader NFC-A Technology
 * @{
 *
 * @brief API for the ST25R3911B NFC Reader NFC-A Technology.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** NFC-A Events count. */
#define ST25R3911B_NFCA_EVENT_CNT 2

/** NFC-A CRC length. */
#define ST25R3911B_NFCA_CRC_LEN 2

/** NFC-A NFCID1 maximum length. */
#define ST25R3911B_NFCA_NFCID1_MAX_LEN 10

/**
 * @defgroup st25r3911b_error NFC-A protocol and transmission error.
 * @{
 */
#define ST25R3911B_NFCA_ERR_CRC 1
#define ST25R3911B_NFCA_ERR_PARITY 2
#define ST25R3911B_NFCA_ERR_FRAMING 3
#define ST25R3911B_NFCA_ERR_SOFT_FRAMING 4
#define ST25R3911B_NFCA_ERR_HARD_FRAMING 5
#define ST25R3911B_NFCA_ERR_LAST_BYTE_INCOMPLETE 6
#define ST25R3911B_NFCA_ERR_TRANSMISSION 7

/**
 * @}
 */

/** @brief NFC-A Tag types.
 */
enum st25r3911b_nfca_tag_type {
	ST25R3911B_NFCA_TAG_TYPE_T2T,
	ST25R3911B_NFCA_TAG_TYPE_T4T,
	ST25R3911B_NFCA_TAG_TYPE_NFCDEP,
	ST25R3911B_NFCA_TAG_TYPE_T4T_NFCDEP,
	ST25R3911B_NFCA_TAG_TYPE_T1T
};

/** @brief NFC-A Tag detect command.
 */
enum st25r3911b_nfca_detect_cmd {
	ST25R3911B_NFCA_DETECT_CMD_ALL_REQ,
	ST25R3911B_NFCA_DETECT_CMD_SENS_REQ
};

/** @brief NFC-A transceiver buffer.
 */
struct st25r3911b_nfca_buf {
	/** Data to TX/RX. */
	uint8_t *data;

	/** Data length. */
	size_t len;
};

/** @brief NFC-A SENS response.
 */
struct st25r3911b_nfca_sens_resp {
	/** Anticollision data. */
	uint8_t anticollison;

	/** Platform information data.*/
	uint8_t platform_info;
};

/** @brief NFC-A Tag information.
 */
struct st25r3911b_nfca_tag_info {
	/** Tag type. */
	enum st25r3911b_nfca_tag_type type;

	/** SENS response.*/
	struct st25r3911b_nfca_sens_resp sens_resp;

	/** SEL response. */
	uint8_t sel_resp;

	/** NFCID1 length. */
	uint8_t nfcid1_len;

	/** NFCID1. */
	uint8_t nfcid1[ST25R3911B_NFCA_NFCID1_MAX_LEN];

	/** Tag sleep.*/
	bool sleep;
};

/** @brief NFCA_A CRC structure.
 */
struct st25r3911b_nfca_crc {
	/** CRC - 16. */
	uint8_t crc[ST25R3911B_NFCA_CRC_LEN];
};

/** @brief NFC-A callback.
 *
 *  This structure is used for tracking and synchronized NFC-A operation.
 */
struct st25r3911b_nfca_cb {
	/** @brief Field on indication callback.
	 *
	 *  Field is on, reader is ready to next operation.
	 */
	void (*field_on)(void);

	/** @brief Field off indication callback.
	 */
	void (*field_off)(void);

	/** @brief Tag detected callback.
	 *
	 *  The SENS response is received.
	 *
	 *  @param sens_resp Response to ALL request or SENS request.
	 */
	void (*tag_detected)(const struct st25r3911b_nfca_sens_resp *sens_resp);

	/** @brief Collision resolution procedure completed.
	 *
	 *  @param tag_info NFC-A Tag information.
	 *  @param err Procedure error.
	 */
	void (*anticollision_completed)(const struct st25r3911b_nfca_tag_info *tag_info,
					int err);

	/** @brief NFC-A transceive operation completed.
	 *
	 *  @param data Received data from tag.
	 *  @param len Received data length.
	 *  @param err Transfer error. If 0, the transfer was successful.
	 */
	void (*transfer_completed)(const uint8_t *data, size_t len, int err);

	/** @brief NFC-A receive time-out.
	 *
	 *  @param tag_sleep If set, the tag is in sleep mode.
	 */
	void (*rx_timeout)(bool tag_sleep);

	/** @brief NFC-A tag sleep state notification.
	 */
	void (*tag_sleep)(void);
};

/** @brief Initialize NFC Reader NFC-A technology.
 *
 *  @details This function initializes everything that is required,
 *           like interrupts and common reader functionality.
 *
 *  @param[out] events NFC-A Events.
 *  @param[in] cnt Event count. This driver needs 2 events.
 *  @param[in] cb Callback structure.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_init(struct k_poll_event *events, uint8_t cnt,
			  const struct st25r3911b_nfca_cb *cb);

/** @brief Switch on the NFC Reader field.
 *
 *  @details If the field is switched on successfully,
 *           the @em nfca_cb.field_on() callback is called.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_field_on(void);

/** @brief Switch off the NFC Reader field.
 *
 *  @details If the field is switched off successfully,
 *           the @em nfca_cb.field_off() callback is called.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_field_off(void);

/** @brief Detect tag by sending a detection command.
 *
 *  @details The command is sent according to NFC Forum Digital 2.0
 *           6.6. After success, the @em nfca_cb.tag_detected()
 *           callback is called.
 *
 *  @param cmd Command.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_tag_detect(enum st25r3911b_nfca_detect_cmd cmd);

/** @brief NFC-A collision resolution procedure.
 *
 *  @details The collision resolution procedure is described in
 *           NFC Forum Activity 2.0 9.4.4. After this
 *           procedure, @em nfca_cb.anticollision_completed() is called.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_anticollision_start(void);

/** @brief Exchange the specified amount of data.
 *
 *  @details This is a generic function for NFC-A data exchange.
 *
 *  @param[in] tx TX data buffer.
 *  @param[in,out] rx RX data buffer.
 *  @param[in] fdt Maximum frame delay time (according to NFC Forum Digital 2.0
 *                 6.10.1).
 *  @param[in] auto_crc If set, the CRC is generated automatically by the
 *                      NFC Reader. Otherwise, the CRC must be added
 *                      manually to the TX buffer.
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_transfer(const struct st25r3911b_nfca_buf *tx,
			     const struct st25r3911b_nfca_buf *rx,
			     uint32_t fdt, bool auto_crc);

/** @brief Exchange the specified amount of data with an automatically
 *         generated CRC.
 *
 *  @param[in] tx TX data buffer.
 *  @param[in,out] rx RX data buffer.
 *  @param[in] fdt Maximum frame delay time (according to NFC Forum Digital 2.0
 *                 6.10.1).
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
static inline int st25r3911b_nfca_transfer_with_crc(const struct st25r3911b_nfca_buf *tx,
						    const struct st25r3911b_nfca_buf *rx,
						    uint32_t fdt)
{
	return st25r3911b_nfca_transfer(tx, rx, fdt, true);
}

/** @brief Exchange the specified amount of data with a manually
 *         generated CRC.
 *
 *  @details The CRC must be added manually to the TX buffer.
 *
 *  @param[in] tx TX data buffer.
 *  @param[in,out] rx RX data buffer.
 *  @param[in] fdt Maximum frame delay time (according to NFC Forum Digital 2.0
 *                 6.10.1).
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
static inline int st25r3911b_nfca_transfer_without_crc(const struct st25r3911b_nfca_buf *tx,
						       const struct st25r3911b_nfca_buf *rx,
						       uint32_t fdt)
{
	return st25r3911b_nfca_transfer(tx, rx, fdt, false);
}

/** @brief Send NFC-A tag sleep command.
 *
 *  @details This function sends the tag sleep command
 *           (according to NFC Forum Digital 2.0 6.9).
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_tag_sleep(void);

/** @brief Calculate CRC 16 for NFC-A payload.
 *
 *  @details This function generates an NFC-A CRC (according to
 *           NFC Forum Digital 2.0 6.4.1.3).
 *
 *  @param[in] data Payload data.
 *  @param[in] len Data length.
 *  @param[out] crc_val NFC-A CRC value.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_crc_calculate(const uint8_t *data, size_t len,
				  struct st25r3911b_nfca_crc *crc_val);

/** @brief Process NFC-A library.
 *
 *  @details This function must be called periodically from
 *           thread to process the NFC reader state and interrupts.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_process(void);


#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* ST25R3911B_NFCA_H_ */
