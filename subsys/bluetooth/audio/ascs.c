/* @file
 * @brief Bluetooth ASCS
 */
/*
 * Copyright (c) 2020 Intel Corporation
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/check.h>

#include <zephyr/device.h>
#include <zephyr/init.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include "zephyr/bluetooth/iso.h"
#include <zephyr/bluetooth/audio/audio.h>
#include <zephyr/bluetooth/audio/capabilities.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_DEBUG_ASCS)
#define LOG_MODULE_NAME bt_ascs
#include "common/log.h"

#include "../host/hci_core.h"
#include "../host/conn_internal.h"

#include "audio_internal.h"
#include "endpoint.h"
#include "unicast_server.h"
#include "pacs_internal.h"
#include "cap_internal.h"

#if defined(CONFIG_BT_AUDIO_UNICAST_SERVER)

#define ASE_ID(_ase) ase->ep.status.id
#define ASE_DIR(_id) \
	(_id > CONFIG_BT_ASCS_ASE_SNK_COUNT ? BT_AUDIO_DIR_SOURCE : BT_AUDIO_DIR_SINK)
#define ASE_UUID(_id) \
	(_id > CONFIG_BT_ASCS_ASE_SNK_COUNT ? BT_UUID_ASCS_ASE_SRC : BT_UUID_ASCS_ASE_SNK)
#define ASE_COUNT (CONFIG_BT_ASCS_ASE_SNK_COUNT + CONFIG_BT_ASCS_ASE_SRC_COUNT)

struct bt_ascs_ase {
	struct bt_ascs *ascs;
	struct bt_audio_ep ep;
	struct k_work work;
};

struct bt_ascs {
	struct bt_conn *conn;
	uint8_t id;
	bt_addr_le_t peer;
	struct bt_ascs_ase ases[ASE_COUNT];
	/* A single iso_channel may be used for 1 or 2 ases.
	 * Controlled by the client.
	 */
	struct bt_audio_iso isos[ASE_COUNT];
	struct bt_gatt_notify_params params;
};

static struct bt_ascs sessions[CONFIG_BT_MAX_CONN];

static struct bt_ascs *ascs_find(struct bt_conn *conn);
static struct bt_ascs_ase *ase_find(struct bt_ascs *ascs, uint8_t id);
static int control_point_notify(struct bt_conn *conn, const void *data, uint16_t len);

static void ase_status_changed(struct bt_audio_ep *ep, uint8_t old_state,
			       uint8_t state)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(ep, struct bt_ascs_ase, ep);

	BT_DBG("ase %p conn %p", ase, ase->ascs->conn);

	k_work_submit(&ase->work);
}

static void ascs_ep_unbind_audio_iso(struct bt_audio_ep *ep)
{
	struct bt_audio_iso *audio_iso = ep->iso;
	struct bt_audio_stream *stream = ep->stream;

	if (audio_iso != NULL) {
		struct bt_iso_chan_qos *qos;

		qos = audio_iso->iso_chan.qos;

		BT_ASSERT_MSG(stream != NULL, "Stream was NULL");

		stream->iso = NULL;
		if (audio_iso->sink_stream == stream) {
			audio_iso->sink_stream = NULL;
			qos->rx = NULL;
		} else if (audio_iso->source_stream == stream) {
			audio_iso->source_stream = NULL;
			qos->tx = NULL;
		} else {
			BT_ERR("Stream %p not linked to audio_iso %p", stream, audio_iso);
		}
	}

	ep->iso = NULL;
}

void ascs_ep_set_state(struct bt_audio_ep *ep, uint8_t state)
{
	struct bt_audio_stream *stream;
	uint8_t old_state;

	if (!ep) {
		return;
	}

	/* TODO: Verify state changes */

	old_state = ep->status.state;
	ep->status.state = state;

	BT_DBG("ep %p id 0x%02x %s -> %s", ep, ep->status.id,
	       bt_audio_ep_state_str(old_state),
	       bt_audio_ep_state_str(state));

	/* Notify clients*/
	ase_status_changed(ep, old_state, state);

	if (ep->stream == NULL || old_state == state) {
		return;
	}

	stream = ep->stream;

	if (stream->ops != NULL) {
		const struct bt_audio_stream_ops *ops = stream->ops;

		switch (state) {
		case BT_AUDIO_EP_STATE_IDLE:
			if (ops->released != NULL) {
				ops->released(stream);
			}

			break;
		case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
			switch (old_state) {
			case BT_AUDIO_EP_STATE_IDLE:
			case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
			case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			case BT_AUDIO_EP_STATE_RELEASING:
				break;
			default:
				BT_ASSERT_MSG(false,
					      "Invalid state transition: %s -> %s",
					      bt_audio_ep_state_str(old_state),
					      bt_audio_ep_state_str(ep->status.state));
				return;
			}

			if (ops->configured != NULL) {
				ops->configured(stream, &ep->qos_pref);
			}

			break;
		case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			/* QoS configured have different allowed states
			 * depending on the endpoint type
			 */
			if (ep->dir == BT_AUDIO_DIR_SOURCE) {
				switch (old_state) {
				case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
				case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
				case BT_AUDIO_EP_STATE_DISABLING:
					break;
				default:
					BT_ASSERT_MSG(false,
						      "Invalid state transition: %s -> %s",
						      bt_audio_ep_state_str(old_state),
						      bt_audio_ep_state_str(ep->status.state));
					return;
				}
			} else {
				switch (old_state) {
				case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
				case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
				case BT_AUDIO_EP_STATE_ENABLING:
				case BT_AUDIO_EP_STATE_STREAMING:
					break;
				default:
					BT_ASSERT_MSG(false,
						      "Invalid state transition: %s -> %s",
						      bt_audio_ep_state_str(old_state),
						      bt_audio_ep_state_str(ep->status.state));
					return;
				}
			}

			if (ops->qos_set != NULL) {
				ops->qos_set(stream);
			}

			break;
		case BT_AUDIO_EP_STATE_ENABLING:
			switch (old_state) {
			case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			case BT_AUDIO_EP_STATE_ENABLING:
				break;
			default:
				BT_ASSERT_MSG(false,
					      "Invalid state transition: %s -> %s",
					      bt_audio_ep_state_str(old_state),
					      bt_audio_ep_state_str(ep->status.state));
				return;
			}

			if (ops->enabled != NULL) {
				ops->enabled(stream);
			}

			break;
		case BT_AUDIO_EP_STATE_STREAMING:
			switch (old_state) {
			case BT_AUDIO_EP_STATE_ENABLING:
			case BT_AUDIO_EP_STATE_STREAMING:
				break;
			default:
				BT_ASSERT_MSG(false,
					      "Invalid state transition: %s -> %s",
					      bt_audio_ep_state_str(old_state),
					      bt_audio_ep_state_str(ep->status.state));
				return;
			}

			if (ops->started != NULL) {
				ops->started(stream);
			}

			break;
		case BT_AUDIO_EP_STATE_DISABLING:
			if (ep->dir == BT_AUDIO_DIR_SOURCE) {
				switch (old_state) {
				case BT_AUDIO_EP_STATE_ENABLING:
				case BT_AUDIO_EP_STATE_STREAMING:
					ep->receiver_ready = false;
					break;
				default:
					BT_ASSERT_MSG(false,
						      "Invalid state transition: %s -> %s",
						      bt_audio_ep_state_str(old_state),
						      bt_audio_ep_state_str(ep->status.state));
					return;
				}
			} else {
				/* Sinks cannot go into the disabling state */
				BT_ASSERT_MSG(false,
					      "Invalid state transition: %s -> %s",
					      bt_audio_ep_state_str(old_state),
					      bt_audio_ep_state_str(ep->status.state));
				return;
			}

			if (ops->disabled != NULL) {
				ops->disabled(stream);
			}

			break;
		case BT_AUDIO_EP_STATE_RELEASING:
			switch (old_state) {
			case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
			case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
			case BT_AUDIO_EP_STATE_ENABLING:
			case BT_AUDIO_EP_STATE_STREAMING:
				ep->receiver_ready = false;
				break;
			case BT_AUDIO_EP_STATE_DISABLING:
				if (ep->dir == BT_AUDIO_DIR_SOURCE) {
					break;
				} /* else fall through for sink */

				/* fall through */
			default:
				BT_ASSERT_MSG(false,
					      "Invalid state transition: %s -> %s",
					      bt_audio_ep_state_str(old_state),
					      bt_audio_ep_state_str(ep->status.state));
				return;
			}

			break; /* no-op*/
		default:
			BT_ERR("Invalid state: %u", state);
			break;
		}
	}

	if (state == BT_AUDIO_EP_STATE_CODEC_CONFIGURED &&
	    old_state != BT_AUDIO_EP_STATE_IDLE) {
		ascs_ep_unbind_audio_iso(ep);
	}
}

static void ascs_codec_data_add(struct net_buf_simple *buf, const char *prefix,
				uint8_t num, struct bt_codec_data *data)
{
	struct bt_ascs_codec_config *cc;
	int i;

	for (i = 0; i < num; i++) {
		struct bt_data *d = &data[i].data;

		BT_DBG("#%u: %s type 0x%02x len %u", i, prefix, d->type,
		       d->data_len);
		BT_HEXDUMP_DBG(d->data, d->data_len, prefix);

		cc = net_buf_simple_add(buf, sizeof(*cc));
		cc->len = d->data_len + sizeof(cc->type);
		cc->type = d->type;
		net_buf_simple_add_mem(buf, d->data, d->data_len);
	}
}

