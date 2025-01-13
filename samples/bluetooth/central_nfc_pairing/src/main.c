/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic Central NFC pairing sample
 */

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <bluetooth/scan.h>

#include <nfc/tnep/ch.h>
#include <nfc/ndef/le_oob_rec_parser.h>

#include <st25r3911b_nfca.h>

#include <zephyr/settings/settings.h>

#include <dk_buttons_and_leds.h>

#include "nfc_poller.h"

#define NFC_NDEF_LE_OOB_REC_PARSER_BUFF_SIZE 150
#define K_POOL_EVENTS_CNT (ST25R3911B_NFCA_EVENT_CNT + 1)

#define KEY_BOND_REMOVE_MASK DK_BTN4_MSK
#define KEY_NFC_FIELD_ON_MASK DK_BTN3_MSK

static struct k_poll_signal pair_signal;
static struct k_poll_event events[K_POOL_EVENTS_CNT];

static bool use_remote_tk;
static struct bt_le_oob oob_local;
static uint8_t tk_value[NFC_NDEF_LE_OOB_REC_TK_LEN];
static uint8_t remote_tk_value[NFC_NDEF_LE_OOB_REC_TK_LEN];
static struct bt_le_oob oob_remote;
static struct bt_conn *default_conn;

static int tk_value_generate(void)
{
	int err;

	err = bt_rand(tk_value, sizeof(tk_value));
	if (err) {
		printk("Random TK value generation failed: %d\n", err);
	}

	return err;
}

static void pair_key_generate_init(void)
{
	k_poll_signal_init(&pair_signal);
	k_poll_event_init(&events[ST25R3911B_NFCA_EVENT_CNT],
			  K_POLL_TYPE_SIGNAL, K_POLL_MODE_NOTIFY_ONLY,
			  &pair_signal);
}

static int paring_key_generate(void)
{
	int err;

	printk("Generating new pairing keys.\n");

	err = bt_le_oob_get_local(BT_ID_DEFAULT, &oob_local);
	if (err) {
		printk("Error while fetching local OOB data: %d\n", err);
	}

	return tk_value_generate();
}

static void paring_key_process(void)
{
	int err;

	if (events[ST25R3911B_NFCA_EVENT_CNT].state == K_POLL_STATE_SIGNALED) {
		err = paring_key_generate();
		if (err) {
			printk("Pairing key generation error: %d\n", err);
		}

		k_poll_signal_reset(events[ST25R3911B_NFCA_EVENT_CNT].signal);
		events[ST25R3911B_NFCA_EVENT_CNT].state =
						K_POLL_STATE_NOT_READY;
	}
}

static void scan_filter_match(struct bt_scan_device_info *device_info,
			      struct bt_scan_filter_match *filter_match,
			      bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	printk("Filters matched. Address: %s connectable: %s\n",
		addr, connectable ? "yes" : "no");
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

static void scan_filter_no_match(struct bt_scan_device_info *device_info,
				 bool connectable)
{
	int err;
	struct bt_conn *conn = NULL;
	char addr[BT_ADDR_LE_STR_LEN];

	if (device_info->recv_info->adv_type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));
		printk("Direct advertising received from %s\n", addr);
		bt_scan_stop();

		err = bt_conn_le_create(device_info->recv_info->addr,
					BT_CONN_LE_CREATE_CONN,
					device_info->conn_param, &conn);

		if (!err) {
			default_conn = bt_conn_ref(conn);
			bt_conn_unref(conn);
		}
	}
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, scan_filter_no_match,
		scan_connecting_error, scan_connecting);


static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		printk("Failed to connect to %s, 0x%02x %s\n", addr, conn_err,
		       bt_hci_err_to_str(conn_err));
		if (conn == default_conn) {
			bt_conn_unref(default_conn);
			default_conn = NULL;

			/* This demo doesn't require active scan */
			err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err) {
				printk("Scanning failed to start (err %d)\n",
				       err);
			}
		}

		return;
	}

	printk("Connected: %s\n", addr);

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err) {
		printk("Failed to set security: %d\n", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s, reason 0x%02x %s\n", addr, reason, bt_hci_err_to_str(reason));

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
			     enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err) {
		printk("Security changed: %s level %u\n", addr, level);
	} else {
		printk("Security failed: %s level %u err %d %s\n", addr, level, err,
		       bt_security_err_to_str(err));
	}
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed
};

