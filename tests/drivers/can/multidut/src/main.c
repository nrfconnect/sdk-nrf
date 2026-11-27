/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/shell/shell.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/byteorder.h>

#define BURST_TX_TIMEOUT        K_MSEC(1000)
#define BURST_PATTERN           0xa5
#define BURST_MAX_CONSEC_FAILED 3U

#define CHOSEN_NODE DT_CHOSEN(zephyr_canbus)

#if DT_NODE_HAS_COMPAT(CHOSEN_NODE, nordic_nrf_can)
#define CHOSEN_CONTROLLER "mcan"
#elif DT_NODE_HAS_COMPAT(CHOSEN_NODE, nordic_nrf_scan)
#define CHOSEN_CONTROLLER "scan"
#else
#define CHOSEN_CONTROLLER "unknown"
#endif

static const struct device *chosen = DEVICE_DT_GET(CHOSEN_NODE);

static K_SEM_DEFINE(burst_tx_done, 0, 1);
static int burst_tx_err;

static struct {
	int filter_id;
	atomic_t count;
	atomic_t gaps;
	atomic_t duplicates;
	atomic_t expected_seq;
} rx_counter = {
	.filter_id = -1,
};

static bool chosen_ready(const struct shell *sh)
{
	if (!device_is_ready(chosen)) {
		shell_error(sh, "zephyr,canbus device %s not ready", chosen->name);
		return false;
	}

	return true;
}

static int parse_id(const struct shell *sh, const char *arg, bool ext, uint32_t *id)
{
	uint32_t mask = ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK;
	char *endptr;

	*id = (uint32_t)strtoul(arg, &endptr, 16);
	if (*endptr != '\0' || *id > mask) {
		shell_error(sh, "invalid CAN ID %s", arg);
		return -EINVAL;
	}

	return 0;
}

static int cmd_can_host_chosen(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (!chosen_ready(sh)) {
		return -ENODEV;
	}

	shell_print(sh, "zephyr,canbus: %s", chosen->name);
	shell_print(sh, "controller: %s", CHOSEN_CONTROLLER);

	return 0;
}

static void burst_tx_callback(const struct device *dev, int error, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	burst_tx_err = error;
	k_sem_give(&burst_tx_done);
}

/* Blocking can_send() waits forever for TX completion, e.g. when no node ACKs */
static int burst_send(const struct can_frame *frame)
{
	int err;

	k_sem_reset(&burst_tx_done);

	err = can_send(chosen, frame, BURST_TX_TIMEOUT, burst_tx_callback, NULL);
	if (err != 0) {
		return err;
	}

	if (k_sem_take(&burst_tx_done, BURST_TX_TIMEOUT) != 0) {
		return -ETIMEDOUT;
	}

	return burst_tx_err;
}

static int cmd_can_host_burst(const struct shell *sh, size_t argc, char **argv)
{
	struct can_frame frame = {0};
	uint32_t sent = 0U;
	uint32_t failed = 0U;
	uint32_t consec_failed = 0U;
	bool ext = false;
	int argidx = 1;
	uint32_t count;
	char *endptr;
	int last_err = 0;
	int err;

	if (!chosen_ready(sh)) {
		return -ENODEV;
	}

	if (argc > argidx && strcmp(argv[argidx], "-e") == 0) {
		ext = true;
		argidx++;
	}

	if (argc - argidx != 2) {
		shell_error(sh, "usage: can_host burst [-e] <CAN ID> <count>");
		return -EINVAL;
	}

	err = parse_id(sh, argv[argidx++], ext, &frame.id);
	if (err != 0) {
		return err;
	}

	count = (uint32_t)strtoul(argv[argidx], &endptr, 10);
	if (*endptr != '\0' || count == 0U) {
		shell_error(sh, "invalid count %s", argv[argidx]);
		return -EINVAL;
	}

	frame.flags = ext ? CAN_FRAME_IDE : 0U;
	frame.dlc = can_bytes_to_dlc(8);
	memset(&frame.data[4], BURST_PATTERN, 4);

	for (uint32_t seq = 0U; seq < count; seq++) {
		sys_put_be32(seq, frame.data);

		err = burst_send(&frame);
		if (err == 0) {
			sent++;
			consec_failed = 0U;
		} else {
			failed++;
			last_err = err;
			/* A timed out frame is still pending in the controller */
			if (err == -ETIMEDOUT || ++consec_failed >= BURST_MAX_CONSEC_FAILED) {
				break;
			}
		}
	}

	shell_print(sh, "burst done: sent %u, failed %u, last err %d", sent, count - sent,
		    last_err);

	return failed == 0U ? 0 : last_err;
}