static void ascs_ep_get_status_config(struct bt_audio_ep *ep,
				      struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_config *cfg;
	struct bt_codec_qos_pref *pref = &ep->qos_pref;

	cfg = net_buf_simple_add(buf, sizeof(*cfg));
	cfg->framing = pref->unframed_supported ? BT_ASCS_QOS_FRAMING_UNFRAMED
						: BT_ASCS_QOS_FRAMING_FRAMED;
	cfg->phy = pref->phy;
	cfg->rtn = pref->rtn;
	cfg->latency = sys_cpu_to_le16(pref->latency);
	sys_put_le24(pref->pd_min, cfg->pd_min);
	sys_put_le24(pref->pd_max, cfg->pd_max);
	sys_put_le24(pref->pref_pd_min, cfg->prefer_pd_min);
	sys_put_le24(pref->pref_pd_max, cfg->prefer_pd_max);
	cfg->codec.id = ep->codec.id;
	cfg->codec.cid = sys_cpu_to_le16(ep->codec.cid);
	cfg->codec.vid = sys_cpu_to_le16(ep->codec.vid);

	BT_DBG("dir 0x%02x unframed_supported 0x%02x phy 0x%02x rtn %u "
	       "latency %u pd_min %u pd_max %u codec 0x%02x",
	       ep->dir, pref->unframed_supported, pref->phy,
	       pref->rtn, pref->latency, pref->pd_min, pref->pd_max,
	       ep->stream->codec->id);

	cfg->cc_len = buf->len;
	ascs_codec_data_add(buf, "data", ep->codec.data_count, ep->codec.data);
	cfg->cc_len = buf->len - cfg->cc_len;
}

static void ascs_ep_get_status_qos(struct bt_audio_ep *ep,
				   struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_qos *qos;

	qos = net_buf_simple_add(buf, sizeof(*qos));
	qos->cig_id = ep->cig_id;
	qos->cis_id = ep->cis_id;
	sys_put_le24(ep->stream->qos->interval, qos->interval);
	qos->framing = ep->stream->qos->framing;
	qos->phy = ep->stream->qos->phy;
	qos->sdu = sys_cpu_to_le16(ep->stream->qos->sdu);
	qos->rtn = ep->stream->qos->rtn;
	qos->latency = sys_cpu_to_le16(ep->stream->qos->latency);
	sys_put_le24(ep->stream->qos->pd, qos->pd);

	BT_DBG("dir 0x%02x codec 0x%02x interval %u framing 0x%02x phy 0x%02x "
	       "rtn %u latency %u pd %u",
	       ep->dir, ep->stream->codec->id,
	       ep->stream->qos->interval, ep->stream->qos->framing,
	       ep->stream->qos->phy, ep->stream->qos->rtn,
	       ep->stream->qos->latency, ep->stream->qos->pd);
}

static void ascs_ep_get_status_enable(struct bt_audio_ep *ep,
				      struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status_enable *enable;

	enable = net_buf_simple_add(buf, sizeof(*enable));
	enable->cig_id = ep->cig_id;
	enable->cis_id = ep->cis_id;

	enable->metadata_len = buf->len;
	ascs_codec_data_add(buf, "meta", ep->codec.meta_count, ep->codec.meta);
	enable->metadata_len = buf->len - enable->metadata_len;

	BT_DBG("dir 0x%02x cig 0x%02x cis 0x%02x",
	       ep->dir, ep->cig_id, ep->cis_id);
}

static int ascs_ep_get_status(struct bt_audio_ep *ep,
			      struct net_buf_simple *buf)
{
	struct bt_ascs_ase_status *status;

	if (!ep || !buf) {
		return -EINVAL;
	}

	BT_DBG("ep %p id 0x%02x state %s", ep, ep->status.id,
		bt_audio_ep_state_str(ep->status.state));

	/* Reset if buffer before using */
	net_buf_simple_reset(buf);

	status = net_buf_simple_add_mem(buf, &ep->status,
					sizeof(ep->status));

	switch (ep->status.state) {
	case BT_AUDIO_EP_STATE_IDLE:
	/* Fallthrough */
	case BT_AUDIO_EP_STATE_RELEASING:
		break;
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
		ascs_ep_get_status_config(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		ascs_ep_get_status_qos(ep, buf);
		break;
	case BT_AUDIO_EP_STATE_ENABLING:
	/* Fallthrough */
	case BT_AUDIO_EP_STATE_STREAMING:
	/* Fallthrough */
	case BT_AUDIO_EP_STATE_DISABLING:
		ascs_ep_get_status_enable(ep, buf);
		break;
	default:
		BT_ERR("Invalid Endpoint state");
		break;
	}

	return 0;
}

static void ascs_iso_recv(struct bt_iso_chan *chan,
			  const struct bt_iso_recv_info *info,
			  struct net_buf *buf)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_stream *stream = audio_iso->sink_stream;
	const struct bt_audio_stream_ops *ops;

	if (stream == NULL) {
		BT_ERR("Could not lookup stream by iso %p", chan);
		return;
	} else if (stream->ep == NULL) {
		BT_ERR("Stream not associated with an ep");
		return;
	}

	ops = stream->ops;

	if (IS_ENABLED(CONFIG_BT_AUDIO_DEBUG_STREAM_DATA)) {
		BT_DBG("stream %p ep %p len %zu",
		       stream, stream->ep, net_buf_frags_len(buf));
	}

	if (ops != NULL && ops->recv != NULL) {
		ops->recv(stream, info, buf);
	} else {
		BT_WARN("No callback for recv set");
	}
}

static void ascs_iso_sent(struct bt_iso_chan *chan)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_stream *stream = audio_iso->source_stream;
	struct bt_audio_stream_ops *ops = stream->ops;

	BT_DBG("stream %p ep %p", stream, stream->ep);

	if (ops != NULL && ops->sent != NULL) {
		ops->sent(stream);
	}
}

static int ase_stream_start(struct bt_audio_stream *stream)
{
	int err = 0;

	if (unicast_server_cb != NULL && unicast_server_cb->start != NULL) {
		err = unicast_server_cb->start(stream);
	} else {
		err = -ENOTSUP;
	}

	ascs_ep_set_state(stream->ep, BT_AUDIO_EP_STATE_STREAMING);

	return err;
}

static void ascs_iso_connected(struct bt_iso_chan *chan)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_stream *source_stream = audio_iso->source_stream;
	struct bt_audio_stream *sink_stream = audio_iso->sink_stream;
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	int err;

	if (sink_stream != NULL && sink_stream->iso == chan) {
		stream = sink_stream;
	} else if (source_stream != NULL && source_stream->iso == chan) {
		stream = source_stream;
	} else {
		stream = NULL;
	}

	if (stream == NULL) {
		BT_ERR("Could not lookup stream by iso %p", chan);
		return;
	} else if (stream->ep == NULL) {
		BT_ERR("Stream not associated with an ep");
		return;
	}

	ep = stream->ep;

	BT_DBG("stream %p ep %p dir %u", stream, ep, ep->dir);

	if (ep->status.state != BT_AUDIO_EP_STATE_ENABLING) {
		BT_DBG("endpoint not in enabling state: %s",
		       bt_audio_ep_state_str(ep->status.state));
		return;
	}

	if (ep->dir == BT_AUDIO_DIR_SOURCE && !ep->receiver_ready) {
		return;
	}

	err = ase_stream_start(stream);
	if (err) {
		BT_ERR("Could not start stream %d", err);
	}
}

static void ascs_iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	struct bt_audio_iso *audio_iso = CONTAINER_OF(chan, struct bt_audio_iso,
						      iso_chan);
	struct bt_audio_stream *source_stream = audio_iso->source_stream;
	struct bt_audio_stream *sink_stream = audio_iso->sink_stream;
	const struct bt_audio_stream_ops *ops;
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;

	if (sink_stream != NULL && sink_stream->iso == chan) {
		stream = sink_stream;
	} else if (source_stream != NULL && source_stream->iso == chan) {
		stream = source_stream;
	} else {
		stream = NULL;
	}

	if (stream == NULL) {
		BT_ERR("Could not lookup stream by iso %p", chan);
		return;
	} else if (stream->ep == NULL) {
		BT_ERR("Stream not associated with an ep");
		return;
	}

	ops = stream->ops;

	BT_DBG("stream %p ep %p reason 0x%02x", stream, stream->ep, reason);

	if (ops != NULL && ops->stopped != NULL) {
		ops->stopped(stream);
	} else {
		BT_WARN("No callback for stopped set");
	}

	ep = stream->ep;
	if (ep->status.state == BT_AUDIO_EP_STATE_RELEASING) {
		struct bt_ascs_ase *ase;
		struct bt_ascs *ascs;

		ascs = ascs_find(stream->conn);
		if (ascs == NULL) {
			BT_WARN("Could find ASCS based on ISO %p with ACL %p",
				chan, stream->conn);
			return;
		}

		ase = ase_find(ascs, ep->status.id);
		if (ase == NULL) {
			BT_WARN("Could find ASE based on ASCS %p with ep %p",
				ascs, ep);
			return;
		}

		/* Trigger a call to ase_process to handle the cleanup */
		k_work_submit(&ase->work);
	} else {
		int err;

		/* The ASE state machine goes into different states from this operation
		 * based on whether it is a source or a sink ASE.
		 */
		if (ep->dir == BT_AUDIO_DIR_SOURCE) {
			ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_DISABLING);
		} else {
			ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);
		}

		err = bt_audio_stream_iso_listen(stream);
		if (err != 0) {
			BT_ERR("Could not make stream listen: %d", err);
		}
	}
}

