/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief ESL client shell module
 */

#include <stdio.h>
#include <stdlib.h>
#include <zephyr/bluetooth/conn.h>
#include <host/settings.h>
#include <host/keys.h>
#include <host/id.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/crc.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/init.h>
#include <zephyr/logging/log.h>

#include "esl_client.h"
#include "esl_dummy_cmd.h"
#include "esl_internal.h"
#include "esl_client_internal.h"
#include "esl_client_tag_storage.h"
#include "esl_client_settings.h"

LOG_MODULE_DECLARE(esl_c);
#define ESL_SHELL_MODULE "esl_c"
extern bt_security_t esl_ap_security_level;
extern bool esl_c_auto_ap_mode;
extern bool esl_c_write_wo_rsp;
extern uint16_t esl_c_tags_per_group;
extern uint8_t groups_per_button;
static struct {
	const struct shell *shell;
} context;

static int cmd_esl_c_state(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "ESL client state ? there is no such thing\n");

	return 0;
}

static int cmd_esl_c_dump(const struct shell *shell, size_t argc, char *argv[])
{
	uint16_t idx, idx2;
	char addr[BT_ADDR_LE_STR_LEN];
	char handles_str[256];
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	shell_fprintf(shell, SHELL_NORMAL, "Dump ESL AP data\n");
	shell_fprintf(shell, SHELL_NORMAL, "AP Sync Key\n");
	shell_hexdump(shell, esl_c_obj->esl_ap_key.key_v, EAD_KEY_MATERIAL_LEN);
	shell_fprintf(shell, SHELL_NORMAL, "Randomizer\n");
	shell_hexdump(shell, esl_c_obj->esl_randomizer, EAD_RANDOMIZER_LEN);
	shell_fprintf(shell, SHELL_NORMAL, "#ABSTIME:%u\n", esl_c_get_abs_time());
	shell_fprintf(shell, SHELL_NORMAL, "Connected Tag counts %d\n",
		      esl_c_obj->current_esl_count);
	for (idx = 0; idx < esl_c_get_free_conn_idx(); idx++) {
		bt_addr_le_to_str(&esl_c_obj->gatt[idx].esl_device.ble_addr, addr, sizeof(addr));

		shell_fprintf(shell, SHELL_NORMAL, "Tag %d\n", idx);
		shell_fprintf(shell, SHELL_NORMAL, "ESL handles:");
		for (idx2 = 0; idx2 < NUMBERS_OF_HANDLE; idx2++) {
			shell_fprintf(shell, SHELL_NORMAL, " 0x%04x",
				      esl_c_obj->gatt[idx].gatt_handles.handles[idx2]);
		}

		shell_fprintf(shell, SHELL_NORMAL, "\n");
		shell_fprintf(shell, SHELL_NORMAL, "OTS client handles:\n");
		dump_ots_client_handles(handles_str, esl_c_obj->gatt[idx].otc);
		shell_fprintf(shell, SHELL_NORMAL, "%s", handles_str);
		shell_fprintf(shell, SHELL_NORMAL, "BLE ADDR:%s\n", addr);
		esl_c_dump_chrc(&esl_c_obj->gatt[idx].esl_device);
	}

	return 0;
}

static int cmd_esl_c_bond_dump(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Dump bonded device\n");
	bt_esl_c_bond_dump(NULL);

	return 0;
}

static int cmd_esl_c_unassociated(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr>\n");
		return -ENOEXEC;
	}

	int err;
	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	uint16_t conn_idx = esl_addr_to_conn_idx(esl_addr);

	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}
	err = bt_c_esl_unassociate(esl_c_get_esl_c_obj(), conn_idx);

	return err;
}

static int cmd_esl_c_unbond_all(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_c_esl_unbond(-1);

	return err;
}

static int cmd_esl_c_force_unassociated(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	int conn_idx = esl_addr_to_conn_idx(esl_addr);

	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}
	bt_c_esl_post_unassociate(conn_idx);

	return 0;
}

static int cmd_pawr_sync_buf_status(const struct shell *shell, size_t argc, char *argv[])
{
	int8_t sync_buf_idx = -1;

	if (argc >= 2) {
		sync_buf_idx = strtol(argv[1], NULL, 10);
	}

	esl_c_sync_buf_status(sync_buf_idx);

	return 0;
}

/* Parameter <group_id><paylod_hex_string><response_slot><response_key 0><response_key 1>... */
static int cmd_pawr_push_sync_buf(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t sync_buf_idx;
	int err;
	uint8_t data[ESL_SYNC_PKT_PAYLOAD_MAX_LEN];
	size_t len;

	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <group_id><paylod_hex_string>\n");
		return -ENOEXEC;
	}

	sync_buf_idx = strtol(argv[1], NULL, 16);
	data[0] = sync_buf_idx;
	len = hex2bin(argv[2], strlen(argv[2]), (data + 1), ESL_SYNC_PKT_PAYLOAD_MAX_LEN) + 1;

	if (len == 0) {
		shell_fprintf(shell, SHELL_ERROR,
			      "#PUSH_SYNC_BUF:%03d failed payload out of length\n", sync_buf_idx);
		return -EINVAL;
	}

	if (argc > 4) {
		struct bt_esl_rsp_buffer rsp_buffer[CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER];
		size_t idx = 0;

		memset(rsp_buffer, 0,
		       sizeof(struct bt_esl_rsp_buffer) *
			       CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER);
		while ((idx < CONFIG_ESL_CLIENT_MAX_RESPONSE_SLOT_BUFFER) && (idx + 3) < argc) {
			hex2bin(argv[idx + 3], strlen(argv[idx + 3]), rsp_buffer[idx].rsp_key.key_v,
				ESL_SYNC_PKT_PAYLOAD_MAX_LEN);
			idx++;
		}

		esl_c_push_rsp_key(rsp_buffer, sync_buf_idx);
	}

	err = esl_c_push_sync_buf(data, len, sync_buf_idx);
	if (err != 0) {
		shell_fprintf(shell, SHELL_ERROR, "#PUSH_SYNC_BUF:%03d failed (%d)\n", sync_buf_idx,
			      err);
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "#PUSH_SYNC_BUF:%03d len %d\n", sync_buf_idx,
			      len);
	}

	return 0;
}

