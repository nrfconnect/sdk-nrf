/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief Nordic mesh light fixture sample
 */
#include <zephyr/init.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/mesh/models.h>
#include <bluetooth/mesh/dk_prov.h>
#include <dk_buttons_and_leds.h>
#include <zephyr/dfu/mcuboot.h>
#include "model_handler.h"
#include "lc_pwm_led.h"

#ifdef CONFIG_MCUMGR_SMP_BT
#include <mgmt/mcumgr/smp_bt.h>
#endif
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
#include "os_mgmt/os_mgmt.h"
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
#include "img_mgmt/img_mgmt.h"
#endif
#include <device.h>
#include <soc.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>

#define BT_DBG_ENABLED IS_ENABLED(CONFIG_BT_MESH_DEBUG)
#define LOG_MODULE_NAME main_c
#include "common/log.h"

#ifdef CONFIG_EMDS
#include <emds/emds.h>

#define EMDS_DEV_IRQ 24
#define EMDS_DEV_PRIO 0
#define EMDS_ISR_ARG 0
#define EMDS_IRQ_FLAGS 0

static void pending_adv_start(struct k_work *work);

static struct bt_le_ext_adv *adv;
static struct bt_le_ext_adv_start_param ext_adv_param = { .num_events = 0, .timeout = 0 };
static struct k_work_delayable adv_work = Z_WORK_DELAYABLE_INITIALIZER(pending_adv_start);

static struct bt_le_adv_param smp_adv_params = {
	.id = BT_ID_DEFAULT,
	.sid = 0,
	.secondary_max_skip = 0,
	.options = BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME,
	.interval_min = BT_GAP_ADV_SLOW_INT_MIN,
	.interval_max = BT_GAP_ADV_SLOW_INT_MAX,
	.peer = NULL
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID128_ALL,
		      0x84, 0xaa, 0x60, 0x74, 0x52, 0x8a, 0x8b, 0x86,
		      0xd3, 0x4c, 0xb7, 0x1d, 0x1d, 0xdc, 0x53, 0x8d),
};

static void pending_adv_start(struct k_work *work)
{
	int err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		printk("KW: Advertising SMP service failed (err %d)\n", err);
		return;
	}

	printk("KW: Advertising SMP service\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	struct bt_conn_info cinfo;
	int ec = bt_conn_get_info(conn, &cinfo);
	if (ec) {
		printk("bt_conn_get_info: err %d\n", ec);
		return;
	}

	printk("Connected: err %d id %d\n", err, cinfo.id);

	if (cinfo.id != smp_adv_params.id) {
		return;
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	int err;
	struct bt_conn_info cinfo;

	err = bt_conn_get_info(conn, &cinfo);
	if (err) {
		printk("bt_conn_get_info: err %d\n", err);
		return;
	}

	printk("Disconnected: reason %d id %d\n", reason, cinfo.id);

	if (cinfo.id != smp_adv_params.id) {
		return;
	}

	k_work_schedule(&adv_work, K_NO_WAIT);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

/* To correctly create new identity, this must be done after bt_enable()
 * has completed */
static void ble_hdl_init(void)
{
#ifdef CONFIG_MCUMGR_CMD_OS_MGMT
	os_mgmt_register_group();
#endif
#ifdef CONFIG_MCUMGR_CMD_IMG_MGMT
	img_mgmt_register_group();
#endif
	bt_conn_cb_register(&conn_callbacks);
#ifdef CONFIG_MCUMGR_SMP_BT
	smp_bt_register();
#endif
}

static void ble_hdl_start(void)
{
	int err;
	size_t id_count = 0xFF;
	int id;

	bt_id_get(NULL, &id_count);

	if (id_count < CONFIG_BT_ID_MAX)
	{
		id = bt_id_create(NULL, NULL);
		if (id < 0) {
			printk("Error in creating new ID for SMP\n");
			id = BT_ID_DEFAULT;
		}
		printk("New id for SMP: %d\n", id);
	} else {
		id = BT_ID_DEFAULT + 1;
		printk("id for SMP: %d\n", id);
	}

	smp_adv_params.id = id;

	err = bt_le_ext_adv_create(&smp_adv_params, NULL, &adv);
	if (err) {
		printk("Creating SMP service adv instance failed (err %d)\n", err);
	}

	err = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Setting SMP service adv data failed (err %d)\n", err);
	}

	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		printk("Starting advertising of SMP service failed (err %d)\n", err);
	}

	printk("Advertising SMP service\n");
}

static void button_handler_cb(uint32_t pressed, uint32_t changed)
{
	if (!bt_mesh_is_provisioned()) {
		return;
	}

	if (pressed & changed & BIT(3)) {
		NVIC_SetPendingIRQ(EMDS_DEV_IRQ);
	}
}

static void app_emds_cb(void)
{
	printk("SAMPLE HALTED!!!\n");
	dk_set_leds(DK_LED2_MSK | DK_LED3_MSK | DK_LED4_MSK);
	k_fatal_halt(K_ERR_CPU_EXCEPTION);
}

static void isr_emds_cb(void *arg)
{
	ARG_UNUSED(arg);

#if defined(CONFIG_BT_CTLR)
	/* Stop mpsl to reduce power usage. */
	irq_disable(TIMER0_IRQn);
	irq_disable(RTC0_IRQn);
	irq_disable(RADIO_IRQn);

	mpsl_uninit();
#endif

	emds_store();
}
#endif

static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	dk_leds_init();
	dk_buttons_init(NULL);

#ifdef CONFIG_EMDS
	static struct button_handler button_handler = {
		.cb = button_handler_cb,
	};

	dk_button_handler_add(&button_handler);

	err = emds_init(&app_emds_cb);
	if (err) {
		printk("Initializing emds failed (err %d)\n", err);
		return;
	}
#endif

	err = bt_mesh_init(bt_mesh_dk_prov_init(), model_handler_init());
	if (err) {
		printk("Initializing mesh failed (err %d)\n", err);
		return;
	}

#ifdef CONFIG_EMDS
	err = emds_load();
	if (err) {
		printk("Restore of emds data failed (err %d)\n", err);
		return;
	}

	err = emds_prepare();
	if (err) {
		printk("Preparation emds failed (err %d)\n", err);
		return;
	}

	IRQ_CONNECT(EMDS_DEV_IRQ, EMDS_DEV_PRIO, isr_emds_cb,
		    EMDS_ISR_ARG, EMDS_IRQ_FLAGS);
	irq_enable(EMDS_DEV_IRQ);
#endif

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	/* This will be a no-op if settings_load() loaded provisioning info */
	bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);

	printk("Mesh initialized\n");

	model_handler_start();
}

void main(void)
{
	int err;

	printk("Initializing...\n");
	lc_pwm_led_init();
	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}

	ble_hdl_init();
	ble_hdl_start();
}
