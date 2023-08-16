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
LOG_MODULE_REGISTER(broadcast_src, CONFIG_ISO_TEST_LOG_LEVEL);

K_THREAD_STACK_DEFINE(broadcaster_thread_stack, 4096);
static struct k_thread broadcaster_thread;

static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);

#define BUF_ALLOC_TIMEOUT_MS (10)

NET_BUF_POOL_FIXED_DEFINE(bis_tx_pool, CONFIG_BIS_ISO_CHAN_COUNT_MAX,
			  BT_ISO_SDU_BUF_SIZE(CONFIG_BT_ISO_TX_MTU), 8, NULL);

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
	.rtn = 2,
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
	.latency = 10, /* in milliseconds */
	.packing = 0, /* 0 - sequential, 1 - interleaved */
	.framing = 0, /* 0 - unframed, 1 - framed */
};

static struct bt_le_ext_adv *adv;
static struct bt_iso_big *big;
static uint32_t iso_send_count;
static uint8_t iso_data[CONFIG_BT_ISO_TX_MTU] = { 0 };

static void broadcaster_t(void *arg1, void *arg2, void *arg3)
{
	static uint8_t initial_send = 2;

	while (1) {
		int ret;

		k_sleep(K_USEC(big_create_param.interval));
		if (!running) {
			k_msleep(100);
			initial_send = 2;
			continue;
		}

		if (!initial_send) {
			ret = k_sem_take(&tx_sent, K_USEC(big_create_param.interval * 2));
			if (ret) {
				LOG_ERR("Sent semaphore timed out");
			}
		}

		for (uint8_t chan = 0U; chan < big_create_param.num_bis; chan++) {
			struct net_buf *buf;

			buf = net_buf_alloc(&bis_tx_pool, K_MSEC(BUF_ALLOC_TIMEOUT_MS));
			if (!buf) {
				LOG_ERR("Data buffer allocate timeout on channel %u", chan);
			}

			net_buf_reserve(buf, BT_ISO_CHAN_SEND_RESERVE);
			sys_put_le32(iso_send_count, iso_data);
			net_buf_add_mem(buf, iso_data, iso_tx_qos.sdu);
			ret = bt_iso_chan_send(&bis_iso_chan[chan], buf, seq_num,
					       BT_ISO_TIMESTAMP_NONE);
			if (ret < 0) {
				LOG_ERR("Unable to broadcast data on channel %u"
					" : %d",
					chan, ret);
				net_buf_unref(buf);
			}
		}

		iso_send_count++;
		seq_num++;
		if (initial_send) {
			initial_send--;
		}

		if ((iso_send_count % CONFIG_PRINT_CONN_INTERVAL) == 0) {
			LOG_INF("Sending value %u", iso_send_count);
			if ((iso_send_count / CONFIG_PRINT_CONN_INTERVAL) % 2 == 0) {
				ret = gpio_pin_set_dt(&led, 1);
			} else {
				ret = gpio_pin_set_dt(&led, 0);
			}
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

int iso_broadcast_src_start(const struct shell *shell, size_t argc, char **argv)
{
	int ret;

	ret = gpio_pin_set_dt(&led, 1);
	if (ret) {
		return ret;
	}

	/* Create a non-connectable non-scannable advertising set */
	ret = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (ret) {
		LOG_ERR("Failed to create advertising set (ret %d)", ret);
		return ret;
	}

	/* Set periodic advertising parameters */
	ret = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
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
			LOG_ERR("failed (ret %d)", ret);
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

	shell_print(shell,
		    "sdu_size %d\nphy %d\nrtn %d\nnum_bis %d\nsdu_interval_us %d\n"
		    "latency_ms %d\npacking %d\nframing %d",
		    iso_tx_qos.sdu, iso_tx_qos.phy, iso_tx_qos.rtn, big_create_param.num_bis,
		    big_create_param.interval, big_create_param.latency, big_create_param.packing,
		    big_create_param.framing);

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

int iso_broadcast_src_init(void)
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

	k_thread_create(&broadcaster_thread, broadcaster_thread_stack,
			K_THREAD_STACK_SIZEOF(broadcaster_thread_stack), broadcaster_t, NULL, NULL,
			NULL, 5, K_USER, K_NO_WAIT);

	return 0;
}

static struct option long_options[] = { { "sdu_size", required_argument, NULL, 's' },
					{ "phy", required_argument, NULL, 'p' },
					{ "rtn", required_argument, NULL, 'r' },
					{ "num_bis:", required_argument, NULL, 'n' },
					{ "sdu_int_us", required_argument, NULL, 'S' },
					{ "latency_ms", required_argument, NULL, 'l' },
					{ "packing", required_argument, NULL, 'P' },
					{ "framing", required_argument, NULL, 'f' },
					{ 0, 0, 0, 0 } };

static const char short_options[] = "s:p:r:n:S:l:P:f:";

static int param_set(const struct shell *shell, size_t argc, char **argv)
{
	int result = argument_check(shell, argv[2]);
	int long_index = 0;
	int opt;

	if (result < 0) {
		return result;
	}

	if (running) {
		shell_error(shell, "Stop src before changing parameters");
		return -EPERM;
	}

	optreset = 1;
	optind = 1;

	while ((opt = getopt_long(argc, argv, short_options, long_options, &long_index)) != -1) {
		switch (opt) {
		case 's':
			iso_tx_qos.sdu = result;
			broadcaster_print_cfg(shell, 0, NULL);
			break;
		case 'p':
			iso_tx_qos.phy = result;
			broadcaster_print_cfg(shell, 0, NULL);
			break;
		case 'r':
			iso_tx_qos.rtn = result;
			broadcaster_print_cfg(shell, 0, NULL);
			break;
		case 'n':
			big_create_param.num_bis = result;
			broadcaster_print_cfg(shell, 0, NULL);
			break;
		case 'S':
			big_create_param.interval = result;
			broadcaster_print_cfg(shell, 0, NULL);
			break;
		case 'l':
			big_create_param.latency = result;
			broadcaster_print_cfg(shell, 0, NULL);
			break;
		case 'P':
			big_create_param.packing = result;
			broadcaster_print_cfg(shell, 0, NULL);
			break;
		case 'f':
			big_create_param.framing = result;
			broadcaster_print_cfg(shell, 0, NULL);
			break;
		case ':':
			shell_error(shell, "Missing option parameter");
			break;
		case '?':
			shell_error(shell, "Unknown option: %c", opt);
			break;
		default:
			shell_error(shell, "Invalid option: %c", opt);
			break;
		}
	}

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(
	broadcaster_cmd, SHELL_CMD(start, NULL, "Start ISO broadcaster.", iso_broadcast_src_start),
	SHELL_CMD(stop, NULL, "Stop ISO broadcaster.", iso_broadcaster_stop),
	SHELL_CMD(cfg, NULL, "Print config.", broadcaster_print_cfg),
	SHELL_CMD_ARG(set, NULL, "set", param_set, 3, 0), SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(brcast_src, &broadcaster_cmd, "ISO Broadcast source commands", NULL);
