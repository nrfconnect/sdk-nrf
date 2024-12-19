/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <unistd.h>

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/shell/shell.h>
#include <zephyr/drivers/gpio.h>

#include "iso_broadcast_src.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(brcast_src, CONFIG_ISO_TEST_LOG_LEVEL);

K_THREAD_STACK_DEFINE(broadcaster_thread_stack, 4096);
static struct k_thread broadcaster_thread;

#define BUF_ALLOC_TIMEOUT_MS	       (10)
#define BROADCAST_PERIODIC_ADV_INT_MIN (32) /* Periodic ADV min interval = 40ms */
#define BROADCAST_PERIODIC_ADV_INT_MAX (32) /* Periodic ADV max interval = 40ms */
#define BROADCAST_PERIODIC_ADV                                                                     \
	BT_LE_PER_ADV_PARAM(BROADCAST_PERIODIC_ADV_INT_MIN, BROADCAST_PERIODIC_ADV_INT_MAX,        \
			    BT_LE_PER_ADV_OPT_NONE)

NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, CONFIG_BIS_ISO_CHAN_COUNT_MAX,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU),
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

static K_SEM_DEFINE(sem_big_cmplt, 0, CONFIG_BIS_ISO_CHAN_COUNT_MAX);
static K_SEM_DEFINE(sem_big_term, 0, CONFIG_BIS_ISO_CHAN_COUNT_MAX);

static uint16_t seq_num;
static bool running;
static K_SEM_DEFINE(tx_sent, 0, 1);

static void iso_connected(struct bt_iso_chan *chan)
{
	LOG_INF("ISO Channel %p connected", chan);

	seq_num = 0U;

	k_sem_give(&sem_big_cmplt);
}

static void iso_disconnected(struct bt_iso_chan *chan, uint8_t reason)
{
	LOG_INF("ISO Channel %p disconnected with reason 0x%02x", chan, reason);
	k_sem_give(&sem_big_term);
}

static void iso_sent(struct bt_iso_chan *chan)
{
	k_sem_give(&tx_sent);
}

static struct bt_iso_chan_ops iso_ops = {
	.connected = iso_connected,
	.disconnected = iso_disconnected,
	.sent = iso_sent,
};

static struct bt_iso_chan_io_qos iso_tx_qos = {
	.sdu = 120,
	.rtn = 1,
	.phy = BT_GAP_LE_PHY_2M,
};

static struct bt_iso_chan_qos bis_iso_qos = {
	.tx = &iso_tx_qos,
};

static struct bt_iso_chan bis_iso_chan[] = {
	{
		.ops = &iso_ops,
		.qos = &bis_iso_qos,
	},
	{
		.ops = &iso_ops,
		.qos = &bis_iso_qos,
	},
};

static struct bt_iso_chan *bis[] = {
	&bis_iso_chan[0],
	&bis_iso_chan[1],
};

static struct bt_iso_big_create_param big_create_param = {
	.num_bis = 1,
	.bis_channels = bis,
	.interval = 10000, /* in microseconds */
	.latency = 10,	   /* in milliseconds */
	.packing = 0,	   /* 0 - sequential, 1 - interleaved */
	.framing = 0,	   /* 0 - unframed, 1 - framed */
	.encryption = false,
};

static struct bt_le_ext_adv *adv;
static struct bt_iso_big *big;
static uint8_t iso_data[CONFIG_BT_ISO_TX_MTU] = {0};

static const struct bt_data ad[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, CONFIG_BT_DEVICE_NAME, sizeof(CONFIG_BT_DEVICE_NAME) - 1),
};

static void broadcaster_t(void *arg1, void *arg2, void *arg3)
{
	static uint8_t initial_send = 2;
	uint32_t iso_send_attempts = 0;

	while (1) {
		int ret;

		if (!running) {
			k_msleep(100);
			initial_send = 2;
			continue;
		} else {
			iso_send_attempts++;
		}

		if ((iso_send_attempts % CONFIG_PRINT_ISO_INTERVAL) == 0) {
			LOG_INF("\t ISO TX: Count: %7u,", iso_send_attempts);
		}

		if (!initial_send) {
			ret = k_sem_take(&tx_sent, K_USEC(big_create_param.interval * 2));
			if (ret) {
				LOG_INF("Sent semaphore timed out");
				continue;
			}
		}

		for (uint8_t chan = 0U; chan < big_create_param.num_bis; chan++) {
			struct net_buf *buf;

			buf = net_buf_alloc(&bis_tx_pool, K_MSEC(BUF_ALLOC_TIMEOUT_MS));
			if (!buf) {
				LOG_ERR("Data buffer allocate timeout on channel %u", chan);
			}

			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
			sys_put_le32(iso_send_attempts, iso_data);
			net_buf_add_mem(buf, iso_data, iso_tx_qos.sdu);

			ret = bt_iso_chan_send(&bis_iso_chan[chan], buf, seq_num);
			if (ret < 0) {
				LOG_ERR("Unable to broadcast data on channel %u"
					" : %d",
					chan, ret);
				net_buf_unref(buf);
			}
		}

		seq_num++;
		if (initial_send) {
			initial_send--;
		}
	}
}