static struct bt_iso_chan_ops ascs_iso_ops = {
	.recv = ascs_iso_recv,
	.sent = ascs_iso_sent,
	.connected = ascs_iso_connected,
	.disconnected = ascs_iso_disconnected,
};

static void ascs_ase_cfg_changed(const struct bt_gatt_attr *attr,
				 uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

NET_BUF_SIMPLE_DEFINE_STATIC(rsp_buf, CONFIG_BT_L2CAP_TX_MTU);

static void ascs_cp_rsp_alloc(uint8_t op)
{
	struct bt_ascs_cp_rsp *rsp;

	rsp = net_buf_simple_add(&rsp_buf, sizeof(*rsp));
	rsp->op = op;
	rsp->num_ase = 0;
}

/* Add response to an opcode/ASE ID */
static void ascs_cp_rsp_add(uint8_t id, uint8_t op, uint8_t code,
			    uint8_t reason)
{
	struct bt_ascs_cp_rsp *rsp = (void *)rsp_buf.__buf;
	struct bt_ascs_cp_ase_rsp *ase_rsp;

	BT_DBG("id 0x%02x op %s (0x%02x) code %s (0x%02x) reason %s (0x%02x)",
	       id, bt_ascs_op_str(op), op, bt_ascs_rsp_str(code), code,
	       bt_ascs_reason_str(reason), reason);

	/* Allocate response if buffer is empty */
	if (!rsp_buf.len) {
		ascs_cp_rsp_alloc(op);
	}

	if (rsp->num_ase == 0xff) {
		return;
	}

	switch (code) {
	/* If the Response_Code value is 0x01 or 0x02, Number_of_ASEs shall be
	 * set to 0xFF.
	 */
	case BT_ASCS_RSP_NOT_SUPPORTED:
	case BT_ASCS_RSP_TRUNCATED:
		rsp->num_ase = 0xff;
		break;
	default:
		rsp->num_ase++;
		break;
	}

	ase_rsp = net_buf_simple_add(&rsp_buf, sizeof(*ase_rsp));
	ase_rsp->id = id;
	ase_rsp->code = code;
	ase_rsp->reason = reason;
}

static void ascs_cp_rsp_add_errno(uint8_t id, uint8_t op, int err,
				  uint8_t reason)
{
	BT_DBG("id %u op %u err %d reason %u", id, op, err, reason);

	switch (err) {
	case -ENOBUFS:
	case -ENOMEM:
		return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_NO_MEM,
				       BT_ASCS_REASON_NONE);
	case -EINVAL:
		switch (op) {
		case BT_ASCS_CONFIG_OP:
		/* Fallthrough */
		case BT_ASCS_QOS_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_CONF_INVALID,
					       reason);
		case BT_ASCS_ENABLE_OP:
		/* Fallthrough */
		case BT_ASCS_METADATA_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_METADATA_INVALID,
					       reason);
		default:
			return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_UNSPECIFIED,
					       BT_ASCS_REASON_NONE);
		}
	case -ENOTSUP:
		switch (op) {
		case BT_ASCS_CONFIG_OP:
		/* Fallthrough */
		case BT_ASCS_QOS_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_CONF_UNSUPPORTED,
					       reason);
		case BT_ASCS_ENABLE_OP:
		/* Fallthrough */
		case BT_ASCS_METADATA_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_METADATA_UNSUPPORTED,
					       reason);
		default:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_NOT_SUPPORTED,
					       BT_ASCS_REASON_NONE);
		}
	case -EBADMSG:
		return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_INVALID_ASE_STATE,
					       BT_ASCS_REASON_NONE);
	case -EACCES:
		switch (op) {
		case BT_ASCS_METADATA_OP:
			return ascs_cp_rsp_add(id, op,
					       BT_ASCS_RSP_METADATA_REJECTED,
					       reason);
		default:
			return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_UNSPECIFIED,
					       BT_ASCS_REASON_NONE);
		}
	default:
		return ascs_cp_rsp_add(id, op, BT_ASCS_RSP_UNSPECIFIED,
				       BT_ASCS_REASON_NONE);
	}
}

static void ascs_cp_rsp_success(uint8_t id, uint8_t op)
{
	ascs_cp_rsp_add(id, op, BT_ASCS_RSP_SUCCESS, BT_ASCS_REASON_NONE);
}

static void ase_release(struct bt_ascs_ase *ase)
{
	int err;

	BT_DBG("ase %p state %s",
	       ase, bt_audio_ep_state_str(ase->ep.status.state));

	if (ase->ep.status.state == BT_AUDIO_EP_STATE_RELEASING) {
		/* already releasing */
		return;
	}

	if (unicast_server_cb != NULL && unicast_server_cb->release != NULL) {
		err = unicast_server_cb->release(ase->ep.stream);
	} else {
		err = -ENOTSUP;
	}

	if (err) {
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_RELEASE_OP, err,
				      BT_ASCS_REASON_NONE);
		return;
	}

	ascs_ep_set_state(&ase->ep, BT_AUDIO_EP_STATE_RELEASING);

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_RELEASE_OP);
}

static void ascs_clear(struct bt_ascs *ascs)
{
	int i;

	BT_DBG("ascs %p", ascs);

	bt_addr_le_copy(&ascs->peer, BT_ADDR_LE_ANY);

	for (i = 0; i < ASE_COUNT; i++) {
		struct bt_ascs_ase *ase = &ascs->ases[i];

		if (ase->ep.status.state == BT_AUDIO_EP_STATE_IDLE) {
			continue;
		}

		/* ase_process will handle the final state transition into idle state */
		ase_release(ase);
	}
}

static void ase_disable(struct bt_ascs_ase *ase)
{
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	int err;

	BT_DBG("ase %p", ase);

	ep = &ase->ep;

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	 /* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
		break;
	default:
		BT_WARN("Invalid operation in state: %s", bt_audio_ep_state_str(ep->status.state));
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_DISABLE_OP,
				      -EBADMSG, BT_ASCS_REASON_NONE);
		return;
	}

	stream = ep->stream;

	if (unicast_server_cb != NULL && unicast_server_cb->disable != NULL) {
		err = unicast_server_cb->disable(stream);
	} else {
		err = -ENOTSUP;
	}

	if (err) {
		BT_ERR("Disable failed: %d", err);
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_DISABLE_OP,
				      err, BT_ASCS_REASON_NONE);
		return;
	}

	/* The ASE state machine goes into different states from this operation
	 * based on whether it is a source or a sink ASE.
	 */
	if (ep->dir == BT_AUDIO_DIR_SOURCE) {
		ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_DISABLING);
	} else {
		ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);
	}

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_DISABLE_OP);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int i;

	BT_DBG("");

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		struct bt_ascs *ascs = &sessions[i];

		if (ascs->conn != conn) {
			continue;
		}

		/* Release existing ASEs if not bonded */
		ascs_clear(ascs);

		bt_conn_unref(ascs->conn);
		ascs->conn = NULL;

		for (size_t i = 0U; i < ARRAY_SIZE(ascs->ases); i++) {
			struct bt_audio_stream *stream = ascs->ases[i].ep.stream;

			if (stream != NULL && stream->conn != NULL) {
				bt_conn_unref(stream->conn);
				stream->conn = NULL;
			}
		}
	}
}

static struct bt_conn_cb conn_cb = {
	.disconnected = disconnected,
};

static struct bt_audio_iso *audio_iso_get_or_new(struct bt_ascs *ascs, uint8_t cig_id,
						 uint8_t cis_id)
{
	struct bt_audio_iso *free_audio_iso = NULL;

	BT_DBG("ascs %p cig_id 0x%02x cis_id 0x%02x", ascs, cis_id, cis_id);

	for (size_t i = 0; i < ARRAY_SIZE(ascs->isos); i++) {
		struct bt_audio_iso *audio_iso = &ascs->isos[i];
		const struct bt_audio_ep *ep;

		if (audio_iso->sink_stream == NULL && audio_iso->source_stream == NULL) {
			free_audio_iso = audio_iso;
			continue;
		} else if (audio_iso->sink_stream && audio_iso->sink_stream->ep) {
			ep = audio_iso->sink_stream->ep;
		} else if (audio_iso->source_stream && audio_iso->source_stream->ep) {
			ep = audio_iso->source_stream->ep;
		} else {
			/* XXX: Stream not associated with endpoint. Should we assert??? */
			continue;
		}

		if (ep->cig_id == cig_id && ep->cis_id == cis_id) {
			return audio_iso;
		}
	}

	return free_audio_iso;
}

static struct bt_ascs *ascs_new(struct bt_conn *conn)
{
	static bool conn_cb_registered;
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (!sessions[i].conn &&
		    !bt_addr_le_cmp(&sessions[i].peer, BT_ADDR_LE_ANY)) {
			struct bt_ascs *ascs = &sessions[i];

			memset(ascs->ases, 0, sizeof(ascs->ases));
			ascs->conn = bt_conn_ref(conn);

			/* Register callbacks if not registered */
			if (!conn_cb_registered) {
				bt_conn_cb_register(&conn_cb);
				conn_cb_registered = true;
			}

			return ascs;
		}
	}

	return NULL;
}

