/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdlib.h>
#include <bluetooth/bluetooth.h>
#include <shell/shell.h>
#include <settings/settings.h>

#define MODULE ble_bond
#include "module_state_event.h"
#include "button_event.h"
#include "ble_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_BOND_LOG_LEVEL);


#define SHORT_CLICK_MAX K_MSEC(500)
#define LONG_CLICK_MIN  K_MSEC(5000)

#define OPERATION_TIMEOUT K_SECONDS(30)

#define PEER_ID_STORAGE_NAME "peer_id"
#define BT_ID_LUT_STORAGE_NAME "bt_lut"

enum state {
	STATE_DISABLED,
	STATE_IDLE,
	STATE_ERASE_PEER,
	STATE_ERASE_ADV,
	STATE_SELECT_PEER
};

enum click {
	CLICK_NONE,
	CLICK_SHORT,
	CLICK_LONG,
	CLICK_DOUBLE,
};

struct state_switch {
	enum state state;
	enum click click;
	enum state next_state;
	void (*func)(void);
};


static void select_start(void);
static void select_next(void);
static void select_confirm(void);

static void erase_start(void);
static void erase_adv_confirm(void);
static void erase_confirm(void);

static const struct state_switch state_switch[] = {
	 /* State           Event         Next state         Callback */
#if CONFIG_DESKTOP_BLE_PEER_SELECT
	{STATE_IDLE,        CLICK_SHORT,  STATE_SELECT_PEER, select_start},
	{STATE_SELECT_PEER, CLICK_SHORT,  STATE_SELECT_PEER, select_next},
	{STATE_SELECT_PEER, CLICK_DOUBLE, STATE_IDLE,        select_confirm},
#endif

#if CONFIG_DESKTOP_BLE_PEER_ERASE
	{STATE_IDLE,        CLICK_LONG,   STATE_ERASE_PEER,  erase_start},
#if CONFIG_BT_PERIPHERAL
	{STATE_ERASE_PEER,  CLICK_DOUBLE, STATE_ERASE_ADV,   erase_adv_confirm},
#else
	{STATE_ERASE_PEER,  CLICK_DOUBLE, STATE_IDLE,	     erase_confirm},
#endif /* CONFIG_BT_PERIPHERAL */
#endif /* CONFIG_DESKTOP_BLE_PEER_ERASE */
};


static enum state state;
static u32_t timestamp;
static u8_t cur_peer_id;

static u8_t tmp_peer_id;
static bool button_pressed_on_init;


#ifdef CONFIG_BT_PERIPHERAL
#define BT_STACK_ID_LUT_SIZE CONFIG_BT_MAX_PAIRED
#elif CONFIG_BT_CENTRAL
#define BT_STACK_ID_LUT_SIZE 0
#else
#error Device must be Bluetooth peripheral or central.
#endif

static u8_t bt_stack_id_lut[BT_STACK_ID_LUT_SIZE];
#define TEMP_PEER_ID (CONFIG_BT_MAX_PAIRED - 1)


static struct k_delayed_work timeout;
static struct k_delayed_work single_click;
static struct k_delayed_work long_click;


static u8_t get_bt_stack_peer_id(u8_t id)
{
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		if ((state == STATE_ERASE_PEER) ||
		    (state == STATE_ERASE_ADV)) {
			return bt_stack_id_lut[TEMP_PEER_ID];
		} else {
			return bt_stack_id_lut[id];
		}
	} else {
		/* For central device there is only default identity. */
		return BT_ID_DEFAULT;
	}
}

static int store_bt_stack_id_lut(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[] = MODULE_NAME "/" BT_ID_LUT_STORAGE_NAME;

		int err = settings_save_one(key, bt_stack_id_lut,
					    sizeof(bt_stack_id_lut));
		if (err) {
			LOG_ERR("Problem storing bt_stack_id_lut: (err = %d)",
				err);
			return err;
		}
	}
	return 0;
}