static int cmd_pawr_dump_sync_buf(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <group_id>\n");
		return -ENOEXEC;
	}

	uint8_t sync_buf_idx = strtol(argv[1], NULL, 16);

	esl_c_dump_sync_buf(sync_buf_idx);

	return 0;
}

static int cmd_pawr_dump_resp_buf(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <group_id>\n");
		return -ENOEXEC;
	}

	uint8_t sync_buf_idx = strtol(argv[1], NULL, 16);

	esl_c_dump_resp_buf(sync_buf_idx);

	return 0;
}

static int cmd_pawr_start_pawr(const struct shell *shell, size_t argc, char *argv[])
{
	esl_start_pawr();

	return 0;
}

static int cmd_pawr_stop_pawr(const struct shell *shell, size_t argc, char *argv[])
{
	esl_stop_pawr();

	return 0;
}

static int cmd_pawr_set_pawr_data(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <sync packet type><group_id>\n");
		return -ENOEXEC;
	}
	uint8_t esl_sync_type = strtol(argv[1], NULL, 16) & 0xff;
	uint8_t group_id = strtol(argv[2], NULL, 16) & 0xff;

	esl_dummy_ap_ad_data(esl_sync_type, group_id);
	load_all_tags_in_storage(group_id);

	return 0;
}

static int cmd_esl_c_update_abs(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <absolute time>\n");
		return -ENOEXEC;
	}

	uint32_t new_abs_time = strtoul(argv[1], NULL, 10);
	uint16_t esl_addr;
	int16_t conn_idx;

	esl_c_abs_timer_update(new_abs_time);

	if (argc >= 3) {
		esl_addr = strtol(argv[2], NULL, 16);
		conn_idx = esl_addr_to_conn_idx(esl_addr);
		if (conn_idx == -1) {
			shell_fprintf(shell, SHELL_ERROR,
				      "can't find match tag with esl_addr 0x%04x\n", esl_addr);
			return -ENOEXEC;
		}

		esl_c_tag_abs_timer_update(conn_idx);
	}

	return 0;
}

static int cmd_acl_scan(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter on/off <1/0> oneshot/continuously <1/0>\n");
		return -ENOEXEC;
	}

	uint16_t esl_scan_onoff = strtol(argv[1], NULL, 10);
	uint16_t esl_scan_oneshot = strtol(argv[2], NULL, 10);

	if (esl_scan_onoff > 1 && esl_scan_onoff < 0) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter on/off <1/0> oneshot/continuously <1/0>\n");
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "#SCAN:%s\n",
			      (esl_scan_onoff == 1) ? "Start" : "Stop");
		esl_c_scan(esl_scan_onoff, esl_scan_oneshot);
	}

	return 0;
}

static int cmd_acl_list_scanned(const struct shell *shell, size_t argc, char *argv[])
{
	esl_c_list_scanned(true);

	return 0;
}

static int cmd_acl_list_conn(const struct shell *shell, size_t argc, char *argv[])
{
	esl_c_list_connection();

	return 0;
}

static int cmd_acl_connect(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <TAG_ID>\n");
		return -ENOEXEC;
	}

	int16_t tag_idx = strtol(argv[1], NULL, 16);
	int ret = 0;
	char addr[BT_ADDR_LE_STR_LEN];
	struct tag_addr_node *tag_addr = NULL;

	tag_addr = esl_c_get_tag_addr(tag_idx, NULL);
	shell_fprintf(shell, SHELL_NORMAL, "Connect to scanned tag 0x%02x ", tag_idx);
	if (tag_addr == NULL) {
		shell_fprintf(shell, SHELL_ERROR, "Could not found scanned tag idx 0x%02x\n",
			      tag_idx);
	} else {
		bt_addr_le_to_str(&tag_addr->addr, addr, sizeof(addr));
		shell_fprintf(shell, SHELL_NORMAL, "%s\n", addr);
		ret = esl_c_connect(&tag_addr->addr, ESL_ADDR_BROADCAST);

		if (ret) {
			shell_fprintf(shell, SHELL_NORMAL, "Disconnected:Conn_idx: %d\n", ret);
		}
	}

	return ret;
}

static int cmd_acl_connect_addr(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <address_type><address>\n");
		return -ENOEXEC;
	}
	/**
	 * #define BT_ADDR_LE_PUBLIC       0x00
	 * #define BT_ADDR_LE_RANDOM       0x01
	 * #define BT_ADDR_LE_PUBLIC_ID    0x02
	 * #define BT_ADDR_LE_RANDOM_ID    0x03
	 * #define BT_ADDR_LE_UNRESOLVED   0xFE Resolvable Private Address (Controller unable to
	 *resolve) #define BT_ADDR_LE_ANONYMOUS    0xFF (anonymous advertisement)
	 **/
	uint8_t addr_type = strtol(argv[1], NULL, 16);
	bt_addr_le_t peer_addr;
	uint16_t esl_addr = ESL_ADDR_BROADCAST;
	int ret;
	char addr[BT_ADDR_LE_STR_LEN];

	switch (addr_type) {
	case BT_ADDR_LE_PUBLIC:
		ret = bt_addr_le_from_str(argv[2], "public", &peer_addr);
		break;
	case BT_ADDR_LE_PUBLIC_ID:
		ret = bt_addr_le_from_str(argv[2], "public-id", &peer_addr);
		break;
	case BT_ADDR_LE_RANDOM_ID:
		ret = bt_addr_le_from_str(argv[2], "random-id", &peer_addr);
		break;
	case BT_ADDR_LE_RANDOM:
	default:
		ret = bt_addr_le_from_str(argv[2], "random", &peer_addr);
		break;
	}

	if (argc > 3) {
		esl_addr = strtol(argv[3], NULL, 16);
	}

	if (ret != 0) {
		shell_fprintf(shell, SHELL_ERROR, "Create BLE Addr failed %d %s %d", addr_type,
			      argv[2], ret);
	} else {
		bt_addr_le_to_str(&peer_addr, addr, sizeof(addr));
		shell_fprintf(shell, SHELL_NORMAL, "Connect to %s esl_addr 0x%04x\n", addr,
			      esl_addr);
		ret = esl_c_connect(&peer_addr, esl_addr);
	}

	if (ret >= 0) {
		shell_fprintf(shell, SHELL_NORMAL, "Conn_idx:0x%02X\n", ret);
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "Conn_idx:Failed %d\n", ret);
	}

	return 0;
}

