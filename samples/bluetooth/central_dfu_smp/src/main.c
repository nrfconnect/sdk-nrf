/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <cbor.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/gatt_dm.h>
#include <sys/byteorder.h>
#include <bluetooth/scan.h>
#include <bluetooth/services/dfu_smp_c.h>
#include <dk_buttons_and_leds.h>


#define CBOR_BUFFER_SIZE 128

#define KEY_ECHO_MASK  DK_BTN1_MSK


static struct bt_conn *default_conn;
static struct bt_gatt_dfu_smp_c dfu_smp_c;
static struct bt_gatt_exchange_params exchange_params;

/* Buffer for response */
struct smp_buffer {
	struct dfu_smp_header header;
	u8_t payload[CBOR_BUFFER_SIZE];
};
static struct smp_buffer smp_rsp_buff;


static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %s\n",
		addr, connectable ? "yes" : "no");

	err = bt_scan_stop();
	if (err) {
		printk("Stop LE scan failed (err %d)\n", err);
	}
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	printk("Connecting failed\n");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
			    struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
		scan_connecting_error, scan_connecting);

static void discovery_completed_cb(struct bt_gatt_dm *dm,
				   void *context)
{
	int err;

	printk("The discovery procedure succeeded\n");

	bt_gatt_dm_data_print(dm);

	err = bt_gatt_dfu_smp_c_handles_assign(dm, &dfu_smp_c);
	if (err) {
		printk("Could not init DFU SMP client object, error: %d\n",
		       err);
	}

	err = bt_gatt_dm_data_release(dm);
	if (err) {
		printk("Could not release the discovery data, error "
		       "code: %d\n", err);
	}
}

static void discovery_service_not_found_cb(struct bt_conn *conn,
					   void *context)
{
	printk("The service could not be found during the discovery\n");
}

static void discovery_error_found_cb(struct bt_conn *conn,
				     int err,
				     void *context)
{
	printk("The discovery procedure failed with %d\n", err);
}

static const struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_completed_cb,
	.service_not_found = discovery_service_not_found_cb,
	.error_found = discovery_error_found_cb,
};

static void exchange_func(struct bt_conn *conn, u8_t err,
			  struct bt_gatt_exchange_params *params)
{
	printk("MTU exchange %s\n", err == 0 ? "successful" : "failed");
	printk("Current MTU: %u\n", bt_gatt_get_mtu(conn));
}

static void connected(struct bt_conn *conn, u8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}

	printk("Connected: %s\n", addr);

	if (bt_conn_set_security(conn, BT_SECURITY_L2)) {
		printk("Failed to set security\n");
	}

	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err) {
		printk("MTU exchange failed (err %d)\n", err);
	} else {
		printk("MTU exchange pending\n");
	}

	if (conn == default_conn) {
		err = bt_gatt_dm_start(conn,
				       DFU_SMP_UUID_SERVICE,
				       &discovery_cb,
				       NULL);
		if (err) {
			printk("Could not start the discovery procedure "
			       "(err %d)\n", err);
		}
	}
}

static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason %u)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	/* This demo doesn't require active scan */
	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d\n", addr, level,
			err);
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static void scan_init(void)
{
	int err;

	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID,
				 DFU_SMP_UUID_SERVICE);
	if (err) {
		printk("Scanning filters cannot be set (err %d)\n", err);

		return;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err) {
		printk("Filters cannot be turned on (err %d)\n", err);
	}
}


static void dfu_smp_c_on_error(struct bt_gatt_dfu_smp_c *dfu_smp_c, int err)
{
	printk("DFU SMP generic error: %d\n", err);
}


static const struct bt_gatt_dfu_smp_c_init_params init_params = {
	.error_cb = dfu_smp_c_on_error
};

static CborError cbor_stream(void *token, const char *fmt, ...)
{
	va_list ap;

	(void)token;
	va_start(ap, fmt);
	vprintk(fmt, ap);
	va_end(ap);

	return CborNoError;
}