static void scan_init(void)
{
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
		.scan_param = NULL,
		.conn_param = BT_LE_CONN_PARAM_DEFAULT
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);
}

static void lesc_oob_data_set(struct bt_conn *conn,
			      struct bt_conn_oob_info *oob_info)
{
	int err;
	char addr[BT_ADDR_LE_STR_LEN];
	struct bt_conn_info info;

	err = bt_conn_get_info(conn, &info);
	if (err) {
		return;
	}

	struct bt_le_oob_sc_data *oob_data_local =
		oob_info->lesc.oob_config != BT_CONN_OOB_REMOTE_ONLY
					  ? &oob_local.le_sc_data
					  : NULL;
	struct bt_le_oob_sc_data *oob_data_remote =
		oob_info->lesc.oob_config != BT_CONN_OOB_LOCAL_ONLY
					  ? &oob_remote.le_sc_data
					  : NULL;

	if (oob_data_remote &&
	    bt_addr_le_cmp(info.le.remote, &oob_remote.addr)) {
		bt_addr_le_to_str(info.le.remote, addr, sizeof(addr));
		printk("No OOB data available for remote %s", addr);
		bt_conn_auth_cancel(conn);
		return;
	}

	if (oob_data_local &&
	    bt_addr_le_cmp(info.le.local, &oob_local.addr)) {
		bt_addr_le_to_str(info.le.local, addr, sizeof(addr));
		printk("No OOB data available for local %s", addr);
		bt_conn_auth_cancel(conn);
		return;
	}

	err = bt_le_oob_set_sc_data(conn, oob_data_local, oob_data_remote);
	if (err) {
		printk("Error while setting OOB data: %d\n", err);
	}
}

static void legacy_tk_value_set(struct bt_conn *conn)
{
	int err;
	const uint8_t *tk = use_remote_tk ? remote_tk_value : tk_value;

	err = bt_le_oob_set_legacy_tk(conn, tk);
	if (err) {
		printk("TK value set error: %d\n", err);
	}

	use_remote_tk = false;
}

static void auth_oob_data_request(struct bt_conn *conn,
				  struct bt_conn_oob_info *info)
{
	if (info->type == BT_CONN_OOB_LE_SC) {
		printk("LESC OOB data requested\n");
		lesc_oob_data_set(conn, info);
	}

	if (info->type == BT_CONN_OOB_LE_LEGACY) {
		printk("Legacy TK value requested\n");
		legacy_tk_value_set(conn);
	}
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}


static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing completed: %s, bonded: %d\n", addr, bonded);

	k_poll_signal_raise(&pair_signal, 0);
	bt_le_oob_set_sc_flag(false);
	bt_le_oob_set_legacy_flag(false);
}


static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing failed conn: %s, reason %d %s\n", addr, reason,
	       bt_security_err_to_str(reason));

	k_poll_signal_raise(&pair_signal, 0);
	bt_le_oob_set_sc_flag(false);
	bt_le_oob_set_legacy_flag(false);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.cancel = auth_cancel,
	.oob_data_request = auth_oob_data_request,
};

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed
};

static int check_oob_carrier(const struct nfc_tnep_ch_record *ch_record,
			     const struct nfc_ndef_record_desc **oob_data)
{
	const struct nfc_ndef_ch_ac_rec *ac_rec = NULL;

	if (!ch_record->count) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ch_record->count; i++) {
		if (nfc_ndef_le_oob_rec_check(ch_record->carrier[i])) {
			*oob_data = ch_record->carrier[i];
		}
	}

	if (!oob_data) {
		printk("Connection Handover Requester not supporting OOB BLE\n");
		return -EINVAL;
	}

	/* Look for the corresponding Alternative Carrier Record. */
	for (size_t i = 0; i < ch_record->count; i++) {
		if (((*oob_data)->id_length == ch_record->ac[i].carrier_data_ref.length) &&
		    (memcmp((*oob_data)->id,
			    ch_record->ac[i].carrier_data_ref.data,
			    (*oob_data)->id_length) == 0)) {
			ac_rec = &ch_record->ac[i];
		}
	}

	if (!ac_rec) {
		printk("No Alternative Carrier Record for OOB LE carrier\n");
		return -EINVAL;
	}

	/* Check carrier state */
	if ((ac_rec->cps != NFC_AC_CPS_ACTIVE) &&
	    (ac_rec->cps != NFC_AC_CPS_ACTIVATING)) {
		printk("LE OBB Carrier inactive\n");
		return -EINVAL;
	}

	return 0;
}

