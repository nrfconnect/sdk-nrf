/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/bluetooth/bluetooth.h>
#include <bluetooth/enocean.h>
#include <zephyr/settings/settings.h>
#include <dk_buttons_and_leds.h>

static void enocean_button(struct bt_enocean_device *device,
			   enum bt_enocean_button_action action, uint8_t changed,
			   const uint8_t *opt_data, size_t opt_data_len)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&device->addr, addr, sizeof(addr));

	printk("EnOcean button RX: %s: %s %u %u %u %u\n", addr,
	       action == BT_ENOCEAN_BUTTON_PRESS ? "pressed: " : "released:",
	       !!(changed & BT_ENOCEAN_SWITCH_OA),
	       !!(changed & BT_ENOCEAN_SWITCH_IA),
	       !!(changed & BT_ENOCEAN_SWITCH_OB),
	       !!(changed & BT_ENOCEAN_SWITCH_IB));

	if (changed & (BT_ENOCEAN_SWITCH_OA | BT_ENOCEAN_SWITCH_IA)) {
		dk_set_led(DK_LED1, (action == BT_ENOCEAN_BUTTON_PRESS));
		dk_set_led(DK_LED3, !!(changed & BT_ENOCEAN_SWITCH_OA));
	}

	if (changed & (BT_ENOCEAN_SWITCH_OB | BT_ENOCEAN_SWITCH_IB)) {
		dk_set_led(DK_LED2, (action == BT_ENOCEAN_BUTTON_PRESS));
		dk_set_led(DK_LED4, !!(changed & BT_ENOCEAN_SWITCH_OB));
	}
}

static void enocean_sensor(struct bt_enocean_device *device,
			   const struct bt_enocean_sensor_data *data,
			   const uint8_t *opt_data, size_t opt_data_len)
{
	char addr[BT_ADDR_LE_STR_LEN];

	dk_set_leds_state(DK_LED1_MSK | DK_LED2_MSK, 0);
	k_sleep(K_MSEC(50));
	dk_set_leds_state(0, DK_LED1_MSK | DK_LED2_MSK);

	bt_addr_le_to_str(&device->addr, addr, sizeof(addr));
	printk("EnOcean sensor RX: %s:\n", addr);

	if (data->occupancy) {
		printk("\tOccupancy:          %s\n",
		       *data->occupancy ? "true" : "false");
	}

	if (data->light_sensor) {
		printk("\tLight (sensor):     %u lx\n", *data->light_sensor);
	}

	if (data->energy_lvl) {
		printk("\tEnergy level:       %u %%\n", *data->energy_lvl);
	}

	if (data->battery_voltage) {
		printk("\tBattery voltage:    %u mV\n", *data->battery_voltage);
	}

	if (data->light_solar_cell) {
		printk("\tLight (solar cell): %u lx\n",
		       *data->light_solar_cell);
	}
}

static void enocean_commissioned(struct bt_enocean_device *device)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&device->addr, addr, sizeof(addr));
	printk("EnOcean Device commissioned: %s\n", addr);

	/* Blink leds number of times to show commissioned */
	for (int i = 0; i < 4; ++i) {
		dk_set_leds_state(DK_LED1_MSK | DK_LED2_MSK, 0);
		k_sleep(K_MSEC(100));
		dk_set_leds_state(0, DK_LED1_MSK | DK_LED2_MSK);
		k_sleep(K_MSEC(100));
	}
}

static void enocean_decommissioned(struct bt_enocean_device *device)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&device->addr, addr, sizeof(addr));
	printk("EnOcean Device decommissioned: %s\n", addr);

	/* Blink leds number of times to show decommissioned */
	for (int i = 0; i < 4; ++i) {
		dk_set_leds_state(DK_LED3_MSK | DK_LED4_MSK, 0);
		k_sleep(K_MSEC(100));
		dk_set_leds_state(0, DK_LED3_MSK | DK_LED4_MSK);
		k_sleep(K_MSEC(100));
	}
}

static void enocean_loaded(struct bt_enocean_device *device)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&device->addr, addr, sizeof(addr));
	printk("EnOcean Device loaded: %s\n", addr);
}

int main(void)
{
	int err;

	err = dk_leds_init();
	if (err) {
		printk("Initializing dk_leds failed (err: %d)\n", err);
		return 0;
	}

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	static const struct bt_enocean_callbacks enocean_callbacks = {
		.button = enocean_button,
		.sensor = enocean_sensor,
		.commissioned = enocean_commissioned,
		.decommissioned = enocean_decommissioned,
		.loaded = enocean_loaded,
	};

	bt_enocean_init(&enocean_callbacks);

	if (IS_ENABLED(CONFIG_SETTINGS)) {
		settings_load();
	}

	err = bt_le_scan_start(BT_LE_SCAN_PASSIVE, NULL);
	if (err) {
		printk("Bluetooth scan start failed (err %d)\n", err);
		return 0;
	}

	bt_enocean_commissioning_enable();

	printk("EnOcean sample is ready!\n");

	return 0;
}
