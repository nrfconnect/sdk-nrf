/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include "perf_common.h"

int main(void)
{
	const uint32_t *bitrates;
	size_t bitrate_count;
	int err;

	printk("CAN performance benchmark %s\n", CONFIG_BOARD_TARGET);
	err = can_perf_bench_init();
	if (err != 0) {
		printk("CAN perf: bench init failed (%d)\n", err);
		return 0;
	}

#if defined(CONFIG_TEST_CAN_PERF_SHORT_RUN)
	bitrates = &can_perf_classic_bitrate_short;
	bitrate_count = 1;
#else
	bitrates = can_perf_classic_bitrates;
	bitrate_count = can_perf_classic_bitrate_count;
#endif

	k_msleep(CONFIG_TEST_CAN_PERF_DEAD_TIME_MS);

	for (size_t i = 0; i < bitrate_count; i++) {
		const uint32_t bitrate = bitrates[i];

		printk("*********************************************\n");
		printk("**** Classic CAN @ %u bit/s ****\n", bitrate);

		can_perf_run_single_frame_tx_latency(bitrate);
		can_perf_idle_window("after single TX latency");

		can_perf_run_single_frame_rx_latency(bitrate);
		can_perf_idle_window("after single RX latency");

		can_perf_run_roundtrip_latency(bitrate);
		can_perf_idle_window("after roundtrip latency");

		can_perf_run_sustained_roundtrip(bitrate);
		can_perf_idle_window("after sustained round-trip");

		can_perf_run_tx_throughput_classic(bitrate);
		can_perf_idle_window("after TX throughput");

		can_perf_run_rx_burst(bitrate);
		can_perf_idle_window("after RX burst");
	}

	printk("*********************************************\n");
	printk("**** CAN FD throughput ****\n");
	can_perf_run_tx_throughput_fd();
	can_perf_idle_window("after FD throughput");

	can_perf_bench_teardown();
	printk("Done\n");

	return 0;
}