static void ase_stream_add(struct bt_ascs *ascs, struct bt_ascs_ase *ase,
			   struct bt_audio_stream *stream)
{
	BT_DBG("ase %p stream %p", ase, stream);
	ase->ep.stream = stream;
	stream->conn = ascs->conn;
	stream->ep = &ase->ep;
	stream->iso = &ase->ep.iso->iso_chan;
}

static void ascs_attach(struct bt_ascs *ascs, struct bt_conn *conn)
{
	int i;

	BT_DBG("ascs %p conn %p", ascs, conn);

	ascs->conn = bt_conn_ref(conn);

	/* TODO: Load the ASEs from the settings? */

	for (i = 0; i < ASE_COUNT; i++) {
		if (ascs->ases[i].ep.stream) {
			ase_stream_add(ascs, &ascs->ases[i],
				       ascs->ases[i].ep.stream);
		}
	}
}

static struct bt_ascs *ascs_find(struct bt_conn *conn)
{
	int i;

	for (i = 0; i < CONFIG_BT_MAX_CONN; i++) {
		if (sessions[i].conn == conn) {
			return &sessions[i];
		}

		/* Check if there is an existing session for the peer */
		if (!sessions[i].conn &&
		    bt_conn_is_peer_addr_le(conn, sessions[i].id,
					    &sessions[i].peer)) {
			ascs_attach(&sessions[i], conn);
			return &sessions[i];
		}
	}

	return NULL;
}

static struct bt_ascs *ascs_get(struct bt_conn *conn)
{
	struct bt_ascs *ascs;

	ascs = ascs_find(conn);
	if (ascs) {
		return ascs;
	}

	return ascs_new(conn);
}

NET_BUF_SIMPLE_DEFINE_STATIC(ase_buf, CONFIG_BT_L2CAP_TX_MTU);

static void ase_process(struct k_work *work)
{
	struct bt_ascs_ase *ase = CONTAINER_OF(work, struct bt_ascs_ase, work);
	struct bt_audio_ep *ep = &ase->ep;
	const struct bt_audio_iso *audio_iso = ep->iso;
	struct bt_audio_stream *stream = ep->stream;
	const uint8_t ep_state = ep->status.state;
	struct bt_conn *conn = ase->ascs->conn;

	BT_DBG("ase %p, ep %p, ep.stream %p", ase, ep, stream);

	if (conn != NULL && conn->state == BT_CONN_CONNECTED) {
		ascs_ep_get_status(ep, &ase_buf);

		bt_gatt_notify(conn, ep->server.attr,
			       ase_buf.data, ase_buf.len);
	}

	/* Stream shall be NULL in the idle state, and non-NULL otherwise */
	__ASSERT(ep_state == BT_AUDIO_EP_STATE_IDLE ?
			stream == NULL : stream != NULL,
		 "stream is NULL");

	if (ep_state == BT_AUDIO_EP_STATE_RELEASING) {

		if (audio_iso == NULL ||
		    audio_iso->iso_chan.state == BT_ISO_STATE_DISCONNECTED) {
			ascs_ep_unbind_audio_iso(ep);
			bt_audio_stream_detach(stream);
			ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_IDLE);
		} else {
			/* Either the client or the server may disconnect the
			 * CISes when entering the releasing state.
			 */
			const int err = bt_audio_stream_disconnect(stream);

			if (err != 0) {
				BT_ERR("Failed to disconnect stream %p: %d",
				       stream, err);
			}
		}
	} else if (ep_state == BT_AUDIO_EP_STATE_ENABLING) {
		/* SINK ASEs can autonomously go into the streaming state if
		 * the CIS is connected
		 */
		if (ep->dir == BT_AUDIO_DIR_SINK &&
		    audio_iso != NULL &&
		    audio_iso->iso_chan.state == BT_ISO_STATE_CONNECTED) {
			ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_STREAMING);
		}
	}
}

static uint8_t ase_attr_cb(const struct bt_gatt_attr *attr, uint16_t handle,
			   void *user_data)
{
	struct bt_ascs_ase *ase = user_data;

	if (ase->ep.status.id == POINTER_TO_UINT(BT_AUDIO_CHRC_USER_DATA(attr))) {
		ase->ep.server.attr = attr;

		return BT_GATT_ITER_STOP;
	}

	return BT_GATT_ITER_CONTINUE;
}

static int ascs_ep_stream_bind_audio_iso(struct bt_audio_stream *stream,
					 struct bt_audio_iso *audio_iso)
{
	const enum bt_audio_dir dir = stream->ep->dir;

	BT_DBG("stream %p, dir %u audio_iso %p", stream, dir, audio_iso);

	if (dir == BT_AUDIO_DIR_SOURCE) {
		if (audio_iso->source_stream == NULL) {
			audio_iso->source_stream = stream;
		} else if (audio_iso->source_stream != stream) {
			BT_WARN("Bound with source_stream %p already", audio_iso->source_stream);
			return -EADDRINUSE;
		}

		audio_iso->iso_chan.ops = &ascs_iso_ops;
		audio_iso->iso_chan.qos = &audio_iso->iso_qos;
		audio_iso->iso_chan.qos->tx = &audio_iso->source_io_qos;
		audio_iso->iso_chan.qos->tx->path = &audio_iso->source_path;
		audio_iso->iso_chan.qos->tx->path->cc = audio_iso->source_path_cc;
	} else if (dir == BT_AUDIO_DIR_SINK) {
		if (audio_iso->sink_stream == NULL) {
			audio_iso->sink_stream = stream;
		} else if (audio_iso->sink_stream != stream) {
			BT_WARN("Bound with sink_stream %p already", audio_iso->sink_stream);
			return -EADDRINUSE;
		}

		audio_iso->iso_chan.ops = &ascs_iso_ops;
		audio_iso->iso_chan.qos = &audio_iso->iso_qos;
		audio_iso->iso_chan.qos->rx = &audio_iso->sink_io_qos;
		audio_iso->iso_chan.qos->rx->path = &audio_iso->sink_path;
		audio_iso->iso_chan.qos->rx->path->cc = audio_iso->sink_path_cc;
	} else {
		__ASSERT(false, "Invalid dir: %u", dir);
	}

	stream->iso = &audio_iso->iso_chan;
	stream->ep->iso = audio_iso;

	return 0;
}

void ascs_ep_init(struct bt_audio_ep *ep, uint8_t id)
{
	BT_DBG("ep %p id 0x%02x", ep, id);

	(void)memset(ep, 0, sizeof(*ep));
	ep->status.id = id;
	ep->dir = ASE_DIR(id);
}

static void ase_init(struct bt_ascs_ase *ase, uint8_t id)
{
	memset(ase, 0, sizeof(*ase));
	ascs_ep_init(&ase->ep, id);

	/* Lookup ASE characteristic */
	bt_gatt_foreach_attr_type(0x0001, 0xffff, ASE_UUID(id), NULL, 0, ase_attr_cb, ase);

	__ASSERT(ase->ep.server.attr, "ASE characteristic not found\n");

	k_work_init(&ase->work, ase_process);
}

static struct bt_ascs_ase *ase_new(struct bt_ascs *ascs, uint8_t id)
{
	struct bt_ascs_ase *ase;
	int i;

	if (id) {
		if (id > ASE_COUNT) {
			return NULL;
		}
		i = id;
		ase = &ascs->ases[i - 1];
		goto done;
	}

	for (i = 0; i < ASE_COUNT; i++) {
		ase = &ascs->ases[i];

		if (!ase->ep.status.id) {
			i++;
			goto done;
		}
	}

	return NULL;

done:
	ase_init(ase, i);
	ase->ascs = ascs;

	return ase;
}

static struct bt_ascs_ase *ase_find(struct bt_ascs *ascs, uint8_t id)
{
	struct bt_ascs_ase *ase;

	if (!id || id > ASE_COUNT) {
		return NULL;
	}

	ase = &ascs->ases[id - 1];
	if (ase->ep.status.id == id) {
		return ase;
	}

	return NULL;
}

static struct bt_ascs_ase *ase_get(struct bt_ascs *ascs, uint8_t id)
{
	struct bt_ascs_ase *ase;

	ase = ase_find(ascs, id);
	if (ase) {
		return ase;
	}

	return ase_new(ascs, id);
}

static ssize_t ascs_ase_read(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     uint16_t len, uint16_t offset)
{
	struct bt_ascs *ascs;
	struct bt_ascs_ase *ase;

	BT_DBG("conn %p attr %p buf %p len %u offset %u", conn, attr, buf, len,
	       offset);

	ascs = ascs_get(conn);
	if (!ascs) {
		BT_ERR("Unable to get ASCS session");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	ase = ase_get(ascs, POINTER_TO_UINT(BT_AUDIO_CHRC_USER_DATA(attr)));
	if (!ase) {
		BT_ERR("Unable to get ASE");
		return BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
	}

	ascs_ep_get_status(&ase->ep, &ase_buf);

	return bt_gatt_attr_read(conn, attr, buf, len, offset, ase_buf.data,
				 ase_buf.len);
}

static void ascs_cp_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	BT_DBG("attr %p value 0x%04x", attr, value);
}

