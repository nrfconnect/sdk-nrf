/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include "unicast_client_internal.h"

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/audio/cap.h>
#include <zephyr/bluetooth/audio/bap.h>
#include <zephyr/bluetooth/audio/audio.h>

/* TODO: Remove when more info is available in public APIs */
#include <../subsys/bluetooth/audio/cap_internal.h>
#include <../subsys/bluetooth/audio/bap_endpoint.h>

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include "le_audio.h"
#include "server_store.h"
#include "macros_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(unicast_client_internal, CONFIG_UNICAST_CLIENT_LOG_LEVEL);

/* Find the smallest commonly acceptable set of presentation delays */
static int pres_delay_compute(struct pd_struct *common, struct bt_bap_qos_cfg_pref const *const in)
{
	if (in->pd_min) {
		common->pd_min = MAX(in->pd_min, common->pd_min);
	} else {
		LOG_ERR("pd_min is zero");
		return -EINVAL;
	}

	if (in->pref_pd_min) {
		common->pref_pd_min = MAX(in->pref_pd_min, common->pref_pd_min);
	}

	if (in->pref_pd_max) {
		common->pref_pd_max = MIN(in->pref_pd_max, common->pref_pd_max);
	}

	if (in->pd_max) {
		common->pd_max = MIN(in->pd_max, common->pd_max);
	} else {
		LOG_ERR("No max presentation delay required");
		return -EINVAL;
	}

	return 0;
}

struct foreach_stream_pres_dly {
	int ret;
	struct bt_cap_unicast_group *unicast_group;
	struct pd_struct *common_pd_snk;
	struct pd_struct *common_pd_src;
	uint8_t streams_checked_snk;
	uint8_t streams_checked_src;
	uint32_t existing_pres_dly_us_snk;
	uint32_t existing_pres_dly_us_src;
};

static bool foreach_stream_pres_dly_calc(struct bt_cap_stream *stream, void *user_data)
{
	int ret;
	struct foreach_stream_pres_dly *ctx = (struct foreach_stream_pres_dly *)user_data;

	if (stream->bap_stream.ep == NULL) {
		LOG_ERR("Existing stream has no ep set yet.");
		ctx->ret = -EINVAL;
		return true;
	}

	if (!le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_CODEC_CONFIGURED) &&
	    !le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_QOS_CONFIGURED) &&
	    !le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_ENABLING) &&
	    !le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_STREAMING)) {
		LOG_DBG("Existing stream not in codec configured, QoS configured, enabling or "
			"streaming state");
		return true;
	}

	struct bt_bap_ep_info ep_info;

	ret = bt_bap_ep_get_info(stream->bap_stream.ep, &ep_info);
	if (ret != 0) {
		LOG_ERR("Failed to get existing stream ep info: %d", ret);
		return true;
	}

	if (ep_info.dir == BT_AUDIO_DIR_SINK) {
		/* Sink stream */
		ctx->streams_checked_snk++;
		if (ctx->existing_pres_dly_us_snk == UINT32_MAX) {
			ctx->existing_pres_dly_us_snk = stream->bap_stream.qos->pd;
		}

		if (ctx->existing_pres_dly_us_snk == 0) {
			LOG_ERR("Existing presentation delay is zero");
			ctx->ret = -EINVAL;
			return true;
		}

		ctx->ret = pres_delay_compute(ctx->common_pd_snk, &stream->bap_stream.ep->qos_pref);
		if (ctx->ret) {
			return true;
		}

	} else if (ep_info.dir == BT_AUDIO_DIR_SOURCE) {
		/* Source stream */
		ctx->streams_checked_src++;
		if (ctx->existing_pres_dly_us_src == UINT32_MAX) {
			ctx->existing_pres_dly_us_src = stream->bap_stream.qos->pd;
		}

		if (ctx->existing_pres_dly_us_src == 0) {
			LOG_ERR("Existing presentation delay is zero");
			ctx->ret = -EINVAL;
			return true;
		}

		ctx->ret = pres_delay_compute(ctx->common_pd_src, &stream->bap_stream.ep->qos_pref);
		if (ctx->ret) {
			return true;
		}
	} else {
		LOG_ERR("Unknown stream direction");
		ctx->ret = -EINVAL;
		return true;
	}

	/* Continue iteration */
	return false;
}

