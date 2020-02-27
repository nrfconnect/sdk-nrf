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
#include "click_event.h"
#include "ble_event.h"
#include "selector_event.h"
#include "config_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_BOND_LOG_LEVEL);

#define CONFIRM_TIMEOUT			K_SECONDS(10)
#define ERASE_ADV_TIMEOUT		K_SECONDS(30)

#define PEER_ID_STORAGE_NAME		"peer_id"
#define BT_ID_LUT_STORAGE_NAME		"bt_lut"

#define ON_START_CLICK(CLICK)		(CLICK + CLICK_COUNT)
#define ON_START_CLICK_UPTIME_MAX	K_SECONDS(6)


enum state {
	STATE_DISABLED,
	STATE_IDLE,
	STATE_ERASE_PEER,
	STATE_ERASE_ADV,
	STATE_SELECT_PEER,
	STATE_DONGLE_CONN
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

static void scan_request(void);

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

#if CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST
	{STATE_IDLE,        CLICK_SHORT,  STATE_IDLE,        scan_request},
#endif

#if CONFIG_DESKTOP_BLE_PEER_ERASE
	{STATE_IDLE,        CLICK_LONG,   STATE_ERASE_PEER,  erase_start},
#if CONFIG_BT_PERIPHERAL
	{STATE_ERASE_PEER,  CLICK_DOUBLE, STATE_ERASE_ADV,   erase_adv_confirm},
#elif CONFIG_BT_CENTRAL
	{STATE_ERASE_PEER,  CLICK_DOUBLE, STATE_IDLE,	     erase_confirm},
#endif /* CONFIG_BT_PERIPHERAL */
#endif /* CONFIG_DESKTOP_BLE_PEER_ERASE */

#if CONFIG_DESKTOP_BLE_PEER_ERASE_ON_START
	{STATE_IDLE,	ON_START_CLICK(CLICK_LONG), STATE_ERASE_ADV, erase_adv_confirm},
#endif /* CONFIG_DESKTOP_BLE_PEER_ERASE_ON_START */
};


static enum state state;
static u8_t cur_peer_id;
static bool cur_peer_id_valid;
static u8_t tmp_peer_id;
static bool dongle_peer_selected_on_init;


#if CONFIG_BT_PERIPHERAL
#define BT_STACK_ID_LUT_SIZE CONFIG_BT_MAX_PAIRED
#elif CONFIG_BT_CENTRAL
#define BT_STACK_ID_LUT_SIZE 0
#else
#error Device must be Bluetooth peripheral or central.
#endif

static u8_t bt_stack_id_lut[BT_STACK_ID_LUT_SIZE];
static bool bt_stack_id_lut_valid;

#define TEMP_PEER_ID (CONFIG_BT_MAX_PAIRED - 1)
#if CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE
#define DONGLE_PEER_ID (TEMP_PEER_ID - 1)
#else
#define DONGLE_PEER_ID (TEMP_PEER_ID)
#endif


static struct k_delayed_work timeout;

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	ssize_t rc;

	if (!strcmp(key, PEER_ID_STORAGE_NAME)) {
		/* Ignore record when size is improper. */
		if (len_rd != sizeof(cur_peer_id)) {
			cur_peer_id_valid = false;
			return 0;
		}

		rc = read_cb(cb_arg, &cur_peer_id, sizeof(cur_peer_id));

		if (rc == sizeof(cur_peer_id)) {
			cur_peer_id_valid = true;
		} else {
			cur_peer_id_valid = false;

			if (rc < 0) {
				LOG_ERR("Settings read-out error");
				return rc;
			}
		}
	} else if (!strcmp(key, BT_ID_LUT_STORAGE_NAME)) {
		/* Ignore record when size is improper. */
		if (len_rd != sizeof(bt_stack_id_lut)) {
			bt_stack_id_lut_valid = false;
			return 0;
		}

		rc = read_cb(cb_arg, &bt_stack_id_lut, sizeof(bt_stack_id_lut));

		if (rc == sizeof(bt_stack_id_lut)) {
			bt_stack_id_lut_valid = true;
		} else {
			bt_stack_id_lut_valid = false;

			if (rc < 0) {
				LOG_ERR("Settings read-out error");
				return rc;
			}
		}
	}

	return 0;
}

#ifdef CONFIG_BT_PERIPHERAL
SETTINGS_STATIC_HANDLER_DEFINE(ble_bond, MODULE_NAME, NULL, settings_set, NULL,
			       NULL);
