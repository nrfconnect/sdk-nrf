/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_TNEP_CH_COMMON_H_
#define NFC_TNEP_CH_COMMON_H_

#include <stdint.h>
#include <stddef.h>

#include <zephyr/net_buf.h>

#include <nfc/ndef/ch.h>
#include <nfc/tnep/ch.h>

#ifdef _cplusplus
extern "C" {
#endif


/* @brief Parse the Connection Handover Request Message.
 *
 * @param[in] msg Pointer to the Connection Request Message.
 * @param[out] ch_req Pointer to the Connection Request structure.
 * @param[in,out] buf Buffer for parsed data.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_request_msg_parse(const struct nfc_ndef_msg_desc *msg,
				  struct nfc_tnep_ch_request *ch_req,
				  struct net_buf_simple *buf);

/* @brief Parse the Connection Handover Select Message.
 *
 * @param[in] msg Pointer to the Connection Select Message.
 * @param[out] ch_select Pointer to the Connection Handover structure.
 * @param[in,out] buf Buffer for parsed data.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_select_msg_parse(const struct nfc_ndef_msg_desc *msg,
				 struct nfc_tnep_ch_record *ch_select,
				 struct net_buf_simple *buf);

/* @brief Parse the Connection Handover Initiate Message.
 *
 * @param[in] msg Pointer to the Connection Initiate Message.
 * @param[out] ch_select Pointer to the Connection Handover structure.
 * @param[in,out] buf Buffer for parsed data.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_initiate_msg_parse(const struct nfc_ndef_msg_desc *msg,
				   struct nfc_tnep_ch_record *ch_init,
				   struct net_buf_simple *buf);

/* @brief Parse the Connection Handover Carrier Record.
 *
 * @param[in,out] hc_rec Connection Handover Carrier Record structure.
 * @param[in] rec Pointer to the Handover Carrier Record descriptor.
 * @param[in,out] buf Buffer for parsed data.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_hc_rec_parse(const struct nfc_ndef_ch_hc_rec **hc_rec,
			     const struct nfc_ndef_record_desc *rec,
			     struct net_buf_simple *buf);

/**@brief Prepare the Connection Handover Request Message.
 *
 * @param[in] records Pointer to the Connection Handover message structure.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_request_msg_prepare(const struct nfc_ndef_ch_msg_records *records);

/**@brief Prepare the Connection Handover Select Message.
 *
 * @param[in] records Pointer to the Connection Handover message structure.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_select_msg_prepare(const struct nfc_ndef_ch_msg_records *records);

/**@brief Prepare the Connection Handover Mediation Message.
 *
 * @param[in] records Pointer to the Connection Handover message structure.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_tnep_mediation_msg_prepare(const struct nfc_ndef_ch_msg_records *records);

/**@brief Prepare the Connection Handover Initiate Message.
 *
 * @param[in] records Pointer to the Connection Handover message structure.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_initiate_msg_prepare(const struct nfc_ndef_ch_msg_records *records);

/**@brief Write Connection Handover Message to the NFC Forum Device.
 *
 * This function must be defined specifically for NFC Tag Device
 * and for the NFC Poller Device.
 *
 * @param[in] Pointer to the Connection Handover Message to write.
 *
 * @retval 0 If the operation was successful.
 *         Otherwise, a (negative) error code is returned.
 */
int nfc_tnep_ch_msg_write(struct nfc_ndef_msg_desc *msg);

/**@brief Check if any Alternative Carrier is active.
 *
 * @param[in] ch_rec Pointer to the Connection Handover Record structure.
 *
 * @retval true If the any Alternative Carrier is active.
 *              Otherwise, a false is returned.
 */
bool nfc_tnep_ch_active_carrier_check(struct nfc_tnep_ch_record *ch_rec);

#ifdef _cplusplus
}
#endif

#endif /* NFC_TNEP_CH_COMMON_H_ */