static void swap_bt_stack_peer_id(void)
{
	__ASSERT_NO_MSG(state == STATE_ERASE_ADV);

	/* Temporary fix for tests */
	if ((bt_stack_id_lut[0] == BT_ID_DEFAULT) &&
	    (cur_peer_id == 0)) {
		int err = bt_unpair(BT_ID_DEFAULT, NULL);

		if (err) {
			LOG_ERR("Cannot unpair for default id");
			module_set_state(MODULE_STATE_ERROR);
			return;
		}
		bt_stack_id_lut[0] = 1;
	}
	u8_t temp = bt_stack_id_lut[TEMP_PEER_ID];

	bt_stack_id_lut[TEMP_PEER_ID] = bt_stack_id_lut[cur_peer_id];
	bt_stack_id_lut[cur_peer_id] = temp;

	int err = store_bt_stack_id_lut();
	if (err) {
		module_set_state(MODULE_STATE_ERROR);
		return;
	}
}

static int remove_peers(u8_t identity)
{
	LOG_INF("Remove peers on identity %u", identity);

	int err = bt_unpair(get_bt_stack_peer_id(identity), BT_ADDR_LE_ANY);
	if (err) {
		LOG_ERR("Failed to remove");
	}

	return err;
}

static void show_single_peer(const struct bt_bond_info *info, void *user_data)
{
	const struct shell *shell = user_data;
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(&info->addr, addr, sizeof(addr));
	shell_print(shell, "\t\t%s", addr);
}

static int shell_show_peers(const struct shell *shell, size_t argc, char **argv)
{
	shell_print(shell, "Bonded peers (bt_stack):");

	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count;

	bt_id_get(addrs, &count);
	for (size_t i = 0; i < count; i++) {
		bt_addr_le_to_str(&addrs[i], addr_str, sizeof(addr_str));
		shell_print(shell, "\tIdentity %zu (%s):", i, addr_str);
		bt_foreach_bond(i, show_single_peer, (void *)shell);
	}

	return 0;
}

static int shell_remove_peers(const struct shell *shell, size_t argc,
			      char **argv)
{
	return remove_peers(cur_peer_id);
}

static void cancel_operation(void)
{
	LOG_INF("Cancel peer operation");

	timestamp = 0;

	k_delayed_work_cancel(&timeout);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = (state == STATE_ERASE_ADV) ?
		(PEER_OPERATION_ERASE_ADV_CANCEL) : (PEER_OPERATION_CANCEL);
	event->bt_app_id = cur_peer_id;

	/* Updating state to send proper bt_stack_id. */
	state = STATE_IDLE;
	event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

	EVENT_SUBMIT(event);
}

static u8_t next_peer_id(u8_t id)
{
	id++;
	BUILD_ASSERT_MSG(TEMP_PEER_ID == (CONFIG_BT_MAX_PAIRED - 1), "");
	if (id == TEMP_PEER_ID) {
		id = 0;
	}

	return id;
}

static void select_start(void)
{
	LOG_INF("Start peer select");

	tmp_peer_id = next_peer_id(cur_peer_id);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SELECT;
	event->bt_app_id = tmp_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(tmp_peer_id);

	EVENT_SUBMIT(event);
}

static void select_next(void)
{
	LOG_INF("Cycle peers");

	tmp_peer_id = next_peer_id(tmp_peer_id);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SELECT;
	event->bt_app_id = tmp_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(tmp_peer_id);

	EVENT_SUBMIT(event);
}

static void store_peer_id(u8_t peer_id)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[] = MODULE_NAME "/" PEER_ID_STORAGE_NAME;

		int err = settings_save_one(key, &peer_id, sizeof(peer_id));

		if (err) {
			LOG_ERR("Problem storing peer_id: (err = %d)", err);
		}
	}
}

static void select_confirm(void)
{
	LOG_INF("Select peer");

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	if (tmp_peer_id != cur_peer_id) {
		cur_peer_id = tmp_peer_id;

		event->op = PEER_OPERATION_SELECTED;
		store_peer_id(cur_peer_id);
	} else {
		event->op = PEER_OPERATION_CANCEL;
	}

	event->bt_app_id = cur_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

	EVENT_SUBMIT(event);
}

static void erase_start(void)
{
	LOG_INF("Start peer erase");

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_ERASE;
	event->bt_app_id = cur_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

	EVENT_SUBMIT(event);
}

static void erase_confirm(void)
{
	remove_peers(BT_ID_DEFAULT);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_ERASED;
	event->bt_app_id = cur_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

	EVENT_SUBMIT(event);
}