static bool ascs_codec_config_store(struct bt_data *data, void *user_data)
{
	struct bt_codec *codec = user_data;
	struct bt_codec_data *cdata;

	if (codec->data_count >= ARRAY_SIZE(codec->data)) {
		BT_ERR("No slot available for Codec Config");
		return false;
	}

	cdata = &codec->data[codec->data_count];

	if (data->data_len > sizeof(cdata->value)) {
		BT_ERR("Not enough space for Codec Config: %u > %zu",
		       data->data_len, sizeof(cdata->value));
		return false;
	}

	BT_DBG("#%u type 0x%02x len %u", codec->data_count, data->type,
	       data->data_len);

	cdata->data.type = data->type;
	cdata->data.data_len = data->data_len;

	/* Deep copy data contents */
	cdata->data.data = cdata->value;
	(void)memcpy(cdata->value, data->data, data->data_len);

	BT_HEXDUMP_DBG(cdata->value, data->data_len, "data");

	codec->data_count++;

	return true;
}

struct codec_lookup_id_data {
	uint8_t id;
	struct bt_codec *codec;
};

static bool codec_lookup_id(const struct bt_audio_capability *capability, void *user_data)
{
	struct codec_lookup_id_data *data = user_data;

	if (capability->codec->id == data->id) {
		data->codec = capability->codec;

		return false;
	}

	return true;
}

static int ascs_ep_set_codec(struct bt_audio_ep *ep, uint8_t id, uint16_t cid,
			     uint16_t vid, struct net_buf_simple *buf,
			     uint8_t len, struct bt_codec *codec)
{
	struct net_buf_simple ad;
	struct codec_lookup_id_data lookup_data = {
		.id = id,
	};

	if (ep == NULL && codec == NULL) {
		return -EINVAL;
	}

	BT_DBG("ep %p dir %u codec id 0x%02x cid 0x%04x vid 0x%04x len %u",
	       ep, ep->dir, id, cid, vid, len);

	bt_audio_foreach_capability(ep->dir, codec_lookup_id, &lookup_data);

	if (lookup_data.codec == NULL) {
		return -ENOENT;
	}

	if (codec == NULL) {
		codec = &ep->codec;
	}

	codec->id = id;
	codec->cid = cid;
	codec->vid = vid;
	codec->data_count = 0;
	codec->path_id = lookup_data.codec->path_id;

	if (len == 0) {
		return 0;
	}

	net_buf_simple_init_with_data(&ad, net_buf_simple_pull_mem(buf, len),
				      len);

	/* Parse LTV entries */
	bt_data_parse(&ad, ascs_codec_config_store, codec);

	/* Check if all entries could be parsed */
	if (ad.len) {
		BT_ERR("Unable to parse Codec Config: len %u", ad.len);
		(void)memset(codec, 0, sizeof(*codec));

		return -EINVAL;
	}

	return 0;
}

static int ase_config(struct bt_ascs *ascs, struct bt_ascs_ase *ase,
		      const struct bt_ascs_config *cfg,
		      struct net_buf_simple *buf)
{
	struct bt_audio_stream *stream;
	struct bt_codec codec;
	int err;

	BT_DBG("ase %p latency 0x%02x phy 0x%02x codec 0x%02x "
		"cid 0x%04x vid 0x%04x codec config len 0x%02x", ase,
		cfg->latency, cfg->phy, cfg->codec.id, cfg->codec.cid,
		cfg->codec.vid, cfg->cc_len);

	if (cfg->latency < BT_ASCS_CONFIG_LATENCY_LOW ||
	    cfg->latency > BT_ASCS_CONFIG_LATENCY_HIGH) {
		BT_WARN("Invalid latency: 0x%02x", cfg->latency);
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
				BT_ASCS_RSP_CONF_INVALID,
				BT_ASCS_REASON_LATENCY);
		return 0;
	}

	if (cfg->phy < BT_ASCS_CONFIG_PHY_LE_1M ||
	    cfg->phy > BT_ASCS_CONFIG_PHY_LE_CODED) {
		BT_WARN("Invalid PHY: 0x%02x", cfg->phy);
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
				BT_ASCS_RSP_CONF_INVALID, BT_ASCS_REASON_PHY);
		return 0;
	}

	switch (ase->ep.status.state) {
	/* Valid only if ASE_State field = 0x00 (Idle) */
	case BT_AUDIO_EP_STATE_IDLE:
	 /* or 0x01 (Codec Configured) */
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
	 /* or 0x02 (QoS Configured) */
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		BT_WARN("Invalid operation in state: %s",
			bt_audio_ep_state_str(ase->ep.status.state));
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
				BT_ASCS_RSP_INVALID_ASE_STATE, 0x00);
		return 0;
	}

	/* Store current codec configuration to be able to restore it
	 * in case of error.
	 */
	(void)memcpy(&codec, &ase->ep.codec, sizeof(codec));

	if (ascs_ep_set_codec(&ase->ep, cfg->codec.id,
			      sys_le16_to_cpu(cfg->codec.cid),
			      sys_le16_to_cpu(cfg->codec.vid),
			      buf, cfg->cc_len, &ase->ep.codec)) {
		(void)memcpy(&ase->ep.codec, &codec, sizeof(codec));
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
				BT_ASCS_RSP_CONF_INVALID,
				BT_ASCS_REASON_CODEC_DATA);
		return 0;
	}

	if (ase->ep.stream != NULL) {
		if (unicast_server_cb != NULL &&
		    unicast_server_cb->reconfig != NULL) {
			err = unicast_server_cb->reconfig(ase->ep.stream,
							  ase->ep.dir,
							  &ase->ep.codec,
							  &ase->ep.qos_pref);
		} else {
			err = -ENOTSUP;
		}

		if (err != 0) {
			uint8_t reason = BT_ASCS_REASON_CODEC_DATA;

			BT_ERR("Reconfig failed: %d", err);

			(void)memcpy(&ase->ep.codec, &codec, sizeof(codec));
			ascs_cp_rsp_add_errno(ASE_ID(ase),
					      BT_ASCS_CONFIG_OP,
					      err, reason);
			return 0;
		}

		stream = ase->ep.stream;
	} else {
		stream = NULL;
		if (unicast_server_cb != NULL &&
		    unicast_server_cb->config != NULL) {
			err = unicast_server_cb->config(ascs->conn, &ase->ep,
							ase->ep.dir,
							&ase->ep.codec, &stream,
							&ase->ep.qos_pref);
		} else {
			err = -ENOTSUP;
		}

		if (err != 0 || stream == NULL) {
			BT_ERR("Config failed, err: %d, stream %p",
			       err, stream);

			(void)memcpy(&ase->ep.codec, &codec, sizeof(codec));
			ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_CONFIG_OP,
					BT_ASCS_RSP_CONF_REJECTED,
					BT_ASCS_REASON_CODEC_DATA);

			return err;
		}

		ase_stream_add(ascs, ase, stream);
	}

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_CONFIG_OP);

	/* TODO: bt_audio_stream_attach duplicates some of the
	 * ase_stream_add. Should be cleaned up.
	 */
	bt_audio_stream_attach(ascs->conn, stream, &ase->ep,
			       &ase->ep.codec);

	ascs_ep_set_state(&ase->ep, BT_AUDIO_EP_STATE_CODEC_CONFIGURED);

	return 0;
}