static int cmd_acl_configure(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_idx><esl_addr>\n");
		return -ENOEXEC;
	}

	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	uint16_t esl_addr = strtol(argv[2], NULL, 16);

	shell_fprintf(shell, SHELL_NORMAL, "Configure conn_idx %d esl_addr 0x%04x\n", conn_idx,
		      esl_addr);
	bt_esl_configure(conn_idx, esl_addr);

	return 0;
}

static int cmd_acl_discovery(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_idx>\n");
		return -ENOEXEC;
	}

	uint16_t conn_idx = strtol(argv[1], NULL, 16);

	shell_fprintf(shell, SHELL_NORMAL, "Discover conn_idx 0x%04x\n", conn_idx);
	esl_discovery_start(conn_idx);

	return 0;
}

static int cmd_esl_c_led(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Control Tags LED\n");

	if (argc < 9) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <esl_addr> <led_idx> <color_brightness> "
			      "<flash_pattern> <off_period> <on_period> <repeat_type> "
			      "<repeat_duration> <abs_time>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	uint8_t led_idx = strtol(argv[2], NULL, 16);
	int16_t conn_idx = esl_addr_to_conn_idx(esl_addr);
	uint8_t color_brightness = strtol(argv[3], NULL, 16);
	uint64_t flash_pattern = strtoull(argv[4], NULL, 16);
	uint8_t off_period = strtol(argv[5], NULL, 16);
	uint8_t on_period = strtol(argv[6], NULL, 16);
	uint8_t repeat_type = strtol(argv[7], NULL, 16);
	uint16_t repeat_duration = strtol(argv[8], NULL, 16);
	uint32_t abs_time = 0;
	struct led_obj led;
	bool timed = false;

	/* dump parsed parameters ,can be removed aftermath */
	shell_fprintf(shell, SHELL_NORMAL, "esl_addr 0x%04x\n", esl_addr);
	shell_fprintf(shell, SHELL_NORMAL, "led_idx %d\n", led_idx);
	shell_fprintf(shell, SHELL_NORMAL, "conn_idx %d\n", conn_idx);
	shell_fprintf(shell, SHELL_NORMAL, "color_brightness 0x%02x\n", color_brightness);
	shell_fprintf(shell, SHELL_NORMAL, "flash_pattern %08llx\n", flash_pattern);
	shell_fprintf(shell, SHELL_NORMAL, "off_period 0x%02x\n", off_period);
	shell_fprintf(shell, SHELL_NORMAL, "on_period 0x%02x\n", on_period);
	shell_fprintf(shell, SHELL_NORMAL, "repeat_type 0x%02x\n", repeat_type);
	shell_fprintf(shell, SHELL_NORMAL, "repeat_duration 0x%04x\n", repeat_duration);
	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}

	if (argc >= 10) {
		abs_time = strtoul(argv[9], NULL, 10);
		shell_fprintf(shell, SHELL_NORMAL, "abs_time %u\n", abs_time);
		timed = true;
	}

	led.color_brightness = color_brightness;
	led.flash_pattern[0] = (flash_pattern & 0xFF);
	led.flash_pattern[1] = (flash_pattern & 0xFF00) >> 8;
	led.flash_pattern[2] = (flash_pattern & 0xFF0000) >> 16;
	led.flash_pattern[3] = (flash_pattern & 0xFF000000) >> 24;
	led.flash_pattern[4] = (flash_pattern & 0xFF00000000) >> 32;
	led.bit_off_period = off_period;
	led.bit_on_period = on_period;
	led.repeat_type = repeat_type;
	led.repeat_duration = repeat_duration;

	return bt_esl_c_led_control(esl_c_get_esl_c_obj(), conn_idx, led_idx, &led, timed,
				    abs_time);
}

static int cmd_esl_c_display(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Control Tags DISPLAY\n");

	if (argc < 4) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <esl_addr> <disp_idx> <img_idx> <abs_time>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	uint8_t disp_idx = strtol(argv[2], NULL, 16);
	int16_t conn_idx = esl_addr_to_conn_idx(esl_addr);
	uint8_t img_idx = strtol(argv[3], NULL, 16);
	uint32_t abs_time = 0;
	bool timed = false;

	/* dump parsed parameters ,can be removed aftermath */
	shell_fprintf(shell, SHELL_NORMAL, "esl_addr 0x%04x\n", esl_addr);
	shell_fprintf(shell, SHELL_NORMAL, "disp_idx %d\n", disp_idx);
	shell_fprintf(shell, SHELL_NORMAL, "conn_idx %d\n", conn_idx);
	shell_fprintf(shell, SHELL_NORMAL, "img_idx %d\n", img_idx);
	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}

	if (argc >= 5) {
		abs_time = strtoul(argv[4], NULL, 10);
		shell_fprintf(shell, SHELL_NORMAL, "abs_time %u\n", abs_time);
		timed = true;
	}

	return bt_esl_c_display_control(esl_c_get_esl_c_obj(), conn_idx, disp_idx, img_idx, timed,
					abs_time);
}

