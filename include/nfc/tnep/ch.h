/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_TNEP_CH_H_
#define NFC_TNEP_CH_H_

/**@file
 *
 * @defgroup nfc_tnep_ch NFC Connection Handover TNEP service
 * @{
 * @brief NFC Connection Handover TNEP service API
 */
#include <stdint.h>
#include <stddef.h>
#include <nfc/ndef/ch.h>
#include <nfc/tnep/tag.h>

#ifdef _cplusplus
extern "C" {
#endif

/** NFC Connection Handover service URI name length. */
#define NFC_TNEP_CH_URI_LENGTH 19

/**@brief NFC Connection Handover service URI. */
extern const uint8_t nfc_tnep_ch_svc_uri[NFC_TNEP_CH_URI_LENGTH];

/**@brief Connection Handover Record structure. */
struct nfc_tnep_ch_record {
	/** Pointer to the first Alternative Carrier Record. */
	const struct nfc_ndef_ch_ac_rec *ac;

	/** Pointer to the first Carrier Record. */
	const struct nfc_ndef_record_desc **carrier;

	/** Count of the Alternative Carriers Records and Carrier Records. */
	size_t count;

	/** Connection Handover major version. */
	uint8_t major_ver;

	/** Connection Handover minor version. */
	uint8_t minor_ver;
};

/**@brief Connection Handover Request Record structure. */
struct nfc_tnep_ch_request {
	/** Pointer to the Collision Resolution Record data. */
	const struct nfc_ndef_ch_cr_rec *cr;

	/** Connection Handover Record data. */
	struct nfc_tnep_ch_record ch_record;
};

/**@brief Connection Handover service callback structure. */
struct nfc_tnep_ch_cb {
	/**@brief The Connection Handover Request Message prepare callback.
	 *
	 * This callback is called every time when the NFC Forum Device
	 * takes the Connection Handover Requester role and the
	 * Connection Handover Message must be sent to the other
	 * NFC Forum Device.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a (negative) error code is returned.
	 */
	int (*request_msg_prepare)(void);

	/**@brief The Connection Handover Request Message received callback.
	 *
	 * This callback is called always when NFC Forum Device which
	 * supports TNEP received the Connection Handover Request Message.
	 * After receiving this message, application shall respond with
	 * the Handover Select Message by calling @ref nfc_tnep_ch_carrier_set.
	 *
	 * @param[in] ch_req Pointer to the Connection Handover Request
	 *                   structure which contains the parsed Connection
	 *                   Handover Request Message.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a (negative) error code is returned.
	 */
	int (*request_msg_recv)(const struct nfc_tnep_ch_request *ch_req);

	/**@brief The Connection Handover Select Message received callback.
	 *
	 * This callback is called always when the NFC Device receive
	 * the Connection Select Message in response to the Connection
	 * Handover Request message.
	 *
	 * @param[in] ch_select Pointer to Connection Handover structure
	 *                      which contains the parsed Connection Handover
	 *                      Select Message.
	 * @param[in] inactive Indicates that all Alternative Carriers are
	 *                     inactive. The Handover Request Message should
	 *                     be sent again with only one Alternative Carrier
	 *                     using @ref nfc_tnep_ch_carrier_set.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a (negative) error code is returned.
	 */
	int (*select_msg_recv)(const struct nfc_tnep_ch_record *ch_select,
			       bool inactive);

#if defined(CONFIG_NFC_TNEP_TAG)
	/**@brief The Connection Handover Mediation Request Message received
	 *        callback.
	 *
	 * This callback is called when an NFC Forum Tag Device that
	 * supports TNEP receives a Connection Handover Request Message
	 * that contains, in a Handover Carrier Record, the information for
	 * a single alternative carrier of NFC Forum Well Known type “Hm”.
	 * After receiving this message, the application shall respond with
	 * the Handover Mediation Message by calling
	 * @ref nfc_tnep_ch_carrier_set.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a (negative) error code is returned.
	 */
	int (*mediation_req_recv)(void);

	/**@brief The Connection Handover Initial Message received callback.
	 *
	 * This callback is called always when NFC Forum Tag Device which
	 * supports TNEP receives the Connection Handover Initial Message.
	 * After receiving this message, application shall respond also with
	 * the Handover Initial Message by calling @ref nfc_tnep_ch_carrier_set.
	 *
	 * @param[in] ch_initial Pointer to Connection Handover structure
	 *                       which contains the parsed Connection Handover
	 *                       Initial Message.
	 *
	 * @retval 0 If the operation was successful.
	 *         Otherwise, a (negative) error code is returned.
	 */
	int (*initial_msg_recv)(const struct nfc_tnep_ch_record *ch_initial);
#endif /* defined(CONFIG_NFC_TNEP_TAG) */

	/**@brief Error callback.
	 *
	 * Called always when internal error was detected.
	 *
	 * @param[in] err Detected error code.
	 */
	void (*error)(int err);
};

/**@brief Initialize the TNEP Connection Handover service.
 *
 * @param[in] cb Callback structure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_service_init(struct nfc_tnep_ch_cb *cb);

/**@brief Set Connection Handover carriers.
 *
 * Function for setting the Connection Handover carriers.
 * It should be used in the callback from @ref nfc_tnep_ch_cb when
 * the Application should reply to the Connection Handover Message.
 * The type of message is determined automatically based on the received
 * message and the role of the device in the data exchange.
 *
 * @param[in] ac_rec Array of the Alternative Carrier Records.
 * @param[in] carrier_rec Array of the Carrier Records.
 * @param[in] count Count of the Alternative Carrier Records and
 *                  the Carrier Records.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_carrier_set(struct nfc_ndef_record_desc *ac_rec,
			    struct nfc_ndef_record_desc *carrier_rec,
			    size_t count);

/**@brief Helper function for searching the Connection Handover service
 *        in the Initial NDEF Message.
 *
 * @param[in] services Array of the TNEP services.
 * @param[in] cnt Services count.
 *
 * @return Pointer to the Connection Handover service.
 *         NULL if service is not found.
 */
const struct nfc_ndef_tnep_rec_svc_param *nfc_tnep_ch_svc_search(
	const struct nfc_ndef_tnep_rec_svc_param *services,
	size_t cnt
);


#ifdef _cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_TNEP_CH_H_ */
