/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 * @brief ESL shell module
 */

#include <stdio.h>
#include <stdlib.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/init.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <host/id.h>

#include "esl.h"
#include "esl_settings.h"
#include "esl_internal.h"

LOG_MODULE_REGISTER(ESL_shell, LOG_LEVEL_INF);
#define ESL_SHELL_MODULE "esl"
static struct {
	const struct shell *shell;
} context;
extern struct bt_conn *current_conn;
extern struct bt_conn *auth_conn;

#define print(shell, level, fmt, ...)                                                              \
	do {                                                                                       \
		if (shell) {                                                                       \
			shell_fprintf(shell, level, fmt, ##__VA_ARGS__);                           \
		} else {                                                                           \
			printk(fmt, ##__VA_ARGS__);                                                \
		}                                                                                  \
	} while (false)

static int cmd_esl_state(const struct shell *shell, size_t argc, char *argv[])
{
	struct bt_esls *esl = esl_get_esl_obj();

	if (argc >= 2) {
		uint8_t state = strtol(argv[1], NULL, 10);

		bt_esl_state_transition(state);
	}

	shell_fprintf(shell, SHELL_NORMAL, "ESL state %s\n",
		      esl_state_to_string((uint8_t)esl->esl_state));
	return 0;
}

static int cmd_esl_dump(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Dump bonded device\n");
	uint32_t abs_time = esl_get_abs_time();
	struct bt_esls *esl = esl_get_esl_obj();

	bt_esl_bond_dump(NULL);
	shell_fprintf(shell, SHELL_NORMAL, "Dump ESL data\n");
	shell_fprintf(shell, SHELL_NORMAL, "ESL ADDR 0x%04x\n", esl->esl_chrc.esl_addr);
	shell_fprintf(shell, SHELL_NORMAL, "AP Sync Key\n");
	shell_hexdump(shell, esl->esl_chrc.esl_ap_key.key_v, EAD_KEY_MATERIAL_LEN);
	shell_fprintf(shell, SHELL_NORMAL, "ESL RESP Key\n");
	shell_hexdump(shell, esl->esl_chrc.esl_rsp_key.key_v, EAD_KEY_MATERIAL_LEN);
	shell_fprintf(shell, SHELL_NORMAL, "Randomizer\n");
	shell_hexdump(shell, esl->esl_chrc.esl_randomizer, EAD_RANDOMIZER_LEN);
	shell_fprintf(shell, SHELL_NORMAL, "Abs time %u\n", abs_time);
#if (CONFIG_BT_ESL_DISPLAY_MAX > 0)
	dump_disp_inf((uint8_t *)esl->esl_chrc.displays, CONFIG_BT_ESL_DISPLAY_MAX * 5,
		      (uint8_t *)&esl->display_works[0].img_idx);
#endif
#if IS_ENABLED(CONFIG_BT_ESL_IMAGE_AVAILABLE)
	shell_fprintf(shell, SHELL_NORMAL, "Max image idx 0x%02x\n", esl->esl_chrc.max_image_index);
	for (uint8_t idx = 0; idx < CONFIG_BT_ESL_IMAGE_MAX; idx++) {
		shell_fprintf(shell, SHELL_NORMAL, "img %d size %d\n", idx,
			      esl->stored_image_size[idx]);
	}
#endif
#if (CONFIG_BT_ESL_LED_MAX > 0)
	dump_led_infs((uint8_t *)esl->esl_chrc.led_type, CONFIG_BT_ESL_LED_MAX);
#endif
#if (CONFIG_BT_ESL_SENSOR_MAX > 0)

#endif
	return 0;
}

static int cmd_esl_bond_dump(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Dump bonded device\n");
	bt_esl_bond_dump(NULL);
	return 0;
}
static int cmd_esl_unbond(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Clear bonded data\n");
	(void)bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	return 0;
}

static int cmd_esl_unassociated(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_unassociated\n");
	bt_esl_unassociate();
	return 0;
}
static int cmd_esl_factory_reset(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_factory_reset\n");
	bt_esl_factory_reset();
	return 0;
}
static int cmd_esl_disconnect(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_disconnect\n");
	bt_esl_disconnect();
	return 0;
}
static int cmd_get_esl_basic(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_get_esl_basic\n");
	struct bt_esls *esl = esl_get_esl_obj();

	print_binary(esl->basic_state, ESL_BASIC_RFU);
	print_basic_state((atomic_t)esl->basic_state);

	return 0;
}

static int cmd_set_esl_basic(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_set_esl_basic\n");
	if (argc < 3) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <bit><on/off>\n");
		return -ENOEXEC;
	}

	uint8_t bit = strtoul(argv[1], NULL, 10);
	uint8_t value = strtoul(argv[2], NULL, 10);
	struct bt_esls *esl = esl_get_esl_obj();

	if (value == 1) {
		atomic_set_bit(&esl->basic_state, bit);
	} else {
		atomic_clear_bit(&esl->basic_state, bit);
	}
	return 0;
}
static int cmd_esl_addr(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_addr\n");

	struct bt_esls *esl = esl_get_esl_obj();

	if (argc >= 2) {
		uint16_t esl_addr = strtoul(argv[1], NULL, 16);

		shell_fprintf(shell, SHELL_NORMAL, "update esl_addr to 0x%04x\n", esl_addr);
		esl->esl_chrc.esl_addr = esl_addr;
		esl_settings_addr_save();
	}

	shell_fprintf(shell, SHELL_NORMAL, "ESL ADDR 0x%04x\n", esl->esl_chrc.esl_addr);

	return 0;
}
static int cmd_esl_adv(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_adv\n");
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <1/0>\n");
		return -ENOEXEC;
	}

	uint8_t onoff = strtoul(argv[1], NULL, 10);

	if (onoff) {
		bt_esl_adv_start(ESL_ADV_DEFAULT);
	} else {
		bt_esl_adv_stop();
	}

	return 0;
}