static ssize_t ascs_config(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_config_op *req;
	const struct bt_ascs_config *cfg;
	int i;

	if (buf->len < sizeof(*req)) {
		BT_WARN("Malformed ASE Config");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (req->num_ases < 1) {
		BT_WARN("Number_of_ASEs parameter value is less than 1");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (buf->len < req->num_ases * sizeof(*cfg)) {
		BT_WARN("Malformed ASE Config: len %u < %zu", buf->len,
			req->num_ases * sizeof(*cfg));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		int err;

		if (buf->len < sizeof(*cfg)) {
			BT_WARN("Malformed ASE Config: len %u < %zu", buf->len, sizeof(*cfg));
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		cfg = net_buf_simple_pull_mem(buf, sizeof(*cfg));

		if (buf->len < cfg->cc_len) {
			BT_WARN("Malformed ASE Codec Config len %u != %u", buf->len, cfg->cc_len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		BT_DBG("ase 0x%02x cc_len %u", cfg->ase, cfg->cc_len);

		if (cfg->ase) {
			ase = ase_get(ascs, cfg->ase);
		} else {
			ase = ase_new(ascs, 0);
		}

		if (!ase) {
			ascs_cp_rsp_add(cfg->ase, BT_ASCS_CONFIG_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_WARN("Unknown ase 0x%02x", cfg->ase);
			continue;
		}

		err = ase_config(ascs, ase, cfg, buf);
		if (err != 0) {
			BT_WARN("Malformed ASE Config");
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}
	}

	return buf->size;
}

static int ase_stream_qos(struct bt_audio_stream *stream,
			  struct bt_codec_qos *qos,
			  struct bt_ascs *ascs,
			  uint8_t cig_id,
			  uint8_t cis_id)
{
	struct bt_audio_iso *audio_iso;
	struct bt_audio_ep *ep;
	int err;

	BT_DBG("stream %p ep %p qos %p", stream, stream->ep, qos);

	if (stream == NULL || stream->ep == NULL || qos == NULL) {
		BT_DBG("Invalid input stream, ep or qos pointers");
		return -EINVAL;
	}

	ep = stream->ep;

	switch (ep->status.state) {
	/* Valid only if ASE_State field = 0x01 (Codec Configured) */
	case BT_AUDIO_EP_STATE_CODEC_CONFIGURED:
	/* or 0x02 (QoS Configured) */
	case BT_AUDIO_EP_STATE_QOS_CONFIGURED:
		break;
	default:
		BT_WARN("Invalid operation in state: %s", bt_audio_ep_state_str(ep->status.state));
		return -EBADMSG;
	}

	if (!bt_audio_valid_qos(qos)) {
		return -EINVAL;
	}

	if (!bt_audio_valid_stream_qos(stream, qos)) {
		return -EINVAL;
	}

	if (unicast_server_cb != NULL && unicast_server_cb->qos != NULL) {
		int err;

		err = unicast_server_cb->qos(stream, qos);
		if (err != 0) {
			BT_DBG("Application returned error: %d", err);
			return err;
		}
	}

	audio_iso = audio_iso_get_or_new(ascs, cig_id, cis_id);
	if (audio_iso == NULL) {
		BT_ERR("Could not allocate audio_iso");
		return -ENOMEM;
	}

	err = ascs_ep_stream_bind_audio_iso(stream, audio_iso);
	if (err < 0) {
		return err;
	}

	stream->qos = qos;

	ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);

	bt_audio_stream_iso_listen(stream);

	return 0;
}

static void ase_qos(struct bt_ascs_ase *ase, const struct bt_ascs_qos *qos)
{
	struct bt_audio_ep *ep = &ase->ep;
	struct bt_audio_stream *stream = ep->stream;
	struct bt_codec_qos *cqos = &ep->qos;
	const uint8_t cig_id = qos->cig;
	const uint8_t cis_id = qos->cis;
	int err;

	cqos->interval = sys_get_le24(qos->interval);
	cqos->framing = qos->framing;
	cqos->phy = qos->phy;
	cqos->sdu = sys_le16_to_cpu(qos->sdu);
	cqos->rtn = qos->rtn;
	cqos->latency = sys_le16_to_cpu(qos->latency);
	cqos->pd = sys_get_le24(qos->pd);

	BT_DBG("ase %p cig 0x%02x cis 0x%02x interval %u framing 0x%02x "
	       "phy 0x%02x sdu %u rtn %u latency %u pd %u", ase, qos->cig,
	       qos->cis, cqos->interval, cqos->framing, cqos->phy, cqos->sdu,
	       cqos->rtn, cqos->latency, cqos->pd);

	err = ase_stream_qos(stream, cqos, ase->ascs, cig_id, cis_id);
	if (err) {
		uint8_t reason = BT_ASCS_REASON_NONE;

		BT_ERR("QoS failed: err %d", err);

		if (err == -ENOTSUP) {
			if (cqos->interval == 0) {
				reason = BT_ASCS_REASON_INTERVAL;
			} else if (cqos->framing == 0xff) {
				reason = BT_ASCS_REASON_FRAMING;
			} else if (cqos->phy == 0) {
				reason = BT_ASCS_REASON_PHY;
			} else if (cqos->sdu == 0xffff) {
				reason = BT_ASCS_REASON_SDU;
			} else if (cqos->latency == 0) {
				reason = BT_ASCS_REASON_LATENCY;
			} else if (cqos->pd == 0) {
				reason = BT_ASCS_REASON_PD;
			}
		} else if (err == -EADDRINUSE) {
			reason = BT_ASCS_REASON_CIS;
			/* FIXME: Ugly workaround to send Response_Code
			 *        0x09 = Invalid Configuration Parameter Value
			 */
			err = -EINVAL;
		}

		memset(cqos, 0, sizeof(*cqos));

		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_QOS_OP,
				      err, reason);
		return;
	} else {
		/* We setup the data path here, as this is the earliest where
		 * we have the ISO <-> EP coupling completed (due to setting
		 * the CIS ID in the QoS procedure).
		 */
		if (ep->dir == BT_AUDIO_DIR_SINK) {
			bt_audio_codec_to_iso_path(&ep->iso->sink_path,
						   stream->codec);
		} else {
			bt_audio_codec_to_iso_path(&ep->iso->source_path,
						   stream->codec);
		}
	}

	ep->cig_id = cig_id;
	ep->cis_id = cis_id;

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_QOS_OP);
}

static ssize_t ascs_qos(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_qos_op *req;
	const struct bt_ascs_qos *qos;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (req->num_ases < 1) {
		BT_WARN("Number_of_ASEs parameter value is less than 1");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (buf->len < req->num_ases * sizeof(*qos)) {
		BT_WARN("Malformed ASE QoS: len %u < %zu", buf->len, req->num_ases * sizeof(*qos));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;

		qos = net_buf_simple_pull_mem(buf, sizeof(*qos));

		BT_DBG("ase 0x%02x", qos->ase);

		ase = ase_find(ascs, qos->ase);
		if (!ase) {
			ascs_cp_rsp_add(qos->ase, BT_ASCS_QOS_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_WARN("Unknown ase 0x%02x", qos->ase);
			continue;
		}

		ase_qos(ase, qos);
	}

	return buf->size;
}

static bool ascs_codec_store_metadata(struct bt_data *data, void *user_data)
{
	struct bt_codec *codec = user_data;
	struct bt_codec_data *meta;

	meta = &codec->meta[codec->meta_count];
	meta->data.type = data->type;
	meta->data.data_len = data->data_len;

	/* Deep copy data contents */
	meta->data.data = meta->value;
	(void)memcpy(meta->value, data->data, data->data_len);

	BT_DBG("#%zu: data: %s",
	       codec->meta_count,
	       bt_hex(meta->value, data->data_len));

	codec->meta_count++;

	return true;
}

struct ascs_parse_result {
	int err;
	size_t count;
	const struct bt_audio_ep *ep;
};

static bool ascs_parse_metadata(struct bt_data *data, void *user_data)
{
	struct ascs_parse_result *result = user_data;
	const struct bt_audio_ep *ep = result->ep;
	const uint8_t data_len = data->data_len;
	const uint8_t data_type = data->type;
	const uint8_t *data_value = data->data;

	result->count++;

	BT_DBG("#%u type 0x%02x len %u", result->count, data_type, data_len);

	if (result->count > CONFIG_BT_CODEC_MAX_METADATA_COUNT) {
		BT_ERR("Not enough buffers for Codec Config Metadata: %zu > %zu",
		       result->count, CONFIG_BT_CODEC_MAX_METADATA_LEN);
		result->err = -ENOMEM;

		return false;
	}

	if (data_len > CONFIG_BT_CODEC_MAX_METADATA_LEN) {
		BT_ERR("Not enough space for Codec Config Metadata: %u > %zu",
		       data->data_len, CONFIG_BT_CODEC_MAX_METADATA_LEN);
		result->err = -ENOMEM;

		return false;
	}

	/* The CAP acceptor shall not accept metadata with
	 * unsupported stream context.
	 */
	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR)) {
		if (data_type == BT_AUDIO_METADATA_TYPE_STREAM_CONTEXT) {
			const uint16_t context = sys_get_le16(data_value);

			if (!bt_pacs_context_available(ep->dir, context)) {
				BT_WARN("Context 0x%04x is unavailable", context);

				result->err = -EACCES;

				return false;
			}
		} else if (data_type == BT_AUDIO_METADATA_TYPE_CCID_LIST) {
			/* Verify that the CCID is a known CCID on the
			 * writing device
			 */
			for (uint8_t i = 0; i < data_len; i++) {
				const uint8_t ccid = data_value[i];

				if (!bt_cap_acceptor_ccid_exist(ep->stream->conn,
								ccid)) {
					BT_WARN("CCID %u is unknown", ccid);

					/* TBD:
					 * Should we reject the Metadata?
					 *
					 * Should unknown CCIDs trigger a
					 * discovery procedure for TBS or MCS?
					 *
					 * Or should we just accept as is, and
					 * then let the application decide?
					 */
				}
			}
		}
	}

	return true;
}

static int ascs_verify_metadata(const struct net_buf_simple *buf,
				struct bt_audio_ep *ep)
{
	struct ascs_parse_result result = {
		.count = 0U,
		.err = 0,
		.ep = ep
	};
	struct net_buf_simple meta_ltv;

	/* Clone the buf to avoid pulling data from the original buffer */
	net_buf_simple_clone(buf, &meta_ltv);

	/* Parse LTV entries */
	bt_data_parse(&meta_ltv, ascs_parse_metadata, &result);

	/* Check if all entries could be parsed */
	if (meta_ltv.len != 0) {
		BT_ERR("Unable to parse Metadata: len %u", meta_ltv.len);

		if (meta_ltv.len > 2) {
			/* Value of the Metadata Type field in error */
			return meta_ltv.data[2];
		}

		return -EINVAL;
	}

	return result.err;
}

int ascs_ep_set_metadata(struct bt_audio_ep *ep,
			 struct net_buf_simple *buf, uint8_t len,
			 struct bt_codec *codec)
{
	struct net_buf_simple meta_ltv;
	int err;

	if (ep == NULL && codec == NULL) {
		return -EINVAL;
	}

	BT_DBG("ep %p len %u codec %p", ep, len, codec);

	if (len == 0) {
		(void)memset(codec->meta, 0, sizeof(codec->meta));
		return 0;
	}

	if (codec == NULL) {
		codec = &ep->codec;
	}

	/* Extract metadata LTV for this specific endpoint */
	net_buf_simple_init_with_data(&meta_ltv,
				      net_buf_simple_pull_mem(buf, len),
				      len);

	err = ascs_verify_metadata(&meta_ltv, ep);
	if (err != 0) {
		return err;
	}

	/* reset cached metadata */
	ep->codec.meta_count = 0;

	/* store data contents */
	bt_data_parse(&meta_ltv, ascs_codec_store_metadata, codec);

	return 0;
}

static int ase_metadata(struct bt_ascs_ase *ase, uint8_t op,
			struct bt_ascs_metadata *meta,
			struct net_buf_simple *buf)
{
	struct bt_codec_data metadata_backup[CONFIG_BT_CODEC_MAX_DATA_COUNT];
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	uint8_t state;
	int err;

	BT_DBG("ase %p meta->len %u", ase, meta->len);

	ep = &ase->ep;
	state = ep->status.state;

	switch (state) {
	/* Valid for an ASE only if ASE_State field = 0x03 (Enabling) */
	case BT_AUDIO_EP_STATE_ENABLING:
	/* or 0x04 (Streaming) */
	case BT_AUDIO_EP_STATE_STREAMING:
		break;
	default:
		BT_WARN("Invalid operation in state: %s", bt_audio_ep_state_str(state));
		err = -EBADMSG;
		ascs_cp_rsp_add_errno(ASE_ID(ase), op, err,
				      buf->len ? *buf->data : 0x00);
		return err;
	}

	if (!meta->len) {
		goto done;
	}

	/* Backup existing metadata */
	(void)memcpy(metadata_backup, ep->codec.meta, sizeof(metadata_backup));
	err = ascs_ep_set_metadata(ep, buf, meta->len, &ep->codec);
	if (err) {
		if (err < 0) {
			ascs_cp_rsp_add_errno(ASE_ID(ase), op, err, 0x00);
		} else {
			ascs_cp_rsp_add(ASE_ID(ase), op,
					BT_ASCS_RSP_METADATA_INVALID, err);
		}
		return 0;
	}

	stream = ep->stream;
	if (unicast_server_cb != NULL && unicast_server_cb->metadata != NULL) {
		err = unicast_server_cb->metadata(stream, ep->codec.meta,
						  ep->codec.meta_count);
	} else {
		err = -ENOTSUP;
	}

	if (err) {
		/* Restore backup */
		(void)memcpy(ep->codec.meta, metadata_backup,
			     sizeof(metadata_backup));

		BT_ERR("Metadata failed: %d", err);
		ascs_cp_rsp_add_errno(ASE_ID(ase), op, err,
				      buf->len ? *buf->data : 0x00);
		return err;
	}

	/* Set the state to the same state to trigger the notifications */
	ascs_ep_set_state(ep, ep->status.state);
done:
	ascs_cp_rsp_success(ASE_ID(ase), op);

	return 0;
}

static int ase_enable(struct bt_ascs_ase *ase, struct bt_ascs_metadata *meta,
		      struct net_buf_simple *buf)
{
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	int err;

	BT_DBG("ase %p buf->len %u", ase, buf->len);

	ep = &ase->ep;

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (ep->status.state != BT_AUDIO_EP_STATE_QOS_CONFIGURED) {
		err = -EBADMSG;
		BT_WARN("Invalid operation in state: %s", bt_audio_ep_state_str(ep->status.state));
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_ENABLE_OP, err,
				      BT_ASCS_REASON_NONE);
		return err;
	}

	err = ascs_ep_set_metadata(ep, buf, meta->len, &ep->codec);
	if (err) {
		if (err < 0) {
			ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_ENABLE_OP,
					      err, 0x00);
		} else {
			ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_ENABLE_OP,
					BT_ASCS_RSP_METADATA_INVALID, err);
		}
		return 0;
	}

	stream = ep->stream;
	if (unicast_server_cb != NULL && unicast_server_cb->enable != NULL) {
		err = unicast_server_cb->enable(stream, ep->codec.meta,
						ep->codec.meta_count);
	} else {
		err = -ENOTSUP;
	}

	if (err) {
		BT_ERR("Enable rejected: %d", err);
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_ENABLE_OP, err,
				      BT_ASCS_REASON_NONE);
		return -EFAULT;
	}

	ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_ENABLING);

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_ENABLE_OP);

	return 0;
}

