#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* This file contains functions for faking CAP and BAP behaviors as well as Bluetooth host calls
 *  For simpliclity, faking has been done on this level, i.e. including
 * /src/bluetooth/bt_stream/le_audio.c as part of the tests.
 */

typedef bool (*callback_fn_cap_fake)(struct bt_cap_stream *stream, void *user_data);

typedef bool (*callback_fn_bap_fake)(struct bt_bap_stream *stream, void *user_data);

struct cap_unicast_group_foreach_stream_data {
	bt_cap_unicast_group_foreach_stream_func_t func;
	void *user_data;
};

static bool bap_unicast_group_foreach_stream_cb(struct bt_bap_stream *bap_stream, void *user_data)
{
	struct cap_unicast_group_foreach_stream_data *data = user_data;

	return data->func(CONTAINER_OF(bap_stream, struct bt_cap_stream, bap_stream),
			  data->user_data);
}

DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, bt_cap_unicast_group_foreach_stream, struct bt_cap_unicast_group *,
		callback_fn_cap_fake, void *);

int bt_cap_unicast_group_foreach_stream_custom_fake(
	struct bt_cap_unicast_group *unicast_group,
	bool (*callback_fn_cap_fake)(struct bt_cap_stream *stream, void *user_data),
	void *user_data)
{
	struct cap_unicast_group_foreach_stream_data data = {
		.func = callback_fn_cap_fake,
		.user_data = user_data,
	};

	if (unicast_group == NULL) {
		return -EINVAL;
	}

	if (callback_fn_cap_fake == NULL) {
		return -EINVAL;
	}

	struct bt_bap_stream *stream, *next;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&unicast_group->bap_unicast_group->streams, stream, next,
					  _node) {
		const bool stop = bap_unicast_group_foreach_stream_cb(stream, &data);

		if (stop) {
			return -ECANCELED;
		}
	}

	return 0;
}

struct bt_bap_ep *bt_bap_iso_get_paired_ep_fake(const struct bt_bap_ep *ep)
{
	if (ep->iso->rx.ep == ep) {
		return ep->iso->tx.ep;
	} else {
		return ep->iso->rx.ep;
	}
}

FAKE_VALUE_FUNC(int, bt_bap_ep_get_info, const struct bt_bap_ep *, struct bt_bap_ep_info *);

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

FAKE_VALUE_FUNC(const bt_addr_le_t *, bt_conn_get_dst, const struct bt_conn *);

const bt_addr_le_t *bt_conn_get_dst_custom_fake(const struct bt_conn *conn)
{
	return &conn->le.dst;
}
