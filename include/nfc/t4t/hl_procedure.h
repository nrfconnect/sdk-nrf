/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_T4T_HL_PROCEDURE_
#define NFC_T4T_HL_PROCEDURE_

/**
 * @file
 * @defgroup nfc_t4t_hl_procedure NFC T4T High Level Procedure
 * @{
 * @brief NFC T4T High Level Procedure API
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>
#include <nfc/t4t/cc_file.h>

/**@brief Type of selection procedure. */
enum nfc_t4t_hl_procedure_select {
	/** NDEF App select. */
	NFC_T4T_HL_PROCEDURE_NDEF_APP_SELECT,

	/** CC file select. */
	NFC_T4T_HL_PROCEDURE_CC_SELECT,

	/** NDEF file select. */
	NFC_T4T_HL_PROCEDURE_NDEF_FILE_SELECT
};

/**@brief NFC T4T High Level Procedure callback structure.
 *
 * This structure is used to control command exchange with
 * NFC Type 4 Tag.
 */
struct nfc_t4t_hl_procedure_cb {
	/**@brief HL Procedure selected callback.
	 *
	 * The selection of applications or files is completed.
	 *
	 * @param[in] type Type of selection command.
	 */
	void (*selected)(enum nfc_t4t_hl_procedure_select type);

	/**@brief HL Procedure CC file read callback.
	 *
	 * The CC file of Type 4 Tag read operation is
	 * completed successfully.
	 *
	 * @param[in] cc Pointer to CC file data.
	 */
	void (*cc_read)(struct nfc_t4t_cc_file *cc);

	/**@brief HL Procedure NDEF file read callback.
	 *
	 * The NDEF file of Type 4 Tag read operation is
	 * completed successfully.
	 *
	 * @param[in] file_id File Identifier
	 * @param[in] data Pointer to received NDEF file data. The data
	 *                 buffer is assigned by @ref nfc_t4t_hl_procedure_ndef_read
	 *                 function.
	 * @param[in] len Received data length.
	 */
	void (*ndef_read)(uint16_t file_id, const uint8_t *data, size_t len);

	/**@brief HL Procedure NDEF file updated callback.
	 *
	 * The NDEF file of Type 4 Tag update  operation is
	 * completed successfully.
	 *
	 * @param[in] file id File Identifier.
	 */
	void (*ndef_updated)(uint16_t file_id);
};

/**@brief Handle High Level Procedure received data.
 *
 * Function for handling the received data. It should be called
 * immediately upon receiving the ISO-DEP data.
 *
 * @param[in] data Pointer to received data.
 * @param[in] len Received data length.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_hl_procedure_on_data_received(const uint8_t *data, size_t len);

/**@brief Register High Level Procedure callback.
 *
 * Function for register callback. It should be used
 * to provides synchronization between NFC T4T operation.
 *
 * @param[in] cb Pointer to callback structure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_hl_procedure_cb_register(const struct nfc_t4t_hl_procedure_cb *cb);

/**@brief Perform NDEF Tag Application Select Procedure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_hl_procedure_ndef_tag_app_select(void);

/**@brief Perform NDEF Tag Capability Container Select Procedure.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_hl_procedure_cc_select(void);

/**@brief Perform Capability Container Read Procedure.
 *
 * @param[out] cc Pointer to the Capability Container descriptor.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_hl_procedure_cc_read(struct nfc_t4t_cc_file *cc);

/**@brief Perform NDEF file Select Procedure.
 *
 * @param[in] id File Identifier
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_hl_procedure_ndef_file_select(uint16_t id);

/**@brief Perform NDEF Read Procedure.
 *
 * @param[in,out] cc Pointer to Capability Containers descriptor.
 * @param[out] ndef_buff Pointer to buffer where the NDEF file will be stored.
 *                       The NDEF Read procedure is an asynchronous operation,
 *                       the data buffer have to be kept until this procedure
 *                       has completed.
 * @param[in] ndef_len Length of NDEF file buffer.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_hl_procedure_ndef_read(struct nfc_t4t_cc_file *cc,
				   uint8_t *ndef_buff, uint16_t ndef_len);

/**@brief Perform NDEF Update Procedure.
 *
 * @param[in] cc Pointer to Capability Containers descriptor.
 * @param[in] ndef_data Pointer to the NDEF file data. The NDEF Read
 *                      procedure is an asynchronous operation, the data
 *                      buffer have to be kept until this procedure
 *                      has completed.
 * @param[in] ndef_len Length of NDEF file.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int nfc_t4t_hl_procedure_ndef_update(struct nfc_t4t_cc_file *cc,
				     uint8_t *ndef_data, uint16_t ndef_len);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* NFC_T4T_HL_PROCEDURE_ */