#endif /* CONFIG_BT_PERIPHERAL */

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

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USE_DEFAULT_ID)) {
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
	BUILD_ASSERT(TEMP_PEER_ID == (CONFIG_BT_MAX_PAIRED - 1));
	BUILD_ASSERT(DONGLE_PEER_ID <= TEMP_PEER_ID);
	if (id >= DONGLE_PEER_ID) {
		__ASSERT_NO_MSG(id == DONGLE_PEER_ID);
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

static int store_peer_id(u8_t peer_id)
{
	if (IS_ENABLED(CONFIG_SETTINGS)) {
		char key[] = MODULE_NAME "/" PEER_ID_STORAGE_NAME;

		int err = settings_save_one(key, &peer_id, sizeof(peer_id));

		if (err) {
			LOG_ERR("Problem storing peer_id: (err %d)", err);
			return err;
		}
	}

	return 0;
}

static void select_confirm(void)
{
	LOG_INF("Select peer");

	enum peer_operation op;

	if (tmp_peer_id != cur_peer_id) {
		cur_peer_id = tmp_peer_id;
		op = PEER_OPERATION_SELECTED;

		int err = store_peer_id(cur_peer_id);

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
			return;
		}
	} else {
		op = PEER_OPERATION_CANCEL;
	}

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = op;
	event->bt_app_id = cur_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

	EVENT_SUBMIT(event);
}

static void scan_request(void)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST)) {
		LOG_WRN("Peer scan request not supported");
		return;
	}

	LOG_INF("Peer scan request");

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SCAN_REQUEST;
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
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_BT_CENTRAL));

	remove_peers(BT_ID_DEFAULT);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_ERASED;
	event->bt_app_id = cur_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

	EVENT_SUBMIT(event);
}

static void erase_adv_confirm(void)
{
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_BT_PERIPHERAL));

	/* Update state to ensure that proper bt_stack_id will be used. */
	if (state == STATE_IDLE) {
		state = STATE_ERASE_PEER;
	}

	int err = bt_id_reset(get_bt_stack_peer_id(cur_peer_id), NULL, NULL);

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

static void select_dongle_peer(void)
{
	state = STATE_DONGLE_CONN;

	LOG_INF("Selected dongle peer");
	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SELECTED;
	event->bt_app_id = DONGLE_PEER_ID;
	event->bt_stack_id = get_bt_stack_peer_id(DONGLE_PEER_ID);

	/* BT stack ID for dongle peer should not be changed. */
	__ASSERT_NO_MSG(event->bt_stack_id == (event->bt_app_id + 1));

	EVENT_SUBMIT(event);
}

static void select_ble_peers(void)
{
	LOG_INF("Selected BLE peers");
	state = STATE_IDLE;

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SELECTED;
	event->bt_app_id = cur_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(cur_peer_id);

	EVENT_SUBMIT(event);
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
					s32_t work_delay;

					if (state == STATE_ERASE_ADV) {
						work_delay = ERASE_ADV_TIMEOUT;
					} else {
						work_delay = CONFIRM_TIMEOUT;
					}

					k_delayed_work_submit(&timeout,
							      work_delay);
				} else {
					k_delayed_work_cancel(&timeout);
				}

				return;
			}
		}
	}

	if (state != STATE_DONGLE_CONN) {
		cancel_operation();
	}
}

static void timeout_handler(struct k_work *work)
{
	__ASSERT_NO_MSG(state != STATE_DISABLED);

	cancel_operation();
}

static void init_bt_stack_id_lut(void)
{
	if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		for (size_t i = 0; i < ARRAY_SIZE(bt_stack_id_lut); i++) {
			/* BT_ID_DEFAULT cannot be reset. */
			bt_stack_id_lut[i] = i + 1;
		}

		if (IS_ENABLED(CONFIG_DESKTOP_BLE_USE_DEFAULT_ID)) {
			bt_stack_id_lut[0] = BT_ID_DEFAULT;
		}
	}
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
	if (!IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
		ARG_UNUSED(settings_set);
	};

	if (!IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_SELECT)) {
		ARG_UNUSED(tmp_peer_id);
		ARG_UNUSED(select_start);
		ARG_UNUSED(select_next);
		ARG_UNUSED(select_confirm);
	}

	if (!IS_ENABLED(CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST)) {
		ARG_UNUSED(scan_request);
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

static bool storage_data_is_valid(void)
{
	if (!bt_stack_id_lut_valid || !cur_peer_id_valid) {
		return false;
	}

	if (cur_peer_id >= DONGLE_PEER_ID) {
		return false;
	}

	for (size_t i = 0; i < ARRAY_SIZE(bt_stack_id_lut); i++) {
		if (bt_stack_id_lut[i] >= CONFIG_BT_ID_MAX) {
			return false;
		}
	}

	return true;
}

static void storage_data_overwrite(void)
{
	LOG_WRN("No valid data in storage, write default values");

	cur_peer_id = 0;
	init_bt_stack_id_lut();

	int err = store_peer_id(cur_peer_id);

	if (!err) {
		err = store_bt_stack_id_lut();
	}

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	err = bt_unpair(BT_ID_DEFAULT, NULL);

	if (err) {
		LOG_ERR("Cannot unpair for default ID");
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	/* Reset Bluetooth local identities. */
	for (size_t i = 1; i < CONFIG_BT_ID_MAX; i++) {
		err = bt_id_reset(i, NULL, NULL);

		if (err < 0) {
			LOG_ERR("Cannot reset id %u (err %d)", i, err);
			module_set_state(MODULE_STATE_ERROR);
			break;
		}
	}
}

static int init(void)
{
	silence_unused();

	state = STATE_IDLE;

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_CONTROL)) {
		k_delayed_work_init(&timeout, timeout_handler);
	}

	load_identities();

	if (IS_ENABLED(CONFIG_BT_PERIPHERAL) &&
	    !storage_data_is_valid()) {
		storage_data_overwrite();
		bt_stack_id_lut_valid = true;
		cur_peer_id_valid = true;
	}

	if (dongle_peer_selected_on_init) {
		select_dongle_peer();
	} else {
		select_ble_peers();
	}

	return 0;
}

