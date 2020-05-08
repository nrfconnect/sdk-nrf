/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NFC_TNEP_TAG_H_
#define NFC_TNEP_TAG_H_

/**
 * @file
 * @defgroup nfc_tnep_tag Tag NDEF Exchange Protocol (TNEP) API
 * @{
 * @brief TAG NDEF Exchange Protocol for the NFC Tag Device.
 */

#include <nfc/tnep/base.h>

/** NFC TNEP library event count. */
#define NFC_TNEP_EVENTS_NUMBER 2

/** Maximum Service Waiting Time. */
#define NFC_TNEP_TAG_MAX_WAIT_TIME 63

/** Maximum Waiting Time extension. */
#define NFC_TNEP_TAG_MAX_N_WAIT_TIME 15

struct nfc_tnep_tag_service_cb {
	/** @brief Function called when service was selected. */
	void (*selected)(void);

	/** @brief Function called when service deselected. */
	void (*deselected)(void);

	/**
	 *  @brief Function called when new message was received.
	 *
	 *  @param[in] data Pointer to received data.
	 *  @param[in] len Received data length.
	 */
	void (*message_received)(const u8_t *data, size_t len);

	/**
	 * @brief Function called when an internal error in TNEP detected.
	 *
	 * @param err Detected error code.
	 */
	void (*error_detected)(int err);
};

/**
 * @brief Service structure.
 *
 * This structure contains all information about user service.
 * It contains service parameters, service parameters record
 * and services callbacks. It is used by @ref tnep_init function.
 */
struct nfc_tnep_tag_service {
	/** Services parameters. */
	struct nfc_ndef_tnep_rec_svc_param *parameters;

	/** NDEF records data. */
	struct nfc_ndef_record_desc *ndef_record;

	/** Callbacks structure. */
	const struct nfc_tnep_tag_service_cb *callbacks;
};

/**
 * @brief Macro to define TNEP service.
 *
 * TNEP Service instance contains information about Services Parameter
 * and callback s for NDEF application. The Service Parameter contains
 * the Service Name URI of the Service and the TNEP parameters used to
 * communicate with the Service. The Reader/Writer will configure
 * the TNEP communication according to the announced parameter
 * in the Service Parameter record.
 *
 * @param[in] _name Service instance name used in code.
 *                  Use @ref NFC_TNEP_TAG_SERVICE to get service by name.
 * @param[in] _uri Service Name URI.
 * @param[in] _uri_length Service Name URI length in bytes.
 * @param[in] _mode Service mode - TNEP Communication Mode: Single Response
 *                  or Service specific
 * @param[in] _t_wait Minimum Waiting Time measured between the end of the last
 *                    write command of an NDEF Write Procedure and the start of
 *                    the first command of the first NDEF Read Procedure
 *                    following the NDEF Write Procedures. T_wait has a value
 *                    between 0 and 63 and is converted to time units using
 *                    protocol specified formula.
 * @param[in] _n_wait Maximum Number of Waiting Time Extensions is the maximum
 *                    number of requested waiting time extensions n_wait for a
 *                    specific Service. N_wait can vary between 0 and 15
 *                    repetitions.
 * @param[in] _max_msg_size Maximum NDEF message size in bytes.
 * @param[in] _select_cb Callback function, called by protocol library when
 *                       service is selected by the Reader/Writer.
 * @param[in] _deselect_cb Callback function, called by protocol library when
 *                         service is deselected by the Reader/Writer.
 * @param[in] _message_cb Callback function, called by protocol library when
 *                         new message is received from the Reader/Writer.
 * @parami[in] _error_cb Callback function, called by protocol library when
 *                       an internal error occurred.
 */
#define NFC_TNEP_TAG_SERVICE_DEF(_name, _uri, _uri_length,                  \
				 _mode, _t_wait, _n_wait,                   \
				 _max_msg_size,                             \
				 _select_cb,                                \
				 _deselect_cb,                              \
				 _message_cb,                               \
				 _error_cb)                                 \
									    \
	BUILD_ASSERT(_t_wait <= NFC_TNEP_TAG_MAX_WAIT_TIME,                                  \
			 "The Waiting time has to be equal or smaller than 63");                 \
	BUILD_ASSERT(_n_wait <= NFC_TNEP_TAG_MAX_WAIT_TIME,                                  \
			 "The Waiting time extension count has to be equal or smaller then 15"); \
												 \
	NFC_TNEP_SERIVCE_PARAM_RECORD_DESC_DEF(_name, NFC_TNEP_VERSION,	\
					       _uri_length, _uri,       \
					       _mode, _t_wait, _n_wait, \
					       _max_msg_size);          \
	static const struct nfc_tnep_tag_service_cb _name##_cb = {      \
		.selected = _select_cb,                                 \
		.deselected = _deselect_cb,                             \
		.message_received = _message_cb,                        \
		.error_detected = _error_cb,                            \
	}

