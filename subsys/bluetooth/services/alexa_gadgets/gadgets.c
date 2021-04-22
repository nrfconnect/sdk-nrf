/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/gadgets.h>

#include <sys/byteorder.h>

#include <logging/log.h>
LOG_MODULE_REGISTER(bt_gadgets, CONFIG_BT_ALEXA_GADGETS_LOG_LEVEL);

#define GADGETS_TRANSACTION_TYPE_FIRST 0b00
#define GADGETS_TRANSACTION_TYPE_CONT 0b01
#define GADGETS_TRANSACTION_TYPE_LAST 0b10
#define GADGETS_TRANSACTION_TYPE_CTRL 0b11

#define GADGET_HEADER_BASE_ACK 0
#define GADGET_HEADER_BASE_EXT 0

#define GADGET_HEADER_FIRST_RES 0

#define GADGET_HEADER_ACK_ACK 1
#define GADGET_HEADER_ACK_NOACK 0
#define GADGET_HEADER_ACK_EXT 0
#define GADGET_HEADER_ACK_PAYLOAD_LEN 2
#define GADGET_HEADER_ACK_RES1 0
#define GADGET_HEADER_ACK_RES2 0x01

#define GADGETS_ACK_ID_NO_ACK -1

/* Base part of header found in all stream packets */
struct gadgets_header_base {
	uint8_t trxn_id : 4;
	uint8_t stream_id : 4;
	uint8_t ext : 1;
	uint8_t ack : 1;
	uint8_t type : 2;
	uint8_t sequence_no : 4;
};

/* Header in first packet in a transaction */
struct gadgets_header_first {
	struct gadgets_header_base base;
	uint8_t reserved;
	/* 16-bit variable, using uint8 array for endinaness conversion */
	uint8_t total_transaction_length[2];
	uint8_t payload_length;
};

/* Extended version of header in first packet in a transaction */
struct gadgets_header_first_ext {
	struct gadgets_header_base base;
	uint8_t reserved;
	/* 16-bit variables, using uint8 array for endinaness conversion */
	uint8_t total_transaction_length[2];
	uint8_t payload_length[2];
};

/* Header in subsequent packets in a transaction */
struct gadgets_header_subs {
	struct gadgets_header_base base;
	uint8_t payload_length;
};

/* Extended version of header in subsequent packets in a transaction */
struct gadgets_header_subs_ext {
	struct gadgets_header_base base;
	/* 16-bit variable, using uint8 array for endinaness conversion */
	uint8_t payload_length[2];
};

/* Ack packet. Only sent upon explicit request */
struct gadgets_ack_pkt {
	struct gadgets_header_base base;
	uint8_t reserved1;
	uint8_t payload_length;
	uint8_t reserved2;
	uint8_t result_code;
};

struct stream_control {
	enum bt_gadgets_stream_id stream_id;
	uint8_t sequence_no;
	bool active;
	int8_t ack_id;
	uint16_t total_length;
	uint16_t received_length;
};

static const struct bt_gadgets_cb *gadgets_cb;

static uint8_t tx_buffer[CONFIG_BT_L2CAP_TX_MTU - 3]; /* 3 = ATT overhead*/

static atomic_t transmit_busy;
static uint8_t transaction_counter;
static uint8_t tx_sequence_number;

/* Payload is fragmented when it cannot fit in a single MTU */
static struct {
	const uint8_t *stream_payload;
	size_t stream_payload_offset;
	size_t stream_payload_remainder;
} stream_ctl;

static struct stream_control streams[] = {
	{ .stream_id = BT_GADGETS_STREAM_ID_CONTROL, .active = false },
	{ .stream_id = BT_GADGETS_STREAM_ID_ALEXA, .active = false },
	{ .stream_id = BT_GADGETS_STREAM_ID_OTA, .active = false },
};

static void encode_ack(
	struct gadgets_ack_pkt *buf,
	enum bt_gadgets_stream_id stream_id,
	uint8_t trxn_id,
	bool ack);

static int bt_gadgets_send_perform(
	struct bt_conn *conn, void *data, uint16_t size);

static inline void reset_stream(
	struct stream_control *stream,
	uint16_t total_length)
{
	stream->sequence_no = 0;
	stream->active = true;
	stream->ack_id = GADGETS_ACK_ID_NO_ACK;
	stream->total_length = total_length;
	stream->received_length = 0;
}

