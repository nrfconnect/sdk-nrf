/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cbor.h>
#include <errno.h>
#include <kernel.h>
#include <zephyr/types.h>

#include <bluetooth/addr.h>

#if defined(NRF5340_XXAA_NETWORK)
#include <bluetooth/services/nus.h>
#endif

#include "bt_ser.h"
#include "rpmsg.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(bt_ser);

#define BT_CMD_GATT_NUS_SEND_ARRAY_SIZE 0x03
#define BT_EVT_SEND_ARRAY_SIZE 0x05
#define BT_RSP_RESULT_ARRAY_SIZE 0x02
#define BT_NUS_INIT_CMD_ARRAY_SIZE 0x02

#define BT_CMD_BT_NUS_INIT 0x01
#define BT_CMD_GATT_NUS_SEND 0x02

#define BT_EVENT_CONNECTED 0x01
#define BT_EVENT_DISCONNECTED 0x02
#define BT_EVENT_DATA_RECEIVED 0x03

#define BT_TYPE_CMD 0x01
#define BT_TYPE_EVENT 0x02
#define BT_TYPE_RSP 0x03

#define APP_TO_NETWORK_MAX_PKT_SIZE CONFIG_APP_TO_NETWORK_MAX_PACKET_SIZE
#define NETWORK_TO_APP_MAX_PKT_SIZE CONFIG_NETWORK_TO_APP_MAX_PACKET_SIZE

#define APP_WAIT_FOR_RSP_TIME CONFIG_APP_WAIT_FOR_RESPONSE_TIME

K_SEM_DEFINE(rsp_sem, 0, 1);

typedef int (*cmd_rsp_handler_t)(CborValue *it);
typedef int (*cmd_handler_t)(CborValue *it, CborEncoder *encoder);

struct cmd_list {
	u8_t cmd;
	cmd_handler_t func;
};

static const struct bt_nus_cb *bt_cb;
static int return_value;
static cmd_rsp_handler_t rsp_handler;

static int response_code_encode(CborEncoder *encoder, int rsp)
{
	int err;
	CborEncoder array;

	err = cbor_encoder_create_array(encoder, &array,
					BT_RSP_RESULT_ARRAY_SIZE);
	if (err) {
		return -EFAULT;
	}

	err = cbor_encode_simple_value(&array, BT_TYPE_RSP);
	if (err) {
		return -EFAULT;
	}

	if (cbor_encode_int(&array, rsp)) {
		return -EFAULT;
	}

	err = cbor_encoder_close_container(encoder, &array);
	if (err) {
		return -EFAULT;
	}

	return 0;
}

#if defined(NRF5340_XXAA_NETWORK)
__weak void bt_ready(int err)
{
	LOG_INF("Bluetooth initialized, err %d.", err);
}

/**@brief NUS init command handler. This function has be of
 *        @ref cmd_handler_t type.
 */
static int bt_cmd_bt_nus_init(CborValue *it, CborEncoder *encoder)
{
	int err;

	err = bt_enable(bt_ready);
	if (err) {
		LOG_ERR("Failed to enable Bluetooth.");
	}

	return err;
}

/**@brief NUS send command handler. This function has be of
 *        @ref cmd_handler_t type.
 */
static int bt_cmd_gatt_nus_exec(CborValue *it, CborEncoder *encoder)
{
	ARG_UNUSED(encoder);

	u8_t buf[APP_TO_NETWORK_MAX_PKT_SIZE];
	size_t len = sizeof(buf);

	if (!cbor_value_is_byte_string(it) ||
	    cbor_value_copy_byte_string(it, buf, &len, it) != CborNoError) {
		return -EINVAL;
	}

	return bt_gatt_nus_send(NULL, buf, len);
}

static const struct cmd_list ser_cmd_list[] = {
	{BT_CMD_GATT_NUS_SEND, bt_cmd_gatt_nus_exec},
	{BT_CMD_BT_NUS_INIT, bt_cmd_bt_nus_init},
};
#else
static const struct cmd_list ser_cmd_list[0];
#endif /* defined(NRF5340_XXAA_NETWORK) */