int iso_broadcaster_stop(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	running = false;

	LOG_INF("BIG Terminate");
	ret = bt_iso_big_terminate(big);
	if (ret) {
		LOG_ERR("failed (ret %d)", ret);
		return ret;
	}

	for (uint8_t chan = 0U; chan < big_create_param.num_bis; chan++) {
		LOG_INF("Waiting for BIG terminate complete"
			" chan %u...",
			chan);
		ret = k_sem_take(&sem_big_term, K_MSEC(100));
		if (ret) {
			LOG_ERR("failed (ret %d)", ret);
			return ret;
		}
		LOG_INF("BIG terminate complete chan %u.", chan);
	}

	ret = bt_le_ext_adv_stop(adv);
	if (ret) {
		LOG_ERR("failed (ret %d)", ret);
		return ret;
	}

	ret = bt_le_per_adv_stop(adv);
	if (ret) {
		LOG_ERR("failed (ret %d)", ret);
		return ret;
	}

	ret = bt_le_ext_adv_delete(adv);
	if (ret) {
		LOG_ERR("Failed to delete advertising set (ret %d)\n", ret);
		return ret;
	}

	return 0;
}

int iso_broadcast_src_start(size_t argc, char **argv)
{
	int ret;

	/* Create a non-connectable non-scannable advertising set */
	ret = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, &adv);
	if (ret) {
		LOG_ERR("Failed to create advertising set (ret %d)", ret);
		return ret;
	}

	/* Set advertising data to have complete local name set */
	ret = bt_le_ext_adv_set_data(adv, ad, ARRAY_SIZE(ad), NULL, 0);
	if (ret) {
		LOG_ERR("Failed to set advertising data (ret %d)", ret);
		return 0;
	}

	/* Set periodic advertising parameters */
	ret = bt_le_per_adv_set_param(adv, BROADCAST_PERIODIC_ADV);
	if (ret) {
		LOG_ERR("Failed to set periodic advertising parameters"
			" (ret %d)",
			ret);
		return ret;
	}

	/* Enable Periodic Advertising */
	ret = bt_le_per_adv_start(adv);
	if (ret) {
		LOG_ERR("Failed to enable periodic advertising (ret %d)", ret);
		return ret;
	}

	/* Start extended advertising */
	ret = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (ret) {
		LOG_ERR("Failed to start extended advertising (ret %d)", ret);
		return ret;
	}

	/* Create BIG */
	ret = bt_iso_big_create(adv, &big_create_param, &big);
	if (ret) {
		LOG_ERR("Failed to create BIG (ret %d)", ret);
		return ret;
	}

	for (uint8_t chan = 0U; chan < big_create_param.num_bis; chan++) {
		LOG_INF("Waiting for BIG complete chan %u...", chan);
		ret = k_sem_take(&sem_big_cmplt, K_FOREVER);
		if (ret) {
			LOG_ERR("k_sem_take failed (ret %d)", ret);
			return ret;
		}
		LOG_INF("BIG create complete chan %u.", chan);
	}

	running = true;

	return 0;
}

int broadcaster_print_cfg(const struct shell *shell, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	LOG_INF("\n\tsdu_size: %d\n\tphy: %d\n\trtn: %d\n\tnum_bis: %d\n\tsdu_interval_us: %d\n\t"
		"latency_ms: %d\n\tpacking: %d\n\tframing: %d",
		iso_tx_qos.sdu, iso_tx_qos.phy, iso_tx_qos.rtn, big_create_param.num_bis,
		big_create_param.interval, big_create_param.latency, big_create_param.packing,
		big_create_param.framing);

	return 0;
}

static int argument_check(uint8_t const *const input)
{
	char *end;

	if (running) {
		LOG_ERR("Stop src before changing parameters");
		return -EPERM;
	}

	int arg_val = strtol(input, &end, 10);

	if (*end != '\0' || (uint8_t *)end == input || (arg_val == 0 && !isdigit(input[0])) ||
	    arg_val < 0) {
		LOG_ERR("Argument must be a positive integer %s", input);
		return -EINVAL;
	}

	return arg_val;
}