static struct stream_control *get_stream(uint8_t stream_id)
{
	for (int i = 0; i < ARRAY_SIZE(streams); ++i) {
		if (streams[i].stream_id == (enum bt_gadgets_stream_id) stream_id) {
			return &streams[i];
		}
	}

	return NULL;
}

static ssize_t on_receive(struct bt_conn *conn,
			  const struct bt_gatt_attr *attr,
			  const void *buf,
			  uint16_t len,
			  uint16_t offset,
			  uint8_t flags)
{
	LOG_DBG("Received data, handle %d, conn %p, len %d",
		attr->handle, (void *)conn, len);

	if (len < sizeof(struct gadgets_header_base)) {
		LOG_WRN("Unexpected length: %d", len);
		return len;
	}

	struct gadgets_header_base *base = (struct gadgets_header_base *)buf;
	struct stream_control *stream = get_stream(base->stream_id);
	uint16_t payload_len;
	uint16_t payload_offset;

	if (offset != 0) {
		LOG_WRN("Unexpected offset: %d", offset);
		return len;
	}

	if (stream == NULL) {
		LOG_WRN("Unexpected stream: 0x%02X", base->stream_id);
		return len;
	}

	/* Streams have the same base header.
	 * There could be different header extensions,
	 * determined by values in base header.
	 * The first packet header is also different from subsequent
	 * packet headers in a transaction.
	 */

	if (stream->active) {
		/* Continuation packet in stream */
		if (base->ext) {
			struct gadgets_header_subs_ext *ext =
				(struct gadgets_header_subs_ext *) buf;

			payload_len = sys_get_be16(ext->payload_length);
			payload_offset = sizeof(*ext);
		} else {
			struct gadgets_header_subs *sub =
				(struct gadgets_header_subs *) buf;

			payload_len = sub->payload_length;
			payload_offset = sizeof(*sub);
		}
	} else {
		uint16_t stream_len;

		/* First packet in stream */
		if (base->ext) {
			struct gadgets_header_first_ext *ext =
				(struct gadgets_header_first_ext *) buf;

			stream_len =
				sys_get_be16(ext->total_transaction_length);
			payload_len =
				sys_get_be16(ext->payload_length);
			payload_offset = sizeof(*ext);
		} else {
			struct gadgets_header_first *first =
				(struct gadgets_header_first *) buf;

			stream_len =
				sys_get_be16(first->total_transaction_length);
			payload_len = first->payload_length;
			payload_offset = sizeof(*first);
		}
		LOG_DBG("  total_transaction_length: %d", stream_len);

		reset_stream(stream, stream_len);
	}

	if (base->ack) {
		/* Any of the headers in a sequence can request an ACK */
		stream->ack_id = base->trxn_id;
	}

	LOG_DBG("  payload_len: %d", payload_len);

	stream->received_length += payload_len;

	if (stream->received_length == stream->total_length) {
		stream->active = false;
	} else if (stream->received_length > stream->total_length) {
		LOG_WRN("Stream overrun: %d / %d",
			stream->received_length, stream->total_length);
		stream->active = false;
	}

	bool supported = false;

	switch ((enum bt_gadgets_stream_id) base->stream_id) {
	case BT_GADGETS_STREAM_ID_CONTROL:
		LOG_DBG("STREAM_ID_CONTROL");
		supported =
			gadgets_cb->control_stream_cb(
				conn,
				&((uint8_t *)buf)[payload_offset],
				payload_len,
				stream->active);
		break;
	case BT_GADGETS_STREAM_ID_ALEXA:
		LOG_DBG("STREAM_ID_ALEXA");
		supported =
			gadgets_cb->alexa_stream_cb(
				conn,
				&((uint8_t *)buf)[payload_offset],
				payload_len,
				stream->active);
		break;
	default:
		LOG_WRN("UNKNOWN STREAM ID: %d",
			(enum bt_gadgets_stream_id) base->stream_id);
		break;
	}

	if (!stream->active && stream->ack_id != GADGETS_ACK_ID_NO_ACK) {
		struct gadgets_ack_pkt ack;
		int err;

		/* Note: it is possible that a callback function has
		 * triggered a transmission here already.
		 * Ack function seems to be relevant only for OTA,
		 * so this behavior needs to be revisited if/when
		 * adding support for the Gadgets OTA protocol.
		 */

		LOG_DBG("Sending %s",
			supported ? log_strdup("ACK") : log_strdup("NACK"));

		encode_ack(&ack, base->stream_id, stream->ack_id, supported);
		err = bt_gadgets_send_perform(conn, &ack, sizeof(ack));
		if (err) {
			LOG_WRN("Failed to send ack");
		}
	}

	return len;
}

