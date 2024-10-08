/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <malloc.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/sys_heap.h>
#include <zephyr/linker/linker-defs.h>
#include <zephyr/shell/shell.h>
#include <modem/nrf_modem_lib.h>

#include "desh_print.h"

extern struct k_poll_signal desh_signal;

int sleep_shell(const struct shell *shell, size_t argc, char **argv)
{
	long sleep_duration = strtol(argv[1], NULL, 10);

	if (sleep_duration > 0) {
		k_sleep(K_SECONDS(sleep_duration));
		return 0;
	}
	desh_print("sleep: duration must be greater than zero");

	return -EINVAL;
}

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS)
extern struct sys_heap _system_heap;
#endif

int heap_shell(const struct shell *shell, size_t argc, char **argv)
{
	struct mallinfo system_stats;

#if defined(CONFIG_SYS_HEAP_RUNTIME_STATS)
	int err;
	struct sys_memory_stats kernel_stats;

	err = sys_heap_runtime_stats_get(&_system_heap, &kernel_stats);
	if (err) {
		desh_error("heap: failed to read kernel heap statistics, error: %d", err);
	} else {
		desh_print("kernel heap statistics:");
		desh_print("free:           %6d", kernel_stats.free_bytes);
		desh_print("allocated:      %6d", kernel_stats.allocated_bytes);
		desh_print("max. allocated: %6d\n", kernel_stats.max_allocated_bytes);
	}
#endif /* CONFIG_SYS_HEAP_RUNTIME_STATS */

	/* Calculate the system heap maximum size. */
#define USED_RAM_END_ADDR POINTER_TO_UINT(&_end)
#define HEAP_BASE USED_RAM_END_ADDR
#define MAX_HEAP_SIZE (KB(CONFIG_SRAM_SIZE) - (HEAP_BASE - CONFIG_SRAM_BASE_ADDRESS))

	system_stats = mallinfo();

	desh_print("system heap statistics:");
	desh_print("max. size:      %6d", MAX_HEAP_SIZE);
	desh_print("size:           %6d", system_stats.arena);
	desh_print("free:           %6d", system_stats.fordblks);
	desh_print("allocated:      %6d", system_stats.uordblks);

	return 0;
}

int version_shell(const struct shell *shell, size_t argc, char **argv)
{
	desh_print_version_info();

	return 0;
}

SHELL_CMD_ARG_REGISTER(sleep, NULL,
	"Sleep for n seconds.",
	sleep_shell, 2, 0);

SHELL_CMD_ARG_REGISTER(heap, NULL,
	"Print heap usage statistics.",
	heap_shell, 1, 0);

SHELL_CMD_ARG_REGISTER(version, NULL,
	"Print application version information.",
	version_shell, 1, 0);