static ssize_t ascs_enable(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_enable_op *req;
	struct bt_ascs_metadata *meta;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (req->num_ases < 1) {
		BT_WARN("Number_of_ASEs parameter value is less than 1");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (buf->len < req->num_ases * sizeof(*meta)) {
		BT_WARN("Malformed ASE Metadata: len %u < %zu", buf->len,
			req->num_ases * sizeof(*meta));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;

		meta = net_buf_simple_pull_mem(buf, sizeof(*meta));

		BT_DBG("ase 0x%02x meta->len %u", meta->ase, meta->len);

		if (buf->len < meta->len) {
			BT_WARN("Malformed ASE Enable Metadata len %u != %u", buf->len, meta->len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		ase = ase_find(ascs, meta->ase);
		if (!ase) {
			ascs_cp_rsp_add(meta->ase, BT_ASCS_ENABLE_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_WARN("Unknown ase 0x%02x", meta->ase);
			continue;
		}

		ase_enable(ase, meta, buf);
	}

	return buf->size;
}

static void ase_start(struct bt_ascs_ase *ase)
{
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;

	BT_DBG("ase %p", ase);

	ep = &ase->ep;

	/* Valid for an ASE only if ASE_State field = 0x02 (QoS Configured) */
	if (ep->status.state != BT_AUDIO_EP_STATE_ENABLING) {
		BT_WARN("Invalid operation in state: %s", bt_audio_ep_state_str(ep->status.state));
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_START_OP, -EBADMSG,
				      BT_ASCS_REASON_NONE);
		return;
	}

	/* If the ASE_ID  written by the client represents a Sink ASE, the
	 * server shall not accept the Receiver Start Ready operation for that
	 * ASE. The server shall send a notification of the ASE Control Point
	 * characteristic to the client, and the server shall set the
	 * Response_Code value for that ASE to 0x05 (Invalid ASE direction).
	 */
	if (ep->dir == BT_AUDIO_DIR_SINK) {
		BT_WARN("Start failed: invalid operation for Sink");
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_START_OP,
				BT_ASCS_RSP_INVALID_DIR, BT_ASCS_REASON_NONE);
		return;
	}

	ep->receiver_ready = true;

	stream = ep->stream;
	if (stream->iso->state == BT_ISO_STATE_CONNECTED) {
		int err;

		err = ase_stream_start(stream);
		if (err) {
			BT_ERR("Start failed: %d", err);
			ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_START_OP, err,
					BT_ASCS_REASON_NONE);
			return;
		}
	}

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_START_OP);
}

