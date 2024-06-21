/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <app_event_manager.h>

#define MODULE main
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE);


int main(void)
{
	if (app_event_manager_init()) {
		LOG_ERR("Application Event Manager not initialized");
	} else {
		module_set_state(MODULE_STATE_READY);
	}
	return 0;
}

#if defined(CONFIG_NRFS_MRAM_SERVICE_ENABLED)

#include <services/nrfs_mram.h>
#include <nrfs_backend_ipc_service.h>

/**
 * @brief Handle mram latency events from sysctrl
 *
 * @param p_evt incoming event
 * @param context context matching request
 */
void mram_latency_handler(nrfs_mram_latency_evt_t const *p_evt, void *context)
{
	switch (p_evt->type) {
	case NRFS_MRAM_LATENCY_REQ_APPLIED:
		LOG_INF("MRAM latency handler: response received");
		break;
	case NRFS_MRAM_LATENCY_REQ_REJECTED:
		LOG_ERR("MRAM latency handler - request rejected!");
		break;
	default:
		LOG_ERR("MRAM latency handler - unexpected event: 0x%x", p_evt->type);
		break;
	}
}

static int turn_off_suspend_mram(void)
{
	uint32_t ctx = 0;
	nrfs_err_t err = NRFS_SUCCESS;
	/*
	 * Turn off mram automatic suspend as it causes radio core not to manage timings.
	 */

	LOG_INF("Waiting for backend init");
	/* Wait for ipc initialization */
	nrfs_backend_wait_for_connection(K_FOREVER);

	err = nrfs_mram_init(mram_latency_handler);
	if (err != NRFS_SUCCESS) {
		LOG_ERR("Mram service init failed: %d", err);
	} else {
		LOG_INF("MRAM init done");
	}

	LOG_INF("MRAM: set latency: NOT ALLOWED");
	err = nrfs_mram_set_latency(MRAM_LATENCY_NOT_ALLOWED, (void *)ctx);

	if (err) {
		LOG_ERR("Settings init failed (%d)", err);
	}

	return err;
}

SYS_INIT(turn_off_suspend_mram, APPLICATION, 90);
#endif