static int cmd_esl_led(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_led\n");
	if (argc < 4) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <LED_IDX> <Color_brightness> <onoff>\n");
		return -ENOEXEC;
	}

	uint8_t led_idx = strtoul(argv[1], NULL, 16);
	uint8_t color_brightness = strtoul(argv[2], NULL, 16);
	uint8_t onoff = strtoul(argv[3], NULL, 10);
	struct bt_esls *esl = esl_get_esl_obj();

	if (esl->cb.led_control) {
		esl->cb.led_control(led_idx, color_brightness, onoff);
	}

	if (onoff == 0) {
		esl->led_dworks[led_idx].work_enable = false;
		esl->led_dworks[led_idx].active = false;
		bt_esl_basic_state_update();
	}

	return 0;
}

static int cmd_esl_display(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_display\n");
	if (argc < 4) {
		shell_fprintf(shell, SHELL_ERROR,
			      "no valid parameter <DISPLAY_IDX> <image_idx> <on/off>\n");
		return -ENOEXEC;
	}

	uint8_t disp_idx = strtoul(argv[1], NULL, 16);
	uint8_t image_idx = strtoul(argv[2], NULL, 16);
	uint8_t onoff = strtoul(argv[3], NULL, 10);
	struct bt_esls *esl = esl_get_esl_obj();

	if (esl->cb.display_control) {
		esl->cb.display_control(disp_idx, image_idx, onoff);
	}

	return 0;
}
static int cmd_esl_led_work_dump(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_led_work_dump\n");
	struct bt_esls *esl = esl_get_esl_obj();

	for (size_t idx = 0; idx < CONFIG_BT_ESL_LED_MAX; idx++) {
		shell_fprintf(shell, SHELL_NORMAL,
			      "Work %d LED_IDX %d, stop %d start %d enabled %d activate %d\n", idx,
			      esl->led_dworks[idx].led_idx, esl->led_dworks[idx].stop_abs_time,
			      esl->led_dworks[idx].start_abs_time, esl->led_dworks[idx].work_enable,
			      esl->led_dworks[idx].active);
		shell_fprintf(
			shell, SHELL_NORMAL, "pending %d expires %d ramaining %d\n",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(
				k_work_delayable_is_pending(&esl->led_dworks[idx].work), 1) /
				NSEC_PER_MSEC,
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(
				k_work_delayable_expires_get(&esl->led_dworks[idx].work), 1) /
				NSEC_PER_MSEC,
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(
				k_work_delayable_remaining_get(&esl->led_dworks[idx].work), 1) /
				NSEC_PER_MSEC);
	}

	return 0;
}
static int cmd_esl_display_work_dump(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_display_work_dump\n");
	struct bt_esls *esl = esl_get_esl_obj();

	for (size_t idx = 0; idx < CONFIG_BT_ESL_DISPLAY_MAX; idx++) {
		shell_fprintf(shell, SHELL_NORMAL, "Work %d DISP_IDX %d, start %d activate %d\n",
			      idx, esl->display_works[idx].display_idx,
			      esl->display_works[idx].start_abs_time,
			      esl->display_works[idx].active);
		shell_fprintf(
			shell, SHELL_NORMAL, "pending %d expires %d ramaining %d\n",
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(
				k_work_delayable_is_pending(&esl->display_works[idx].work), 1) /
				NSEC_PER_MSEC,
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(
				k_work_delayable_expires_get(&esl->display_works[idx].work), 1) /
				NSEC_PER_MSEC,
			SYS_CLOCK_HW_CYCLES_TO_NS_AVG(
				k_work_delayable_remaining_get(&esl->display_works[idx].work), 1) /
				NSEC_PER_MSEC);
	}

	return 0;
}
static int cmd_esl_sensor(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_sensor\n");
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <SENSOR_IDX>\n");
		return -ENOEXEC;
	}

	uint8_t sensor_idx = strtoul(argv[1], NULL, 16);
	int ret;
	uint8_t len;
	uint8_t data[ESL_RESPONSE_SENSOR_LEN];
	struct bt_esls *esl = esl_get_esl_obj();

	if (esl->cb.sensor_control) {
		ret = esl->cb.sensor_control(sensor_idx, &len, data);
		if (ret) {
			shell_fprintf(shell, SHELL_ERROR, "ret %d\n", ret);
		}
	}

	return 0;
}

