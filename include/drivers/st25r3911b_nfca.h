/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ST25R3911B_NFCA_H_
#define ST25R3911B_NFCA_H_

#include <kernel.h>
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

/** @brief NFC-A Tag types
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

/** @brief NFC-A transceive buffer.
 */
struct st25r3911b_nfca_buf {
	/** Data to TX/RX. */
	u8_t *data;

	/** Data length. */
	size_t len;
};

/** @brief NFC-A SENS Response.
 */
struct st25r3911b_nfca_sens_resp {
	/** Anticollision data. */
	u8_t anticollison;

	/** Platform info data.*/
	u8_t platform_info;
};

/** @brief NFC-A Tag information structure.
 */
struct st25r3911b_nfca_tag_info {
	/** Tag type. */
	enum st25r3911b_nfca_tag_type type;

	/** SENS response.*/
	struct st25r3911b_nfca_sens_resp sens_resp;

	/** SEL Response. */
	u8_t sel_resp;

	/** NFCID1 length. */
	u8_t nfcid1_len;

	/** NFCID1. */
	u8_t nfcid1[ST25R3911B_NFCA_NFCID1_MAX_LEN];

	/** Tag sleep.*/
	bool sleep;
};

/** @brief NFCA_A crc structure.
 */
struct st25r3911b_nfca_crc {
	/** CRC - 16. */
	u8_t crc[ST25R3911B_NFCA_CRC_LEN];
};

/** @brief NFC-A callback structure.
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
	 *  The SENS Response is received.
	 *
	 *  @param sens_resp Response to ALL Request or SENS Request.
	 */
	void (*tag_detected)(const struct st25r3911b_nfca_sens_resp *sens_resp);

	/** @brief Collision resolution procedure completed.
	 *
	 *  @param tag_info NFC-A Tag information.
	 *  @param err Procedure error.
	 */
	void (*anticollision_completed)(const struct st25r3911b_nfca_tag_info *tag_info,
					int err);

	/** @brief NFC-A transceive completed.
	 *
	 *  @param data Received data from Tag.
	 *  @param len Received data length.
	 *  @param err Transfer error. If 0, transfer was successful.
	 */
	void (*transfer_completed)(const u8_t *data, size_t len, int err);

	/** @brief NFC-A Receive timeout.
	 *
	 *  @param tag_sleep if, set tag is in sleep mode.
	 */
	void (*rx_timeout)(bool tag_sleep);
};

/** @brief Initialize NFC Reader NFC-A technology
 *
 *  @details All necessary things will be initialized like
 *           interrupts and common reader functionality.
 *
 *  @param[out] events NFC-A Events.
 *  @param[in] cnt Event count. This driver needs 2 events.
 *  @param[in] cb Callback structure.
 *
 *  @return Returns 0 if initialization was successful,
 *          otherwise negative value.
 */
int st25r3911b_nfca_init(struct k_poll_event *events, u8_t cnt,
			  const struct st25r3911b_nfca_cb *cb);

/** @brief Switch on NFC Reader field.
 *
 *  @details If field will be switched on successfully,
 *           then @ref nfca_cb.field_on callback will be
 *           called.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_nfca_field_on(void);

/** @brief Switch off NFC Reader field.
 *
 *  @details If field will be switched off successfully,
 *           then @ref nfca_cb.field_off callback will be
 *           called.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_nfca_field_off(void);

/** @brief Detect tag by sending a detection command.
 *
 *  @details Command is send according to NFC Forum Digital 2.0
 *           6.6. After success the @ref nfca_cb.tag_detected callback,
 *           will be called.
 *
 *  @param cmd Command.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_nfca_tag_detect(enum st25r3911b_nfca_detect_cmd cmd);

/** @brief NFC-A Collision Avoidance procedure.
 *
 *  @details Collision Avoidance procedure describing in
 *           NFC Forum Activity 2.0 9.4.4. After this
 *           procedure @ref nfca_cb.anticollision_completed will
 *           be called.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_nfca_anticollision_start(void);

/** @brief Exchange the specified amount of data by NFC reader.
 *
 *  @details Generic function for NFC-A data exchange.
 *
 *  @param[in] tx TX data buffer.
 *  @param[in,out] rx RX data buffer.
 *  @param[in] fdt Maximum Frame Delay Time. According to NFC Forum Digital 2.0
 *                 6.10.1
 *  @param[in] auto_crc If set, CRC will be generated automatically by Reader.
 *                      Otherwise, CRC have to be added manually to Tx buffer.
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
int st25r3911b_nfca_transfer(const struct st25r3911b_nfca_buf *tx,
			     const struct st25r3911b_nfca_buf *rx,
			     u32_t fdt, bool auto_crc);

/** @brief Exchange the specified amount of data by NFC reader.
 *
 *  @details Function for NFC-A data exchange with automatically
 *           generated CRC.
 *
 *  @param[in] tx Transmit data buffer.
 *  @param[in,out] rx Rx data buffer.
 *  @param[in] fdt Maximum Frame Delay Time. According to NFC Forum Digital 2.0
 *                 6.10.1
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
static inline int st25r3911b_nfca_transfer_with_crc(const struct st25r3911b_nfca_buf *tx,
						    const struct st25r3911b_nfca_buf *rx,
						    u32_t fdt)
{
	return st25r3911b_nfca_transfer(tx, rx, fdt, true);
}

/** @brief Exchange the specified amount of data by NFC reader.
 *
 *  @details Function for NFC-A data exchange with manually
 *           generated CRC. CRC have to be add to the Tx buffer.
 *
 *  @param[in] tx Transmit data buffer.
 *  @param[in,out] rx Rx data buffer.
 *  @param[in] fdt Maximum Frame Delay Time. According to NFC Forum Digital 2.0
 *                 6.10.1
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.
 */
static inline int st25r3911b_nfca_transfer_without_crc(const struct st25r3911b_nfca_buf *tx,
						       const struct st25r3911b_nfca_buf *rx,
						       u32_t fdt)
{
	return st25r3911b_nfca_transfer(tx, rx, fdt, false);
}

/** @brief Send NFC-A tag sleep command.
 *
 *  @details This function sending tag sleep command.
 *           According to NFC Forum Digital 2.0 6.9
 *
 *  @return Returns 0 if operation was successful,
 *          otherwise negative value.

 */
int st25r3911b_nfca_tag_sleep(void);

/** @brief Calculate CRC 16 for NFC-A payload.
 *
 *  @details Generate NFC-A CRC according to NFC Forum Digital 2.0
 *           6.4.1.3.
 *
 *  @param[in] data Payload data.
 *  @param[in] len Data length.
 *  @param[out] crc_val NFC-A CRC value.
 *
 *  @retval 0 If the operation was successful.
 *            Otherwise, a (negative) error code is returned.
 */
int st25r3911b_nfca_crc_calculate(const u8_t *data, size_t len,
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
