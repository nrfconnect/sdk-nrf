/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _BT_FAKES_H_
#define _BT_FAKES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <zephyr/fff.h>
#include <zephyr/sys/slist.h>
#include <zephyr/tc_util.h>
#include <../subsys/bluetooth/audio/bap_endpoint.h>
#include <../subsys/bluetooth/audio/bap_iso.h>

FAKE_VALUE_FUNC(int, bt_bap_ep_get_info, const struct bt_bap_ep *, struct bt_bap_ep_info *);
FAKE_VALUE_FUNC(struct bt_iso_chan *, bt_bap_stream_iso_chan_get, struct bt_bap_stream *);
FAKE_VALUE_FUNC(int, bt_cap_stream_send_ts, struct bt_cap_stream *, struct net_buf *, uint16_t,
		uint32_t);
FAKE_VALUE_FUNC(int, bt_cap_stream_send, struct bt_cap_stream *, struct net_buf *, uint16_t);
FAKE_VALUE_FUNC(int, bt_audio_codec_cfg_get_frame_dur, const struct bt_audio_codec_cfg *);
FAKE_VALUE_FUNC(int, bt_audio_codec_cfg_frame_dur_to_frame_dur_us,
		enum bt_audio_codec_cfg_frame_dur);
FAKE_VALUE_FUNC(int, bt_audio_codec_cfg_get_octets_per_frame, const struct bt_audio_codec_cfg *);
FAKE_VALUE_FUNC(int, bt_hci_get_conn_handle, const struct bt_conn *, uint16_t *);

#define TEST_CAP_STREAM(name, dir_in, pd_in, group_in, interval_in)                                \
	struct bt_cap_stream name = {0};                                                           \
	struct bt_bap_ep name##_ep_var = {0};                                                      \
	struct bt_iso_chan name##_bap_iso = {0};                                                   \
	struct bt_bap_qos_cfg name##_qos = {0};                                                    \
	struct bt_bap_iso name##_iso = {0};                                                        \
	name##_qos.pd = pd_in;                                                                     \
	name##_qos.interval = interval_in;                                                         \
	name##_ep_var.dir = dir_in;                                                                \
	name.bap_stream.ep = &name##_ep_var;                                                       \
	name.bap_stream.iso = &name##_bap_iso;                                                     \
	name.bap_stream.group = (void *)group_in;                                                  \
	name.bap_stream.qos = &name##_qos;                                                         \
	name.bap_stream.ep->iso = &name##_iso

struct bt_bap_ep *bt_bap_iso_get_paired_ep_fake(const struct bt_bap_ep *ep)
{
	if (ep->iso->rx.ep == ep) {
		return ep->iso->tx.ep;
	} else {
		return ep->iso->rx.ep;
	}
}

int bt_bap_ep_get_info_custom_fake(const struct bt_bap_ep *ep, struct bt_bap_ep_info *info)
{
	enum bt_audio_dir dir;

	if (ep == NULL) {
		TC_PRINT("ep is NULL\n");
		return -EINVAL;
	}

	if (info == NULL) {
		TC_PRINT("info is NULL\n");
		return -EINVAL;
	}

	dir = ep->dir;

	info->id = ep->id;
	info->state = ep->state;
	info->dir = dir;
	info->qos_pref = &ep->qos_pref;

	if (ep->iso == NULL) {
		info->paired_ep = NULL;
		info->iso_chan = NULL;
	} else {
		info->paired_ep = bt_bap_iso_get_paired_ep_fake(ep);
		info->iso_chan = &ep->iso->chan;
	}

	info->can_send = false;
	info->can_recv = false;
	if (IS_ENABLED(CONFIG_BT_AUDIO_TX) && ep->stream != NULL) {

		if (IS_ENABLED(CONFIG_BT_BAP_UNICAST_CLIENT)) {
			/* dir is not initialized before the connection is set */
			if (ep->stream->conn != NULL) {
				info->can_send = dir == BT_AUDIO_DIR_SINK;
				info->can_recv = dir == BT_AUDIO_DIR_SOURCE;
			}
		} else {
			TC_PRINT("This mock is only for unicast client\n");
		}
		return -EINVAL;
	}

	return 0;
}

int bt_cap_stream_send_ts_custom_fake(struct bt_cap_stream *stream, struct net_buf *buf,
				      uint16_t seq_num, uint32_t ts)
{
	if (stream == NULL || buf == NULL) {
		return -EINVAL;
	}
	net_buf_unref(buf);
	return 0;
}

int bt_cap_stream_send_custom_fake(struct bt_cap_stream *stream, struct net_buf *buf,
				   uint16_t seq_num)
{

	if (stream == NULL || buf == NULL) {
		return -EINVAL;
	}
	net_buf_unref(buf);
	return 0;
}

#endif /* _BT_FAKES_H_ */