static int parse_common_qos_for_pres_dly(struct pd_struct *common_pd,
					 uint32_t *computed_pres_dly_us)
{

	if (common_pd->pd_min > common_pd->pd_max) {
		LOG_ERR("No common ground for pd_min %u and pd_max %u", common_pd->pd_min,
			common_pd->pd_max);
		*computed_pres_dly_us = UINT32_MAX;

		return -ESPIPE;
	}

	/* No streams have a preferred min */
	if (common_pd->pref_pd_min == 0) {
		*computed_pres_dly_us = common_pd->pd_min;

		return 0;
	}

	if (common_pd->pref_pd_min < common_pd->pd_min) {
		/* Preferred min is lower than min, use min */
		LOG_ERR("pref PD min is lower than min. Using min");
		*computed_pres_dly_us = common_pd->pd_min;
	} else if (common_pd->pref_pd_min > common_pd->pd_min &&
		   common_pd->pref_pd_min <= common_pd->pd_max) {
		/* Preferred min is in range, use pref min */
		*computed_pres_dly_us = common_pd->pref_pd_min;
	} else {
		*computed_pres_dly_us = common_pd->pd_min;
	}

	return 0;
}

static void pd_print(struct pd_struct const *const pref, uint32_t existing_pd_us,
		     uint32_t calculated_pd_us)
{
	char existing_pd_buf[20] = {0};
	char calculated_pd_buf[20] = {0};

	if (existing_pd_us != UINT32_MAX) {
		snprintf(existing_pd_buf, sizeof(existing_pd_buf), "%u", existing_pd_us);
	} else {
		strcpy(existing_pd_buf, "N/A");
	}

	if (calculated_pd_us != UINT32_MAX) {
		snprintf(calculated_pd_buf, sizeof(calculated_pd_buf), "%u", calculated_pd_us);
	} else {
		strcpy(calculated_pd_buf, "N/A");
	}

	LOG_INF("\tPD abs min:\t %u ", pref->pd_min);
	LOG_INF("\tPD pref min:\t %u ", pref->pref_pd_min);
	LOG_INF("\tPD pref max:\t %u ", pref->pref_pd_max);
	LOG_INF("\tPD abs max:\t %u ", pref->pd_max);
	LOG_INF("\tExisting PD:\t %s us, selected: %s us", existing_pd_buf, calculated_pd_buf);
}

int unicast_client_internal_pres_dly_get(struct bt_cap_unicast_group *unicast_group,
					 uint32_t *pres_dly_snk_us, uint32_t *pres_dly_src_us,
					 struct pd_struct *common_pd_snk,
					 struct pd_struct *common_pd_src)
{
	int ret;

	if (unicast_group == NULL || pres_dly_snk_us == NULL || pres_dly_src_us == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	common_pd_src->pd_min = 0;
	common_pd_src->pref_pd_min = 0;
	common_pd_src->pref_pd_max = UINT32_MAX;
	common_pd_src->pd_max = UINT32_MAX;

	common_pd_snk->pd_min = 0;
	common_pd_snk->pref_pd_min = 0;
	common_pd_snk->pref_pd_max = UINT32_MAX;
	common_pd_snk->pd_max = UINT32_MAX;

	struct foreach_stream_pres_dly foreach_data = {
		.ret = 0,
		.unicast_group = unicast_group,
		.common_pd_snk = common_pd_snk,
		.common_pd_src = common_pd_src,
		.streams_checked_snk = 0,
		.streams_checked_src = 0,
		.existing_pres_dly_us_snk = UINT32_MAX,
		.existing_pres_dly_us_src = UINT32_MAX,
	};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, foreach_stream_pres_dly_calc,
						  (void *)&foreach_data);
	if (ret) {
		LOG_ERR("Failed to iterate streams in group: %d", ret);
		return ret;
	}

