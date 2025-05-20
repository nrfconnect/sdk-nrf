/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _POWER_MANAGER_EVENT_H_
#define _POWER_MANAGER_EVENT_H_

/**
 * @file
 * @defgroup caf_power_manager_event CAF Power Manager Event
 * @{
 * @brief CAF Power Manager Event.
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief Available power levels
 *
 * The power levels provided by the power manager.
 */
enum power_manager_level {
	/**
	 * @brief Stay alive.
	 */
	POWER_MANAGER_LEVEL_ALIVE = -1,
	/**
	 * @brief Suspend but do not go to power off
	 */
	POWER_MANAGER_LEVEL_SUSPENDED,
	/**
	 * @brief Go to full power off
	 */
	POWER_MANAGER_LEVEL_OFF,
	/**
	 * @brief Number of supported levels
	 */
	POWER_MANAGER_LEVEL_MAX,

	/** Unused in code, required for inter-core compatibility. */
	APP_EM_ENFORCE_ENUM_SIZE(POWER_MANAGER_LEVEL)
};


/**
 * @brief An event to specify which power state is allowed by module
 *
 * Any module can set maximum allowed power state that can be configured
 * by power manager.
 */
struct power_manager_restrict_event {
	/** Event header. */
	struct app_event_header header;
	/**
	 * @brief The module index
	 *
	 * The index of the module that wish to block possibility to set power
	 * mode below specified level.
	 */
	size_t module_idx;
	/**
	 * @brief The deepest sleep mode allowed
	 */
	enum power_manager_level level;
};

APP_EVENT_TYPE_DECLARE(power_manager_restrict_event);

/**
 * @brief Set the deepest power sleep mode allowed
 *
 * @param module_idx The index of the module
 * @param lvl        Maximal allowed mode
 */
static inline void power_manager_restrict(size_t module_idx, enum power_manager_level lvl)
{
	struct power_manager_restrict_event *event = new_power_manager_restrict_event();

	event->module_idx = module_idx;
	event->level = lvl;
	APP_EVENT_SUBMIT(event);
}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _POWER_MANAGER_EVENT_H_ */
