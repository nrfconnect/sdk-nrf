/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/console/console.h>
#include <string.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>

#if defined(CONFIG_USB_DEVICE_STACK)
#include <zephyr/usb/usb_device.h>
#include <zephyr/drivers/uart.h>
#endif

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <sdc_hci_vs.h>


static bool on_vs_evt(struct net_buf_simple *buf)
{
	uint8_t code;
	sdc_hci_subevent_vs_qos_channel_survey_report_t *evt;

	code = net_buf_simple_pull_u8(buf);
	if (code != SDC_HCI_SUBEVENT_VS_QOS_CHANNEL_SURVEY_REPORT) {
		return false;
	}

	evt = (void *)buf->data;
	for (uint32_t chan = 0; chan < 40; chan++) {
		/*RSSI will always be higher than -127,
		  so adding 127 brings it into positive
		*/
		int8_t rssi = evt->channel_energy[chan];
		uint8_t symobls = (rssi+127)/10;

		printk("chan %i ", chan);
		for (uint32_t i = 0; i < symobls; i++){
			printk("-");
		}
		printk(" rssi: %i\n", rssi);
	}
	return true;
}

static int enable_channel_survey_report(void)
{
	int err;
	struct net_buf *buf;

	err = bt_hci_register_vnd_evt_cb(on_vs_evt);
	if (err) {
		printk("Failed registering vendor specific callback (err %d)\n",
			   err);
		return err;
	}

	sdc_hci_cmd_vs_qos_channel_survey_enable_t *cmd_enable;

	buf = bt_hci_cmd_create(SDC_HCI_OPCODE_CMD_VS_QOS_CHANNEL_SURVEY_ENABLE,
				sizeof(*cmd_enable));
	if (!buf) {
		printk("Could not allocate command buffer\n");
		return -ENOMEM;
	}

	cmd_enable = net_buf_add(buf, sizeof(*cmd_enable));
	cmd_enable->enable = true;
	cmd_enable->interval_us = 4000000;

	err = bt_hci_cmd_send_sync(
		SDC_HCI_OPCODE_CMD_VS_QOS_CHANNEL_SURVEY_ENABLE, buf, NULL);
	if (err) {
		printk("Could not send command buffer (err %d)\n", err);
		return err;
	}

	printk("Channel survey reports enabled\n");
	return 0;
}

int main(void)
{
	int err;

	console_init();

	printk("Starting Bluetooth LLPM example\n");

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	printk("Bluetooth initialized\n");

	if (enable_channel_survey_report()) {
		printk("Enable Channel survey failed\n");
		return 0;
	}

	for (;;) {
		k_sleep(K_MSEC(1000));
	}
}