static cmd_handler_t cmd_handler_get(u8_t cmd)
{
	cmd_handler_t cmd_handler = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(ser_cmd_list); i++) {
		if (cmd == ser_cmd_list[i].cmd) {
			cmd_handler = ser_cmd_list[i].func;
			break;
		}
	}

	return cmd_handler;
}

static int cmd_execute(u8_t cmd, CborValue *it, CborEncoder *encoder)
{
	int err;
	cmd_handler_t cmd_handler;

	cmd_handler = cmd_handler_get(cmd);

	if (cmd_handler) {
		err = cmd_handler(it, encoder);
	} else {
		LOG_ERR("Unsupported command received");
		err = -ENOTSUP;
	}

	return response_code_encode(encoder, err);
}

static int bt_nus_init_encode(u8_t *buf, size_t *buf_len)
{
	CborError err;
	CborEncoder encoder;
	CborEncoder array;

	cbor_encoder_init(&encoder, buf, *buf_len, 0);
	err = cbor_encoder_create_array(&encoder, &array,
					BT_NUS_INIT_CMD_ARRAY_SIZE);
	if (err) {
		goto error;
	}

	err = cbor_encode_simple_value(&array, BT_TYPE_CMD);
	if (err) {
		goto error;
	}

	err = cbor_encode_simple_value(&array, BT_CMD_BT_NUS_INIT);
	if (err) {
		goto error;
	}

	err = cbor_encoder_close_container(&encoder, &array);
	if (err) {
		goto error;
	}

	*buf_len = cbor_encoder_get_buffer_size(&encoder, buf);

	return 0;

error:
	LOG_ERR("Encoding %0x02x command error %d", BT_CMD_BT_NUS_INIT, err);
	return err;

}

static int bt_send_encode(const u8_t *data, u16_t len,
			  u8_t *buf, size_t *buf_len)
{
	CborError err;
	CborEncoder encoder;
	CborEncoder array;

	cbor_encoder_init(&encoder, buf, *buf_len, 0);
	err = cbor_encoder_create_array(&encoder, &array,
					BT_CMD_GATT_NUS_SEND_ARRAY_SIZE);
	if (err) {
		goto error;
	}

	err = cbor_encode_simple_value(&array, BT_TYPE_CMD);
	if (err) {
		goto error;
	}

	err = cbor_encode_simple_value(&array, BT_CMD_GATT_NUS_SEND);
	if (err) {
		goto error;
	}

	err = cbor_encode_byte_string(&array, data, len);
	if (err) {
		goto error;
	}

	err = cbor_encoder_close_container(&encoder, &array);
	if (err) {
		goto error;
	}

	*buf_len = cbor_encoder_get_buffer_size(&encoder, buf);

	return 0;

error:
	LOG_ERR("Encoding 0x%02x command error %d", BT_CMD_GATT_NUS_SEND, err);
	return err;
}

static int bt_send_rsp(CborValue *it)
{
	int error;

	if (!cbor_value_is_integer(it) ||
		cbor_value_get_int(it, &error) != CborNoError) {
		return -EINVAL;
	}

	return_value = error;

	return 0;
}

static int cmd_send(const u8_t *data, size_t length,
		    cmd_rsp_handler_t rsp)
{
	int err = 0;

	rsp_handler = rsp;

	if (ipc_transmit(data, length) < 0) {
		return -EFAULT;
	}

	if (rsp) {
		err = k_sem_take(&rsp_sem, K_MSEC(APP_WAIT_FOR_RSP_TIME));
		if (err) {
			return err;
		}

		LOG_DBG("Got response with return value %d", return_value);

		return return_value;
	}

	return err;
}