static int oob_le_data_handle(const struct nfc_ndef_record_desc *rec,
			      bool request)
{
	int err;
	const struct nfc_ndef_le_oob_rec_payload_desc *oob;
	uint8_t desc_buf[NFC_NDEF_LE_OOB_REC_PARSER_BUFF_SIZE];
	uint32_t desc_buf_len = sizeof(desc_buf);

	err = nfc_ndef_le_oob_rec_parse(rec, desc_buf,
					&desc_buf_len);
	if (err) {
		printk("Error during NDEF LE OOB Record parsing, err: %d.\n",
			err);
	}

	oob = (struct nfc_ndef_le_oob_rec_payload_desc *) desc_buf;

	nfc_ndef_le_oob_rec_printout(oob);

	if ((*oob->le_role != NFC_NDEF_LE_OOB_REC_LE_ROLE_PERIPH_ONLY) &&
	    (*oob->le_role != NFC_NDEF_LE_OOB_REC_LE_ROLE_PERIPH_PREFFERED)) {
		printk("Unsupported Device LE Role\n");
		return -EINVAL;
	}

	if (oob->le_sc_data) {
		bt_le_oob_set_sc_flag(true);
		oob_remote.le_sc_data = *oob->le_sc_data;
		bt_addr_le_copy(&oob_remote.addr, oob->addr);
	}

	if (oob->tk_value) {
		bt_le_oob_set_legacy_flag(true);
		memcpy(remote_tk_value, oob->tk_value, sizeof(remote_tk_value));
		use_remote_tk = request;
	}

	/* Remove all scanning filters. */
	bt_scan_filter_remove_all();

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_ADDR, oob->addr);
	if (err) {
		printk("Set filter on device address error: %d\n", err);
	}

	err = bt_scan_filter_enable(BT_SCAN_ADDR_FILTER, false);
	if (err) {
		return err;
	}

	return bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
}

/** .. include_startingpoint_nfc_tnep_ch_poller_rst */
static int carrier_prepare(void)
{
	static struct nfc_ndef_le_oob_rec_payload_desc rec_payload;

	NFC_NDEF_LE_OOB_RECORD_DESC_DEF(oob_rec, '0', &rec_payload);
	NFC_NDEF_CH_AC_RECORD_DESC_DEF(oob_ac, NFC_AC_CPS_ACTIVE, 1, "0", 0);

	memset(&rec_payload, 0, sizeof(rec_payload));

	rec_payload.addr = &oob_local.addr;
	rec_payload.le_sc_data = &oob_local.le_sc_data;
	rec_payload.tk_value = tk_value;
	rec_payload.local_name = bt_get_name();
	rec_payload.le_role = NFC_NDEF_LE_OOB_REC_LE_ROLE(
		NFC_NDEF_LE_OOB_REC_LE_ROLE_CENTRAL_ONLY);
	rec_payload.appearance = NFC_NDEF_LE_OOB_REC_APPEARANCE(
		CONFIG_BT_DEVICE_APPEARANCE);
	rec_payload.flags = NFC_NDEF_LE_OOB_REC_FLAGS(BT_LE_AD_NO_BREDR);

	return nfc_tnep_ch_carrier_set(&NFC_NDEF_CH_AC_RECORD_DESC(oob_ac),
				       &NFC_NDEF_LE_OOB_RECORD_DESC(oob_rec),
				       1);
}

