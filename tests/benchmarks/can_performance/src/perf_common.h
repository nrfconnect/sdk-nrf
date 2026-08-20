/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef PERF_COMMON_H_
#define PERF_COMMON_H_

#include <stddef.h>
#include <stdint.h>
#include <zephyr/device.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>

/* Same IDs/timeouts as zephyr/tests/drivers/can/api/src/common.h */
#define CAN_PERF_STD_ID	      0x123U	  /* TEST_CAN_SOME_STD_ID */
#define CAN_PERF_SEND_TIMEOUT K_MSEC(100) /* TEST_SEND_TIMEOUT */
#define CAN_PERF_RECV_TIMEOUT K_MSEC(100) /* TEST_RECEIVE_TIMEOUT */

#define CAN_PERF_TEST_BITRATE_1 125000	/* TEST_BITRATE_1 */
#define CAN_PERF_TEST_BITRATE_2 250000	/* TEST_BITRATE_2 */
#define CAN_PERF_TEST_BITRATE_3 1000000 /* TEST_BITRATE_3 */

extern const uint32_t can_perf_classic_bitrates[];
extern const size_t can_perf_classic_bitrate_count;
extern const uint32_t can_perf_classic_bitrate_short;

struct can_perf_result {
	uint32_t frame_count;
	uint64_t elapsed_us;
	uint32_t frames_per_sec;
	uint32_t bytes_per_sec;
	uint32_t bus_efficiency_permille;
};

struct can_perf_latency_stats {
	uint32_t samples;
	uint64_t avg_us;
	uint64_t min_us;
	uint64_t max_us;
	uint64_t p99_us;
};

extern const struct device *const can_perf_dev;

uint32_t can_perf_cycles_elapsed(uint32_t start, uint32_t end);

uint32_t can_perf_classic_frame_bits(uint8_t dlc);

uint32_t can_perf_theoretical_fps(uint32_t bitrate, uint8_t dlc, bool fd, uint32_t bitrate_data);

void can_perf_fill_result(struct can_perf_result *result, uint32_t frame_count,
			  uint32_t payload_bytes, uint32_t start_cycles, uint32_t end_cycles,
			  uint32_t bitrate, uint8_t dlc, bool fd, uint32_t bitrate_data);

void can_perf_print_result(const char *label, const struct can_perf_result *result);

void can_perf_assert_thresholds(const char *label, const struct can_perf_result *result,
				uint32_t min_fps);

int can_perf_setup_loopback(can_mode_t mode);

int can_perf_set_bitrate_classic(uint32_t bitrate);

int can_perf_set_bitrate_fd(uint32_t bitrate, uint32_t bitrate_data);

bool can_perf_prepare_classic(uint32_t bitrate);

bool can_perf_prepare_fd(void);

void can_perf_teardown(void);

void can_perf_latency_compute_stats(uint64_t *samples, uint32_t count,
				    struct can_perf_latency_stats *stats);

void can_perf_print_latency_stats(const char *label, const struct can_perf_latency_stats *stats);

int can_perf_bench_init(void);

void can_perf_bench_teardown(void);

void can_perf_idle_window(const char *label);

void can_perf_run_tx_throughput_classic(uint32_t bitrate);

void can_perf_run_tx_throughput_fd(void);

void can_perf_run_single_frame_tx_latency(uint32_t bitrate);

void can_perf_run_single_frame_rx_latency(uint32_t bitrate);

void can_perf_run_roundtrip_latency(uint32_t bitrate);

void can_perf_run_sustained_roundtrip(uint32_t bitrate);

void can_perf_run_rx_burst(uint32_t bitrate);

#endif /* PERF_COMMON_H_ */
