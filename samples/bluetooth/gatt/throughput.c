/** @file
 *  @brief Throughput service sample
 */

/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

#include <kernel.h>
#include <misc/printk.h>
#include <string.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>

#include "throughput.h"

static struct metrics met;

static ssize_t write_callback(struct bt_conn *conn,
			      const struct bt_gatt_attr *attr, const void *buf,
			      u16_t len, u16_t offset, u8_t flags)
{
	static u32_t clock_cycles;
	static u32_t kb;

	u64_t delta;

	struct metrics *met_data = attr->user_data;

	delta = k_cycle_get_32() - clock_cycles;
	delta = SYS_CLOCK_HW_CYCLES_TO_NS64(delta);

	if (len == 1) {
		/* reset metrics */
		kb = 0;
		met_data->write_count = 0;
		met_data->write_len = 0;
		met_data->write_rate = 0;
		clock_cycles = k_cycle_get_32();
		printk("\n");
	} else {
		met_data->write_count++;
		met_data->write_len += len;
		met_data->write_rate =
		    ((u64_t)met_data->write_len << 3) * 1000000000 / delta;

		if ((met_data->write_len / 1024) != kb) {
			kb = (met_data->write_len / 1024);
			printk("=");
		}
	}

	return len;
}

static ssize_t read_callback(struct bt_conn *conn,
			     const struct bt_gatt_attr *attr, void *buf,
			     u16_t len, u16_t offset)
{
	const struct metrics *metrics = attr->user_data;
	len = min(sizeof(struct metrics), len);

	printk("\n[local] received %u bytes (%u KB)"
	       " in %u GATT writes at %u bps\n",
	       metrics->write_len, metrics->write_len / 1024,
	       metrics->write_count, metrics->write_rate);

	return bt_gatt_attr_read(
		conn, attr, buf, len, offset, attr->user_data, len);
}

/* Throughput service declaration */
static struct bt_gatt_attr attrs[] = {
	BT_GATT_PRIMARY_SERVICE(BT_UUID_THROUGHPUT),
	BT_GATT_CHARACTERISTIC(BT_UUID_THROUGHPUT_CHAR,
		BT_GATT_CHRC_READ | BT_GATT_CHRC_WRITE_WITHOUT_RESP,
		BT_GATT_PERM_READ | BT_GATT_PERM_WRITE,
		read_callback, write_callback, &met),
};

static struct bt_gatt_service throughput_svc = BT_GATT_SERVICE(attrs);

void throughput_init(void)
{
	bt_gatt_service_register(&throughput_svc);
}