static void erase_adv_confirm(void)
{
	int err = bt_id_reset(get_bt_stack_peer_id(cur_peer_id),
			NULL, NULL);

	if (err < 0) {
		LOG_ERR("Cannot reset id %u (err:%d)",
			get_bt_stack_peer_id(cur_peer_id), err);
		module_set_state(MODULE_STATE_ERROR);
		return;
	}
	LOG_INF("Start advertising for peer erase");

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_ERASE_ADV;
	event->bt_app_id = cur_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

	EVENT_SUBMIT(event);
}

static enum click get_click(u32_t time_diff)
{
	if (time_diff < SHORT_CLICK_MAX) {
		return CLICK_SHORT;
	}
	return CLICK_NONE;
}

static void handle_click(enum click click)
{
	if (click != CLICK_NONE) {
		for (size_t i = 0; i < ARRAY_SIZE(state_switch); i++) {
			if ((state_switch[i].state == state) &&
			    (state_switch[i].click == click)) {
				if (state_switch[i].func) {
					state_switch[i].func();
				}
				state = state_switch[i].next_state;
				if (state != STATE_IDLE) {
					k_delayed_work_submit(&timeout,
							OPERATION_TIMEOUT);
				} else {
					k_delayed_work_cancel(&timeout);
				}
				return;
			}
		}
	}

	cancel_operation();
}

static void timeout_handler(struct k_work *work)
{
	__ASSERT_NO_MSG(state != STATE_DISABLED);

	cancel_operation();
}

static void single_click_handler(struct k_work *work)
{
	handle_click(CLICK_SHORT);

	/* Reset workqueue to make sure cancel will notice work was done. */
	single_click.work_q = NULL;
}

static void long_click_handler(struct k_work *work)
{
	timestamp = 0;
	handle_click(CLICK_LONG);

	/* Reset workqueue to make sure cancel will notice work was done. */
	single_click.work_q = NULL;
}

static void load_identities(void)
{
	bt_addr_le_t addrs[CONFIG_BT_ID_MAX];
	size_t count = ARRAY_SIZE(addrs);

	bt_id_get(addrs, &count);

	LOG_INF("Device has %zu identities", count);

	/* Default identity should always exist */
	__ASSERT_NO_MSG(count >= 1);

	for (; count < CONFIG_BT_ID_MAX; count++) {
		int err = bt_id_create(NULL, NULL);

		if (err < 0) {
			LOG_ERR("Cannot create identity (err:%d)", err);
			module_set_state(MODULE_STATE_ERROR);
			break;
		} else {
			__ASSERT_NO_MSG(err == count);
			LOG_INF("Identity %zu created", count);
		}
	}
}

static void silence_unused(void)
{
	/* These things will be opt-out by the compiler. */

	if (!IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_SELECT)) {
		ARG_UNUSED(tmp_peer_id);
		ARG_UNUSED(select_start);
		ARG_UNUSED(select_next);
		ARG_UNUSED(select_confirm);
	}

	if (!IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_ERASE)) {
		ARG_UNUSED(erase_start);
		ARG_UNUSED(erase_adv_confirm);
		ARG_UNUSED(erase_confirm);
	}

	if (!IS_ENABLED(CONFIG_SHELL)) {
		ARG_UNUSED(shell_show_peers);
		ARG_UNUSED(shell_remove_peers);
	}
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	if (!strcmp(key, PEER_ID_STORAGE_NAME)) {
		ssize_t len = read_cb(cb_arg, &cur_peer_id, sizeof(cur_peer_id));
		if (len != sizeof(cur_peer_id)) {
			LOG_ERR("Can't read peer id from storage");
			return len;
		}
	}

	if (!strcmp(key, BT_ID_LUT_STORAGE_NAME)) {
		ssize_t len = read_cb(cb_arg, &bt_stack_id_lut,
				      sizeof(bt_stack_id_lut));

		if (len != sizeof(bt_stack_id_lut)) {
			LOG_ERR("Can't read bt_stack_id_lut from storage");
			module_set_state(MODULE_STATE_ERROR);
			return len;
		}
	}

	return 0;
}

static int commit(void)
{
	struct ble_peer_operation_event *event_selected =
		new_ble_peer_operation_event();

	event_selected->op = PEER_OPERATION_SELECTED;
	event_selected->bt_app_id = cur_peer_id;
	event_selected->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);
	EVENT_SUBMIT(event_selected);

	return 0;
}

