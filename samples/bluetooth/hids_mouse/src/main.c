/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <misc/printk.h>
#include <misc/byteorder.h>
#include <zephyr.h>
#include <gpio.h>
#include <board.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>

#include <gatt/dis.h>
#include <gatt/bas.h>
#include <bluetooth/services/hids.h>

#define DEVICE_NAME     CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define BASE_USB_HID_SPEC_VERSION   0x0101

/* Number of pixels by which the cursor is moved when a button is pushed. */
#define MOVEMENT_SPEED              5
/* Number of input reports in this application. */
#define INPUT_REPORT_COUNT          3
/* Length of Mouse Input Report containing button data. */
#define INPUT_REP_BUTTONS_LEN       3
/* Length of Mouse Input Report containing movement data. */
#define INPUT_REP_MOVEMENT_LEN      3
/* Length of Mouse Input Report containing media player data. */
#define INPUT_REP_MEDIA_PLAYER_LEN  1
/* Index of Mouse Input Report containing button data. */
#define INPUT_REP_BUTTONS_INDEX     0
/* Index of Mouse Input Report containing movement data. */
#define INPUT_REP_MOVEMENT_INDEX    1
/* Index of Mouse Input Report containing media player data. */
#define INPUT_REP_MPLAYER_INDEX     2
/* Id of reference to Mouse Input Report containing button data. */
#define INPUT_REP_REF_BUTTONS_ID    1
/* Id of reference to Mouse Input Report containing movement data. */
#define INPUT_REP_REF_MOVEMENT_ID   2
/* Id of reference to Mouse Input Report containing media player data. */
#define INPUT_REP_REF_MPLAYER_ID    3

HIDS_DEF(hids_obj); /* HIDS instance. */

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      0x12, 0x18,       /* HID Service */
		      0x0f, 0x18),      /* Battery Service */
};

static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),
};

static struct device       *gpio_devs[4];
static struct gpio_callback gpio_cbs[4];
static bool		    in_boot_mode;


static void advertising_start(void)
{
	int err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad),
				  sd, ARRAY_SIZE(sd));

	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}


static void connected(struct bt_conn *conn, u8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err) {
		printk("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected %s\n", addr);

	if (bt_conn_security(conn, BT_SECURITY_MEDIUM)) {
		printk("Failed to set security\n");
	}
}


static void disconnected(struct bt_conn *conn, u8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected from %s (reason %u)\n", addr, reason);

	advertising_start();
}


static void security_changed(struct bt_conn *conn, bt_security_t level)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Security changed: %s level %u\n", addr, level);
}


static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};


static void hids_pm_evt_handler(enum hids_pm_evt evt)
{
	switch (evt) {
	case HIDS_PM_EVT_BOOT_MODE_ENTERED:
		printk("Boot mode entered\n");
		in_boot_mode = true;
		break;

	case HIDS_PM_EVT_REPORT_MODE_ENTERED:
		printk("Report mode entered\n");
		in_boot_mode = false;
		break;

	default:
		break;
	}
}


