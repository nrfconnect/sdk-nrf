/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <bluetooth/bluetooth.h>
#include <shell/shell.h>

#define MODULE ble_bond
#include "module_state_event.h"
#include "button_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_BOND_LOG_LEVEL);

static bool bonds_initialized;
static bool remove;

static int bonds_remove(void)
{
	int err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err) {
		LOG_ERR("Failed to remove bonds");
	} else {
		LOG_INF("Removed bonded devices");
	}

	return err;
}

static void init(void)
{
	int err = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_BOND_REMOVAL)) {
		if (remove) {
			err = bonds_remove();
		}

		bonds_initialized = true;
	}

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
	} else {
		module_set_state(MODULE_STATE_READY);
	}
}

#if CONFIG_SHELL
void peer_info_show(const struct bt_bond_info *info, void *user_data)
{
	const struct shell *shell = (const struct shell *)user_data;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&info->addr, addr, sizeof(addr));
	shell_print(shell, "\t%s", addr);
}

static int cmd_bonds_show(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Bonded peers:");
	bt_foreach_bond(BT_ID_DEFAULT, peer_info_show, (void *)shell);

	return 0;
}

static int cmd_bonds_remove(const struct shell *shell, size_t argc, char **argv)
{
	int err = bonds_remove();

	return err;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_bonds,
	SHELL_CMD_ARG(show, NULL, "Show bonds", cmd_bonds_show, 0, 0),
	SHELL_CMD_ARG(remove, NULL, "Remove all bonded devices",
		      cmd_bonds_remove, 0, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(bonds, &sub_bonds, "BLE bonds commands", NULL);
#endif /* CONFIG_SHELL */

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(config), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);

			/* Settings need to be initialized before bonds can be removed */
			init();
			initialized = true;
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_BOND_REMOVAL)) {
		if (is_button_event(eh)) {
			if (bonds_initialized) {
				return false;
			}

			const struct button_event *event =
				cast_button_event(eh);

			if ((event->key_id ==
			    CONFIG_DESKTOP_BLE_BOND_REMOVAL_BUTTON) &&
			    event->pressed) {
				remove = true;
			}

			return false;
		}
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
#if CONFIG_DESKTOP_BLE_BOND_REMOVAL
EVENT_SUBSCRIBE(MODULE, button_event);
#endif /* CONFIG_DESKTOP_BLE_BOND_REMOVAL */
