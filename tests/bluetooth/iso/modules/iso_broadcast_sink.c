/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/shell/shell.h>
#include <getopt.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(broadcast_sink, CONFIG_ISO_TEST_LOG_LEVEL);

#define TIMEOUT_SYNC_CREATE K_SECONDS(10)
#define NAME_LEN 30

#define BT_LE_SCAN_CUSTOM                                                                          \
	BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE, BT_GAP_SCAN_FAST_INTERVAL,   \
			 BT_GAP_SCAN_FAST_WINDOW)

#define PA_RETRY_COUNT 6

K_THREAD_STACK_DEFINE(broadcaster_sink_thread_stack, 4096);
static struct k_thread broadcaster_thread;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

static bool per_adv_found;
static bool per_adv_lost;
static bt_addr_le_t per_addr;
static uint8_t per_sid;
static uint32_t per_interval_us;
static bool running;

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);
static K_SEM_DEFINE(sem_per_big_info, 0, 1);
static K_SEM_DEFINE(sem_big_sync, 0, CONFIG_BIS_ISO_CHAN_COUNT_MAX);
static K_SEM_DEFINE(sem_big_sync_lost, 0, CONFIG_BIS_ISO_CHAN_COUNT_MAX);

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0:
		return "No packets";
	case BT_GAP_LE_PHY_1M:
		return "LE 1M";
	case BT_GAP_LE_PHY_2M:
		return "LE 2M";
	case BT_GAP_LE_PHY_CODED:
		return "LE Coded";
	default:
		return "Unknown";
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	LOG_DBG("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
		"C:%u S:%u D:%u SR:%u E:%u Prim: %s, Secn: %s, "
		"Interval: 0x%04x (%u us), SID: %u",
		le_addr, info->adv_type, info->tx_power, info->rssi, name,
		(info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
		(info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
		(info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
		(info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
		(info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0, phy2str(info->primary_phy),
		phy2str(info->secondary_phy), info->interval,
		BT_CONN_INTERVAL_TO_US(info->interval), info->sid);

	if (!per_adv_found && info->interval) {
		per_adv_found = true;

		per_sid = info->sid;
		per_interval_us = BT_CONN_INTERVAL_TO_US(info->interval);
		bt_addr_le_copy(&per_addr, info->addr);

		k_sem_give(&sem_per_adv);
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void sync_cb(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_DBG("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
		"Interval 0x%04x (%u ms), PHY %s",
		bt_le_per_adv_sync_get_index(sync), le_addr, info->interval, info->interval * 5 / 4,
		phy2str(info->phy));

	k_sem_give(&sem_per_sync);
}

static void term_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	LOG_DBG("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated",
		bt_le_per_adv_sync_get_index(sync), le_addr);

	per_adv_lost = true;
	k_sem_give(&sem_per_sync_lost);
}

static void recv_cb(struct bt_le_per_adv_sync *sync,
		    const struct bt_le_per_adv_sync_recv_info *info, struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char data_str[129];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	bin2hex(buf->data, buf->len, data_str, sizeof(data_str));

	LOG_DBG("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
		"RSSI %i, CTE %u, data length %u, data: %s",
		bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power, info->rssi,
		info->cte_type, buf->len, data_str);
}

static void biginfo_cb(struct bt_le_per_adv_sync *sync, const struct bt_iso_biginfo *biginfo)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(biginfo->addr, le_addr, sizeof(le_addr));

	LOG_DBG("BIG INFO[%u]: [DEVICE]: %s, sid 0x%02x, "
		"num_bis %u, nse %u, interval 0x%04x (%u ms), "
		"bn %u, pto %u, irc %u, max_pdu %u, "
		"sdu_interval %u us, max_sdu %u, phy %s, "
		"%s framing, %sencrypted",
		bt_le_per_adv_sync_get_index(sync), le_addr, biginfo->sid, biginfo->num_bis,
		biginfo->sub_evt_count, biginfo->iso_interval, (biginfo->iso_interval * 5 / 4),
		biginfo->burst_number, biginfo->offset, biginfo->rep_count, biginfo->max_pdu,
		biginfo->sdu_interval, biginfo->max_sdu, phy2str(biginfo->phy),
		biginfo->framing ? "with" : "without", biginfo->encryption ? "" : "not ");

	k_sem_give(&sem_per_big_info);
}

static struct bt_le_per_adv_sync_cb sync_callbacks = {
	.synced = sync_cb,
	.term = term_cb,
	.recv = recv_cb,
	.biginfo = biginfo_cb,
};

static void iso_recv(struct bt_iso_chan *chan, const struct bt_iso_recv_info *info,
		     struct net_buf *buf)
{
	int ret = 0;
	uint32_t count = 0;
	static uint32_t last_count;
	static uint32_t counts_fail;
	static uint32_t counts_success;

	if (buf->len == sizeof(count)) {
		count = sys_get_le32(buf->data);
	}

	if (!(info->flags & BT_ISO_FLAGS_VALID)) {
		LOG_ERR("bad frame");
	}

	/* Wrapping not considered. Overflow will take over a year.*/
	if (last_count) {
		if (count != (last_count + 1)) {
			counts_fail++;
		} else {
			counts_success++;
		}
	}

	last_count = count;

	if ((count % CONFIG_PRINT_CONN_INTERVAL) == 0) {
		LOG_INF("RX. Count: %d, Failed: %d, Success: %d", count, counts_fail,
			counts_success);

		if ((count / CONFIG_PRINT_CONN_INTERVAL) % 2 == 0) {
			ret = gpio_pin_set_dt(&led, 1);
		} else {
			ret = gpio_pin_set_dt(&led, 0);
		}
		if (ret) {
			LOG_ERR("Unable to set LED");
		}
	}
}

static void iso_connected(struct bt_iso_chan *chan)
{
	LOG_INF("ISO Channel %p connected", chan);
	k_sem_give(&sem_big_sync);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_INF("ISO Channel %p disconnected with reason 0x%02x", chan, reason);

	if (reason != BT_HCI_ERR_OP_CANCELLED_BY_HOST) {
		k_sem_give(&sem_big_sync_lost);
	}
}

static struct bt_iso_chan_ops iso_ops = {
	.recv = iso_recv,
	.connected = iso_connected,
	.disconnected = iso_disconnected,
};

static struct bt_iso_chan_io_qos iso_rx_qos[CONFIG_BIS_ISO_CHAN_COUNT_MAX];

static struct bt_iso_chan_qos bis_iso_qos[] = {
	{
		.rx = &iso_rx_qos[0],
	},
	{
		.rx = &iso_rx_qos[1],
	},
};

static struct bt_iso_chan bis_iso_chan[] = {
	{
		.ops = &iso_ops,
		.qos = &bis_iso_qos[0],
	},
	{
		.ops = &iso_ops,
		.qos = &bis_iso_qos[1],
	},
};

static struct bt_iso_chan *bis[] = {
	&bis_iso_chan[0],
	&bis_iso_chan[1],
};

static struct bt_iso_big_sync_param big_sync_param = {
	.bis_channels = bis,
	.num_bis = 1,
	.bis_bitfield = (BIT_MASK(1) << 1),
	.mse = 1,
	.sync_timeout = 100, /* in 10 ms units */
};

static void broadcaster_t(void *arg1, void *arg2, void *arg3)
{
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync;
	struct bt_iso_big *big;
	uint32_t sem_timeout_us;
	int err;

	while (!running) {
		k_msleep(100);
		continue;
	}

	bt_le_scan_cb_register(&scan_callbacks);
	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	while (1) {
		per_adv_lost = false;

		LOG_INF("Start scanning...");
		err = bt_le_scan_start(BT_LE_SCAN_CUSTOM, NULL);
		if (err) {
			LOG_ERR("failed (err %d)", err);
			return;
		}

		LOG_INF("Waiting for periodic advertising...");
		per_adv_found = false;
		err = k_sem_take(&sem_per_adv, K_FOREVER);
		if (err) {
			LOG_ERR("failed (err %d)", err);
			return;
		}
		LOG_INF("Found periodic advertising.");

		LOG_INF("Stop scanning...");
		err = bt_le_scan_stop();
		if (err) {
			LOG_ERR("failed (err %d)", err);
			return;
		}

		LOG_INF("Creating Periodic Advertising Sync...");
		bt_addr_le_copy(&sync_create_param.addr, &per_addr);
		sync_create_param.options = 0;
		sync_create_param.sid = per_sid;
		sync_create_param.skip = 0;
		/* Multiple PA interval with retry count and convert to unit of 10 ms */
		sync_create_param.timeout =
			(per_interval_us * PA_RETRY_COUNT) / (10 * USEC_PER_MSEC);
		sem_timeout_us = per_interval_us * PA_RETRY_COUNT;
		err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
		if (err) {
			LOG_INF("failed (err %d)", err);
			return;
		}

		LOG_INF("Waiting for periodic sync...");
		err = k_sem_take(&sem_per_sync, K_USEC(sem_timeout_us));
		if (err) {
			LOG_INF("failed (err %d)", err);

			LOG_INF("Deleting Periodic Advertising Sync...");
			err = bt_le_per_adv_sync_delete(sync);
			if (err) {
				LOG_INF("failed (err %d)", err);
				return;
			}
			continue;
		}
		LOG_INF("Periodic sync established.");

		LOG_INF("Waiting for BIG info...");
		err = k_sem_take(&sem_per_big_info, K_USEC(sem_timeout_us));
		if (err) {
			LOG_INF("failed (err %d)", err);

			if (per_adv_lost) {
				continue;
			}

			LOG_INF("Deleting Periodic Advertising Sync...");
			err = bt_le_per_adv_sync_delete(sync);
			if (err) {
				LOG_INF("failed (err %d)", err);
				return;
			}
			continue;
		}
		LOG_INF("Periodic sync established.");

big_sync_create:
		LOG_INF("Create BIG Sync...");
		err = bt_iso_big_sync(sync, &big_sync_param, &big);
		if (err) {
			LOG_INF("failed (err %d)", err);
			return;
		}
		LOG_INF("success.");

		for (uint8_t chan = 0U; chan < big_sync_param.num_bis; chan++) {
			LOG_INF("Waiting for BIG sync chan %u...", chan);
			err = k_sem_take(&sem_big_sync, TIMEOUT_SYNC_CREATE);
			if (err) {
				break;
			}
			LOG_INF("BIG sync chan %u successful.", chan);
		}
		if (err) {
			LOG_INF("failed (err %d)", err);

			LOG_INF("BIG Sync Terminate...");
			err = bt_iso_big_terminate(big);
			if (err) {
				LOG_INF("failed (err %d)", err);
				return;
			}
			LOG_INF("done.");

			goto per_sync_lost_check;
		}
		LOG_INF("BIG sync established.");

		for (uint8_t chan = 0U; chan < big_sync_param.num_bis; chan++) {
			LOG_INF("Waiting for BIG sync lost chan %u...", chan);
			err = k_sem_take(&sem_big_sync_lost, K_FOREVER);
			if (err) {
				LOG_INF("failed (err %d)", err);
				return;
			}
			LOG_INF("BIG sync lost chan %u.", chan);
		}
		LOG_INF("BIG sync lost.");

per_sync_lost_check:
		err = k_sem_take(&sem_per_sync_lost, K_NO_WAIT);
		if (err) {
			/* Periodic Sync active, go back to creating BIG Sync */
			goto big_sync_create;
		}
		LOG_INF("Periodic sync lost.");
	}
}

int iso_broadcast_sink_init(void)
{
	running = false;

	if (!gpio_is_ready_dt(&led)) {
		return -EBUSY;
	}

	k_thread_create(&broadcaster_thread, broadcaster_sink_thread_stack,
			K_THREAD_STACK_SIZEOF(broadcaster_sink_thread_stack), broadcaster_t, NULL,
			NULL, NULL, 5, K_USER, K_NO_WAIT);

	return 0;
}

int iso_broadcast_sink_start(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return ret;
	}

	running = true;
	return 0;
}

static int argument_check(const struct shell *shell, uint8_t const *const input)
{
	char *end;
	int arg_val = strtol(input, &end, 10);

	if (*end != '\0' || (uint8_t *)end == input || (arg_val == 0 && !isdigit(input[0])) ||
	    arg_val < 0) {
		shell_error(shell, "Argument must be a positive integer %s", input);
		return -EINVAL;
	}

	if (running) {
		shell_error(shell, "Arguments can not be changed while running");
		return -EACCES;
	}

	return arg_val;
}

static struct option long_options[] = { { "num_bis:", required_argument, NULL, 'n' },
					{ 0, 0, 0, 0 } };

static const char short_options[] = "n:";

static int set_param(const struct shell *shell, size_t argc, char **argv)
{
	int result = argument_check(shell, argv[2]);

	if (result < 0) {
		return result;
	}

	if (running) {
		shell_error(shell, "Change sink parameters before starting");
		return -EPERM;
	}

	int long_index = 0;
	int opt;

	optreset = 1;
	optind = 1;

	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		switch (opt) {
		case 'n':
			big_sync_param.num_bis = result;
			big_sync_param.bis_bitfield = (BIT_MASK(result) << 1);
			shell_print(shell, "num_bis: %d", big_sync_param.num_bis);
			break;
		case ':':
			shell_error(shell, "Missing option parameter");
			break;
		case '?':
			shell_error(shell, "Unknown option");
			break;
		default:
			shell_error(shell, "Invalid option");
			break;
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(broadcast_sink_cmd,
			       SHELL_CMD(start, NULL, "Start ISO broadcast sink.",
					 iso_broadcast_sink_start),
			       SHELL_CMD_ARG(set, NULL, "set", set_param, 3, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(brcast_sink, &broadcast_sink_cmd, "ISO Broadcast sink commands", NULL);