static void hog_init(void)
{
	int err;
	struct hids_init hids_init_obj = { 0 };
	struct hids_inp_rep *hids_inp_rep;

	static u8_t buttons_buff[INPUT_REP_BUTTONS_LEN];
	static u8_t movement_buff[INPUT_REP_MOVEMENT_LEN];
	static u8_t media_buff[INPUT_REP_MEDIA_PLAYER_LEN];
	static const u8_t report_map[] = {
		0x05, 0x01,     /* Usage Page (Generic Desktop) */
		0x09, 0x02,     /* Usage (Mouse) */

		0xA1, 0x01,     /* Collection (Application) */

		/* Report ID 1: Mouse buttons + scroll/pan */
		0x85, 0x01,       /* Report Id 1 */
		0x09, 0x01,       /* Usage (Pointer) */
		0xA1, 0x00,       /* Collection (Physical) */
		0x95, 0x05,       /* Report Count (3) */
		0x75, 0x01,       /* Report Size (1) */
		0x05, 0x09,       /* Usage Page (Buttons) */
		0x19, 0x01,       /* Usage Minimum (01) */
		0x29, 0x05,       /* Usage Maximum (05) */
		0x15, 0x00,       /* Logical Minimum (0) */
		0x25, 0x01,       /* Logical Maximum (1) */
		0x81, 0x02,       /* Input (Data, Variable, Absolute) */
		0x95, 0x01,       /* Report Count (1) */
		0x75, 0x03,       /* Report Size (3) */
		0x81, 0x01,       /* Input (Constant) for padding */
		0x75, 0x08,       /* Report Size (8) */
		0x95, 0x01,       /* Report Count (1) */
		0x05, 0x01,       /* Usage Page (Generic Desktop) */
		0x09, 0x38,       /* Usage (Wheel) */
		0x15, 0x81,       /* Logical Minimum (-127) */
		0x25, 0x7F,       /* Logical Maximum (127) */
		0x81, 0x06,       /* Input (Data, Variable, Relative) */
		0x05, 0x0C,       /* Usage Page (Consumer) */
		0x0A, 0x38, 0x02, /* Usage (AC Pan) */
		0x95, 0x01,       /* Report Count (1) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0xC0,             /* End Collection (Physical) */

		/* Report ID 2: Mouse motion */
		0x85, 0x02,       /* Report Id 2 */
		0x09, 0x01,       /* Usage (Pointer) */
		0xA1, 0x00,       /* Collection (Physical) */
		0x75, 0x0C,       /* Report Size (12) */
		0x95, 0x02,       /* Report Count (2) */
		0x05, 0x01,       /* Usage Page (Generic Desktop) */
		0x09, 0x30,       /* Usage (X) */
		0x09, 0x31,       /* Usage (Y) */
		0x16, 0x01, 0xF8, /* Logical maximum (2047) */
		0x26, 0xFF, 0x07, /* Logical minimum (-2047) */
		0x81, 0x06,       /* Input (Data, Variable, Relative) */
		0xC0,             /* End Collection (Physical) */
		0xC0,             /* End Collection (Application) */

		/* Report ID 3: Advanced buttons */
		0x05, 0x0C,       /* Usage Page (Consumer) */
		0x09, 0x01,       /* Usage (Consumer Control) */
		0xA1, 0x01,       /* Collection (Application) */
		0x85, 0x03,       /* Report Id (3) */
		0x15, 0x00,       /* Logical minimum (0) */
		0x25, 0x01,       /* Logical maximum (1) */
		0x75, 0x01,       /* Report Size (1) */
		0x95, 0x01,       /* Report Count (1) */

		0x09, 0xCD,       /* Usage (Play/Pause) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x83, 0x01, /* Usage (Consumer Control Configuration) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xB5,       /* Usage (Scan Next Track) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xB6,       /* Usage (Scan Previous Track) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */

		0x09, 0xEA,       /* Usage (Volume Down) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x09, 0xE9,       /* Usage (Volume Up) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x25, 0x02, /* Usage (AC Forward) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0x0A, 0x24, 0x02, /* Usage (AC Back) */
		0x81, 0x06,       /* Input (Data,Value,Relative,Bit Field) */
		0xC0              /* End Collection */
	};

	hids_init_obj.rep_map.data = report_map;
	hids_init_obj.rep_map.size = sizeof(report_map);

	hids_init_obj.info.bcd_hid = BASE_USB_HID_SPEC_VERSION;
	hids_init_obj.info.b_country_code = 0x00;
	hids_init_obj.info.flags = (HIDS_REMOTE_WAKE |
			HIDS_NORMALLY_CONNECTABLE);

	hids_inp_rep = &hids_init_obj.inp_rep_group_init.reports[0];
	hids_inp_rep->buff.data = buttons_buff;
	hids_inp_rep->buff.size = sizeof(buttons_buff);
	hids_inp_rep->id = INPUT_REP_REF_BUTTONS_ID;
	hids_init_obj.inp_rep_group_init.cnt++;

	hids_inp_rep++;
	hids_inp_rep->buff.data = movement_buff;
	hids_inp_rep->buff.size = sizeof(movement_buff);
	hids_inp_rep->id = INPUT_REP_REF_MOVEMENT_ID;
	hids_init_obj.inp_rep_group_init.cnt++;

	hids_inp_rep++;
	hids_inp_rep->buff.data = media_buff;
	hids_inp_rep->buff.size = sizeof(media_buff);
	hids_inp_rep->id = INPUT_REP_REF_MPLAYER_ID;
	hids_init_obj.inp_rep_group_init.cnt++;

	hids_init_obj.is_mouse = true;
	hids_init_obj.pm_evt_handler = hids_pm_evt_handler;

	err = hids_init(&hids_obj, &hids_init_obj);
	__ASSERT(err == 0, "HIDS initialization failed\n");
}


