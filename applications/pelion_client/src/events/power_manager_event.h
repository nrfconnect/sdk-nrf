/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#ifndef _POWER_MANAGER_EVENT_H_
#define _POWER_MANAGER_EVENT_H_

#include <event_manager.h>

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
	POWER_MANAGER_LEVEL_MAX
};


/**
 * @brief An event to specify which power state is allowed by module
 *
 * Any module can set maximum allowed power state that can be configured
 * by power manager.
 */
struct power_manager_restrict_event {
	struct event_header header;
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

EVENT_TYPE_DECLARE(power_manager_restrict_event);

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
	EVENT_SUBMIT(event);
}

#ifdef __cplusplus
}
#endif

#endif /* _POWER_MANAGER_EVENT_H_ */