static void rx_counter_callback(const struct device *dev, struct can_frame *frame,
				void *user_data)
{
	uint32_t expected = (uint32_t)atomic_get(&rx_counter.expected_seq);
	uint32_t seq;

	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);

	atomic_inc(&rx_counter.count);

	if (can_dlc_to_bytes(frame->dlc) < 4U) {
		atomic_inc(&rx_counter.gaps);
		return;
	}

	seq = sys_get_be32(frame->data);
	if (expected > 0U && seq == expected - 1U) {
		/* Allowed by ISO 11898-1 after an error in the last bit of EOF */
		atomic_inc(&rx_counter.duplicates);
		return;
	}

	if (seq != expected) {
		atomic_inc(&rx_counter.gaps);
	}

	atomic_set(&rx_counter.expected_seq, seq + 1U);
}

static int cmd_can_host_rx_start(const struct shell *sh, size_t argc, char **argv)
{
	struct can_filter filter = {0};
	bool ext = false;
	int argidx = 1;
	int err;

	if (!chosen_ready(sh)) {
		return -ENODEV;
	}

	if (rx_counter.filter_id >= 0) {
		shell_error(sh, "rx counter already started");
		return -EALREADY;
	}

	if (argc > argidx && strcmp(argv[argidx], "-e") == 0) {
		ext = true;
		argidx++;
	}

	if (argc - argidx != 1) {
		shell_error(sh, "usage: can_host rx start [-e] <CAN ID>");
		return -EINVAL;
	}

	err = parse_id(sh, argv[argidx], ext, &filter.id);
	if (err != 0) {
		return err;
	}

	filter.flags = ext ? CAN_FILTER_IDE : 0U;
	filter.mask = ext ? CAN_EXT_ID_MASK : CAN_STD_ID_MASK;

	atomic_set(&rx_counter.count, 0);
	atomic_set(&rx_counter.gaps, 0);
	atomic_set(&rx_counter.duplicates, 0);
	atomic_set(&rx_counter.expected_seq, 0);

	err = can_add_rx_filter(chosen, rx_counter_callback, NULL, &filter);
	if (err < 0) {
		shell_error(sh, "failed to add rx counter filter (err %d)", err);
		return err;
	}

	rx_counter.filter_id = err;
	shell_print(sh, "rx counter started");

	return 0;
}

static int cmd_can_host_rx_stop(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	if (rx_counter.filter_id < 0) {
		shell_error(sh, "rx counter not started");
		return -EINVAL;
	}

	can_remove_rx_filter(chosen, rx_counter.filter_id);
	rx_counter.filter_id = -1;

	shell_print(sh, "rx count: %ld, gaps: %ld, duplicates: %ld",
		    atomic_get(&rx_counter.count), atomic_get(&rx_counter.gaps),
		    atomic_get(&rx_counter.duplicates));

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_can_host_rx_cmds,
	SHELL_CMD(start, NULL,
		"Start counting received frames with sequence check\n"
		"Usage: can_host rx start [-e] <CAN ID>",
		cmd_can_host_rx_start),
	SHELL_CMD(stop, NULL,
		"Stop counting and print the result\n"
		"Usage: can_host rx stop",
		cmd_can_host_rx_stop),
	SHELL_SUBCMD_SET_END
);

SHELL_STATIC_SUBCMD_SET_CREATE(sub_can_host_cmds,
	SHELL_CMD(chosen, NULL,
		"Get zephyr,canbus chosen device name and controller type\n"
		"Usage: can_host chosen",
		cmd_can_host_chosen),
	SHELL_CMD(burst, NULL,
		"Send a burst of frames with a 32-bit sequence number in data[0..3]\n"
		"Usage: can_host burst [-e] <CAN ID> <count>",
		cmd_can_host_burst),
	SHELL_CMD(rx, &sub_can_host_rx_cmds, "RX frame counter commands", NULL),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(can_host, &sub_can_host_cmds, "CAN multi-DUT test commands", NULL);