static ssize_t ascs_start(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_start_op *req;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (req->num_ases < 1) {
		BT_WARN("Number_of_ASEs parameter value is less than 1");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (buf->len < req->num_ases) {
		BT_WARN("Malformed ASE Start: len %u < %u", buf->len, req->num_ases);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		BT_DBG("ase 0x%02x", id);

		ase = ase_find(ascs, id);
		if (!ase) {
			ascs_cp_rsp_add(id, BT_ASCS_START_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_WARN("Unknown ase 0x%02x", id);
			continue;
		}

		ase_start(ase);
	}

	return buf->size;
}

static ssize_t ascs_disable(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_disable_op *req;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (req->num_ases < 1) {
		BT_WARN("Number_of_ASEs parameter value is less than 1");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (buf->len < req->num_ases) {
		BT_WARN("Malformed ASE Disable: len %u < %u", buf->len, req->num_ases);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		BT_DBG("ase 0x%02x", id);

		ase = ase_find(ascs, id);
		if (!ase) {
			ascs_cp_rsp_add(id, BT_ASCS_DISABLE_OP,
					BT_ASCS_RSP_INVALID_ASE_STATE, 0);
			BT_WARN("Unknown ase 0x%02x", id);
			continue;
		}

		ase_disable(ase);
	}

	return buf->size;
}

static void ase_stop(struct bt_ascs_ase *ase)
{
	struct bt_audio_stream *stream;
	struct bt_audio_ep *ep;
	int err;

	BT_DBG("ase %p", ase);

	ep = &ase->ep;

	/* If the ASE_ID  written by the client represents a Sink ASE, the
	 * server shall not accept the Receiver Stop Ready operation for that
	 * ASE. The server shall send a notification of the ASE Control Point
	 * characteristic to the client, and the server shall set the
	 * Response_Code value for that ASE to 0x05 (Invalid ASE direction).
	 */
	if (ase->ep.dir == BT_AUDIO_DIR_SINK) {
		BT_WARN("Stop failed: invalid operation for Sink");
		ascs_cp_rsp_add(ASE_ID(ase), BT_ASCS_STOP_OP,
				BT_ASCS_RSP_INVALID_DIR, BT_ASCS_REASON_NONE);
		return;
	}

	if (ep->status.state != BT_AUDIO_EP_STATE_DISABLING) {
		BT_WARN("Invalid operation in state: %s", bt_audio_ep_state_str(ep->status.state));
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_STOP_OP, -EBADMSG,
				      BT_ASCS_REASON_NONE);
		return;
	}

	stream = ep->stream;
	if (unicast_server_cb != NULL && unicast_server_cb->stop != NULL) {
		err = unicast_server_cb->stop(stream);
	} else {
		err = -ENOTSUP;
	}

	if (err) {
		BT_ERR("Stop failed: %d", err);
		ascs_cp_rsp_add_errno(ASE_ID(ase), BT_ASCS_STOP_OP, err,
				      BT_ASCS_REASON_NONE);
		return;
	}

	/* If the Receiver Stop Ready operation has completed successfully the
	 * Unicast Client or the Unicast Server may terminate a CIS established
	 * for that ASE by following the Connected Isochronous Stream Terminate
	 * procedure defined in Volume 3, Part C, Section 9.3.15.
	 */
	err = bt_audio_stream_disconnect(stream);
	if (err != -ENOTCONN && err != 0) {
		BT_ERR("Could not disconnect the CIS: %d", err);
		return;
	}

	ascs_ep_set_state(ep, BT_AUDIO_EP_STATE_QOS_CONFIGURED);
	err = bt_audio_stream_iso_listen(stream);
	if (err != 0) {
		BT_ERR("Could not make stream listen: %d", err);
		return;
	}

	ascs_cp_rsp_success(ASE_ID(ase), BT_ASCS_STOP_OP);
}

static ssize_t ascs_stop(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_start_op *req;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (req->num_ases < 1) {
		BT_WARN("Number_of_ASEs parameter value is less than 1");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (buf->len < req->num_ases) {
		BT_WARN("Malformed ASE Start: len %u < %u", buf->len, req->num_ases);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;
		uint8_t id;

		id = net_buf_simple_pull_u8(buf);

		BT_DBG("ase 0x%02x", id);

		ase = ase_find(ascs, id);
		if (!ase) {
			ascs_cp_rsp_add(id, BT_ASCS_STOP_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_WARN("Unknown ase 0x%02x", id);
			continue;
		}

		ase_stop(ase);
	}

	return buf->size;
}

static ssize_t ascs_metadata(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_metadata_op *req;
	struct bt_ascs_metadata *meta;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (req->num_ases < 1) {
		BT_WARN("Number_of_ASEs parameter value is less than 1");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (buf->len < req->num_ases * sizeof(*meta)) {
		BT_WARN("Malformed ASE Metadata: len %u < %zu", buf->len,
			req->num_ases * sizeof(*meta));
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		struct bt_ascs_ase *ase;

		meta = net_buf_simple_pull_mem(buf, sizeof(*meta));

		if (buf->len < meta->len) {
			BT_WARN("Malformed ASE Metadata: len %u < %u", buf->len, meta->len);
			return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
		}

		BT_DBG("ase 0x%02x meta->len %u", meta->ase, meta->len);

		ase = ase_find(ascs, meta->ase);
		if (!ase) {
			ascs_cp_rsp_add(meta->ase, BT_ASCS_METADATA_OP,
					BT_ASCS_RSP_INVALID_ASE, 0x00);
			BT_WARN("Unknown ase 0x%02x", meta->ase);
			continue;
		}

		ase_metadata(ase, BT_ASCS_METADATA_OP, meta, buf);
	}

	return buf->size;
}

static ssize_t ascs_release(struct bt_ascs *ascs, struct net_buf_simple *buf)
{
	const struct bt_ascs_release_op *req;
	int i;

	if (buf->len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	req = net_buf_simple_pull_mem(buf, sizeof(*req));

	BT_DBG("num_ases %u", req->num_ases);

	if (req->num_ases < 1) {
		BT_WARN("Number_of_ASEs parameter value is less than 1");
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	} else if (buf->len < req->num_ases) {
		BT_WARN("Malformed ASE Release: len %u < %u", buf->len, req->num_ases);
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	for (i = 0; i < req->num_ases; i++) {
		uint8_t id;
		struct bt_ascs_ase *ase;

		id = net_buf_simple_pull_u8(buf);

		BT_DBG("ase 0x%02x", id);

		ase = ase_find(ascs, id);
		if (!ase) {
			ascs_cp_rsp_add(id, BT_ASCS_RELEASE_OP,
					BT_ASCS_RSP_INVALID_ASE, 0);
			BT_WARN("Unknown ase 0x%02x", id);
			continue;
		}

		if (ase->ep.status.state == BT_AUDIO_EP_STATE_IDLE ||
		    ase->ep.status.state == BT_AUDIO_EP_STATE_RELEASING) {
			BT_WARN("Invalid operation in state: %s",
				bt_audio_ep_state_str(ase->ep.status.state));
			ascs_cp_rsp_add(id, BT_ASCS_RELEASE_OP,
					BT_ASCS_RSP_INVALID_ASE_STATE, BT_ASCS_REASON_NONE);
			continue;
		}

		ase_release(ase);
	}

	return buf->size;
}

static ssize_t ascs_cp_write(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, const void *data,
			     uint16_t len, uint16_t offset, uint8_t flags)
{
	struct bt_ascs *ascs;
	const struct bt_ascs_ase_cp *req;
	struct net_buf_simple buf;
	ssize_t ret;

	if (offset) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_OFFSET);
	}

	if (len < sizeof(*req)) {
		return BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN);
	}

	net_buf_simple_init_with_data(&buf, (void *) data, len);

	req = net_buf_simple_pull_mem(&buf, sizeof(*req));

	BT_DBG("conn %p attr %p buf %p len %u op %s (0x%02x)", conn,
	       attr, data, len, bt_ascs_op_str(req->op), req->op);

	/* Reset/empty response buffer before using it again */
	net_buf_simple_reset(&rsp_buf);

	ascs = ascs_get(conn);
	if (!ascs) {
		BT_ERR("Unable to get ASCS session");
		len = BT_GATT_ERR(BT_ATT_ERR_UNLIKELY);
		ascs_cp_rsp_add(0, req->op, BT_ASCS_RSP_UNSPECIFIED, 0x00);
		goto respond;
	}

	switch (req->op) {
	case BT_ASCS_CONFIG_OP:
		ret = ascs_config(ascs, &buf);
		break;
	case BT_ASCS_QOS_OP:
		ret = ascs_qos(ascs, &buf);
		break;
	case BT_ASCS_ENABLE_OP:
		ret = ascs_enable(ascs, &buf);
		break;
	case BT_ASCS_START_OP:
		ret = ascs_start(ascs, &buf);
		break;
	case BT_ASCS_DISABLE_OP:
		ret = ascs_disable(ascs, &buf);
		break;
	case BT_ASCS_STOP_OP:
		ret = ascs_stop(ascs, &buf);
		break;
	case BT_ASCS_METADATA_OP:
		ret = ascs_metadata(ascs, &buf);
		break;
	case BT_ASCS_RELEASE_OP:
		ret = ascs_release(ascs, &buf);
		break;
	default:
		ascs_cp_rsp_add(0x00, req->op, BT_ASCS_RSP_NOT_SUPPORTED, 0);
		BT_DBG("Unknown opcode");
		goto respond;
	}

	if (ret == BT_GATT_ERR(BT_ATT_ERR_INVALID_ATTRIBUTE_LEN)) {
		ascs_cp_rsp_add(0, req->op, BT_ASCS_RSP_TRUNCATED,
				BT_ASCS_REASON_NONE);
	}

respond:
	control_point_notify(ascs->conn, rsp_buf.data, rsp_buf.len);

	return len;
}

#define BT_ASCS_ASE_DEFINE(_uuid, _id) \
	BT_AUDIO_CHRC(_uuid, \
		      BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY, \
		      BT_GATT_PERM_READ_ENCRYPT, \
		      ascs_ase_read, NULL, UINT_TO_POINTER(_id)), \
	BT_AUDIO_CCC(ascs_ase_cfg_changed)
#define BT_ASCS_ASE_SNK_DEFINE(_n, ...) BT_ASCS_ASE_DEFINE(BT_UUID_ASCS_ASE_SNK, (_n) + 1)
#define BT_ASCS_ASE_SRC_DEFINE(_n, ...) BT_ASCS_ASE_DEFINE(BT_UUID_ASCS_ASE_SRC, (_n) + 1 + \
							   CONFIG_BT_ASCS_ASE_SNK_COUNT)

BT_GATT_SERVICE_DEFINE(ascs_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_ASCS),
	BT_AUDIO_CHRC(BT_UUID_ASCS_ASE_CP,
		      BT_GATT_CHRC_WRITE | BT_GATT_CHRC_WRITE_WITHOUT_RESP | BT_GATT_CHRC_NOTIFY,
		      BT_GATT_PERM_WRITE_ENCRYPT,
		      NULL, ascs_cp_write, NULL),
	BT_AUDIO_CCC(ascs_cp_cfg_changed),
	LISTIFY(CONFIG_BT_ASCS_ASE_SNK_COUNT, BT_ASCS_ASE_SNK_DEFINE, (,)),
	LISTIFY(CONFIG_BT_ASCS_ASE_SRC_COUNT, BT_ASCS_ASE_SRC_DEFINE, (,)),
);

static int control_point_notify(struct bt_conn *conn, const void *data, uint16_t len)
{
	return bt_gatt_notify_uuid(conn, BT_UUID_ASCS_ASE_CP, ascs_svc.attrs, data, len);
}

#endif /* BT_AUDIO_UNICAST_SERVER */