static int cmd_esl_c_refresh_display(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Control Tags Refresh DISPLAY\n");

	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr> <disp_idx>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	uint8_t disp_idx = strtol(argv[2], NULL, 16);
	int16_t conn_idx = esl_addr_to_conn_idx(esl_addr);

	/* dump parsed parameters ,can be removed aftermath */
	shell_fprintf(shell, SHELL_NORMAL, "esl_addr 0x%04x\n", esl_addr);
	shell_fprintf(shell, SHELL_NORMAL, "disp_idx %d\n", disp_idx);
	shell_fprintf(shell, SHELL_NORMAL, "conn_idx %d\n", conn_idx);
	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}

	return bt_esl_c_refresh_display(esl_c_get_esl_c_obj(), conn_idx, disp_idx);
}

static int cmd_esl_c_read_sensor(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Read Tags sensor\n");

	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr> <sensor_idx>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	uint8_t sensor_idx = strtol(argv[2], NULL, 16);
	int16_t conn_idx = esl_addr_to_conn_idx(esl_addr);

	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}

	return bt_esl_c_read_sensor(esl_c_get_esl_c_obj(), conn_idx, sensor_idx);
}

static int cmd_esl_c_pingop(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	int16_t conn_idx = esl_addr_to_conn_idx(esl_addr);

	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}

	bt_esl_c_ping_op(esl_c_get_esl_c_obj(), conn_idx);

	return 0;
}

static int cmd_esl_c_factory_reset(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	int16_t conn_idx = esl_addr_to_conn_idx(esl_addr);

	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}

	bt_esl_c_factory_reset(esl_c_get_esl_c_obj(), conn_idx);

	return 0;
}

static int cmd_esl_c_service_reset(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	int16_t conn_idx = esl_addr_to_conn_idx(esl_addr);

	if (conn_idx == -1) {
		shell_fprintf(shell, SHELL_ERROR, "can't find match tag with esl_addr 0x%04x\n",
			      esl_addr);
		return -ENOEXEC;
	}

	bt_esl_c_service_reset(esl_c_get_esl_c_obj(), conn_idx);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(esl_c_cmds,
			       SHELL_CMD(dump, NULL, "Dump ESL client characters", cmd_esl_c_dump),

			       SHELL_SUBCMD_SET_END);
static int cmd_obj_c_select(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Select Object with ID\n");
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_idx><image index>\n");
		return -ENOEXEC;
	}

	int err;
	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	uint8_t img_idx = strtol(argv[2], NULL, 16);
	uint64_t obj_id = BT_OTS_OBJ_ID_MIN + img_idx;
	char id_str[BT_OTS_OBJ_ID_STR_LEN];
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	bt_ots_obj_id_to_str(obj_id, id_str, sizeof(id_str));
	err = bt_ots_client_select_id(&esl_c_obj->gatt[conn_idx].otc, esl_c_obj->conn[conn_idx],
				      obj_id);

	if (err != 0) {
		shell_fprintf(shell, SHELL_ERROR, "ots client select %s failed %d\n", id_str, err);
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "ots client select %s %d\n", id_str, err);
	}

	return err;
}

static int cmd_obj_c_readmeta(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Read metadata of Object\n");
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_idx>\n");
		return -ENOEXEC;
	}

	int err;
	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	err = bt_ots_client_read_object_metadata(
		&esl_c_obj->gatt[conn_idx].otc, esl_c_obj->conn[conn_idx], BT_OTS_METADATA_REQ_ALL);
	if (err != 0) {
		shell_fprintf(shell, SHELL_ERROR, "Failed to read object metadata %d\n", err);
	}

	return err;
}

/** Write Image through OTS with SMP transferred Image */
static int cmd_obj_c_write(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <conn_idx><tag_img_idx><stored image idx>\n");
		return -ENOEXEC;
	}

	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	uint8_t tag_img_idx = strtol(argv[2], NULL, 16);
	uint16_t img_idx = strtol(argv[3], NULL, 16);

	esl_c_obj_write(conn_idx, tag_img_idx, img_idx);

	return 0;
}

/** Write Image through OTS with SMP transferred Image by filename*/
static int cmd_obj_c_write_filename(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <conn_idx><tag_img_idx><stored image idx>\n");
		return -ENOEXEC;
	}

	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	uint8_t tag_img_idx = strtol(argv[2], NULL, 16);

	esl_c_obj_write_by_name(conn_idx, tag_img_idx, argv[3]);

	return 0;
}

/**
 * TODO: Make OTS checksum check automatic after write object data
 **/
static int cmd_obj_c_checksum(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Compare Tag Image checksum\n");
	int err;

	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_idx>\n");
		return -ENOEXEC;
	}
	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	size_t size = CONFIG_ESL_IMAGE_FILE_SIZE;
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	err = bt_ots_client_get_object_checksum(&esl_c_obj->gatt[conn_idx].otc,
						esl_c_obj->conn[conn_idx], 0, size);

	if (err != 0) {
		shell_fprintf(shell, SHELL_ERROR, "Failed to calculate object checksum %d\n", err);
		return err;
	}

	return 0;
}

static int cmd_esl_c_whoami(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "WHOAMI:AP\n");

	return 0;
}

static int cmd_acl_write_esl_addr(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <conn_id><esl_addr>[<change local only>]\n");
		return -ENOEXEC;
	}

	int err;
	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	uint16_t esl_addr = strtol(argv[2], NULL, 16);
	uint16_t local = 0;
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	if (argc > 3) {
		local = strtol(argv[3], NULL, 16);
	}

	/* Debug command for ESLS/SR/ECP/BI-14-C [Reject Invalid ESL ID] */
	if (local == 1) {
		esl_c_obj->gatt[conn_idx].esl_device.esl_addr = esl_addr;
		return 0;
	}

	/* Debug command for ESLS/SR/ECP/BV-12-C [Factory Reset] */
	if (esl_addr_to_conn_idx(esl_addr)) {
		esl_c_obj->gatt[conn_idx].esl_device.esl_addr = esl_addr;
	}

	err = bt_esl_c_write_esl_addr(conn_idx, esl_addr);

	return err;
}