static int evt_send(const bt_addr_le_t *addr, u8_t evt,
		    const u8_t *evt_data, size_t length)
{
	int err;
	CborEncoder encoder;
	CborEncoder array;
	u8_t buf[NETWORK_TO_APP_MAX_PKT_SIZE];
	size_t buf_len = sizeof(buf);

	cbor_encoder_init(&encoder, buf, buf_len, 0);
	err = cbor_encoder_create_array(&encoder, &array,
					BT_EVT_SEND_ARRAY_SIZE);
	if (err) {
		goto error;
	}

	err = cbor_encode_simple_value(&array, BT_TYPE_EVENT);
	if (err) {
		goto error;
	}

	err = cbor_encode_simple_value(&array, evt);
	if (err) {
		goto error;
	}

	err = cbor_encode_simple_value(&array, addr->type);
	if (err) {
		goto error;
	}

	err = cbor_encode_byte_string(&array, addr->a.val,
				      sizeof(addr->a.val));
	if (err) {
		goto error;
	}

	err = cbor_encode_byte_string(&array, evt_data, length);
	if (err) {
		goto error;
	}

	err = cbor_encoder_close_container(&encoder, &array);
	if (err) {
		goto error;
	}

	buf_len = cbor_encoder_get_buffer_size(&encoder, buf);

	LOG_DBG("Sending 0x%02x event", evt);

	return ipc_transmit(buf, buf_len);

error:
	LOG_ERR("Send event 0x%02x error %d", evt, err);
	return err;
}

static void connected_evt(const bt_addr_le_t *addr, u8_t err)
{
	if (bt_cb->bt_connected) {
		bt_cb->bt_connected(addr, err);
	}
}

static void disconnected_evt(const bt_addr_le_t *addr, u8_t err)
{
	if (bt_cb->bt_disconnected) {
		bt_cb->bt_disconnected(addr, err);
	}
}

static void bt_received_evt(const bt_addr_le_t *addr, const u8_t *data,
			    size_t length)
{
	if (bt_cb->bt_received) {
		bt_cb->bt_received(addr, data, length);
	}
}

static int event_parse(u8_t evt, CborValue *it)
{
	u8_t data[NETWORK_TO_APP_MAX_PKT_SIZE];
	bt_addr_le_t addr;
	size_t data_len = sizeof(data);
	size_t addr_len = sizeof(addr.a.val);

	if (!cbor_value_is_simple_type(it) ||
		cbor_value_get_simple_type(it, &addr.type) != CborNoError) {
		return -EINVAL;
	}

	if (cbor_value_advance_fixed(it) != CborNoError) {
		return -EFAULT;
	}

	if (!cbor_value_is_byte_string(it) ||
	    cbor_value_copy_byte_string(it, addr.a.val,
		    &addr_len, it) != CborNoError) {
		return -EINVAL;
	}

	if (!cbor_value_is_byte_string(it) ||
	    cbor_value_copy_byte_string(it, data,
		    &data_len, it) != CborNoError) {
		return -EINVAL;
	}

	LOG_DBG("Event: 0x%02x", evt);

	switch (evt) {
	case BT_EVENT_CONNECTED:
		if (data_len != 1) {
			return -EINVAL;
		}

		connected_evt(&addr, data[0]);

		break;

	case BT_EVENT_DISCONNECTED:
		if (data_len != 1) {
			return -EINVAL;
		}

		disconnected_evt(&addr, data[0]);

		break;

	case BT_EVENT_DATA_RECEIVED:
		if (data_len < 1) {
			return -EINVAL;
		}

		bt_received_evt(&addr, data, data_len);

		break;

	default:
		return -ENOTSUP;
	}

	return 0;
}

static int rsp_send(CborEncoder *encoder, const u8_t *buf)
{
	size_t buf_len;

	buf_len = cbor_encoder_get_buffer_size(encoder, buf);

	return ipc_transmit(buf, buf_len);
}

static int bt_response_parse(CborValue *it)
{
	int err;

	err = rsp_handler(it);
	k_sem_give(&rsp_sem);

	return err;
}

static int bt_event_parse(CborValue *it)
{
	u8_t event;

	if (!cbor_value_is_simple_type(it) ||
		cbor_value_get_simple_type(it, &event) != CborNoError) {
		return -EINVAL;
	}

	if (cbor_value_advance_fixed(it) != CborNoError) {
		return -EFAULT;
	}

	return event_parse(event, it);
}