	if (foreach_data.ret) {
		LOG_ERR("Failed to compute presentation delay");
		return foreach_data.ret;
	}

	/* All streams have been iterated over. Parse results */
	if (foreach_data.streams_checked_snk == 0) {
		/* No sink streams checked, we don't have a presentation delay computed */
		*pres_dly_snk_us = UINT32_MAX;
	} else {
		ret = parse_common_qos_for_pres_dly(common_pd_snk, pres_dly_snk_us);
		if (ret) {
			LOG_ERR("Failed to parse common sink QoS for pres delay: %d", ret);
			return ret;
		}
		LOG_INF("Sink PD: (%d streams checked)", foreach_data.streams_checked_snk);
		pd_print(common_pd_snk, foreach_data.existing_pres_dly_us_snk, *pres_dly_snk_us);
	}

	if (foreach_data.streams_checked_src == 0) {
		/* No source streams checked, we don't have a presentation delay computed */
		*pres_dly_src_us = UINT32_MAX;
	} else {
		ret = parse_common_qos_for_pres_dly(common_pd_src, pres_dly_src_us);
		if (ret) {
			LOG_ERR("Failed to parse common source QoS for pres delay: %d", ret);
			return ret;
		}
		LOG_INF("Source PD: (%d streams checked)", foreach_data.streams_checked_src);
		pd_print(common_pd_src, foreach_data.existing_pres_dly_us_src, *pres_dly_src_us);
	}

	return 0;
}

struct new_pres_delays {
	enum action_req action;
	uint32_t snk_us;
	uint32_t src_us;
};

/* Set the new action required. Only set if the new action is more severe */
void group_action_set(enum action_req *action, enum action_req new_action)
{
	if (*action < new_action) {
		*action = new_action;
	}
}

static void stream_state_check(struct bt_cap_stream *stream,
			       struct new_pres_delays *new_pres_delays)
{

	if (le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_IDLE) ||
	    le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_RELEASING)) {
		/* This should not be the case as all streams should be in Codec configured
		or higher state at this point.*/
		LOG_ERR("Stream in IDLE or RELEASING state");
		/* Setting for safety */
		group_action_set(&new_pres_delays->action, GROUP_ACTION_REQ_RESTART);
	}

	if (le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_QOS_CONFIGURED)) {
		/* Stream needs to be QoS configured again to update the presentation delay */
		group_action_set(&new_pres_delays->action, STREAM_ACTION_QOS_RECONFIG);
	} else if (le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_ENABLING) ||
		   le_audio_ep_state_check(stream->bap_stream.ep, BT_BAP_EP_STATE_STREAMING)) {
		/* Streams must be restarted (go throught he QoS step again to update PD) */
		group_action_set(&new_pres_delays->action, STREAM_ACTION_QOS_RECONFIG);
	} else {
		group_action_set(&new_pres_delays->action, ACTION_REQ_NONE);
	}
}

static bool new_pres_dly_us_set(struct bt_cap_stream *stream, void *user_data)
{
	struct new_pres_delays *new_pres_delays = (struct new_pres_delays *)user_data;

	if (stream->bap_stream.ep == NULL) {
		LOG_ERR("Stream has no endpoint assigned yet");
		return false;
	}

	if (new_pres_delays->snk_us != UINT32_MAX &&
	    stream->bap_stream.ep->dir == BT_AUDIO_DIR_SINK) {
		if (stream->bap_stream.qos->pd != new_pres_delays->snk_us) {
			stream->bap_stream.qos->pd = new_pres_delays->snk_us;
			stream_state_check(stream, new_pres_delays);
		}

	} else if (new_pres_delays->src_us != UINT32_MAX &&
		   stream->bap_stream.ep->dir == BT_AUDIO_DIR_SOURCE) {
		if (stream->bap_stream.qos->pd != new_pres_delays->src_us) {
			stream->bap_stream.qos->pd = new_pres_delays->src_us;
			stream_state_check(stream, new_pres_delays);
		}
	} else {
		/* No update needed for this stream */
		return false;
	}
	return false;
}