static int cmd_acl_read_chrc(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_id>\n");
		return -ENOEXEC;
	}

	int err;
	uint8_t conn_idx = strtol(argv[1], NULL, 16);

	err = bt_esl_c_read_chrc(esl_c_get_esl_c_obj(), conn_idx);

	return err;
}

static int cmd_acl_read_chrc_multi(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_id>\n");
		return -ENOEXEC;
	}

	int err;
	uint8_t conn_idx = strtol(argv[1], NULL, 16);

	err = bt_esl_c_read_chrc_multi(esl_c_get_esl_c_obj(), conn_idx);

	return err;
}

static int cmd_acl_security(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <conn_id>[<security level>]\n");
		return -ENOEXEC;
	}

	int err;
	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	uint16_t security_level;
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	shell_fprintf(shell, SHELL_NORMAL, "ESL_AP_SECURITY_LEVEL %d, current %d\n",
		      esl_ap_security_level, bt_conn_get_security(esl_c_obj->conn[conn_idx]));
	if (argc > 2) {
		security_level = strtol(argv[2], NULL, 16);
		esl_ap_security_level = security_level;
		err = bt_conn_set_security(esl_c_obj->conn[conn_idx], security_level);
		shell_fprintf(shell, SHELL_NORMAL, "set security level %d\n", security_level);
		if (err != 0) {
			shell_fprintf(shell, SHELL_ERROR, "Security failed: level %u err %d\n",
				      security_level, err);
		}
	}

	return 0;
}

static int cmd_auto_ap(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Old ESL_AP_AUTO_MODE %d\n", esl_c_auto_ap_mode);
	if (argc > 1) {
		uint16_t auto_ap_onoff = strtol(argv[1], NULL, 10);

		if (auto_ap_onoff > 1 && auto_ap_onoff < 0) {
			shell_fprintf(shell, SHELL_ERROR,
				      "ESL_AP_AUTO_MODE no valid parameter on/off <1/0>\n");
		} else {
			esl_c_auto_ap_toggle(auto_ap_onoff);
		}
	}

	return 0;
}

static int cmd_acl_subscribe(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_id>\n");
		return -ENOEXEC;
	}

	int err;
	uint8_t conn_idx = strtol(argv[1], NULL, 16);

	err = bt_esl_c_subscribe_receive(esl_c_get_esl_c_obj(), conn_idx);

	return err;
}
static int cmd_esl_c_reset_ap(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = bt_esl_c_reset_ap();

	return err;
}

#if defined(CONFIG_BT_ESL_TAG_STORAGE)
static int cmd_esl_c_remove_tags(const struct shell *shell, size_t argc, char *argv[])
{
	int err;

	err = remove_all_tags_in_storage();
	shell_fprintf(shell, SHELL_NORMAL, "#REMOVETAGSDATA:%d", (err == 0) ? 1 : 0);

	return err;
}

static int cmd_esl_c_list_tags(const struct shell *shell, size_t argc, char *argv[])
{
	uint8_t type;

	if (argc > 1) {
		type = strtol(argv[1], NULL, 16);
	} else {
		type = 0;
	}

	list_tags_in_storage(type);

	return 0;
}

static int cmd_esl_c_remove_tag(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	int err;

	err = remove_tag_in_storage(esl_addr, NULL);
	shell_fprintf(shell, SHELL_NORMAL, "#REMOVETAGDATA:%d,0x%04x\n", (err == 0) ? 1 : 0,
		      esl_addr);

	return err;
}

static int cmd_tags_per_group(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "#ESL_C_TAGS_PER_GROUP:%03d\n", esl_c_tags_per_group);
	if (argc > 1) {
		uint16_t new_tag_number = strtol(argv[1], NULL, 10);

		esl_c_tags_per_group = new_tag_number;
		esl_c_setting_tags_per_group_save();
		shell_fprintf(shell, SHELL_NORMAL, "#ESL_C_TAGS_PER_GROUP:%03d\n",
			      esl_c_tags_per_group);
	}

	return 0;
}

static int cmd_groups_per_button(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "#ESL_C_GROUPS_PER_BUTTON:%03d\n", groups_per_button);
	if (argc > 1) {
		uint8_t new_group_number = strtol(argv[1], NULL, 10);

		groups_per_button = new_group_number;
		esl_c_setting_group_per_button_save();
		shell_fprintf(shell, SHELL_NORMAL, "#ESL_C_GROUPS_PER_BUTTON:%03d\n",
			      groups_per_button);
	}

	return 0;
}
static int cmd_acl_connect_esl(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <esl_addr>\n");
		return -ENOEXEC;
	}

	uint16_t esl_addr = strtol(argv[1], NULL, 16);
	bt_addr_le_t peer_addr;
	int ret;
	char addr[BT_ADDR_LE_STR_LEN];

	ret = find_tag_in_storage_with_esl_addr(esl_addr, &peer_addr);

	if (ret != 0) {
		shell_fprintf(shell, SHELL_ERROR, "Could not found tag esl_addr 0%04x (ret %d)",
			      esl_addr, ret);
	} else {
		bt_addr_le_to_str(&peer_addr, addr, sizeof(addr));
		shell_fprintf(shell, SHELL_NORMAL, "Connect to %s esl_addr 0x%04x\n", addr,
			      esl_addr);
		ret = esl_c_connect(&peer_addr, esl_addr);
	}

	if (ret >= 0) {
		shell_fprintf(shell, SHELL_NORMAL, "Conn_idx:0x%02X\n", ret);
	} else {
		shell_fprintf(shell, SHELL_NORMAL, "Conn_idx:Failed %d\n", ret);
	}

	return 0;
}
#endif /* CONFIG_BT_ESL_TAG_STORAGE */

