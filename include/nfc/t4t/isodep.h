/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NFC_T4T_ISODEP_H_
#define NFC_T4T_ISODEP_H_

/**
 * @file
 * @defgroup nfc_t4t_isodep NFC Type 4 Tag ISO-DEP API
 * @{
 * @brief NFC Type 4 Tag isodep.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup isodep_error ISO-DEP Protocol error.
 * @{
 */
/** Correct frame was received with invalid content. */
#define NFC_T4T_ISODEP_SYNTAX_ERROR 1

/** Correct frame was received when it is not expected. */
#define NFC_T4T_ISODEP_SEMATIC_ERROR 2

/** Unrecoverable transmission error. */
#define NFC_T4T_ISODEP_TRANSMISSION_ERROR 3

/** Unrecoverable timeout error. */
#define NFC_T4T_ISODEP_TIMEOUT_ERROR 4

/**
 * @}
 */

/** Maximum historical bytes count in RATS Response. */
#define NFC_T4T_ISODEP_HIST_MAX_LEN 15

/**@brief NFC Type 4 TAG data negotiated over
 *        RATS command exchange.
 */
struct nfc_t4t_isodep_tag {
	/** Frame Waiting Time. */
	u32_t fwt;

	/** Start-up Frame Guard Time */
	u32_t sfgt;

	/** Frame size for proximity Card. */
	u16_t fsc;

	/** Logical number of the addressed Listener.*/
	u8_t did;

	/** Listener-Poller bit rate divisor. */
	u8_t lp_divisor;

	/** Poller-Listener bit rate divisor. */
	u8_t pl_divisor;

	/** Historical bytes. */
	u8_t historical[NFC_T4T_ISODEP_HIST_MAX_LEN];

	/** Historical bytes length. */
	u8_t historical_len;

	/** DID supported. */
	bool did_supported;

	/** NAD supported. */
	bool nad_supported;
};

/**@brief NFC Type 4 Tag ISO-DEP frame size.
 */
enum nfc_t4t_isodep_fsd {
	/** 16-byte frame size. */
	NFC_T4T_ISODEP_FSD_16 = 0,

	/** 24 - byte frame size. */
	NFC_T4T_ISODEP_FSD_24,

	/** 32 - byte frame size. */
	NFC_T4T_ISODEP_FSD_32,

	/** 40 - byte frame size. */
	NFC_T4T_ISODEP_FSD_40,

	/** 48 - byte frame size. */
	NFC_T4T_ISODEP_FSD_48,

	/** 64 - byte frame size. */
	NFC_T4T_ISODEP_FSD_64,

	/** 96 - byte frame size. */
	NFC_T4T_ISODEP_FSD_96,

	/** 128 - byte frame size. */
	NFC_T4T_ISODEP_FSD_128,

	/** 256 - byte frame size. */
	NFC_T4T_ISODEP_FSD_256
};

/**@brief ISO-DEP Protocol callback structure.
 *
 * This structure is using to control data exchange over
 * ISO-DEP Protocol.
 */
struct nfc_t4t_isodep_cb {
	/**@brief ISO-DEP data received callback.
	 *
	 * The data exchange over ISO-DEP Protocol is completed
	 * successfully.
	 *
	 * @param[in] data Pointer to received data.
	 * @param[in] data_len Received data length.
	 */
	void (*data_received)(const u8_t *data, size_t data_len);

	/**@brief Type 4 Tag ISO-DEP selected callback.
	 *
	 * The valid ATS frame from tag was received.
	 */
	void (*selected)(const struct nfc_t4t_isodep_tag *t4t_tag);

	/**@brief Type 4 Tag ISO-DEP deselected callback.
	 *
	 * A valid Deselect Response from the tag was received.
	 */
	void (*deselected)(void);

	/**@brief ISO-DEP data ready to send a callback.
	 *
	 * After receiving this callback, ISO-DEP Protocol data
	 * should be send by Reader/Writer.
	 *
	 * @param[in] data Pointer to data to send by Reader/Writer.
	 * @param[in] data_len Data length.
	 * @param[in] ftd Maximum frame delay time.
	 */
	void (*ready_to_send)(u8_t *data, size_t data_len, u32_t ftd);

	/**@brief ISO-DEP error callback.
	 *
	 * ISO-DEP transfer has failed. Called when ISO-DEP
	 * Protocol error occurred.
	 */
	void (*error)(int err);
};

/**@brief Send Request for Answer to Select (RATS).
 *
 * This function sends a RATS command according to NFC Forum
 * Digital Specification 2.0 14.6.1. The RATS command is used
 * by Reader/Writer to negotiate maximum frame size and the
 * bit rate divisors with the Listener. This function have to
 * be called before @ref nfc_t4t_isodep_transmit.
 *
 * @param[in] fsd Frame size for the Reader/Writer.
 * @param[in] did Logical number of the addressed Listener.
 *                Usually it should be set to 0 in case of
 *                communication with one Listener.
 *
 * @note According to NFC Forum Digital Specification 2.0 fsd
 *       have to be set to 256 bytes.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_isodep_rats_send(enum nfc_t4t_isodep_fsd fsd, u8_t did);

/**@brief Send Deselect command.
 *
 * Function for sending S(DESELECT) frame according to NFC Forum
 * Digital Specification 2.0 16.2.7. Reader/Writer deactivates a
 * Listener by sending this command.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_isodep_tag_deselect(void);

/**@brief Handle NFC ISO-DEP Protocol received data.
 *
 * Function for handling received data. It should be called
 * immediately upon receipt data from Reader/Writer. In case
 * of transmission error, the error recovery procedure will
 * be started.
 *
 * @param[in] data Pointer to received data.
 * @param[in] data_len Received data length.
 * @param[in] err Transmission error.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_isodep_data_received(const u8_t *data, size_t data_len, int err);

/**@brief Exchange the specified amount of data.
 *
 * This function can be called when a Tag is in selected state after
 * call @ref nfc_t4t_isodep_rats_send.
 *
 * @param[in] data Pointer to data to transfer over ISO-DEP Protocol.
 * @param[in] data_len Length of data to transmit.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_isodep_transmit(const u8_t *data, size_t data_len);

/**@brief Handle transmission timeout error.
 *
 * This function have to be called when Reader/Writer
 * detect a timeout in Tag Response. When timeout occurred
 * then the timeout error recovery procedure will be started.
 *
 */
void nfc_t4t_isodep_on_timeout(void);

/**@brief Initialize NFC ISO-DEP Protocol .
 *
 * This function prepare buffer for ISO-DEP Protocol and
 * validate it size.
 *
 * @param[in,out] tx_buf Buffer for TX data. Data is set there before sending.
 * @param[in] tx_size Size of TX buffer.
 * @param[in, out] rx_buf Buffer fo RX data. Data is set there after
 *                        received valid data from NFC Tag.
 * @param[in] rx_size Size of RX buffer.
 * @param[in] cb Callback structure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 *
 */
int nfc_t4t_isodep_init(u8_t *tx_buf, size_t tx_size,
			u8_t *rx_buf, size_t rx_size,
			const struct nfc_t4t_isodep_cb *cb);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_T4T_ISODEP_H_ */
