/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <shell/shell.h>
#include <event_manager.h>

u32_t event_manager_displayed_events;

static int show_events(const struct shell *shell, size_t argc,
		char **argv)
{
	shell_fprintf(shell, SHELL_NORMAL,
		      "Registered Events:\n");

	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types); et++) {

		size_t ev_id = et - __start_event_types;

		shell_fprintf(shell,
			      SHELL_NORMAL,
			      "%c %d:\t%s\n",
			      (event_manager_displayed_events & BIT(ev_id)) ?
				'E' : 'D',
			      ev_id,
			      et->name);
	}

	return 0;
}

static int show_listeners(const struct shell *shell, size_t argc,
		char **argv)
{
	shell_fprintf(shell, SHELL_NORMAL, "Registered Listeners:\n");
	for (const struct event_listener *el = __start_event_listeners;
	     el != __stop_event_listeners;
	     el++) {

		__ASSERT_NO_MSG(el != NULL);
		shell_fprintf(shell, SHELL_NORMAL, "|\t[L:%s]\n", el->name);
	}

	return 0;
}

static int show_subscribers(const struct shell *shell, size_t argc,
		char **argv)
{
	shell_fprintf(shell, SHELL_NORMAL, "Registered Subscribers:\n");
	for (const struct event_type *et = __start_event_types;
	     (et != NULL) && (et != __stop_event_types);
	     et++) {

		bool is_subscribed = false;

		for (size_t prio = SUBS_PRIO_MIN;
		     prio <= SUBS_PRIO_MAX;
		     prio++) {
			for (const struct event_subscriber *es =
					et->subs_start[prio];
			     es != et->subs_stop[prio];
			     es++) {

				__ASSERT_NO_MSG(es != NULL);
				const struct event_listener *el = es->listener;

				__ASSERT_NO_MSG(el != NULL);
				shell_fprintf(shell, SHELL_NORMAL,
					      "|\tprio:%u\t[E:%s] -> [L:%s]\n",
					      prio, et->name, el->name);

				is_subscribed = true;
			}
		}

		if (!is_subscribed) {
			shell_fprintf(shell, SHELL_NORMAL,
				      "|\t[E:%s] has no subscribers\n",
				      et->name);
		}
		shell_fprintf(shell, SHELL_NORMAL, "\n");
	}

	return 0;
}

static void set_event_displaying(const struct shell *shell, size_t argc,
				 char **argv, bool enable)
{
	u32_t evt_mask = 0;

	/* If no IDs specified, all registered events are affected */
	if (argc == 1) {
		for (const struct event_type *et = __start_event_types;
		     (et != NULL) && (et != __stop_event_types);
		     et++) {

			size_t ev_id = et - __start_event_types;

			evt_mask |= BIT(ev_id);
		}

		shell_fprintf(shell,
			      SHELL_NORMAL,
			      "Displaying all events %sabled\n",
			      enable ? "en":"dis");
	} else {
		int event_indexes[argc - 1];

		for (size_t i = 0; i < ARRAY_SIZE(event_indexes); i++) {
			char *end;

			event_indexes[i] = strtol(argv[i + 1], &end, 10);

			if ((event_indexes[i] < 0)
			    || (event_indexes[i] >= __stop_event_types - __start_event_types)
			    || (*end != '\0')) {

				shell_error(shell, "Invalid event ID: %s",
					    argv[i + 1]);
				return;
			}
		}

		for (size_t i = 0; i < ARRAY_SIZE(event_indexes); i++) {
			evt_mask |= BIT(event_indexes[i]);
			const struct event_type *et =
				__start_event_types + event_indexes[i];
			const char *event_name = et->name;

			shell_fprintf(shell,
				      SHELL_NORMAL,
				      "Displaying event %s %sabled\n",
				      event_name,
				      enable ? "en":"dis");
		}
	}
	if (enable) {
		event_manager_displayed_events |= evt_mask;
	} else {
		event_manager_displayed_events &= ~evt_mask;
	}
}

static int enable_event_displaying(const struct shell *shell, size_t argc,
				   char **argv)
{
	set_event_displaying(shell, argc, argv, true);
	return 0;
}

static int disable_event_displaying(const struct shell *shell, size_t argc,
				    char **argv)
{
	set_event_displaying(shell, argc, argv, false);
	return 0;
}


SHELL_CREATE_STATIC_SUBCMD_SET(sub_event_manager)
{
	SHELL_CMD_ARG(show_listeners, NULL, "Show listeners",
		      show_listeners, 0, 0),
	SHELL_CMD_ARG(show_subscribers, NULL, "Show subscribers",
		      show_subscribers, 0, 0),
	SHELL_CMD_ARG(show_events, NULL, "Show events", show_events, 0, 0),
	SHELL_CMD_ARG(disable, NULL, "Disable displaying event with given ID",
		      disable_event_displaying, 0,
		      sizeof(event_manager_displayed_events) * 8 - 1),
	SHELL_CMD_ARG(enable, NULL, "Enable displaying event with given ID",
		      enable_event_displaying, 0,
		      sizeof(event_manager_displayed_events) * 8 - 1),
	SHELL_SUBCMD_SET_END
};

SHELL_CMD_REGISTER(event_manager, &sub_event_manager,
		   "Event Manager commands", NULL);