static int cmd_acl_write_wo_rsp(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "ESL_WRITE_WITHOUT_RSP current %d\n",
		      esl_c_write_wo_rsp);
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter [<1/0>]\n");
		return -ENOEXEC;
	}

	uint16_t write_wo_rsp = strtol(argv[1], NULL, 16);

	esl_c_write_wo_rsp = (write_wo_rsp == 0) ? false : true;
	shell_fprintf(shell, SHELL_NORMAL, "set ESL_WRITE_WITHOUT_RSP %d\n", esl_c_write_wo_rsp);

	return 0;
}
static int cmd_acl_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter [<conn_idx>]\n");
		return -ENOEXEC;
	}

	uint16_t conn_idx = strtol(argv[1], NULL, 16);
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	peer_disconnect(esl_c_obj->conn[conn_idx]);

	return 0;
}

static void bt_key_dump(struct bt_keys *key)
{
	uint8_t temp_data[255];

	LOG_INF("id 0x%02x", key->id);
	LOG_HEXDUMP_INF(&key->addr, sizeof(bt_addr_le_t), "addr");
	LOG_INF("state 0x%02x, enc_size 0x%02x, flags 0x%02x keys 0x%04x", key->state,
		key->enc_size, key->flags, key->keys);
	memcpy(temp_data, &key->ltk, sizeof(struct bt_ltk));
	LOG_HEXDUMP_INF(temp_data, sizeof(struct bt_ltk), "ltk");
	LOG_HEXDUMP_INF(&key->irk, sizeof(struct bt_irk), "irk");
#if defined(CONFIG_BT_SIGNING)
	LOG_HEXDUMP_INF(&key->local_csrk, sizeof(struct bt_csrk), "local_csrk");
	LOG_HEXDUMP_INF(&key->remote_csrk, sizeof(struct bt_csrk), "remote_csrk");
#endif /* BT_SIGNING */
#if !defined(CONFIG_BT_SMP_SC_PAIR_ONLY)
	LOG_HEXDUMP_INF(&key->periph_ltk, sizeof(struct bt_ltk), "periph_ltk");
#endif /* CONFIG_BT_SMP_SC_PAIR_ONLY */
#if (defined(CONFIG_BT_KEYS_OVERWRITE_OLDEST))
	LOG_INF("aging_counter 0x%08x", key->aging_counter);
#endif /* CONFIG_BT_KEYS_OVERWRITE_OLDEST */
}

static void dump_bt_key(const struct bt_bond_info *info, void *user_data)
{
	struct bt_keys *bt_key = bt_keys_find_addr(BT_ID_DEFAULT, &info->addr);

	bt_key_dump(bt_key);
}

static void export_bt_key(const struct bt_bond_info *info, void *user_data)
{
	struct bt_keys *bt_key = bt_keys_find_addr(BT_ID_DEFAULT, &info->addr);
	char key_name[BT_SETTINGS_KEY_MAX];
	char id[4];
	const struct shell *shell = (const struct shell *)user_data;

	if (bt_key != NULL) {
		/* Encoded ble address as bt_settings */
		if (bt_key->id) {
			u8_to_dec(id, sizeof(id), bt_key->id);
			snprintk(key_name, sizeof(key_name), "%02x%02x%02x%02x%02x%02x%u/%s",
				 bt_key->addr.a.val[5], bt_key->addr.a.val[4],
				 bt_key->addr.a.val[3], bt_key->addr.a.val[2],
				 bt_key->addr.a.val[1], bt_key->addr.a.val[0], bt_key->addr.type,
				 id);
		} else {
			snprintk(key_name, sizeof(key_name), "%02x%02x%02x%02x%02x%02x%u",
				 bt_key->addr.a.val[5], bt_key->addr.a.val[4],
				 bt_key->addr.a.val[3], bt_key->addr.a.val[2],
				 bt_key->addr.a.val[1], bt_key->addr.a.val[0], bt_key->addr.type);
		}

		shell_fprintf(shell, SHELL_NORMAL, "%s ", key_name);
		for (int i = 0; i < BT_KEYS_STORAGE_LEN; i++) {
			shell_fprintf(shell, SHELL_NORMAL, "%02x", *(bt_key->storage_start + i));
		}

		shell_fprintf(shell, SHELL_NORMAL, "\n");
	}
}

static int cmd_acl_bt_key_import(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <ble addr><key hex>\n");
		return -ENOEXEC;
	}

	int err;
	uint8_t data[sizeof(struct bt_keys)];
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_keys new_key = {0};
	size_t len;

	/* Parse ble addr argument to bt settings encoded */
	if (strlen(argv[1]) != 13) {
		shell_fprintf(shell, SHELL_ERROR, "BLE addr size is not correct %d\n",
			      strlen(argv[1]));
		return -EINVAL;
	}

	if (argv[1][12] == '0') {
		new_key.addr.type = BT_ADDR_LE_PUBLIC;
	} else if (argv[1][12] == '1') {
		new_key.addr.type = BT_ADDR_LE_RANDOM;
	} else {
		return -EINVAL;
	}

	for (uint8_t i = 0; i < 6; i++) {
		hex2bin(&argv[1][i * 2], 2, &new_key.addr.a.val[5 - i], 1);
	}

	bt_addr_le_to_str(&new_key.addr, addr_str, sizeof(addr_str));
	shell_fprintf(shell, SHELL_NORMAL, "Input BD_ADDR: %s\n", addr_str);
	len = hex2bin(argv[2], strlen(argv[2]), data, BT_KEYS_STORAGE_LEN);
	if (len != BT_KEYS_STORAGE_LEN) {
		shell_fprintf(shell, SHELL_ERROR, "#BT_KEY_IMPORT_LEN:%d, bt_keys %d not match\n",
			      len, BT_KEYS_STORAGE_LEN);
		return -EINVAL;
	}

	new_key.id = BT_ID_DEFAULT;
	memcpy(&new_key.enc_size, data, len);

	err = bt_keys_store(&new_key);
	settings_load();
	shell_fprintf(shell, SHELL_NORMAL, "#BT_KEY_IMPORT:%d\n", err);

	return err;
}