static int cmd_esl_busy(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_busy ");
	struct bt_esls *esl = esl_get_esl_obj();

	esl->busy = true;
	shell_fprintf(shell, SHELL_NORMAL, "%d\n", esl->busy);
	return 0;
}

static int cmd_esl_accept_pair(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_comply\n");
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <0/1>\n");
		return -ENOEXEC;
	}
	uint8_t accept = strtoul(argv[1], NULL, 10);

	if (accept) {
		bt_conn_auth_passkey_confirm(auth_conn);
		LOG_INF("Numeric Match, conn %p", (void *)auth_conn);
	} else {
		bt_conn_auth_cancel(auth_conn);
		LOG_INF("Numeric Reject, conn %p", (void *)auth_conn);
	}

	bt_conn_unref(auth_conn);
	auth_conn = NULL;
	return 0;
}
static int cmd_esl_abs(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_abs\n");
	shell_fprintf(shell, SHELL_NORMAL, "#ABSTIME:%u\n", esl_get_abs_time());
	if (argc < 2) {
		return 0;
	}

	uint32_t new_abs_time = strtoul(argv[1], NULL, 10);

	if (new_abs_time < UINT32_MAX) {
		esl_abs_time_update(new_abs_time);
		shell_fprintf(shell, SHELL_NORMAL, "new abs time %u", new_abs_time);
	} else {
		shell_fprintf(shell, SHELL_ERROR, "new abs time is invalid\n");
		return -ENOEXEC;
	}

	return 0;
}
#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
static int cmd_esl_img_dump(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_img_dump\n");
	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <IMAGE_IDX>\n");
		return -ENOEXEC;
	}

	uint8_t img_idx = strtoul(argv[1], NULL, 10);

	if (img_idx >= CONFIG_BT_ESL_IMAGE_MAX) {
		shell_fprintf(shell, SHELL_ERROR, "image index is invalid\n");
	}

	struct bt_esls *esl = esl_get_esl_obj();
	size_t size = esl->cb.read_img_size_from_storage(img_idx);
	void *data;

	(void)ots_obj_cal_checksum(NULL, NULL, img_idx, 0, size, &data);
	shell_hexdump(shell, data, 200);
	return 0;
}

static int cmd_esl_obj_checksum(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "Compare Tag Image checksum\n");

	if (argc < 2) {
		shell_fprintf(shell, SHELL_ERROR, "no valid parameter <stored image idx>\n");
		return -ENOEXEC;
	}
	uint16_t img_idx = strtol(argv[1], NULL, 16);
	uint32_t crc;
	struct bt_esls *esl = esl_get_esl_obj();
	size_t size = esl->cb.read_img_size_from_storage(img_idx);
	void *data;

	shell_fprintf(shell, SHELL_NORMAL, "size in storage %d\n",
		      esl->cb.read_img_size_from_storage(img_idx));
	(void)ots_obj_cal_checksum(NULL, NULL, img_idx, 0, size, &data);
	crc = crc32_ieee(data, size);
	shell_fprintf(shell, SHELL_NORMAL, "Calculated checksum 0x%08x\n", crc);

	return 0;
}

#endif
static int cmd_esl_security(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_security\n");
	struct bt_esls *esl = esl_get_esl_obj();

	shell_fprintf(shell, SHELL_NORMAL, "current %d\n", bt_conn_get_security(esl->conn));

	if (argc > 2) {
		int16_t err;
		uint16_t security_level;

		security_level = strtol(argv[1], NULL, 16);
		if (security_level == BT_SECURITY_L2) {
			security_level |= BT_SECURITY_FORCE_PAIR;
		}

		err = bt_conn_set_security(esl->conn, security_level);
		printk("set bt_conn_set_security %d err %d\n", security_level, err);
	}
	return 0;
}