int unicast_client_internal_pres_dly_set(struct bt_cap_unicast_group *unicast_group,
					 uint32_t pres_dly_snk_us, uint32_t pres_dly_src_us,
					 enum action_req *group_action_required)
{
	int ret;
	struct new_pres_delays new_pres_delays = {
		.action = ACTION_REQ_NONE,
		.snk_us = pres_dly_snk_us,
		.src_us = pres_dly_src_us,
	};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, new_pres_dly_us_set,
						  &new_pres_delays);

	*group_action_required = new_pres_delays.action;
	return ret;
}

struct foreach_stream_mtl {
	int ret;
	struct bt_cap_unicast_group *unicast_group;
	uint8_t streams_checked_snk;
	uint8_t streams_checked_src;
	uint16_t max_trans_lat_snk_ms;
	uint16_t max_trans_lat_src_ms;
};

static bool stream_max_trans_lat_find(struct bt_cap_stream *stream, void *user_data)
{
	struct foreach_stream_mtl *ctx = (struct foreach_stream_mtl *)user_data;

	if (stream->bap_stream.ep == NULL) {
		LOG_ERR("Existing stream has no ep set yet");
		ctx->ret = -EINVAL;
		return true;
	}

	int dir = le_audio_stream_dir_get(&stream->bap_stream);

	if (dir < 0) {
		LOG_ERR("Failed to get stream direction");
		ctx->ret = -EINVAL;
		return true;
	}

	/* Need to find the smallest MTL in a given direction */
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		ctx->streams_checked_snk++;
		if (ctx->max_trans_lat_snk_ms > stream->bap_stream.ep->qos_pref.latency) {
			ctx->max_trans_lat_snk_ms = stream->bap_stream.ep->qos_pref.latency;
		}
		break;
	case BT_AUDIO_DIR_SOURCE:
		ctx->streams_checked_src++;
		if (ctx->max_trans_lat_src_ms > stream->bap_stream.ep->qos_pref.latency) {
			ctx->max_trans_lat_src_ms = stream->bap_stream.ep->qos_pref.latency;
		}
		break;
	default:
		LOG_ERR("Unknown stream direction");
		ctx->ret = -EINVAL;
		return true;
	}

	return false;
}

int unicast_client_internal_max_transp_latency_get(struct bt_cap_unicast_group *unicast_group,
						   uint16_t *max_trans_lat_snk_ms,
						   uint16_t *max_trans_lat_src_ms)
{
	int ret;

	if (unicast_group == NULL || max_trans_lat_snk_ms == NULL || max_trans_lat_src_ms == NULL) {
		LOG_ERR("NULL parameter!");
		return -EINVAL;
	}

	struct foreach_stream_mtl foreach_data = {
		.ret = 0,
		.unicast_group = unicast_group,
		.streams_checked_snk = 0,
		.streams_checked_src = 0,
		.max_trans_lat_snk_ms = UINT16_MAX,
		.max_trans_lat_src_ms = UINT16_MAX,
	};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, stream_max_trans_lat_find,
						  (void *)&foreach_data);
	if (ret != 0 && ret != -ECANCELED) {
		/* There shall already be at least one server added at this point */
		LOG_ERR("Failed to iterate streams in group: %d", ret);
		return ret;
	}

	if (foreach_data.ret) {
		LOG_ERR("Failed to compute max transport latency");
		return foreach_data.ret;
	}

	LOG_INF("Checked %d sink stream(s) and %d source stream(s) in the same group",
		foreach_data.streams_checked_snk, foreach_data.streams_checked_src);

	*max_trans_lat_snk_ms = foreach_data.max_trans_lat_snk_ms;
	*max_trans_lat_src_ms = foreach_data.max_trans_lat_src_ms;

	return 0;
}

