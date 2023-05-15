/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <bluetooth/services/fast_pair.h>
#include <zephyr/sys/reboot.h>

#define MODULE factory_reset
#include <caf/events/module_state_event.h>
#include "config_event.h"

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_FACTORY_RESET_LOG_LEVEL);

/* Use a non-zero timeout to ensure that configuration channel transaction is finished. */
#define FACTORY_RESET_TIMEOUT		K_MSEC(250)

enum factory_reset_opt {
	FACTORY_RESET_OPT_FAST_PAIR,

	FACTORY_RESET_OPT_COUNT
};

const static char * const opt_descr[] = {
	[FACTORY_RESET_OPT_FAST_PAIR] = "fast_pair",
};

static struct k_work_delayable factory_reset_timeout;
static bool ble_bond_initialized;


/* Override weak function to define an application-specific user action for factory reset. */
int bt_fast_pair_factory_reset_user_action_perform(void)
{
	/* The Bluetooth LE advertising must be stopped before Bluetooth local identity reset. */
	int err = bt_le_adv_stop();

	if (err) {
		LOG_ERR("Cannot stop Bluetooth LE advertising (err: %d)", err);
		return err;
	}

	err = bt_unpair(BT_ID_DEFAULT, NULL);
	if (err) {
		LOG_ERR("Cannot unpair for default ID");
		return err;
	}

	for (size_t i = 1; i < CONFIG_BT_ID_MAX; i++) {
		err = bt_id_reset(i, NULL, NULL);
		if (err < 0) {
			LOG_ERR("Cannot reset id %zu (err %d)", i, err);
			return err;
		}
	}

	return 0;
}

static void factory_reset_timeout_fn(struct k_work *work)
{
	ARG_UNUSED(work);

	int err = bt_fast_pair_factory_reset();

	if (err) {
		LOG_ERR("Fast Pair factory reset failed: %d", err);
	} else {
		LOG_INF("Successful Fast Pair factory reset");
	}

	/* Put logger into panic mode to ensure that logs are displayed before reboot. */
	LOG_PANIC();
	sys_reboot(SYS_REBOOT_WARM);
}

static int perform_factory_reset(void)
{
	if (!ble_bond_initialized) {
		return -EBUSY;
	}

	/* Schedule factory reset after a predefined timeout. */
	(void)k_work_schedule(&factory_reset_timeout, FACTORY_RESET_TIMEOUT);

	return 0;
}

static void handle_factory_reset(const uint8_t *data, const size_t size)
{
	ARG_UNUSED(data);
	ARG_UNUSED(size);

	int err = perform_factory_reset();

	if (err) {
		LOG_WRN("Factory reset not allowed, err: %d", err);
	}
}

static void config_set(const uint8_t opt_id, const uint8_t *data, const size_t size)
{
	switch (opt_id) {
	case FACTORY_RESET_OPT_FAST_PAIR:
		handle_factory_reset(data, size);
		break;

	default:
		/* Ignore unknown event. */
		LOG_WRN("Unsupported config channel set option ID: 0x%" PRIx8, opt_id);
		break;
	}
}

static void config_fetch(const uint8_t opt_id, uint8_t *data, size_t *size)
{
	ARG_UNUSED(data);
	ARG_UNUSED(size);

	LOG_WRN("Unsupported config channel fetch option ID: 0x%" PRIx8, opt_id);
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		k_work_init_delayable(&factory_reset_timeout, factory_reset_timeout_fn);
	} else if (check_state(event, MODULE_ID(ble_bond), MODULE_STATE_READY)) {
		ble_bond_initialized = true;
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	GEN_CONFIG_EVENT_HANDLERS(STRINGIFY(MODULE), opt_descr, config_set, config_fetch);

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