static int bt_cmd_parse(CborValue *it)
{
	u8_t cmd;
	int err;
	CborEncoder encoder;
	u8_t buf[NETWORK_TO_APP_MAX_PKT_SIZE];
	size_t buf_len = sizeof(buf);

	cbor_encoder_init(&encoder, buf, buf_len, 0);

	if (!cbor_value_is_simple_type(it) ||
		cbor_value_get_simple_type(it, &cmd) != CborNoError) {
		return -EINVAL;
	}

	if (cbor_value_advance_fixed(it) != CborNoError) {
		return -EFAULT;
	}

	err = cmd_execute(cmd, it, &encoder);
	if (err) {
		return err;
	}

	LOG_DBG("Received command 0x%02x", cmd);

	return rsp_send(&encoder, buf);
}

int bt_nus_rx_parse(const u8_t *data, size_t len)
{
	int err;
	CborParser parser;
	CborValue value;
	CborValue recursed;
	u8_t packet_type;

	err = cbor_parser_init(data, len, 0, &parser, &value);
	if (err) {
		return err;
	}

	if (!cbor_value_is_array(&value)) {
		return -EINVAL;
	}

	if (cbor_value_enter_container(&value, &recursed) != CborNoError) {
		return -EINVAL;
	}

	/* Get BLE packet type. */
	if (!cbor_value_is_simple_type(&recursed) ||
		cbor_value_get_simple_type(&recursed,
			&packet_type) != CborNoError) {
		return -EINVAL;
	}

	cbor_value_advance_fixed(&recursed);

	switch (packet_type) {
	case BT_TYPE_CMD:
		LOG_DBG("Command received");
		return bt_cmd_parse(&recursed);

	case BT_TYPE_EVENT:
		LOG_DBG("Event received");
		return bt_event_parse(&recursed);

	case BT_TYPE_RSP:
		LOG_DBG("Response received");
		return bt_response_parse(&recursed);

	default:
		LOG_ERR("Unknown packet received");
		return -ENOTSUP;

	}

	/* Be sure that we unpacked all data from the array */
	if (!cbor_value_at_end(&recursed)) {
		LOG_ERR("Received more data than expected");
		return -EFAULT;
	}

	return cbor_value_leave_container(&value, &recursed);
}

void bt_nus_callback_register(const struct bt_nus_cb *cb)
{
	if (!cb) {
		return;
	}

	bt_cb = cb;
}

int bt_nus_init(void)
{
	int err;
	u8_t buf[APP_TO_NETWORK_MAX_PKT_SIZE];
	size_t buf_len = sizeof(buf);

	err = bt_nus_init_encode(buf, &buf_len);
	if (err) {
		return err;
	}

	return cmd_send(buf, buf_len, bt_send_rsp);
}

int bt_nus_transmit(const u8_t *data, size_t length)
{
	int err;
	u8_t buf[APP_TO_NETWORK_MAX_PKT_SIZE];
	size_t buf_len = sizeof(buf);

	if (!data || length < 1) {
		return -EINVAL;
	}

	err = bt_send_encode(data, length, buf, &buf_len);
	if (err) {
		return err;
	}

	return cmd_send(buf, buf_len, bt_send_rsp);
}

int bt_nus_connection_evt_send(const bt_addr_le_t *addr, u8_t error)
{
	if (!addr) {
		return -EINVAL;
	}

	return evt_send(addr, BT_EVENT_CONNECTED, &error, sizeof(error));
}

int bt_nus_disconnection_evt_send(const bt_addr_le_t *addr, u8_t reason)
{
	if (!addr) {
		return -EINVAL;
	}

	return evt_send(addr, BT_EVENT_DISCONNECTED, &reason, sizeof(reason));
}

int bt_nus_received_evt_send(const bt_addr_le_t *addr, const u8_t *data,
			     size_t length)
{
	if (!addr || !data) {
		return -EINVAL;
	}

	if (length < 1) {
		return -EINVAL;
	}

	return evt_send(addr, BT_EVENT_DATA_RECEIVED, data, length);
}