struct foreach_trans_latency_set {
	uint16_t new_max_trans_lat_snk_ms;
	uint8_t streams_set_snk;
	uint16_t new_max_trans_lat_src_ms;
	uint8_t streams_set_src;
};

static bool foreach_stream_transp_latency_set(struct bt_cap_stream *existing_stream,
					      void *user_data)
{
	struct foreach_trans_latency_set *ctx = (struct foreach_trans_latency_set *)user_data;

	if (existing_stream->bap_stream.ep == NULL) {
		LOG_ERR("Existing stream has no ep set yet.");
		return true;
	}

	/* If the max transport latency must be changed, we need to tear down and re-establish the
	 * stream in order for the controller to get the new information.
	 */
	int dir = le_audio_stream_dir_get(&existing_stream->bap_stream);
	switch (dir) {
	case BT_AUDIO_DIR_SINK:
		if (ctx->new_max_trans_lat_snk_ms == UINT16_MAX) {
			return false;
		}

		if (existing_stream->bap_stream.qos->latency != ctx->new_max_trans_lat_snk_ms) {

			existing_stream->bap_stream.qos->latency = ctx->new_max_trans_lat_snk_ms;
		}

		ctx->streams_set_snk++;
		break;

	case BT_AUDIO_DIR_SOURCE:
		if (ctx->new_max_trans_lat_src_ms == UINT16_MAX) {
			return false;
		}

		if (existing_stream->bap_stream.qos->latency != ctx->new_max_trans_lat_src_ms) {
			existing_stream->bap_stream.qos->latency = ctx->new_max_trans_lat_src_ms;
		}

		ctx->streams_set_src++;
		break;
	default:
		LOG_ERR("Failed stream direction set");
		return true;
	};

	return false;
}

int unicast_client_internal_max_transp_latency_set(struct bt_cap_unicast_group *unicast_group,
						   uint16_t new_max_trans_lat_snk_ms,
						   uint16_t new_max_trans_lat_src_ms,
						   enum action_req *group_action_needed)
{
	if (unicast_group == NULL || group_action_needed == NULL) {
		LOG_ERR("NULL parameter");
		return -EINVAL;
	}

	int ret;

	struct foreach_trans_latency_set stream_trans_lat_set = {
		.new_max_trans_lat_snk_ms = new_max_trans_lat_snk_ms,
		.streams_set_snk = 0,
		.new_max_trans_lat_src_ms = new_max_trans_lat_src_ms,
		.streams_set_src = 0,
	};

	ret = bt_cap_unicast_group_foreach_stream(unicast_group, foreach_stream_transp_latency_set,
						  (void *)&stream_trans_lat_set);

	if (ret != 0 && ret != -ECANCELED) {
		LOG_ERR("Failed to iterate streams in group: %d", ret);
		return ret;
	}

	if (new_max_trans_lat_snk_ms != UINT16_MAX) {
		LOG_INF("Max transp latency %d ms selected for %d sink streams",
			new_max_trans_lat_snk_ms, stream_trans_lat_set.streams_set_snk);
		if (unicast_group->bap_unicast_group->cig_param.c_to_p_latency !=
		    new_max_trans_lat_snk_ms) {
			*group_action_needed = GROUP_ACTION_REQ_RESTART;
		}
	}

	if (new_max_trans_lat_src_ms != UINT16_MAX) {
		LOG_INF("Max transp latency %d ms selected for %d source streams",
			new_max_trans_lat_src_ms, stream_trans_lat_set.streams_set_src);
		if (unicast_group->bap_unicast_group->cig_param.p_to_c_latency !=
		    new_max_trans_lat_src_ms) {
			*group_action_needed = GROUP_ACTION_REQ_RESTART;
		}
	}

	return 0;
}