/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef NFC_TNEP_POLLER_
#define NFC_TNEP_POLLER_

/**@file
 *
 * @defgroup nfc_tnep_poller TNEP Protocol for NFC Poller
 * @{
 * @brief TNEP Protocol for NFC Poller API
 */

#ifdef _cplusplus
extern "C" {
#endif

#include <nfc/ndef/nfc_ndef_msg.h>
#include <nfc/ndef/tnep_rec.h>
#include <nfc/tnep/base.h>

/**@brief NFC TNEP Tag Type. */
enum nfc_tnep_tag_type {
	/** NFC unsupported Tag Type. */
	NFC_TNEP_TAG_TYPE_UNSUPPORTED,

	/** NFC Type 2 Tag. */
	NFC_TNEP_TAG_TYPE_T2T,

	/** NFC Type 4 Tag. */
	NFC_TNEP_TAG_TYPE_T4T
};

/**@brief TNEP Poller Message structure.
 *
 * This structure contains NDEF Message pointer and
 * TNEP status received from TNEP Status Message after
 * the Tag Device received valid Service Select Message.
 */
struct nfc_tnep_poller_msg {
	/** TNEP Status Type. */
	enum nfc_tnep_status_value status;

	/** TNEP NDEF Message contains additional NDEF Records
	 *  and NDEF Status Record.
	 */
	const struct nfc_ndef_msg_desc *msg;
};

/**@brief TNEP Poller API structure
 *
 *  This structure is used to provide an API for
 *  NFC NDEF Message operations like reading and
 *  writing. Depending on the connected tag type,
 *  appropriate API should be used.
 */
struct nfc_tnep_poller_ndef_api {
	/**@brief Function used to read the NDEF Message.
	 *
	 * It is called when TNEP Poller has to read NDEF data
	 * from NFC TNEP Tag Device.
	 *
	 * @param[out] ndef_buf Pointer to buffer where NDEF Message will be
	 *                       stored. This buffer has to be kept until the
	 *                       read operation is finished.
	 * @param[in] ndef_len Size of the NDEF Buffer
	 *
	 * @retval 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int (*ndef_read)(u8_t *ndef_buf, u16_t ndef_len);

	/**@brief Function used to write the NDEF Message.
	 *
	 * It is called when TNEP Poller attempts to update NDEF data
	 * on NFC TNEP Tag Device.
	 *
	 * @param[in] ndef_buf Buffer with NDEF Message to write. Buffer has to
	 *                      be kept until the write operation is finished.
	 * @param[in] ndef_len Length of NDEF Message to write.
	 *
	 * @retval 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int (*ndef_update)(const u8_t *ndef_buf, u16_t ndef_len);
};

/**@brief TNEP Poller callback structure. */
struct nfc_tnep_poller_cb {
	/**@brief Function called when TNEP Service was selected.
	 *
	 * @note Service should be deselected in case of received
	 *       timeout. The @ref nfc_tnep_poller_svc_deselect should
	 *       be used to it.
	 *
	 * @param[in] param Pointer to the selected service parameters.
	 * @param[in] msg Pointer to TNEP Poller Message structure contains
	 *                status and received additional NDEF data.
	 * @param[in] timeout Indicates that TNEP Tag Device does not prepare
	 *                    NDEF Message within time.
	 */
	void (*svc_selected)(const struct nfc_ndef_tnep_rec_svc_param *param,
			     const struct nfc_tnep_poller_msg *msg,
			     bool timeout);

	/**@brief Function called when current active TNEP Service
	 *        was deselected.
	 */
	void (*svc_deselected)(void);

	/**@brief Function called when TNEP Service was read.
	 *
	 * @note Service should be deselected in case of received
	 *       timeout. The @ref nfc_tnep_poller_svc_deselect should
	 *       be used to it.
	 *
	 * @param[in] param Pointer to the selected service parameters.
	 * @param[in] msg Pointer to TNEP Poller Message structure contains
	 *                status and received additional NDEF data.
	 * @param[in] timeout Indicates that TNEP Tag Device does not prepare
	 *                    NDEF Message within time.
	 */
	void (*svc_received)(const struct nfc_ndef_tnep_rec_svc_param *param,
			     const struct nfc_tnep_poller_msg *msg,
			     bool timeout);

	/**@brief Function called when TNEP Service was written.
	 *
	 * @note Service should be deselected in case of received
	 *       timeout. The @ref nfc_tnep_poller_svc_deselect should
	 *       be used to it.
	 *
	 * @param[in] param Pointer to the selected service parameters.
	 * @param[in] rsp_msg Pointer to TNEP Poller Message structure contains
	 *                    status and received response with additional
	 *                    NDEF data.
	 * @param[in] timeout Indicates that TNEP Tag Device does not prepare
	 *                    NDEF Message within time.
	 */
	void (*svc_sent)(const struct nfc_ndef_tnep_rec_svc_param *param,
			 const struct nfc_tnep_poller_msg *rsp_msg,
			 bool timeout);

	/**@brief Function called when an internal error in TNEP Poller
	 *        library was detected.
	 *
	 * @param[in] err Detected error code.
	 */
	void (*error)(int err);
};

/**@brief TNEP Poller buffer structure. */
struct nfc_tnep_buf {
	/** Pointer to data. */
	u8_t *data;

	/** Buffer size. */
	size_t size;
};

/**@brief Initialize the NFC TNEP Poller Device.
 *
 * @param[in] tx_buf Pointer to TNEP Poller Tx buffer.
 * @param[in] cb Pointer to TNEP Poller callback structure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_init(const struct nfc_tnep_buf *tx_buf,
			 const struct nfc_tnep_poller_cb *cb);

/**@brief Set NDEF API for the NFC TNEP Poller Device
 *
 * This function provides API for NDEF operation and indicates
 * the NFC Device Tag Type. API can be changed in runtime but
 * it must be set immediately after NFC Poller Device detects
 * Tag Type during the Device Activation Activity.
 *
 * @param[in] api Pointer to the NDEF API structure.
 * @param[in] tag_type NFC Device Tag Type.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_api_set(const struct nfc_tnep_poller_ndef_api *api,
			    enum nfc_tnep_tag_type tag_type);

/**@brief Search if NDEF Message contains TNEP Service Parameters Records.
 *
 * Function for searching the TNEP Service Parameters Records in
 * the given NDEF Message. NDEF Message has to be stored
 * until all operations on services are finished.
 *
 * @param[in] ndef_msg The NDEF Message which can be the Initial TNEP Message.
 * @param[out] param Pointer to structure where found service parameters will
 *                   be stored.
 * @param[in,out] cnt Count of service parameters which can be stored as an
 *                    input. Count of found services parameters as an output.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_svc_search(const struct nfc_ndef_msg_desc *ndef_msg,
			       struct nfc_ndef_tnep_rec_svc_param *param,
			       u8_t *cnt);

/**@brief Select the TNEP Service.
 *
 * Function for selecting the given service. After service is selected
 * NFC Poller waits a specific amount of time to perform the NDEF Read
 * procedure to get new data from the NFC Tag device. This operation
 * is asynchronous.
 *
 * @param[in] svc_buf Pointer to the buffer for new data from the NFC TNEP
 *                    Tag Device. Buffer must be stored until the Select
 *                    procedure is finished.
 * @param[in] svc Pointer to The TNEP Service to select.
 * @param[in] max_ndef_area_size Maximum size of NDEF Message.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_svc_select(const struct nfc_tnep_buf *svc_buf,
			       const struct nfc_ndef_tnep_rec_svc_param *svc,
			       u32_t max_ndef_area_size);

/**@brief Deselect the TNEP Service.
 *
 * Function for deselecting the TNEP service. Service can be deselected
 * also by selecting the other TNEP service without calling this function.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_svc_deselect(void);

/**@brief Read the TNEP Service data.
 *
 * Function for reading the TNEP Service data. The Service
 * must be selected first. This operation is asynchronous.
 *
 * @param[in,out] svc_buf Pointer to buffer for data to read. It
 *                        must be kept until this operation is finished.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_svc_read(const struct nfc_tnep_buf *svc_buf);

/**@brief Write the TNEP Service data.
 *
 * Function for writing the TNEP Service data. The Service
 * must be selected first. After successful writing, library attempts to read
 * new Service data after the specific amount of time.
 * This operation is asynchronous.
 *
 * @param[in] msg Pointer to the NDEF Message which will be written.
 * @param[in] resp_data Pointer to received data buffer. Buffer must be stored
 *                      until the update procedure is finished.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_svc_write(const struct nfc_ndef_msg_desc *msg,
			      const struct nfc_tnep_buf *resp_buf);

/**@brief Indicate NDEF data read.
 *
 * This function must be called immediately after NFC Reader/Writer
 * Device read new NDEF message from NFC TNEP Tag Device.
 *
 * @param[in] data NDEF Read raw data.
 * @param[in] Read data length.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_on_ndef_read(const u8_t *data, size_t len);

/**@brief Indicate NDEF data write operation finish.
 *
 * This function must be called immediately after NFC Reader/Writer
 * Device write NDEF message on NFC TNEP Tag Device.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_poller_on_ndef_write(void);

/**@brief Get the selected/active service.
 *
 * @retval Pointer to the selected/active service.
 *         If no slected/active service then NULL pointer is returned.
 */
const struct nfc_ndef_tnep_rec_svc_param *active_service_get(void);

#ifdef _cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_TNEP_POLLER_ */
