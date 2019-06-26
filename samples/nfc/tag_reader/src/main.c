/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <stdbool.h>
#include <zephyr.h>
#include <misc/printk.h>

#include <drivers/st25r3911b_nfca.h>

#define NFC_T2T_READ_CMD 0x30
#define NFC_BLOCK_TO_READ_NUM 0

#define NFC_T2T_READ_CMD_LEN 0x02
#define NFC_T2T_BLOCK_LEN 0x10

#define NFC_TX_DATA_LEN NFC_T2T_READ_CMD_LEN
#define NFC_RX_DATA_LEN NFC_T2T_BLOCK_LEN

#define TRANSMIT_DELAY 3000
#define ALL_REQ_DELAY 2000
#define T2T_READ_CMD_FDT 2068

static u8_t tx_data[NFC_TX_DATA_LEN] = {NFC_T2T_READ_CMD, NFC_BLOCK_TO_READ_NUM};
static u8_t rx_data[NFC_RX_DATA_LEN];

static struct k_poll_event events[ST25R3911B_NFCA_EVENT_CNT];
static struct k_delayed_work transmit_work;

static const struct st25r3911b_nfca_buf tx_buf = {
	.data = tx_data,
	.len = sizeof(tx_data)
};

static const struct st25r3911b_nfca_buf rx_buf = {
	.data = rx_data,
	.len = sizeof(rx_data)
};

static void transfer_handler(struct k_work *work)
{
	int err = st25r3911b_nfca_tag_detect(ST25R3911B_NFCA_DETECT_CMD_SENS_REQ);

	if (err) {
		printk("Tag detect error: %d\n", err);
	}
}

static void nfc_field_on(void)
{
	printk("NFC field on\n");

	int err = st25r3911b_nfca_tag_detect(ST25R3911B_NFCA_DETECT_CMD_SENS_REQ);

	if (err) {
		printk("Tag select error: %d\n", err);
	}
}

static void nfc_timeout(bool tag_sleep)
{
	int err;

	if (tag_sleep) {
		printk("Tag sleep or no detected, sending ALL Request\n");
	}

	/* Sleep will block processing loop. Accepted as it is short. */
	k_sleep(ALL_REQ_DELAY);

	err = st25r3911b_nfca_tag_detect(ST25R3911B_NFCA_DETECT_CMD_ALL_REQ);

	if (err) {
		printk("Tag detect error: %d\n", err);
	}
}

static void nfc_field_off(void)
{
	printk("NFC field off\n");
}

static void tag_detected(const struct st25r3911b_nfca_sens_resp *sens_resp)
{
	printk("Anticollision: 0x%x Platform: 0x%x\n",
		sens_resp->anticollison,
		sens_resp->platform_info);

	int err = st25r3911b_nfca_anticollision_start();

	if (err) {
		printk("Anticollision error: %d\n", err);
	}
}

static void anticollision_completed(const struct st25r3911b_nfca_tag_info *tag_info,
				    int err)
{
	if (err) {
		printk("Error during anticollision avoidance.\n");

		err = st25r3911b_nfca_tag_detect(ST25R3911B_NFCA_DETECT_CMD_SENS_REQ);
		if (err) {
			printk("Tag detect error %d.\n", err);
		}

		return;
	}

	printk("Tag info, type: %d\n", tag_info->type);

	if (tag_info->type == ST25R3911B_NFCA_TAG_TYPE_T2T) {
		printk("Type 2 Tag.\n");
	} else {
		printk("Unsupported tag type.\n");

		err = st25r3911b_nfca_tag_detect(ST25R3911B_NFCA_DETECT_CMD_SENS_REQ);
		if (err) {
			printk("Tag detect error %d.\n", err);
		}

		return;
	}

	err = st25r3911b_nfca_transfer_with_crc(&tx_buf, &rx_buf,
						T2T_READ_CMD_FDT);

	if (err) {
		printk("Transaction error\n");
	}
}

static void transfer_completed(const u8_t *data, size_t len, int err)
{
	if (err) {
		printk("NFC Transfer error: %d\n", err);
	} else {
		printk("Transmission completed.\n");
	}

	printk("Received data:\n");

	for (size_t i = 0; i < len; i++) {
		printk("%u ", data[i]);
	}

	printk("\n");

	st25r3911b_nfca_tag_sleep();

	k_delayed_work_submit(&transmit_work, TRANSMIT_DELAY);
}

static void tag_sleep(void)
{
	printk("Tag entered the Sleep state.\n");
}

static const struct st25r3911b_nfca_cb cb = {
	.field_on = nfc_field_on,
	.field_off = nfc_field_off,
	.tag_detected = tag_detected,
	.anticollision_completed = anticollision_completed,
	.rx_timeout = nfc_timeout,
	.transfer_completed = transfer_completed,
	.tag_sleep = tag_sleep
};

void main(void)
{
	int err;

	printk("NFC reader sample started\n");

	k_delayed_work_init(&transmit_work, transfer_handler);

	err = st25r3911b_nfca_init(events, ARRAY_SIZE(events), &cb);
	if (err) {
		printk("NFCA initialization failed err: %d\n", err);
		return;
	}

	err = st25r3911b_nfca_field_on();
	if (err) {
		printk("Field on error %d", err);
		return;
	}

	while (true) {
		k_poll(events, ARRAY_SIZE(events), K_FOREVER);
		err = st25r3911b_nfca_process();
		if (err) {
			printk("NFC-A process failed, err: %d\n", err);
			return;
		}
	}
}