static int cmd_esl_configuring(const struct shell *shell, size_t argc, char *argv[])
{

	shell_fprintf(shell, SHELL_NORMAL, "cmd_esl_configuring\n");
	struct bt_esls *esl = esl_get_esl_obj();

	shell_fprintf(shell, SHELL_NORMAL, " %ld\n", atomic_get(&esl->configuring_state));

	if (argc >= 3) {
		uint8_t bit = strtol(argv[1], NULL, 16);
		uint8_t on_off = strtol(argv[2], NULL, 16);

		if (on_off == 1) {
			atomic_set_bit(&esl->configuring_state, bit);
		} else {
			atomic_clear_bit(&esl->configuring_state, bit);
		}
	}

	return 0;
}

static int cmd_esl_whoami(const struct shell *shell, size_t argc, char *argv[])
{
	shell_fprintf(shell, SHELL_NORMAL, "WHOAMI:TAG\n");
	return 0;
}

static int cmd_bd_addr(const struct shell *shell, size_t argc, char *argv[])
{
	bt_addr_le_t addr;
	char addr_str[BT_ADDR_LE_STR_LEN];
	struct bt_le_oob oob;
	extern uint32_t sync_timeout_count;
	extern uint32_t acl_timeout_count;

	bt_id_read_public_addr(&addr);
	bt_addr_le_to_str(&addr, addr_str, sizeof(addr_str));
	shell_fprintf(shell, SHELL_NORMAL, "bt_id_read_public_addr BD_ADDR: %s\n", addr_str);
	bt_le_oob_get_local(0, &oob);
	bt_addr_le_to_str(&oob.addr, addr_str, sizeof(addr_str));
	shell_fprintf(shell, SHELL_NORMAL, "bt_le_oob_get_local BD_ADDR: %s\n", addr_str);
	shell_fprintf(shell, SHELL_NORMAL, "sync_timeout_count %d acl_timeout_count %d\n",
		      sync_timeout_count, acl_timeout_count);

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	esl_cmds, SHELL_CMD(dump, NULL, "dump ESL characters", cmd_esl_dump),
	SHELL_CMD(set_bas, NULL, "set ESL basic state bits", cmd_set_esl_basic),
	SHELL_CMD(get_bas, NULL, "get ESL basic state bits", cmd_get_esl_basic),
	SHELL_CMD(addr, NULL, "set/get ESL ADDR", cmd_esl_addr), SHELL_SUBCMD_SET_END);

SHELL_STATIC_SUBCMD_SET_CREATE(
	esl_commands, SHELL_CMD(state, NULL, "Show current state machine", cmd_esl_state),
	SHELL_CMD(unassociated, NULL, "ESL debug commands", cmd_esl_unassociated),
	SHELL_CMD(factory, NULL, "ESL debug commands", cmd_esl_factory_reset),
	SHELL_CMD(bond_dump, NULL, "ESL dump bond peer addr", cmd_esl_bond_dump),
	SHELL_CMD(unbond, NULL, "ESL dump bond peer addr", cmd_esl_unbond),
	SHELL_CMD(disconnect, NULL, "ESL disconect peer", cmd_esl_disconnect),
	SHELL_CMD(adv, NULL, "ESL advertising start/stop", cmd_esl_adv),
	SHELL_CMD(led, NULL, "LED control for debugging", cmd_esl_led),
	SHELL_CMD(display, NULL, "DISPLAY control for debugging", cmd_esl_display),
	SHELL_CMD(led_work_dump, NULL, "LED work item status dump for debugging",
		  cmd_esl_led_work_dump),
	SHELL_CMD(display_work_dump, NULL, "DISPLAY work item status for debugging",
		  cmd_esl_display_work_dump),
	SHELL_CMD(sensor, NULL, "Read Sensor value for debugging", cmd_esl_sensor),
	SHELL_CMD(busy, NULL, "ESL debug command to enable retry response", cmd_esl_busy),
	SHELL_CMD(accept, NULL, "ESL pairing key comply", cmd_esl_accept_pair),
	SHELL_CMD(security, NULL, "ESL request security change", cmd_esl_security),
	SHELL_CMD(configuring_bit, NULL, "ESL set/get configuring bit", cmd_esl_configuring),
	SHELL_CMD(abs, NULL, "ESL debug command to read/write absolute time", cmd_esl_abs),
#if defined(CONFIG_BT_ESL_IMAGE_AVAILABLE)
	SHELL_CMD(check, NULL, "ESL Calculate Image checksum", cmd_esl_obj_checksum),
	SHELL_CMD(image_dump, NULL, "ESL debug command to dump image obj content",
		  cmd_esl_img_dump),
#endif
	SHELL_CMD(bd_addr, NULL, "ESL device BD Addr", cmd_bd_addr),
	SHELL_CMD(esl, &esl_cmds, "ESL debug commands", NULL), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(WHOAMI, NULL, "Tell UI what role I am", cmd_esl_whoami);
SHELL_CMD_REGISTER(esl, &esl_commands, "ESL commands", NULL);

static int esl_shell_init(void)
{
	context.shell = NULL;
	return 0;
}

SYS_INIT(esl_shell_init, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
