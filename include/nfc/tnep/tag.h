/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NFC_TNEP_H__
#define NFC_TNEP_H__

/**
 * @file
 *
 * @defgroup nfc_tnep
 * @{
 * @ingroup nfc_api
 *
 * @brief Tag NDEF Exchange Protocol (TNEP) API.
 *
 */

#include <stddef.h>
#include <zephyr.h>
#include <nfc/ndef/nfc_ndef_msg.h>
#include <nfc/ndef/tnep_rec.h>

/**
 * TNEP Version.
 * A major version number in high nibble, a minor version number in low nibble.
 */
#define NFC_TNEP_VRESION 0x10

#define NFC_TNEP_EVENTS_NUMBER 3
#define NFC_TNEP_MSG_ADD_REC_TIMEOUT 100
#define NFC_TNEP_MSG_MAX_RECORDS 16
#define NFC_TNEP_RECORD_MAX_SZIE 64
#define NFC_TNEP_MSG_MAX_SIZE (NFC_TNEP_MSG_MAX_RECORDS\
			      * NFC_TNEP_RECORD_MAX_SZIE)

/** @brief Service communication modes. */
enum nfc_tnep_comm_mode {
	/** Single response communication mode */
	NFC_TNEP_COMM_MODE_SINGLE_RESPONSE,

	/** Service specific communication mode */
	NFC_TNEP_COMM_MODE_SERVICE_SPECYFIC = 0xFE
};

/** @brief Service status - payload in TNEP Status Record. */
enum nfc_tnep_status_value {
	/** Success */
	NFC_TNEP_STATUS_SUCCESS,

	/** TNEP protocol error */
	NFC_TNEP_STATUS_PROTOCOL_ERROR,

	/** First service error code. */
	NFC_TNEP_STATUS_SERVICE_ERROR_BEGIN = 0x80,

	/** Last service error code. */
	NFC_TNEP_STATUS_SERVICE_ERROR_END = 0xFE,
};

/**
 * @brief macro to define TNEP service.
 *
 * @details TNEP Service instance contains information about Services Parameter
 * and callback s for NDEF application. The Service Parameter contains
 * the Service Name URI of the Service and the TNEP parameters used to
 * communicate with the Service. The Reader/Writer will configure
 * the TNEP communication according to the announced parameter
 * in the Service Parameter record.
 *
 * @param name Service instance name used in code.
 * Use @ref NFC_TNEP_SERVICE to get service by name.
 * @param uri Service Name URI.
 * @param uri_length Service Name URI length in bytes.
 * @param mode Service mode - TNEP Communication Mode: Single Response
 * or Service specific
 * @param t_wait Minimum Waiting Time measured between the end of the last
 * write command of an NDEF Write Procedure and the start of the first command
 * of the first NDEF Read Procedure following the NDEF Write Procedures.
 * T_wait has a value between 0 and 63 and is converted to time units using
 * protocol specified formula.
 * @param n_wait Maximum Number of Waiting Time Extensions is the maximum
 * number of requested waiting time extensions n_wait for a specific Service.
 * N_wait can vary between 0 and 15 repetitions.
 * @param select_cb Callback function, called by protocol library when
 * service is selected by the Reader/Writer.
 * @param deselect_cb Callback function, called by protocol library when
 * service is deselected by the Reader/Writer.
 * @param message_cb Callback function, called by protocol library when
 * new message is received from the Reader/Writer.
 * @param timeout_cb Callback function, called by protocol library when
 * the Maximum Number of Waiting Time Extensions occurred.
 * @param error_cb Callback function, called by protocol library when
 * an internal error occurred.
 */
#define NFC_TNEP_SERVICE_DEF(name, uri, uri_length,			\
			mode, t_wait, n_wait,				\
			select_cb,					\
			deselect_cb,					\
			message_cb,					\
			timeout_cb,					\
			error_cb)					\
	NFC_TNEP_SERIVCE_PARAM_RECORD_DESC_DEF(name,			\
					NFC_TNEP_VRESION,		\
					uri_length,			\
					uri,				\
					mode,				\
					t_wait,				\
					n_wait,				\
					NFC_TNEP_MSG_MAX_SIZE);		\
	const struct nfc_tnep_service_cb name##_cb = {			\
		.selected = select_cb,					\
		.deselected = deselect_cb,				\
		.message_received = message_cb,				\
		.rx_timeout = timeout_cb,				\
		.error_detected = error_cb,				\
	}