/**
 * @brief macro for accessing the TNEP Service.
 */
#define NFC_TNEP_TAG_SERVICE(_name)                \
	{                                          \
		&_name,                            \
		&NFC_NDEF_TNEP_RECORD_DESC(_name), \
		&_name##_cb,                       \
	}


typedef int (*nfc_payload_set_t)(u8_t *, size_t);

/**
 * @brief Register TNEP message buffer.
 *
 * TNEP Tag Device needs two buffers one for current NDEF message
 * and seconds for new message.
 *
 * @param[in] tx_buff Pointer to NDEF message buffer.
 * @param[in] tx_swap_buff Pointer to swap NDEF message buffer.
 * @param[in] len Length of NDEF message buffers.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_tag_tx_msg_buffer_register(u8_t *tx_buff,
					u8_t *tx_swap_buff,
					size_t len);
/**
 * @brief Start communication using TNEP.
 *
 * @param[out] events TNEP Tag Events.
 * @param[in] cnt Event count. This library needs 2 events.
 * @param[in,out] Pointer to NDEF message structure. It is used to
 *                create TNEP NDEF message.
 * @param[in] payload_set Function for setting NDEF data for NFC TNEP
 *                        Tag Device. This library use it internally
 *                        to set raw NDEF message to the Tag NDEF file.
 *                        This function is called from atomic context, so
 *                        sleeping or anyhow delaying is not allowed there.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_tag_init(struct k_poll_event *events, u8_t event_cnt,
		      struct nfc_ndef_msg_desc *msg,
		      nfc_payload_set_t payload_set);

/**
 * @brief Create the initial TNEP NDEF message.
 *
 * This function create the Initial TNEP message. Initial NDEF message
 * has to contain at least one service parameters record. It can
 * contain also optional NDEF Records which can be used by NFC Poller
 * Device which does not support TNEP Protocol.
 *
 * @param[in] svc Pointer to the first service information structure.
 * @param[in] svc_cnt Number of provided services for application.
 * @param[in] records Pointer to the first no TNEP NDEF Records structure.
 * @param[in] records_cnt Number of privided NDEF Records.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_tag_initial_msg_create(const struct nfc_tnep_tag_service *svc,
				    size_t svc_cnt,
				    const struct nfc_ndef_record_desc *records,
				    size_t records_cnt);

/**
 * @brief Waiting for a signals to execute protocol logic.
 *
 * This function must be called periodically from
 * thread to process the TNEP Tag Device.
 *
 * @note This function cannot be called before @ref nfc_tnep_init.
 */
void nfc_tnep_tag_process(void);

/**
 * @brief Indicate about new TNEP message available in buffer.
 *
 * The NFC Tag Device concludes that the NDEF Write Procedure
 * is finished when, after a write command of the NDEF Write
 * Procedure, an NDEF message is available in the data area.
 * If, after the write command of the NDEF Write Procedure, no
 * NDEF message is available in the data area, the NFC Tag Device
 * concludes that the actual NDEF Write Procedure is ongoing.
 *
 * @param[in] rx_buffer Pointer to NDEF message buffer.
 * @param[in] len Length of NDEF message buffer.
 */
void nfc_tnep_tag_rx_msg_indicate(const u8_t *rx_buffer, size_t len);

/**
 * @brief Add application data record to next message.
 *
 * Use this function to set application data after the service
 * selection to set service application data and use it also
 * during service data exchange with the NFC Polling Device.
 * To indicate that application has no more data use
 * @nfc_tnep_tag_tx_msg_no_app_data.
 *
 * @param[in] record Pointer to application data records.
 * @param[in] cnt Records count.
 * @param[in] status TNEP App data message status.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_tag_tx_msg_app_data(const struct nfc_ndef_record_desc *record,
				 size_t cnt, enum nfc_tnep_status_value status);

/**
 * @brief Respond with no more application data.
 *
 * If the NDEF application on the NFC Tag Device has finished, and
 * therefore the NFC Tag Device has no more application data
 * available for the Reader/Writer, then the NFC Tag Device SHALL
 * provide a Status message containing a single record that is a
 * TNEP Status record indicating success.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_tag_tx_msg_no_app_data(void);

/**
 * @brief Handle NFC Tag selected event.
 *
 * If data exchange with poller starts again, NFC TNEP Tag device
 * shall provide TNEP Initial message.
 */
void nfc_tnep_tag_on_selected(void);

/**
 * @}
 */

#endif /** NFC_TNEP_TAG_H_ */
