/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/sys_getopt.h>
#include <zephyr/shell/shell.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(brcast_snk, CONFIG_ISO_TEST_LOG_LEVEL);

#define TIMEOUT_SYNC_CREATE K_SECONDS(10)
#define NAME_LEN	    30
#define PA_RETRY_COUNT	    6

K_THREAD_STACK_DEFINE(broadcaster_sink_thread_stack, 4096);
static struct k_thread broadcaster_sink_thread;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led2), gpios);

static bool per_adv_found;
static bool per_adv_lost;
static bool running;
static bt_addr_le_t per_addr;
static uint8_t per_sid;
static uint32_t per_interval_us;

static K_SEM_DEFINE(sem_per_adv, 0, 1);
static K_SEM_DEFINE(sem_per_sync, 0, 1);
static K_SEM_DEFINE(sem_per_sync_lost, 0, 1);
static K_SEM_DEFINE(sem_per_big_info, 0, 1);
static K_SEM_DEFINE(sem_big_sync, 0, CONFIG_BIS_ISO_CHAN_COUNT_MAX);
static K_SEM_DEFINE(sem_big_sync_lost, 0, CONFIG_BIS_ISO_CHAN_COUNT_MAX);

static bool data_cb(struct bt_data *data, void *user_data)
{
	uint8_t len;
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
	case BT_DATA_BROADCAST_NAME:
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

	if (strcmp(CONFIG_BT_DEVICE_NAME, name) != 0) {
		return;
	}

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

	/* NOTE: The string below is used by the Nordic CI system */
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

	if (!(info->flags & BT_ISO_FLAGS_VALID) && (buf->len != 0)) {
		LOG_ERR("bad frame");
		return;
	}

	if (buf->len >= sizeof(count)) {
		count = sys_get_le32(buf->data);
	} else {
		LOG_WRN("Received too few bytes: %d", buf->len);
		return;
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

	if ((count % CONFIG_PRINT_ISO_INTERVAL) == 0) {
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_INF("RX: Count: %7u, Failed: %6u, Success: %7u", count, counts_fail,
			counts_success);

		if ((count / CONFIG_PRINT_ISO_INTERVAL) % 2 == 0) {
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
	const struct bt_iso_chan_path hci_path = {
		.pid = BT_ISO_DATA_PATH_HCI,
		.format = BT_HCI_CODING_FORMAT_TRANSPARENT,
	};
	int err;

	LOG_INF("ISO Channel %p connected", chan);
	k_sem_give(&sem_big_sync);

	err = bt_iso_setup_data_path(chan, BT_HCI_DATAPATH_DIR_CTLR_TO_HOST, &hci_path);
	if (err != 0) {
		LOG_ERR("Failed to setup ISO RX data path: %d", err);
	}
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
static struct bt_iso_chan_qos bis_iso_qos[CONFIG_BIS_ISO_CHAN_COUNT_MAX];
static struct bt_iso_chan bis_iso_chan[CONFIG_BIS_ISO_CHAN_COUNT_MAX];
static struct bt_iso_chan *bis[CONFIG_BIS_ISO_CHAN_COUNT_MAX];

static struct bt_iso_big_sync_param big_sync_param = {
	.bis_channels = bis,
	.num_bis = 1,
	.bis_bitfield = BIT_MASK(1),
	.mse = BT_ISO_SYNC_MSE_ANY,
	.sync_timeout = 100, /* in 10 ms units */
	.encryption = false,
};

static void broadcaster_sink_trd(void)
{
	int ret;
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync;
	struct bt_iso_big *big;
	uint32_t sem_retry_timeout_ms;

	bt_le_scan_cb_register(&scan_callbacks);
	bt_le_per_adv_sync_cb_register(&sync_callbacks);

	while (1) {
		per_adv_lost = false;

		LOG_INF("Start scanning...");
		ret = bt_le_scan_start(BT_LE_SCAN_PASSIVE_CONTINUOUS, NULL);
		if (ret) {
			LOG_ERR("Scan start failed (ret %d)", ret);
			return;
		}

		LOG_INF("Waiting for periodic advertising...");
		per_adv_found = false;
		ret = k_sem_take(&sem_per_adv, K_FOREVER);
		if (ret) {
			LOG_ERR("Sem take failed (ret %d)", ret);
			return;
		}

		LOG_INF("Found periodic advertising.");

		LOG_INF("Stop scanning...");
		ret = bt_le_scan_stop();
		if (ret) {
			LOG_ERR("Scan stop failed (ret %d)", ret);
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

		sem_retry_timeout_ms = per_interval_us * PA_RETRY_COUNT;
		ret = bt_le_per_adv_sync_create(&sync_create_param, &sync);
		if (ret) {
			LOG_ERR("Per adv sync create failed (ret %d)", ret);
			return;
		}

		LOG_INF("Waiting for periodic sync...");
		ret = k_sem_take(&sem_per_sync, K_USEC(sem_retry_timeout_ms));
		if (ret) {
			LOG_ERR("Sem take failed (ret %d)", ret);

			LOG_INF("Deleting Periodic Advertising Sync...");
			ret = bt_le_per_adv_sync_delete(sync);
			if (ret) {
				LOG_ERR("Per adv sync delete failed (ret %d)", ret);
				return;
			}
			continue;
		}

		LOG_INF("Periodic sync established.");
		LOG_INF("Waiting for BIG info...");
		ret = k_sem_take(&sem_per_big_info, K_USEC(sem_retry_timeout_ms));
		if (ret) {
			LOG_ERR("Sem take failed (ret %d)", ret);

			if (per_adv_lost) {
				continue;
			}

			LOG_INF("Deleting Periodic Advertising Sync...");
			ret = bt_le_per_adv_sync_delete(sync);
			if (ret) {
				LOG_ERR("Per adv sync delete failed (ret %d)", ret);
				return;
			}

			continue;
		}

		LOG_INF("Periodic sync established.");

big_sync_create:
		/* NOTE: The string below is used by the Nordic CI system */
		LOG_INF("Create BIG Sync...");
		ret = bt_iso_big_sync(sync, &big_sync_param, &big);
		if (ret) {
			LOG_ERR("Big sync failed (ret %d)", ret);
			return;
		}
		LOG_INF("success.");

		for (uint8_t chan = 0U; chan < big_sync_param.num_bis; chan++) {
			LOG_INF("Waiting for BIG sync chan %u...", chan);
			ret = k_sem_take(&sem_big_sync, TIMEOUT_SYNC_CREATE);
			if (ret) {
				break;
			}
			LOG_INF("BIG sync chan %u successful.", chan);
		}
		if (ret) {
			LOG_ERR("failed (ret %d)", ret);

			LOG_INF("BIG Sync Terminate...");
			ret = bt_iso_big_terminate(big);
			if (ret) {
				LOG_ERR("Big terminate failed (ret %d)", ret);
				return;
			}

			LOG_INF("Done.");
			goto per_sync_lost_check;
		}

		LOG_INF("BIG sync established.");

		for (uint8_t chan = 0U; chan < big_sync_param.num_bis; chan++) {
			LOG_INF("Waiting for BIG sync lost chan %u...", chan);
			ret = k_sem_take(&sem_big_sync_lost, K_FOREVER);
			if (ret) {
				LOG_ERR("Sem take failed (ret %d)", ret);
				return;
			}

			LOG_INF("BIG sync lost chan %u.", chan);
		}

		LOG_INF("BIG sync lost.");

per_sync_lost_check:
		ret = k_sem_take(&sem_per_sync_lost, K_NO_WAIT);
		if (ret) {
			/* Periodic Sync active, go back to creating BIG Sync */
			goto big_sync_create;
		}

		LOG_INF("Periodic sync lost.");
	}
}

int iso_broadcast_sink_start(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	k_thread_create(&broadcaster_sink_thread, broadcaster_sink_thread_stack,
			K_THREAD_STACK_SIZEOF(broadcaster_sink_thread_stack),
			(k_thread_entry_t)broadcaster_sink_trd, NULL, NULL, NULL, 5, K_USER,
			K_NO_WAIT);

	ret = gpio_pin_set_dt(&led, 1);
	if (ret) {
		return ret;
	}

	running = true;
	return 0;
}

int iso_broadcast_sink_init(void)
{
	int ret;
	running = false;

	if (!gpio_is_ready_dt(&led)) {
		return -EBUSY;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		return ret;
	}

	for (int i = 0; i < CONFIG_BIS_ISO_CHAN_COUNT_MAX; i++) {
		bis_iso_qos[i].rx = &iso_rx_qos[i];
		bis_iso_chan[i].ops = &iso_ops;
		bis_iso_chan[i].qos = &bis_iso_qos[i];
		bis[i] = &bis_iso_chan[i];
	}

	return 0;
}

static int argument_check(const struct shell *shell, uint8_t const *const input)
{
	char *end;
	int arg_val = strtol(input, &end, 10);

	if (*end != '\0' || (uint8_t *)end == input || (arg_val == 0 && !isdigit(input[0])) ||
	    arg_val < 0) {
		LOG_ERR("Argument must be a positive integer %s", input);
		return -EINVAL;
	}

	if (running) {
		LOG_ERR("Stop sink before changing parameters");
		return -EPERM;
	}

	return arg_val;
}

static struct sys_getopt_option long_options[] = {
	{"num_bis:", sys_getopt_required_argument, NULL, 'n'}, {0, 0, 0, 0}};

static const char short_options[] = "n:";

static int param_set(const struct shell *shell, size_t argc, char **argv)
{
	int long_index = 0;
	int opt;
	int result = argument_check(shell, argv[2]);

	if (result < 0) {
		return result;
	}

	if (running) {
		LOG_ERR("Change sink parameters before starting");
		return -EPERM;
	}

	sys_getopt_init();

	while ((opt = sys_getopt_long(argc, argv, short_options, long_options, &long_index)) !=
	       -1) {
		switch (opt) {
		case 'n':
			big_sync_param.num_bis = result;
			big_sync_param.bis_bitfield = BIT_MASK(result);
			LOG_INF("num_bis: %d", big_sync_param.num_bis);
			break;
		case ':':
			LOG_ERR("Missing option parameter");
			break;
		case '?':
			LOG_ERR("Unknown option");
			break;
		default:
			LOG_ERR("Invalid option");
			break;
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(broadcast_sink_cmd,
			       SHELL_CMD(start, NULL, "Start ISO broadcast sink.",
					 iso_broadcast_sink_start),
			       SHELL_CMD_ARG(set, NULL, "set", param_set, 3, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(brcast_sink, &broadcast_sink_cmd, "ISO Broadcast sink commands", NULL);