static void init_bt_stack_lut_id(void)
{
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		for (size_t i = 0; i < ARRAY_SIZE(bt_stack_id_lut); i++) {
			/* BT_ID_DEFAULT cannot be resetted - cannot be used */
			bt_stack_id_lut[i] = i + 1;
		}

		/* Temporary fix for tests */
		bt_stack_id_lut[0] = BT_ID_DEFAULT;
	}
}

static int init_settings(void)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		static struct settings_handler sh = {
			.name = MODULE_NAME,
			.h_set = settings_set,
			.h_commit = commit
		};

		int err = settings_register(&sh);
		if (err) {
			LOG_ERR("Cannot register settings handler (err %d)",
				err);
			return err;
		}
	}

	return 0;
}

static int init(void)
{
	silence_unused();

	state = STATE_IDLE;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_CONTROL)) {
		k_delayed_work_init(&timeout, timeout_handler);
		k_delayed_work_init(&single_click, single_click_handler);
		k_delayed_work_init(&long_click, long_click_handler);
	}

	load_identities();

	return 0;
}

static void peer_control_button_process(bool pressed)
{
	u32_t cur_time = k_uptime_get();

	if (pressed) {
		timestamp = cur_time;
		k_delayed_work_submit(&long_click, LONG_CLICK_MIN);
	} else {
		int err = k_delayed_work_cancel(&single_click);
		bool double_pending = (err != -EINVAL);

		k_delayed_work_cancel(&long_click);

		if (timestamp != 0) {
			enum click click = get_click(cur_time - timestamp);

			if ((click == CLICK_SHORT) && double_pending) {
				click = CLICK_DOUBLE;
			}
			if (click == CLICK_SHORT) {
				k_delayed_work_submit(&single_click, SHORT_CLICK_MAX);
			} else {
				handle_click(click);
			}
		}
	}
}

static bool button_event_handler(const struct button_event *event)
{
	if (likely(event->key_id != CONFIG_DESKTOP_BLE_PEER_CONTROL_BUTTON)) {
		return false;
	}

	if (likely(state != STATE_DISABLED)) {
		peer_control_button_process(event->pressed);
	} else {
		button_pressed_on_init = event->pressed;
	}

	return true;
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			/* Settings initialized before config module */
			init_bt_stack_lut_id();
			if (init_settings()) {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		if (check_state(event, MODULE_ID(config), MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);

			if (!init()) {
				module_set_state(MODULE_STATE_READY);
				if (button_pressed_on_init) {
					peer_control_button_process(true);
				}
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event = cast_ble_peer_event(eh);

		if ((state == STATE_ERASE_ADV) &&
		    (event->state == PEER_STATE_SECURED)) {
			/* Ignore secured for previous local id. */
			struct bt_conn_info bt_info;
			int err = bt_conn_get_info(event->id, &bt_info);

			if (err) {
				LOG_ERR("Cannot get conn info");
				module_set_state(MODULE_STATE_ERROR);
				return false;
			}

			if (bt_info.id != get_bt_stack_peer_id(cur_peer_id)) {
				LOG_INF("Connection for old id - ignored");
				bt_conn_disconnect(event->id,
					BT_HCI_ERR_REMOTE_USER_TERM_CONN);

				 return false;
			}

			LOG_INF("Erased peer");
			if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
				swap_bt_stack_peer_id();
			}

			state = STATE_IDLE;

			struct ble_peer_operation_event *event =
				new_ble_peer_operation_event();

			event->op = PEER_OPERATION_ERASED;
			event->bt_app_id = cur_peer_id;
			event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

			EVENT_SUBMIT(event);
			k_delayed_work_cancel(&timeout);
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_CONTROL) &&
	    is_button_event(eh)) {
		return button_event_handler(cast_button_event(eh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
#if CONFIG_DESKTOP_BLE_PEER_CONTROL
EVENT_SUBSCRIBE(MODULE, button_event);
#endif

#if CONFIG_SHELL
SHELL_STATIC_SUBCMD_SET_CREATE(sub_peers,
	SHELL_CMD_ARG(show, NULL, "Show bonded peers", shell_show_peers, 0, 0),
	SHELL_CMD_ARG(remove, NULL, "Remove bonded devices",
		      shell_remove_peers, 0, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(peers, &sub_peers, "BLE peer commands", NULL);
#endif /* CONFIG_SHELL */
