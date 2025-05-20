/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_rtt.h>
#include <nrf_profiler.h>

static int display_registered_events(const struct shell *shell, size_t argc,
				char **argv)
{
	shell_fprintf(shell, SHELL_NORMAL, "EVENTS REGISTERED IN NRF_PROFILER:\n");
	for (size_t i = 0; i < nrf_profiler_num_events; i++) {
		const char *event_name = nrf_profiler_get_event_descr(i);
		/* Looking for event name delimiter (',') */
		char *event_name_end = strchr(event_name, ',');

		shell_fprintf(shell,
			   SHELL_NORMAL,
			   "%c %d:\t%.*s\n",
			   (atomic_test_bit(_nrf_profiler_event_enabled_bm.flags, i)) ? 'E' : 'D',
			   i,
			   event_name_end - event_name,
			   event_name);
	}

	return 0;
}

static void set_event_profiling(const struct shell *shell, size_t argc,
				char **argv, bool enable)
{
	/* If no IDs specified, all registered events are affected */
	if (argc == 1) {
		for (int i = 0; i < nrf_profiler_num_events; i++) {
			atomic_set_bit_to(_nrf_profiler_event_enabled_bm.flags, i, enable);
		}

		shell_fprintf(shell,
			      SHELL_NORMAL,
			      "Profiling all events %s\n",
			      enable ? "enabled":"disabled");
	} else {
		const size_t index_cnt = argc - 1;
		long int event_indexes[index_cnt];

		for (size_t i = 0; i < index_cnt; i++) {
			char *end;

			event_indexes[i] = strtol(argv[i + 1], &end, 10);

			if (event_indexes[i] < 0 ||
			    event_indexes[i] >= nrf_profiler_num_events ||
			    *end != '\0') {
				shell_error(shell, "Invalid event ID: %s",
					    argv[i + 1]);
				return;
			}
		}

		for (size_t i = 0; i < index_cnt; i++) {
			atomic_set_bit_to(_nrf_profiler_event_enabled_bm.flags,
					  event_indexes[i], enable);
			const char *event_name = nrf_profiler_get_event_descr(
							event_indexes[i]);
			/* Looking for event name delimiter (',') */
			char *event_name_end = strchr(event_name, ',');

			shell_fprintf(shell,
				      SHELL_NORMAL,
				      "Profiling event %.*s %sabled\n",
				      event_name_end - event_name,
				      event_name,
				      enable ? "en":"dis");
		}
	}
}

static int enable_event_profiling(const struct shell *shell, size_t argc,
			      char **argv)
{
	set_event_profiling(shell, argc, argv, true);
	return 0;
}

static int disable_event_profiling(const struct shell *shell, size_t argc,
			      char **argv)
{
	set_event_profiling(shell, argc, argv, false);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_nrf_profiler,
	SHELL_CMD_ARG(list, NULL, "Display list of events",
			display_registered_events, 0, 0),
	SHELL_CMD_ARG(enable, NULL, "Enable profiling of event with given ID",
			enable_event_profiling, 1,
			sizeof(_nrf_profiler_event_enabled_bm) * 8),
	SHELL_CMD_ARG(disable, NULL, "Disable profiling of event with given ID",
			disable_event_profiling, 1,
			sizeof(_nrf_profiler_event_enabled_bm) * 8),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(nrf_profiler, &sub_nrf_profiler, "Profiler commands", NULL);