#define NFC_TNEP_SERVICE(name)				\
	{						\
		&name,					\
		&NFC_NDEF_TNEP_RECORD_DESC(name),	\
		&name##_cb,				\
	}

struct nfc_tnep_service_cb {
	/**
	 * Function called when service was selected.
	 * This function should return service select error status.
	 *
	 * @retval 0 When Service was selected successfully.
	 * @retval 0x80 – 0xFE When service specific error occurred.
	 */
	u8_t (*selected)(void);

	/** Function called when service deselected. */
	void (*deselected)(void);

	/** Function called when new message was received. */
	void (*message_received)(void);

	/** Function called when N_wait timeout occurred. */
	void (*rx_timeout)(void);

	/**
	 * Function called when an internal error in TNEP detected.
	 *
	 * @param err Detected error code.
	 */
	void (*error_detected)(int err);
};

/**
 * @brief  Service structure.
 *
 * @details This structure contains all information about user service.
 * It contains service parameters, service parameters record
 * and services callbacks.
 * It is used by @ref tnep_init function.
 */
struct nfc_tnep_service {
	const struct nfc_ndef_tnep_svc_param *parameters;
	struct nfc_ndef_record_desc *ndef_record;
	const struct nfc_tnep_service_cb *callbacks;
};

/**
 * @brief  Register TNEP message buffer.
 *
 * @param tx_buffer Pointer to NDEF message buffer.
 * @param len Length of NDEF message buffer.
 *
 * @retval -EINVAL When buffer not exist.
 * @retval 0 When success.
 */
int nfc_tnep_tx_msg_buffer_register(u8_t *tx_buffer, size_t len);

/**
 * @brief Start communication using TNEP.
 *
 * @param services Pointer to the first service information structure.
 * @param services_amount Number of services in application.
 *
 * @retval 0 Success.
 * @retval -ENOTSUP TNEP already started.
 * @retval -EINVAL Invalid argument.
 * @retval -EIO No output NDEF message buffer registered.
 */
int nfc_tnep_init(struct nfc_tnep_service *services, size_t services_amount);

/**
 * @brief Stop TNEP communication
 */
void nfc_tnep_uninit(void);

/**
 * @brief Waiting for a signals to execute protocol logic.
 *
 * @retval error code. 0 if success.
 */
int nfc_tnep_process(void);

/**
 * @brief  Indicate about new TNEP message available in buffer.
 *
 * @details The NFC Tag Device concludes that the NDEF Write Procedure
 * is finished when, after a write command of the NDEF Write Procedure,
 * an NDEF message is available in the data area. If, after the write command
 * of the NDEF Write Procedure, no NDEF message is available in the data area,
 * the NFC Tag Device concludes that the actual NDEF Write Procedure
 * is ongoing.
 *
 * @param rx_buffer Pointer to NDEF message buffer.
 * @param len Length of NDEF message buffer.
 */
void nfc_tnep_rx_msg_indicate(u8_t *rx_buffer, size_t len);

/**
 * @brief Add application data record to next message.
 *
 * @param record Pointer to application data record.
 * @param is_last_app_record Set true if this is there are no more application
 * data to transmit until next NDEF Write event occur.
 *
 * @retval error code. 0 if success.
 */
int nfc_tnep_tx_msg_app_data(struct nfc_ndef_record_desc *record);

/**
 * If the NDEF application on the NFC Tag Device has finished,
 * and therefore the NFC Tag Device has no more application data
 * available for the Reader/Writer, then the NFC Tag Device SHALL
 * provide a Status message containing a single record that is
 * a TNEP Status record indicating success.
 */
void nfc_tnep_tx_msg_no_app_data(void);

/**
 * @}
 */

#endif /** NFC_TNEP_H__ */

