/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _UNICAST_CLIENT_INTERNAL_H_
#define _UNICAST_CLIENT_INTERNAL_H_

#include <stdint.h>
#include <zephyr/bluetooth/audio/cap.h>

enum action_req {
	ACTION_REQ_NONE = 0,
	STREAM_ACTION_QOS_RECONFIG,
	GROUP_ACTION_REQ_RESTART,
};

struct pd_struct {
	uint32_t pd_min;
	uint32_t pref_pd_min;
	uint32_t pref_pd_max;
	uint32_t pd_max;
};

/**
 * @brief	Get the common minimum presentation delay for a unicast group if possible.
 *
 * @note	This function should be called in the codec_configured callback,
 *		after all streams have reached this state or further in the ASE state machine.
 *
 * @note	The pres. delay is computed based on the preference of each steam in the group.
 *		The common presentation delay is the one that satisfies all streams in the group,
 *		but also takes into account the preference of each stream.
 *		Note that the presentation delay must be the same for all streams
 *		within a unicast group for a given direction (sink or source).
 *
 *
 * @param[in]	unicast_group		Pointer to the unicast group.
 * @param[out]	pres_dly_snk_us		Pointer to store the sink presentation delay in
 *					microseconds.
 * @param[out]	pres_dly_src_us		Pointer to store the source presentation delay in
 *					microseconds.
 * @param[out]	common_pd_snk		Pointer to store the common presentation delay structure for
 *					sink.
 * @param[out]	common_pd_src		Pointer to store the common presentation delay structure for
 *					source.
 *
 * @retval	0		Success.
 * @retval	Negative error code on failure.
 */
int unicast_client_internal_pres_dly_get(struct bt_cap_unicast_group *unicast_group,
					 uint32_t *pres_dly_snk_us, uint32_t *pres_dly_src_us,
					 struct pd_struct *common_pd_snk,
					 struct pd_struct *common_pd_src);
/**
 * @brief	Set the presentation delay for a unicast group.
 *
 * @note	If the existing presentation delay is not the same as is being set here,
 *		we do not need to reconfigure the CIG as PD is not part of the CIG parameters.
 *		However, if the presentation delay is not the same as what is currently set, we will
 *		need to configure the QoS again to update the PD values of all streams.
 *
 * @note	This call should be called in the codec_configured callback, after all streams
 *		have reached this state or further in the ASE state machine.
 *
 * @param[in]	unicast_group		Pointer to the unicast group.
 * @param[in]	pres_dly_snk_us		Sink presentation delay in microseconds.
 * @param[in]	pres_dly_src_us		Source presentation delay in microseconds.
 * @param[out]	group_action_required	Pointer to store the action required for the group.
 *
 * @retval	0		Success.
 * @retval	Negative error code on failure.
 */
int unicast_client_internal_pres_dly_set(struct bt_cap_unicast_group *unicast_group,
					 uint32_t pres_dly_snk_us, uint32_t pres_dly_src_us,
					 enum action_req *group_action_required);

/**
 * @brief	Get the lowest maximum transport latency (MTL) for a unicast group.
 *
 * @note	This call should be called in the codec_configured callback,
 *		after all streams have reached this state or further in the ASE state machine.
 *		All streams of a given direction within the CIG must have the same
 *		transport latency. See BAP spec (BAP_v1.0.2 section 7.2.1)
 *
 * @param[in]	unicast_group			Pointer to the unicast group.
 * @param[out]	new_max_trans_lat_snk_ms	Pointer to store the maximum transport latency for
 *						sink in milliseconds.
 * @param[out]	new_max_trans_lat_src_ms	Pointer to store the maximum transport latency for
 *						source in milliseconds.
 *
 * @retval	0		Success.
 * @retval	Negative error code on failure.
 */
int unicast_client_internal_max_transp_latency_get(struct bt_cap_unicast_group *unicast_group,
						   uint16_t *new_max_trans_lat_snk_ms,
						   uint16_t *new_max_trans_lat_src_ms);

/**
 * @brief	Set the transport latency for a unicast group.
 *
 * @note	The group action required is set based on if the existing CIG parameters does not
 *		match what is supplied here. Depending on the group action required,
 *		the application may need to reconfigure the unicast group.
 *		All streams of a given direction within the CIG must have the same
 *		transport latency. See BAP spec (BAP_v1.0.2 section 7.2.1).
 *
 * @note	This call should be called in the codec_configured callback, after all streams
 *		have reached this state or further in the ASE state machine.
 *
 * @param[in]	unicast_group			Pointer to the unicast group.
 * @param[in]	new_max_trans_lat_snk_ms	Maximum transport latency for sink in milliseconds.
 * @param[in]	new_max_trans_lat_src_ms	Maximum transport latency for source in
 *						milliseconds.
 * @param[out]	group_action_needed		Pointer to store the action needed for the group.
 *
 * @retval	0		Success.
 * @retval	Negative error code on failure.
 */
int unicast_client_internal_max_transp_latency_set(struct bt_cap_unicast_group *unicast_group,
						   uint16_t new_max_trans_lat_snk_ms,
						   uint16_t new_max_trans_lat_src_ms,
						   enum action_req *group_action_needed);

/**
 * @brief	Set the group action.
 *
 * @note	This is a helper function to set the group action, and ensure that a lower
 *		priority action cannot overwrite a higher priority action.
 *
 * @param[in,out]	action		Pointer to the current action to be updated.
 * @param[in]		new_action	New action to set.
 */
void group_action_set(enum action_req *action, enum action_req new_action);

#endif /* _UNICAST_CLIENT_INTERNAL_H_ */