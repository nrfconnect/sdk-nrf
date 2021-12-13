/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <caf/events/power_manager_event.h>

#include <assert.h>
#include <sys/util.h>
#include <stdio.h>
#include <caf/events/module_state_event.h>



static int log_restrict_event(const struct event_header *eh,
		     char *buf, size_t buf_len)
{
	const struct power_manager_restrict_event *event = cast_power_manager_restrict_event(eh);
	enum power_manager_level lvl = event->level;
	const char *power_state_str;

	__ASSERT_NO_MSG(event->module_idx < module_count());
	__ASSERT_NO_MSG(lvl <= POWER_MANAGER_LEVEL_MAX);
	__ASSERT_NO_MSG(lvl >= POWER_MANAGER_LEVEL_ALIVE);

	switch (lvl) {
	case POWER_MANAGER_LEVEL_ALIVE:
		power_state_str = "ALIVE";
		break;
	case POWER_MANAGER_LEVEL_SUSPENDED:
		power_state_str = "SUSPENDED";
		break;
	case POWER_MANAGER_LEVEL_OFF:
		power_state_str = "OFF";
		break;
	case POWER_MANAGER_LEVEL_MAX:
		power_state_str = "MAX";
		break;
	default:
		power_state_str = "INVALID";
		break;
	}

	EVENT_MANAGER_LOG(eh, "module \"%s\" restricts to %s",
			module_name_get(module_id_get(event->module_idx)),
			power_state_str);
	return 0;
}

static void profile_restrict_event(struct log_event_buf *buf,
			  const struct event_header *eh)
{
	const struct power_manager_restrict_event *event = cast_power_manager_restrict_event(eh);

	profiler_log_encode_uint32(buf, event->module_idx);
	profiler_log_encode_int8(buf, event->level);
}

static int log_configure_event(const struct event_header *eh,
		     char *buf, size_t buf_len)
{
	const struct power_manager_configure_timeout_event *event =
		cast_power_manager_configure_timeout_event(eh);
	__ASSERT_NO_MSG(event->module_idx < module_count());
	enum power_manager_timeout_setting setting = event->timeout_setting;
	const int new_timeout = event->timeout_seconds;
	const char *setting_str;

	switch (setting) {
	case POWER_DOWN_ERROR_TIMEOUT:
		setting_str = "Power down error timeout";
		break;
	case POWER_DOWN_TIMEOUT:
		setting_str = "Power down timeout";
		break;
	}
	return snprintf(buf, buf_len, "%s set to %d",
	setting_str, new_timeout);
}

static void profile_configure_event(struct log_event_buf *buf,
			  const struct event_header *eh)
{
	const struct power_manager_configure_timeout_event *event =
		cast_power_manager_configure_timeout_event(eh);

	profiler_log_encode_uint32(buf, event->module_idx);
	profiler_log_encode_int8(buf, event->timeout_setting);
}

EVENT_INFO_DEFINE(power_manager_restrict_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_S8),
		  ENCODE("module", "level"),
		  profile_restrict_event
);

EVENT_TYPE_DEFINE(power_manager_restrict_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_POWER_MANAGER_EVENTS),
		  log_restrict_event,
		  &power_manager_restrict_event_info
);

EVENT_INFO_DEFINE(power_manager_configure_timeout_event,
		  ENCODE(PROFILER_ARG_U32, PROFILER_ARG_S8),
		  ENCODE("module", "timeout setting"),
		  profile_configure_event
);

EVENT_TYPE_DEFINE(power_manager_configure_timeout_event,
		  IS_ENABLED(CONFIG_CAF_INIT_LOG_POWER_MANAGER_EVENTS),
		  log_configure_event,
		  &power_manager_configure_timeout_event_info
);