static int tnep_ch_request_prepare(void)
{
	return carrier_prepare();
}

static int tnep_ch_select_received(const struct nfc_tnep_ch_record *ch_select,
				   bool inactive)
{
	int err;
	const struct nfc_ndef_record_desc *oob_data = NULL;

	if (!ch_select->count) {
		return -EINVAL;
	}

	/* All alternative carrier are inactive */
	if (inactive) {
		/* Try send request again. */
		return carrier_prepare();
	}

	err = check_oob_carrier(ch_select, &oob_data);
	if (err) {
		return err;
	}

	err = oob_le_data_handle(oob_data, false);
	if (err) {
		return err;
	}

	return 0;

}

static int tnep_ch_request_received(const struct nfc_tnep_ch_request *ch_req)
{
	int err;
	const struct nfc_ndef_record_desc *oob_data = NULL;

	if (!ch_req->ch_record.count) {
		return -EINVAL;
	}

	err = check_oob_carrier(&ch_req->ch_record, &oob_data);
	if (err) {
		return err;
	}

	err = oob_le_data_handle(oob_data, true);
	if (err) {
		return err;
	}

	return carrier_prepare();
}

static struct nfc_tnep_ch_cb ch_cb = {
	.request_msg_prepare = tnep_ch_request_prepare,
	.select_msg_recv = tnep_ch_select_received,
	.request_msg_recv = tnep_ch_request_received
};
/** .. include_endpoint_nfc_tnep_ch_poller_rst */

static void nfc_poller_non_tnep_msg_parse(struct nfc_ndef_msg_desc *ndef_msg)
{
	int err;
	const struct nfc_ndef_record_desc *oob_data = NULL;

	if (!ndef_msg->record_count) {
		return;
	}

	for (size_t i = 0; i < ndef_msg->record_count; i++) {
		if (nfc_ndef_le_oob_rec_check(ndef_msg->record[i])) {
			oob_data = ndef_msg->record[i];
		}
	}

	err = oob_le_data_handle(oob_data, true);
	if (err) {
		printk("OOB data handle error: %d\n", err);
	}
}

void button_changed(uint32_t button_state, uint32_t has_changed)
{
	int err;
	uint32_t buttons = button_state & has_changed;

	if (buttons & KEY_BOND_REMOVE_MASK) {
		err = bt_unpair(BT_ID_DEFAULT, NULL);
		if (err) {
			printk("Bond remove failed err: %d\n", err);
		} else {
			printk("All bond removed\n");
		}

		if (!nfc_poller_field_active()) {
			err = st25r3911b_nfca_field_on();
			if (err) {
				printk("NFC Field on error: %d\n", err);
			}
		}
	}

	if (buttons & KEY_NFC_FIELD_ON_MASK) {
		if (!nfc_poller_field_active()) {
			err = st25r3911b_nfca_field_on();
			if (err) {
				printk("NFC Field on error: %d\n", err);
			}
		} else {
			printk("NFC TNEP Device is busy now. Please try after finish current NFC operation\n");
		}
	}
}

int main(void)
{
	int err;

	printk("Starting NFC Central Pairing sample\n");

	/* Configure buttons */
	err = dk_buttons_init(button_changed);
	if (err) {
		printk("Cannot init buttons (err %d\n", err);
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	scan_init();

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err) {
		printk("Failed to register authorization callbacks.\n");
		return 0;
	}

	err = bt_conn_auth_info_cb_register(&conn_auth_info_callbacks);
	if (err) {
		printk("Failed to register authorization info callbacks.\n");
		return 0;
	}

	err = paring_key_generate();
	if (err) {
		return 0;
	}

	err = nfc_tnep_ch_service_init(&ch_cb);
	if (err) {
		printk("Connection Handover service initialization error: %d\n",
		       err);
	}

	pair_key_generate_init();

	err = nfc_poller_init(events, nfc_poller_non_tnep_msg_parse);
	if (err) {
		printk("NFC Poller initialization error: %d\n", err);
	}

	for (;;) {
		k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		err = nfc_poller_process();
		if (err) {
			return 0;
		}
		paring_key_process();
	}
}
