/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include "event_proxy_def.h"

#include <zephyr/ipc/ipc_service.h>
#include <event_manager_proxy.h>

#define MODULE event_proxy

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_EVENT_PROXY_LOG_LEVEL);

#define EVENT_PROXY_INIT_TIMEOUT CONFIG_ML_APP_REMOTE_CORE_INITIALIZATION_TIMEOUT


static int event_proxy_init(void)
{
	int ret = 0;

	for (size_t i = 0; i < ARRAY_SIZE(proxy_configs); i++) {
		const struct event_proxy_config *pcfg = proxy_configs[i];
		const struct device *inst = pcfg->ipc;

		ret = event_manager_proxy_add_remote(inst);
		if (ret) {
			LOG_ERR("Can not add %s remote: %d", inst->name, ret);
			__ASSERT_NO_MSG(false);
			return ret;
		}

		for (size_t e_idx = 0; e_idx < pcfg->events_cnt; e_idx++) {
			ret = event_manager_proxy_subscribe(inst, pcfg->events[e_idx]);
			if (ret) {
				LOG_ERR("Can not subscribe to remote %s: %d",
					pcfg->events[e_idx]->name, ret);
				__ASSERT_NO_MSG(false);
				return ret;
			}
		}
	}

	ret = event_manager_proxy_start();
	if (ret) {
		LOG_ERR("Can not start: %d", ret);
		__ASSERT_NO_MSG(false);
		return ret;
	}

	ret = event_manager_proxy_wait_for_remotes(K_MSEC(EVENT_PROXY_INIT_TIMEOUT));
	if (ret) {
		LOG_ERR("Remotes not ready: %d", ret);
		__ASSERT_NO_MSG(false);
		return ret;
	}

	return 0;
}

APP_EVENT_MANAGER_HOOK_POSTINIT_REGISTER(event_proxy_init);