static bool click_event_handler(const struct click_event *event)
{
	if (event->key_id != CONFIG_DESKTOP_BLE_PEER_CONTROL_BUTTON) {
		return false;
	}

	if (likely(state != STATE_DISABLED)) {
		if ((k_uptime_get_32() < ON_START_CLICK_UPTIME_MAX) &&
		    (event->click == CLICK_LONG)) {
			handle_click(ON_START_CLICK(event->click));
		} else {
			handle_click(event->click);
		}
	}

	return false;
}

static void selector_event_handler(const struct selector_event *event)
{
	if (event->selector_id != CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_ID) {
		return;
	}

	if (event->position == CONFIG_DESKTOP_BLE_DONGLE_PEER_SELECTOR_POS) {
		switch (state) {
		case STATE_DISABLED:
			dongle_peer_selected_on_init = true;
			break;
		case STATE_DONGLE_CONN:
			/* Ignore */
			break;
		case STATE_ERASE_PEER:
		case STATE_ERASE_ADV:
		case STATE_SELECT_PEER:
			cancel_operation();
			/* Fall-through */
		case STATE_IDLE:
			select_dongle_peer();
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
	} else {
		switch (state) {
		case STATE_DISABLED:
			dongle_peer_selected_on_init = false;
			break;
		case STATE_IDLE:
			/* Ignore */
			break;
		case STATE_ERASE_PEER:
		case STATE_ERASE_ADV:
		case STATE_SELECT_PEER:
			cancel_operation();
			/* Fall-through */
		case STATE_DONGLE_CONN:
			select_ble_peers();
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
	}
}

static bool is_my_config_id(u8_t config_id)
{
	return (GROUP_FIELD_GET(config_id) == EVENT_GROUP_SETUP) &&
	       (MOD_FIELD_GET(config_id) == SETUP_MODULE_BLE_BOND);
}

static void config_event_handler(const struct config_event *event)
{
	if (!is_my_config_id(event->id)) {
		return;
	}

	if (state != STATE_IDLE) {
		LOG_WRN(MODULE_NAME " is busy");
		return;
	}

	switch (OPT_FIELD_GET(event->id)) {
	case BLE_BOND_PEER_SEARCH:
		LOG_INF("Remote scan request");
		scan_request();
		break;

	case BLE_BOND_PEER_ERASE:
		LOG_INF("Remote peer erase request");
		if (IS_ENABLED(CONFIG_BT_PERIPHERAL)) {
			erase_adv_confirm();
			state = STATE_ERASE_ADV;
		} else if (IS_ENABLED(CONFIG_BT_CENTRAL)) {
			erase_confirm();
		}
		break;

	default:
		LOG_WRN("Unsupported config event 0x%" PRIx8, event->id);
		break;
	};
}

static bool event_handler(const struct event_header *eh)
{
	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(settings_load),
				MODULE_STATE_READY)) {
			__ASSERT_NO_MSG(state == STATE_DISABLED);

			if (!init()) {
				module_set_state(MODULE_STATE_READY);
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
	    is_click_event(eh)) {
		return click_event_handler(cast_click_event(eh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE) &&
	    is_selector_event(eh)) {
		selector_event_handler(cast_selector_event(eh));
		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	    is_config_event(eh)) {
		config_event_handler(cast_config_event(eh));
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
#if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
#if CONFIG_DESKTOP_BLE_PEER_CONTROL
EVENT_SUBSCRIBE(MODULE, click_event);
#endif
#if CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE
EVENT_SUBSCRIBE(MODULE, selector_event);
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
