/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <string.h>
#include <stdlib.h>
#include <shell/shell.h>
#include <shell/shell_rtt.h>
#include <profiler.h>

uint32_t profiler_enabled_events;

static int display_registered_events(const struct shell *shell, size_t argc,
				char **argv)
{
	/* Check if flags for enabling/disabling custom event fit on uint32_t */
	BUILD_ASSERT(CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS <=
			 sizeof(profiler_enabled_events) * 8,
			 "Max 32 profiler events may be used");
	shell_fprintf(shell, SHELL_NORMAL, "EVENTS REGISTERED IN PROFILER:\n");
	for (size_t i = 0; i < profiler_num_events; i++) {
		const char *event_name = profiler_get_event_descr(i);
		/* Looking for event name delimiter (',') */
		char *event_name_end = strchr(event_name, ',');

		shell_fprintf(shell,
			      SHELL_NORMAL,
			      "%c %d:\t%.*s\n",
			      (profiler_enabled_events & BIT(i)) ? 'E' : 'D',
			      i,
			      event_name_end - event_name,
			      event_name);
	}

	return 0;
}

static void set_event_profiling(const struct shell *shell, size_t argc,
				char **argv, bool enable)
{
	uint32_t evt_mask = 0;

	/* If no IDs specified, all registered events are affected */
	if (argc == 1) {
		for (int i = 0; i < profiler_num_events; i++) {
			evt_mask |= BIT(i);
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
			    event_indexes[i] >= profiler_num_events ||
			    *end != '\0') {
				shell_error(shell, "Invalid event ID: %s",
					    argv[i + 1]);
				return;
			}
		}

		for (size_t i = 0; i < index_cnt; i++) {
			evt_mask |= BIT(event_indexes[i]);
			const char *event_name = profiler_get_event_descr(
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
	if (enable) {
		profiler_enabled_events |= evt_mask;
	} else {
		profiler_enabled_events &= ~evt_mask;
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

SHELL_STATIC_SUBCMD_SET_CREATE(sub_profiler,
	SHELL_CMD_ARG(list, NULL, "Display list of events",
			display_registered_events, 0, 0),
	SHELL_CMD_ARG(enable, NULL, "Enable profiling of event with given ID",
			enable_event_profiling, 1,
			sizeof(profiler_enabled_events) * 8),
	SHELL_CMD_ARG(disable, NULL, "Disable profiling of event with given ID",
			disable_event_profiling, 1,
			sizeof(profiler_enabled_events) * 8),
	SHELL_SUBCMD_SET_END
);
SHELL_CMD_REGISTER(profiler, &sub_profiler, "Profiler commands", NULL);