static void bt_ready(int err)
{
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	bas_init();
	dis_init(CONFIG_SOC, "Manufacturer");
	hog_init();

	advertising_start();
}


static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}


static void auth_done(struct bt_conn *conn)
{
	printk("%s()\n", __func__);
	bt_conn_auth_pairing_confirm(conn);
}


static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = NULL,
	.passkey_entry = NULL,
	.cancel = auth_cancel,
	.pairing_confirm = auth_done,
};


static void mouse_movement_send(s16_t x_delta, s16_t y_delta)
{
	if (in_boot_mode) {
		x_delta = max(min(x_delta, SCHAR_MAX), SCHAR_MIN);
		y_delta = max(min(y_delta, SCHAR_MAX), SCHAR_MIN);

		hids_boot_mouse_inp_rep_send(&hids_obj,
					     NULL,
					     (s8_t) x_delta,
					     (s8_t) y_delta);
	} else {
		u16_t s12_x_delta;
		u16_t s12_y_delta;
		u8_t x_buff[2];
		u8_t y_buff[2];
		u8_t buffer[INPUT_REP_MOVEMENT_LEN];

		/* Check limits. */
		x_delta = max(min(x_delta, 0x07ff), -0x07ff);
		y_delta = max(min(y_delta, 0x07ff), -0x07ff);

		/* Convert to 12-bit signed integer. */
		s12_x_delta = (x_delta < 0) ? ((1 << 11) | x_delta) : x_delta;
		s12_y_delta = (y_delta < 0) ? ((1 << 11) | y_delta) : y_delta;

		/* Convert to little-endian. */
		sys_put_le16(s12_x_delta, x_buff);
		sys_put_le16(s12_y_delta, y_buff);

		/* Encode report. */
		buffer[0] = x_buff[0];
		buffer[1] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
		buffer[2] = y_buff[1];

		hids_inp_rep_send(&hids_obj, INPUT_REP_MOVEMENT_INDEX,
				  buffer, sizeof(buffer));
	}
}


void button_pressed(struct device *gpio_dev, struct gpio_callback *cb,
		    u32_t pins)
{
	s16_t val_x = 0;
	s16_t val_y = 0;

	if (pins & (1 << SW0_GPIO_PIN)) {
		val_x -= MOVEMENT_SPEED;
		printk("%s(): left\n", __func__);
	}
	if (pins & (1 << SW1_GPIO_PIN)) {
		val_y -= MOVEMENT_SPEED;
		printk("%s(): up\n", __func__);
	}
	if (pins & (1 << SW2_GPIO_PIN)) {
		val_x += MOVEMENT_SPEED;
		printk("%s(): right\n", __func__);
	}
	if (pins & (1 << SW3_GPIO_PIN)) {
		val_y += MOVEMENT_SPEED;
		printk("%s(): down\n", __func__);
	}

	mouse_movement_send(val_x, val_y);
}


void configure_buttons(void)
{
	static const u32_t pin_id[4] = { SW0_GPIO_PIN, SW1_GPIO_PIN,
		SW2_GPIO_PIN, SW3_GPIO_PIN };
	static const char *port_name[4] = { SW0_GPIO_NAME, SW1_GPIO_NAME,
		SW2_GPIO_NAME, SW3_GPIO_NAME };

	for (size_t i = 0; i < ARRAY_SIZE(pin_id); i++) {
		gpio_devs[i] = device_get_binding(port_name[i]);
		if (gpio_devs[i]) {
			printk("%s(): port %zu bound\n", __func__, i);

			gpio_pin_configure(gpio_devs[i], pin_id[i],
					   GPIO_PUD_PULL_UP | GPIO_DIR_IN |
					   GPIO_INT | GPIO_INT_EDGE |
					   GPIO_INT_ACTIVE_LOW);
			gpio_init_callback(&gpio_cbs[i], button_pressed,
					   BIT(pin_id[i]));
			gpio_add_callback(gpio_devs[i], &gpio_cbs[i]);
			gpio_pin_enable_callback(gpio_devs[i], pin_id[i]);
		}
	}
}


void main(void)
{
	int err;

	printk("Start zephyr\n");

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	err = bt_enable(bt_ready);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	configure_buttons();
}