static void encode_ack(
	struct gadgets_ack_pkt *buf,
	enum bt_gadgets_stream_id stream_id,
	uint8_t trxn_id,
	bool ack)
{
	buf->base.stream_id = stream_id;
	buf->base.trxn_id = trxn_id;
	buf->base.sequence_no = 0; /* Don't care */
	buf->base.type = GADGETS_TRANSACTION_TYPE_CTRL;
	buf->base.ack = ack ? GADGET_HEADER_ACK_ACK : GADGET_HEADER_ACK_NOACK;
	buf->base.ext = GADGET_HEADER_ACK_EXT;
	buf->reserved1 = GADGET_HEADER_ACK_RES1;
	buf->payload_length = GADGET_HEADER_ACK_PAYLOAD_LEN;
	buf->reserved2 = GADGET_HEADER_ACK_RES2;
	buf->result_code = ack ?
		BT_GADGETS_RESULT_CODE_SUCCESS : BT_GADGETS_RESULT_CODE_UNSUPPORTED;
}

static void encode_header(
	void *buf,
	enum bt_gadgets_stream_id stream,
	uint8_t type,
	uint8_t payload_len,
	uint16_t transaction_len)
{
	struct gadgets_header_base *base = buf;

	base->stream_id = stream;
	base->sequence_no = tx_sequence_number++;
	base->type = type;
	base->ack = GADGET_HEADER_BASE_ACK;
	base->ext = GADGET_HEADER_BASE_EXT;

	if (type == GADGETS_TRANSACTION_TYPE_FIRST) {
		struct gadgets_header_first *hdr = buf;

		transaction_counter++;

		base->trxn_id = transaction_counter;
		hdr->reserved = GADGET_HEADER_FIRST_RES;
		hdr->payload_length = payload_len;
		sys_put_be16(transaction_len, hdr->total_transaction_length);

	} else {
		struct gadgets_header_subs *hdr = buf;

		base->trxn_id = transaction_counter;
		hdr->payload_length = payload_len;
	}
}

static void on_sent(struct bt_conn *conn, void *user_data)
{
	bool transmit_success = true;

	if (stream_ctl.stream_payload_remainder) {
		const size_t header_size = sizeof(struct gadgets_header_subs);
		uint16_t max_packet_size;
		size_t payload_size;
		uint8_t stream;
		uint8_t stream_type;
		int err;

		max_packet_size = bt_gadgets_max_send(conn);
		if ((header_size + stream_ctl.stream_payload_remainder) > max_packet_size) {
			payload_size = max_packet_size - header_size;
			stream_ctl.stream_payload_remainder -= payload_size;
		} else {
			payload_size = stream_ctl.stream_payload_remainder;
			stream_ctl.stream_payload_remainder = 0;
		}

		/* Get stream ID from preceding transmit */
		stream = ((struct gadgets_header_base *) tx_buffer)->stream_id;
		if (stream_ctl.stream_payload_remainder) {
			stream_type = GADGETS_TRANSACTION_TYPE_CONT;
		} else {
			stream_type = GADGETS_TRANSACTION_TYPE_LAST;
		}

		encode_header(&tx_buffer[0], stream, stream_type, payload_size, 0);
		memcpy(
			&tx_buffer[header_size],
			&stream_ctl.stream_payload[
				stream_ctl.stream_payload_offset],
			payload_size);

		stream_ctl.stream_payload_offset += payload_size;

		err = bt_gadgets_send_perform(
			conn, tx_buffer, header_size + payload_size);
		if (err) {
			LOG_WRN("Failed to send continuation packet");
			transmit_success = false;
		} else {
			return;
		}
	}

	atomic_set(&transmit_busy, false);

	gadgets_cb->sent_cb(
		conn,
		/* In case of fragmented transmission */
		user_data == tx_buffer ? stream_ctl.stream_payload : user_data,
		transmit_success);
}

