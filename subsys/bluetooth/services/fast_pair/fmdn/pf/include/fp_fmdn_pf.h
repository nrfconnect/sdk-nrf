/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _FP_FMDN_PF_H_
#define _FP_FMDN_PF_H_

#include <zephyr/bluetooth/conn.h>
#include <zephyr/net_buf.h>
#include <zephyr/types.h>

#include <bluetooth/services/fast_pair/fhn/pf/pf.h>

#include "fp_common.h"

/**
 * @defgroup fp_fmdn_pf Fast Pair FMDN Precision Finding
 * @brief Internal API for Fast Pair FMDN Precision Finding that manages the
 *        out-of-band (OOB) message exchange according to the Android Ranging
 *        specification: https://source.android.com/docs/core/connect/ranging-oob-spec.
 *
 * @{
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Maximum length of the Ranging Capability Response. */
#define FP_FMDN_PF_RANGING_CAPABILITY_RSP_MAX_LEN (47U)

/** Length of the Ranging Configuration Response. */
#define FP_FMDN_PF_RANGING_CONFIG_RSP_LEN (4U)

/** Length of the Stop Ranging Response. */
#define FP_FMDN_PF_STOP_RANGING_RSP_LEN (4U)

/** Handle the incoming Ranging Capability Request message from the Initiator. The message
 *  format follows the Android ranging specification for out-of-band (OOB) communication.
 *
 * @param[in] conn    Connection object representing the Initiator.
 * @param[in] req_buf Network buffer representing the Ranging Capability Request message.
 *
 * @return True if the message format is correct, False Otherwise.
 */
bool fp_fmdn_pf_ranging_capability_req_handle(struct bt_conn *conn,
					      struct net_buf_simple *req_buf);

/** Handle the incoming Ranging Configuration message from the Initiator. The message
 *  format follows the Android ranging specification for out-of-band (OOB) communication.
 *
 * @param[in] conn    Connection object representing the Initiator.
 * @param[in] req_buf Network buffer representing the Ranging Configuration message.
 *
 * @return True if the message format is correct, False Otherwise.
 */
bool fp_fmdn_pf_ranging_config_req_handle(struct bt_conn *conn,
					  struct net_buf_simple *req_buf);

/** Handle the incoming Stop Ranging message from the Initiator. The message format
 *  follows the Android ranging specification for out-of-band (OOB) communication.
 *
 * @param[in] conn    Connection object representing the Initiator.
 * @param[in] req_buf Network buffer representing the Stop Ranging message.
 *
 * @return True if the message format is correct, False Otherwise.
 */
bool fp_fmdn_pf_stop_ranging_req_handle(struct bt_conn *conn,
					struct net_buf_simple *req_buf);

/** Prepare the Ranging Capability Response message to the Initiator. The message
 *  format follows the Android ranging specification for out-of-band (OOB) communication.
 *
 *  The FP_FMDN_PF_RANGING_CAPABILITY_RSP_MAX_LEN constant can be used to define the
 *  length of the output buffer.
 *
 * @param[in]  conn                   Connection object representing the Initiator.
 * @param[in]  ranging_tech_bm        Supported ranging technologies bitmask.
 * @param[in]  capability_payloads    Array of ranging technology capability payloads.
 * @param[in]  capability_payload_num The number of entries in the array.
 * @param[out] rsp_buf                Network buffer for storing the Ranging Capability
 *                                    Response message.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_pf_ranging_capability_rsp_prepare(
	struct bt_conn *conn,
	uint16_t ranging_tech_bm,
	struct bt_fast_pair_fhn_pf_ranging_tech_payload *capability_payloads,
	uint8_t capability_payload_num,
	struct net_buf_simple *rsp_buf);

/** Prepare the Ranging Configuration Response message to the Initiator. The message format
 *  follows the Android ranging specification for out-of-band (OOB) communication.
 *
 *  The FP_FMDN_PF_RANGING_CONFIG_RSP_LEN constant can be used to define the length
 *  of the output buffer.
 *
 * @param[in]  conn            Connection object representing the Initiator.
 * @param[in]  ranging_tech_bm Bitmask of ranging technologies that successfully set
 *                             their configuration.
 * @param[out] rsp_buf         Network buffer for storing the Ranging Configuration
 *                             Response message.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_pf_ranging_config_rsp_prepare(struct bt_conn *conn,
					  uint16_t ranging_tech_bm,
					  struct net_buf_simple *rsp_buf);

/** Prepare the Stop Ranging Response message to the Initiator. The message format
 *  follows the Android ranging specification for out-of-band (OOB) communication.
 *
 *  The FP_FMDN_PF_STOP_RANGING_RSP_LEN constant can be used to define the length
 *  of the output buffer.
 *
 * @param[in]  conn            Connection object representing the Initiator.
 * @param[in]  ranging_tech_bm Bitmask of ranging technologies that stopped ranging
 *                             successfully.
 * @param[out] rsp_buf         Network buffer for storing the Ranging Configuration
 *                             Response message.
 *
 * @return 0 if the operation was successful. Otherwise, a (negative) error code is returned.
 */
int fp_fmdn_pf_stop_ranging_rsp_prepare(struct bt_conn *conn,
					uint16_t ranging_tech_bm,
					struct net_buf_simple *rsp_buf);

/** Indicate that the response to the Initiator has been sent successfully using
 *  the established communication channel. This API must be used together with
 *  the "*_rsp_prepare" APIs during the response message transfer. It is relevant
 *  to every type of response message:
 *  - Ranging Capability Response
 *  - Ranging Configuration Response
 *  - Stop Ranging Response
 *
 * @param[in]  conn            Connection object representing the Initiator.
 * @param[in]  ranging_tech_bm Bitmask of ranging technologies that has been indicated
 *                             in the response
 */
void fp_fmdn_pf_ranging_rsp_sent_notify(struct bt_conn *conn, uint16_t ranging_tech_bm);

/** Associate the Account Key with the communication channel of the Initiator.
 *
 * @param[in] conn        Connection object representing the Initiator.
 * @param[in] account_key Fast Pair Account Key associated with the connection object.
 */
void fp_fmdn_pf_account_key_set(struct bt_conn *conn, struct fp_account_key *account_key);

/** Get the Account Key associated with the communication channel of the Initiator.
 *
 * @param[in] conn Connection object representing the Initiator.
 *
 * @return Account Key associated with the Initiator or NULL if a key has not been
 *         associated yet or the communication channel with the Intiator has not been
 *         established yet.
 */
struct fp_account_key *fp_fmdn_pf_account_key_get(struct bt_conn *conn);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _FP_FMDN_PF_H_ */
