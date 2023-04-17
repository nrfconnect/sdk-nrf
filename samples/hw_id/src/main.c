/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <hw_id.h>

#if defined(CONFIG_BT)
#include <zephyr/bluetooth/bluetooth.h>
#endif /* defined(CONFIG_BT) */

int main(void)
{
	int err;

#if defined(CONFIG_BT)
	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
#endif /* defined(CONFIG_BT) */

	char buf[HW_ID_LEN] = "unsupported";

	err = hw_id_get(buf, ARRAY_SIZE(buf));
	if (err) {
		printk("hw_id_get failed (err %d)\n", err);
		return 0;
	}

	printk("hw_id: %s\n", buf);

	return 0;
}