int iso_broadcast_src_init(void)
{
	running = false;

	k_thread_create(&broadcaster_thread, broadcaster_thread_stack,
			K_THREAD_STACK_SIZEOF(broadcaster_thread_stack), broadcaster_t, NULL, NULL,
			NULL, 5, K_USER, K_NO_WAIT);

	return 0;
}

static int iso_broadcast_source_sdu_size_set(const struct shell *shell, size_t argc, char **argv)
{
	int arg = argument_check(argv[1]);

	if (arg < 0) {
		return arg;
	}

	iso_tx_qos.sdu = arg;
	broadcaster_print_cfg(shell, 0, NULL);
	return 0;
}

static int iso_broadcast_source_phy_set(const struct shell *shell, size_t argc, char **argv)
{
	int arg = argument_check(argv[1]);

	if (arg < 0) {
		return arg;
	}

	iso_tx_qos.phy = arg;
	broadcaster_print_cfg(shell, 0, NULL);
	return 0;
}

static int iso_broadcast_source_rtn_set(const struct shell *shell, size_t argc, char **argv)
{
	int arg = argument_check(argv[1]);

	if (arg < 0) {
		return arg;
	}

	iso_tx_qos.rtn = arg;
	broadcaster_print_cfg(shell, 0, NULL);
	return 0;
}

static int iso_broadcast_source_num_bis_set(const struct shell *shell, size_t argc, char **argv)
{
	int arg = argument_check(argv[1]);

	if (arg < 0) {
		return arg;
	}

	if (arg != 1) {
		LOG_ERR("Only 1 BIS supported");
		return -EINVAL;
	}

	big_create_param.num_bis = arg;
	broadcaster_print_cfg(shell, 0, NULL);
	return 0;
}

static int iso_broadcast_source_sdu_int_us_set(const struct shell *shell, size_t argc, char **argv)
{
	int arg = argument_check(argv[1]);

	if (arg < 0) {
		return arg;
	}

	big_create_param.interval = arg;
	broadcaster_print_cfg(shell, 0, NULL);
	return 0;
}

static int iso_broadcast_source_latency_ms_set(const struct shell *shell, size_t argc, char **argv)
{
	int arg = argument_check(argv[1]);

	if (arg < 0) {
		return arg;
	}

	big_create_param.latency = arg;
	broadcaster_print_cfg(shell, 0, NULL);
	return 0;
}

static int iso_broadcast_source_packing_set(const struct shell *shell, size_t argc, char **argv)
{
	int arg = argument_check(argv[1]);

	if (arg < 0) {
		return arg;
	}

	big_create_param.packing = arg;
	broadcaster_print_cfg(shell, 0, NULL);
	return 0;
}

static int iso_broadcast_source_framing_set(const struct shell *shell, size_t argc, char **argv)
{
	int arg = argument_check(argv[1]);

	if (arg < 0) {
		return arg;
	}

	big_create_param.framing = arg;
	broadcaster_print_cfg(shell, 0, NULL);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	broadcaster_cmd, SHELL_CMD(start, NULL, "Start ISO broadcaster.", iso_broadcast_src_start),
	SHELL_CMD(stop, NULL, "Stop ISO broadcaster.", iso_broadcaster_stop),
	SHELL_CMD(cfg, NULL, "Print config.", broadcaster_print_cfg),
	SHELL_CMD_ARG(sdu_size_set, NULL, "set", iso_broadcast_source_sdu_size_set, 2, 0),
	SHELL_CMD_ARG(phy_set, NULL, "set", iso_broadcast_source_phy_set, 2, 0),
	SHELL_CMD_ARG(rtn_set, NULL, "set", iso_broadcast_source_rtn_set, 2, 0),
	SHELL_CMD_ARG(num_bis_set, NULL, "set", iso_broadcast_source_num_bis_set, 2, 0),
	SHELL_CMD_ARG(sdu_int_us_set, NULL, "set", iso_broadcast_source_sdu_int_us_set, 2, 0),
	SHELL_CMD_ARG(latency_ms_set, NULL, "set", iso_broadcast_source_latency_ms_set, 2, 0),
	SHELL_CMD_ARG(packing_set, NULL, "set", iso_broadcast_source_packing_set, 2, 0),
	SHELL_CMD_ARG(framing_set, NULL, "set", iso_broadcast_source_framing_set, 2, 0),
	SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(iso_brcast_src, &broadcaster_cmd, "ISO Broadcast source commands", NULL);