static int cmd_acl_bt_key_export(const struct shell *shell, size_t argc, char *argv[])
{
	bt_foreach_bond(BT_ID_DEFAULT, export_bt_key, (void *)shell);

	return 0;
}

static int cmd_bd_addr(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_le_oob oob;

	bt_id_read_public_addr(&addr);
	bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));
	shell_fprintf(shell, SHELL_NORMAL, "bt_id_read_public_addr BD_ADDR: %s\n", addr_str);
	bt_le_oob_get_local(0, &oob);
	bt_addr_le_to_str(&oob.addr, addr_str, sizeof(addr_str));
	shell_fprintf(shell, SHELL_NORMAL, "bt_le_oob_get_local BD_ADDR: %s\n", addr_str);
	bt_foreach_bond(BT_ID_DEFAULT, dump_bt_key, NULL);

	return 0;
}

static int cmd_esl_abs(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "#ABSTIME:%u\n", esl_c_get_abs_time());
	if (argc < 2) {
		return 0;
	}

	uint32_t new_abs_time = strtoul(argv[1], NULL, 10);

	if (new_abs_time < UINT32_MAX) {
		esl_c_abs_timer_update(new_abs_time);
		shell_fprintf(shell, SHELL_NORMAL, "new bas time %u", new_abs_time);
	} else {
		shell_fprintf(shell, SHELL_ERROR, "new abs time is invalid\n");

		return -ENOEXEC;
	}

	return 0;
}

static int cmd_esl_c_default_esl_addr(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	shell_fprintf(shell, SHELL_NORMAL, "#DEFAULT_ESL_ADDR:0x%04x\n", esl_c_obj->esl_addr_start);
	if (argc < 2) {
		return 0;
	}
	uint16_t new_esl_addr = strtoul(argv[1], NULL, 16);

	shell_fprintf(shell, SHELL_NORMAL, "new default_esl_addr 0x%04x\n", new_esl_addr);
	esl_c_obj->esl_addr_start = new_esl_addr;

	return 0;
}
static int cmd_esl_c_update_complete(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_idx>[<esl_addr>]\n");
		return -ENOEXEC;
	}

	int err;
	int16_t conn_idx = strtol(argv[1], NULL, 16);
	struct bt_esl_client *esl_c_obj = esl_c_get_esl_c_obj();

	if (argc > 2) {
		uint16_t esl_addr = strtol(argv[2], NULL, 16);

		esl_c_obj->gatt[conn_idx].esl_device.esl_addr = esl_addr;
	}

	err = bt_esl_c_update_complete(esl_c_obj, conn_idx);

	return err;
}

/* Manually clear GATT read/write flag to avoid IOP test deadlock */
static int cmd_acl_gatt_flag(const struct shell *shell, size_t argc, char *argv[])
{
	esl_c_gatt_flag_dump();
	if (argc > 2) {
		uint8_t flag_idx = strtol(argv[1], NULL, 16);
		uint8_t set_unset = strtol(argv[2], NULL, 16);

		esl_c_gatt_flag_set(flag_idx, set_unset);
	}

	return 0;
}

static int cmd_acl_past(const struct shell *shell, size_t argc, char *argv[])
{
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <conn_id>\n");
		return -ENOEXEC;
	}

	int err;
	uint8_t conn_idx = strtol(argv[1], NULL, 16);

	err = esl_c_past(conn_idx);

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	obj_c_cmds, SHELL_CMD(select, NULL, "Select Object with ID", cmd_obj_c_select),
	SHELL_CMD(read_meta, NULL, "Read metadata of Object", cmd_obj_c_readmeta),
	SHELL_CMD(write, NULL, "Write Image transferred from SMP", cmd_obj_c_write),
	SHELL_CMD(write_filename, NULL, "Write Image transferred from SMP by filename",
		  cmd_obj_c_write_filename),
	SHELL_CMD(check, NULL, "Read check sum of Image", cmd_obj_c_checksum),
	SHELL_SUBCMD_SET_END);
