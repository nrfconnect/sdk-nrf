/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <sys/errno.h>
#include <debug/cpu_load.h>
#include <modem/trace_backend.h>

#define PRINT_PERIOD_MSEC K_MSEC(CONFIG_CUSTOM_BACKEND_PRINT_PERIOD_MSEC)

static uint32_t uptime_prev;
static uint32_t num_bytes_prev;
static uint32_t tot_bytes_rcvd;
static bool show_cpu_load;
static trace_backend_processed_cb trace_processed_callback;

static void print_stats(struct k_work *item);

K_WORK_DELAYABLE_DEFINE(print_stats_work, print_stats);

static void print_stats(struct k_work *item)
{
	uint32_t uptime;
	uint32_t num_bytes;
	double throughput;
	double cpu_load_percent;

	ARG_UNUSED(item);

	uptime = k_uptime_get_32();
	num_bytes = tot_bytes_rcvd;
	throughput = (num_bytes - num_bytes_prev) / ((uptime - uptime_prev) / 1000.0);

	printk("Traces received: %5.1fkB, %4.1fkB/s", num_bytes / 1000.0, throughput / 1000.0);

	if (show_cpu_load) {
		cpu_load_percent = cpu_load_get() / 1000.0;
		printk(", CPU-load: %5.2f%%\n", cpu_load_percent);
		cpu_load_reset();
	} else {
		printk("\n");
	}

	uptime_prev = uptime;
	num_bytes_prev = num_bytes;

	k_work_schedule(&print_stats_work, PRINT_PERIOD_MSEC);
}

int trace_backend_init(trace_backend_processed_cb trace_processed_cb)
{
	int err;

	if (trace_processed_cb == NULL) {
		return -EFAULT;
	}

	trace_processed_callback = trace_processed_cb;

	err = cpu_load_init();
	if (err) {
		if (err == -ENODEV) {
			printk("Failed to allocate PPI channels for cpu load estimation\n");
		} else if (err == -EBUSY) {
			printk("Failed to allocate TIMER instance for cpu load estimation\n");
		} else {
			printk("Failed to initialize cpu load module\n");
		}

		show_cpu_load = false;
	} else {
		show_cpu_load = true;
		cpu_load_reset();
	}

	num_bytes_prev = tot_bytes_rcvd;
	uptime_prev = k_uptime_get_32();

	k_work_schedule(&print_stats_work, PRINT_PERIOD_MSEC);

	printk("Custom trace backend initialized\n");

	return 0;
}

int trace_backend_deinit(void)
{
	k_work_cancel_delayable(&print_stats_work);

	printk("Custom trace backend deinitialized\n");

	return 0;
}

int trace_backend_write(const void *data, size_t len)
{
	ARG_UNUSED(data);

	tot_bytes_rcvd += len;

	trace_processed_callback(len);

	return (int)len;
}

struct nrf_modem_lib_trace_backend trace_backend = {
	.init = trace_backend_init,
	.deinit = trace_backend_deinit,
	.write = trace_backend_write,
};
