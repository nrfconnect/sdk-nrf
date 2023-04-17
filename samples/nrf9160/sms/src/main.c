/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <string.h>
#include <modem/sms.h>
#include <modem/nrf_modem_lib.h>
#include <modem/lte_lc.h>


static void sms_callback(struct sms_data *const data, void *context)
{
	if (data == NULL) {
		printk("%s with NULL data\n", __func__);
		return;
	}

	if (data->type == SMS_TYPE_DELIVER) {
		/* When SMS message is received, print information */
		struct sms_deliver_header *header = &data->header.deliver;

		printk("\nSMS received:\n");
		printk("\tTime:   %02d-%02d-%02d %02d:%02d:%02d\n",
			header->time.year,
			header->time.month,
			header->time.day,
			header->time.hour,
			header->time.minute,
			header->time.second);

		printk("\tText:   '%s'\n", data->payload);
		printk("\tLength: %d\n", data->payload_len);

		if (header->app_port.present) {
			printk("\tApplication port addressing scheme: dest_port=%d, src_port=%d\n",
				header->app_port.dest_port,
				header->app_port.src_port);
		}
		if (header->concatenated.present) {
			printk("\tConcatenated short message: ref_number=%d, msg %d/%d\n",
				header->concatenated.ref_number,
				header->concatenated.seq_number,
				header->concatenated.total_msgs);
		}
	} else if (data->type == SMS_TYPE_STATUS_REPORT) {
		printk("SMS status report received\n");
	} else {
		printk("SMS protocol message with unknown type received\n");
	}
}

int main(void)
{
	int handle = 0;
	int ret = 0;

	printk("\nSMS sample starting\n");

	ret = nrf_modem_lib_init();
	if (ret) {
		printk("Modem library initialization failed, error: %d\n", ret);
		return 0;
	}

	ret = lte_lc_init_and_connect();
	if (ret) {
		printk("Lte_lc failed to initialize and connect, err %d", ret);
		return 0;
	}

	handle = sms_register_listener(sms_callback, NULL);
	if (handle) {
		printk("sms_register_listener returned err: %d\n", handle);
		return 0;
	}

	printk("SMS sample is ready for receiving messages\n");

	/* Sending is done to the phone number specified in the configuration,
	 * or if it's left empty, an information text is printed.
	 */
	if (strcmp(CONFIG_SMS_SEND_PHONE_NUMBER, "")) {
		printk("Sending SMS: number=%s, text=\"SMS sample: testing\"\n",
			CONFIG_SMS_SEND_PHONE_NUMBER);
		ret = sms_send_text(CONFIG_SMS_SEND_PHONE_NUMBER, "SMS sample: testing");
		if (ret) {
			printk("Sending SMS failed with error: %d\n", ret);
		}
	} else {
		printk("\nSMS sending is skipped but receiving will still work.\n"
			"If you wish to send SMS, please configure CONFIG_SMS_SEND_PHONE_NUMBER\n");
	}

	/* In our application, we should unregister SMS in some conditions with:
	 *   sms_unregister_listener(handle);
	 * However, this sample will continue to be registered for
	 * received SMS messages and they can be seen in serial port log.
	 */
	return 0;
}