static void smp_echo_rsp_proc(struct bt_gatt_dfu_smp_c *dfu_smp_c)
{
	u8_t *p_outdata = (u8_t *)(&smp_rsp_buff);
	const struct bt_gatt_dfu_smp_rsp_state *rsp_state;

	rsp_state = bt_gatt_dfu_smp_c_rsp_state(dfu_smp_c);
	printk("Echo response part received, size: %zu.\n",
	       rsp_state->chunk_size);

	if (rsp_state->offset + rsp_state->chunk_size > sizeof(smp_rsp_buff)) {
		printk("Response size buffer overflow\n");
	} else {
		p_outdata += rsp_state->offset;
		memcpy(p_outdata,
		       rsp_state->data,
		       rsp_state->chunk_size);
	}

	if (bt_gatt_dfu_smp_c_rsp_total_check(dfu_smp_c)) {
		printk("Total response received - decoding\n");
		if (smp_rsp_buff.header.op != 3 /* WRITE RSP*/) {
			printk("Unexpected operation code (%u)!\n",
			       smp_rsp_buff.header.op);
			return;
		}
		u16_t group = ((u16_t)smp_rsp_buff.header.group_h8) << 8 |
				      smp_rsp_buff.header.group_l8;
		if (group != 0 /* OS */) {
			printk("Unexpected command group (%u)!\n", group);
			return;
		}
		if (smp_rsp_buff.header.id != 0 /* ECHO */) {
			printk("Unexpected command (%u)",
			       smp_rsp_buff.header.id);
			return;
		}
		size_t payload_len = ((u16_t)smp_rsp_buff.header.len_h8) << 8 |
				      smp_rsp_buff.header.len_l8;

		CborError cbor_error;
		CborParser parser;
		CborValue value;

		cbor_error = cbor_parser_init(smp_rsp_buff.payload,
					      payload_len,
					      0,
					      &parser,
					      &value);
		if (cbor_error != CborNoError) {
			printk("CBOR parser initialization failed (err: %d)\n",
			       cbor_error);
			return;
		}
		cbor_error =
			cbor_value_to_pretty_stream(cbor_stream, NULL,
						    &value,
						    CborPrettyDefaultFlags);
		printk("\n");
		if (cbor_error != CborNoError) {
			printk("Cannot print received CBOR stream (err: %d)\n",
			       cbor_error);
			return;
		}
	}

}

static int send_smp_echo(struct bt_gatt_dfu_smp_c *dfu_smp_c,
			 const char *string)
{
	static struct smp_buffer smp_cmd;
	CborEncoder cbor, cbor_map;
	size_t payload_len;

	cbor_encoder_init(&cbor, smp_cmd.payload, sizeof(smp_cmd.payload), 0);
	cbor_encoder_create_map(&cbor, &cbor_map, 1);
	cbor_encode_text_stringz(&cbor_map, "d");
	cbor_encode_text_stringz(&cbor_map, string);
	cbor_encoder_close_container(&cbor, &cbor_map);

	payload_len = cbor_encoder_get_buffer_size(&cbor, smp_cmd.payload);

	smp_cmd.header.op = 2; /* Write */
	smp_cmd.header.flags = 0;
	smp_cmd.header.len_h8 = (u8_t)((payload_len >> 8) & 0xFF);
	smp_cmd.header.len_l8 = (u8_t)((payload_len >> 0) & 0xFF);
	smp_cmd.header.group_h8 = 0;
	smp_cmd.header.group_l8 = 0; /* OS */
	smp_cmd.header.seq = 0;
	smp_cmd.header.id  = 0; /* ECHO */

	return bt_gatt_dfu_smp_c_command(dfu_smp_c,
					 smp_echo_rsp_proc,
					 sizeof(smp_cmd.header) + payload_len,
					 &smp_cmd);
}


static void button_echo(bool state)
{
	if (state) {
		static unsigned int echo_cnt;
		char buffer[32];
		int ret;

		++echo_cnt;
		printk("Echo test: %d\n", echo_cnt);
		snprintk(buffer, sizeof(buffer), "Echo message: %u", echo_cnt);
		ret = send_smp_echo(&dfu_smp_c, buffer);
		if (ret) {
			printk("Echo command send error (err: %d)\n", ret);
		}
	}
}


static void button_handler(u32_t button_state, u32_t has_changed)
{
	if (has_changed & KEY_ECHO_MASK) {
		button_echo(button_state & KEY_ECHO_MASK);
	}
}


void main(void)
{
	int err;

	printk("Starting Bluetooth Central DFU SMP example\n");

	bt_gatt_dfu_smp_c_init(&dfu_smp_c, &init_params);

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	scan_init();
	bt_conn_cb_register(&conn_callbacks);

	err = dk_buttons_init(button_handler);
	if (err) {
		printk("Failed to initialize buttons (err %d)\n", err);
		return;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err) {
		printk("Scanning failed to start (err %d)\n", err);
		return;
	}

	printk("Scanning successfully started\n");
}