SHELL_STATIC_SUBCMD_SET_CREATE(
	acl_cmds, SHELL_CMD(disconnect, NULL, "ESL AP disconnect TAG", cmd_acl_disconnect),
	SHELL_CMD(security, NULL, "ESL AP connection security level", cmd_acl_security),
	SHELL_CMD(write_wo_rsp, NULL, "ESL AP set write chrc without response",
		  cmd_acl_write_wo_rsp),
	SHELL_CMD(read_chrc, NULL, "ESL AP write ESL ADDR chrc", cmd_acl_read_chrc),
	SHELL_CMD(rd_multi, NULL, "ESL AP write ESL ADDR chrc", cmd_acl_read_chrc_multi),
	SHELL_CMD(gatt_flag, NULL, "ESL AP GATT read/write flag debug command", cmd_acl_gatt_flag),
	SHELL_CMD(scan, NULL, "Scan ESL service tags", cmd_acl_scan),
	SHELL_CMD(list_scanned, NULL, "List advertising ESL service tags", cmd_acl_list_scanned),
	SHELL_CMD(connect, NULL, "Connect advertising ESL service tag", cmd_acl_connect),
	SHELL_CMD(connect_addr, NULL, "Connect advertising ESL service or synced tag with ble addr",
		  cmd_acl_connect_addr),
	SHELL_CMD(bt_key_import, NULL, "ESL AP import BT bonding key runtime",
		  cmd_acl_bt_key_import),
	SHELL_CMD(bt_key_export, NULL, "ESL AP export bond key", cmd_acl_bt_key_export),
	SHELL_CMD(bd_addr, NULL, "ESL AP BD Addr", cmd_bd_addr),
#if defined(CONFIG_BT_ESL_TAG_STORAGE)
	SHELL_CMD(connect_esl, NULL, "Connect ESL service tag with esl addr", cmd_acl_connect_esl),
#endif /* CONFIG_BT_ESL_TAG_STORAGE */
	SHELL_CMD(configure, NULL, "Configure connected tag manually", cmd_acl_configure),
	SHELL_CMD(discovery, NULL, "Discovery connected tag manually", cmd_acl_discovery),
	SHELL_CMD(list_conn, NULL, "List connections status", cmd_acl_list_conn),
	SHELL_CMD(write_esl_addr, NULL, "ESL AP write ESL ADDR chrc", cmd_acl_write_esl_addr),
	SHELL_CMD(subscribe, NULL, "ESL AP subscribe ECP", cmd_acl_subscribe),
	SHELL_CMD(past, NULL, "ESL AP commence PAST(Periodic Advertising Sync Transfer)",
		  cmd_acl_past),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	pawr_commands,
	SHELL_CMD(sync_buf_status, NULL, "ESL AP dump sync buffer status",
		  cmd_pawr_sync_buf_status),
	SHELL_CMD(push_sync_buf, NULL, "ESL AP push sync buffer", cmd_pawr_push_sync_buf),
	SHELL_CMD(dump_sync_buf, NULL, "ESL AP dump sync buffer content", cmd_pawr_dump_sync_buf),
	SHELL_CMD(dump_resp_buf, NULL, "ESL AP dump response buffer content",
		  cmd_pawr_dump_resp_buf),
	SHELL_CMD(start_pawr, NULL, "ESL AP start PAwR", cmd_pawr_start_pawr),
	SHELL_CMD(stop_pawr, NULL, "ESL AP stop PAwR", cmd_pawr_stop_pawr),
	SHELL_CMD(update_pawr, NULL, "ESL AP update predefined PAwR subevent data",
		  cmd_pawr_set_pawr_data),
	SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	esl_c_commands, SHELL_CMD(state, NULL, "Show current state machine", cmd_esl_c_state),
	SHELL_CMD(unassociated, NULL, "ESL AP unassociate tag command", cmd_esl_c_unassociated),
	SHELL_CMD(force_un, NULL, "ESL AP unassociate tag command", cmd_esl_c_force_unassociated),
	SHELL_CMD(bond_dump, NULL, "ESL AP bond tags dump command", cmd_esl_c_bond_dump),
	SHELL_CMD(unbond_all, NULL, "ESL AP unbond all tags debug command", cmd_esl_c_unbond_all),
	SHELL_CMD(ap_abs, NULL, "ESL AP change AP absolute timer tick", cmd_esl_abs),
	SHELL_CMD(update_abs, NULL, "ESL AP change TAG absolute timer tick", cmd_esl_c_update_abs),
	SHELL_CMD(led, NULL, "ESL AP control tag led", cmd_esl_c_led),
	SHELL_CMD(display, NULL, "ESL AP control tag display", cmd_esl_c_display),
	SHELL_CMD(refresh, NULL, "ESL AP refresh tag display", cmd_esl_c_refresh_display),
	SHELL_CMD(sensor, NULL, "ESL AP read tag sensor", cmd_esl_c_read_sensor),
	SHELL_CMD(factory, NULL, "ESL AP send factory OP to tag", cmd_esl_c_factory_reset),
	SHELL_CMD(service_reset, NULL, "ESL AP send service_reset OP to tag",
		  cmd_esl_c_service_reset),
	SHELL_CMD(ping, NULL, "ESL AP send PING OP to tag", cmd_esl_c_pingop),
	SHELL_CMD(reset_ap, NULL, "ESL AP clear all data", cmd_esl_c_reset_ap),
	SHELL_CMD(default_esl_addr, NULL, "ESL AP default esl_addr", cmd_esl_c_default_esl_addr),
	SHELL_CMD(update_complete, NULL, "ESL AP ECP Update Complete", cmd_esl_c_update_complete),
	SHELL_CMD(auto_ap, NULL, "ESL AP Auto mode toggle", cmd_auto_ap),
#if defined(CONFIG_BT_ESL_TAG_STORAGE)
	SHELL_CMD(remove_tags, NULL, "ESL AP remove tags esl address in storage DB",
		  cmd_esl_c_remove_tags),
	SHELL_CMD(list_tags_storage, NULL, "ESL AP list Tag in ESL address format",
		  cmd_esl_c_list_tags),
	SHELL_CMD(remove_tag, NULL, "ESL AP remove tag in storage DB", cmd_esl_c_remove_tag),
	SHELL_CMD(tags_per_group, NULL, "ESL AP set/get how many tags in group for AP Auto mode",
		  cmd_tags_per_group),
	SHELL_CMD(groups_per_button, NULL,
		  "ESL AP set/get how many groups could be controlled by button",
		  cmd_groups_per_button),
#endif /* CONFIG_BT_ESL_TAG_STROAGE */
	SHELL_CMD(obj_c, &obj_c_cmds, "Object Transfer client commands", NULL),
	SHELL_CMD(acl, &acl_cmds, "ESL ACL commands", NULL),
	SHELL_CMD(esl_c, &esl_c_cmds, "ESL debug commands", NULL),
	SHELL_CMD(pawr, &pawr_commands, "ESL client PAwR commands", NULL), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(WHOAMI, NULL, "Tell UI what role I am", cmd_esl_c_whoami);
SHELL_CMD_REGISTER(esl_c, &esl_c_commands, "ESL client commands", NULL);

static int esl_c_shell_init(void)
{
	context.shell = NULL;
	return 0;
}

SYS_INIT(esl_c_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