static ssize_t on_cccd_write(
	struct bt_conn *conn, const struct bt_gatt_attr *attr, uint16_t value)
{
	bool notif_enabled = (value & BT_GATT_CCC_NOTIFY) != 0;

	gadgets_cb->ccc_cb(conn, notif_enabled);

	return sizeof(value);
}

/* GADGETS Service Declaration
 * RX data direction = from this Service to GATT Client
 * TX data direction  = from GATT Client to this Service
 */
BT_GATT_SERVICE_DEFINE(
	gadgets_svc,
	BT_GATT_PRIMARY_SERVICE(BT_UUID_GADGETS),
	BT_GATT_CHARACTERISTIC(
		BT_UUID_GADGETS_RX,
		BT_GATT_CHRC_NOTIFY | BT_GATT_CHRC_READ,
		BT_GATT_PERM_READ_ENCRYPT,
		NULL, NULL, NULL),
	BT_GATT_CCC_MANAGED(
		((struct _bt_gatt_ccc[])
	{ BT_GATT_CCC_INITIALIZER(NULL, on_cccd_write, NULL) }),
		(BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT)),
	BT_GATT_CHARACTERISTIC(
		BT_UUID_GADGETS_TX,
		BT_GATT_CHRC_WRITE |
		BT_GATT_CHRC_WRITE_WITHOUT_RESP,
		BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT,
		NULL, on_receive, NULL),
);

static int bt_gadgets_send_perform(
	struct bt_conn *conn, void *data, uint16_t size)
{
	const struct bt_gatt_attr *attr = &gadgets_svc.attrs[2];
	struct bt_gatt_notify_params params = {
		.attr = attr,
		.data = data,
		.len = size,
		.func = on_sent,
		.user_data = data,
	};

	return bt_gatt_notify_cb(conn, &params);
}

int bt_gadgets_stream_send(
	struct bt_conn *conn,
	enum bt_gadgets_stream_id stream,
	void *data,
	uint16_t size)
{
	uint16_t max_packet_size;
	uint16_t payload_size;
	uint16_t header_size;
	int err;

	if (conn == NULL) {
		return -EINVAL;
	}

	if (atomic_set(&transmit_busy, true)) {
		return -EBUSY;
	}

	__ASSERT_NO_MSG(stream_ctl.stream_payload_remainder == 0);

	tx_sequence_number = 0;

	max_packet_size = bt_gadgets_max_send(conn);
	stream_ctl.stream_payload = data;

	header_size = sizeof(struct gadgets_header_first);
	if ((header_size + size) > max_packet_size) {
		stream_ctl.stream_payload_remainder =
			size + header_size - max_packet_size;
	} else {
		stream_ctl.stream_payload_remainder = 0;
	}

	payload_size = size - stream_ctl.stream_payload_remainder;
	stream_ctl.stream_payload_offset = payload_size;

	encode_header(&tx_buffer[0], stream, GADGETS_TRANSACTION_TYPE_FIRST, payload_size, size);
	memcpy(&tx_buffer[header_size], data, payload_size);

	err = bt_gadgets_send_perform(
		conn, tx_buffer, header_size + payload_size);
	if (err) {
		atomic_set(&transmit_busy, false);
	}

	return err;
}

int bt_gadgets_send(struct bt_conn *conn, void *data, uint16_t size)
{
	int err;

	if (atomic_set(&transmit_busy, true)) {
		return -EBUSY;
	}

	__ASSERT_NO_MSG(stream_ctl.stream_payload_remainder == 0);

	err = bt_gadgets_send_perform(conn, data, size);
	if (err) {
		atomic_set(&transmit_busy, false);
	}

	return err;
}

int bt_gadgets_init(const struct bt_gadgets_cb *callbacks)
{
	if (callbacks == NULL ||
	    callbacks->control_stream_cb == NULL ||
	    callbacks->alexa_stream_cb == NULL ||
	    callbacks->sent_cb == NULL ||
	    callbacks->ccc_cb == NULL) {
		return -EINVAL;
	}

	gadgets_cb = callbacks;

	atomic_set(&transmit_busy, false);

	return 0;
}
