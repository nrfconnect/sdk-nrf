/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/shell/shell.h>
#include <zephyr/settings/settings.h>

#define MODULE ble_bond
#include <caf/events/module_state_event.h>
#include <caf/events/click_event.h>
#include <caf/events/ble_common_event.h>
#include "ble_dongle_peer_event.h"
#include "selector_event.h"
#include "config_event.h"
#include <caf/events/power_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_BLE_BOND_LOG_LEVEL);

#define CONFIRM_TIMEOUT			K_SECONDS(10)
#define ERASE_ADV_TIMEOUT		K_SECONDS(30)

#if CONFIG_DESKTOP_BLE_ENABLE_PASSKEY
 #define ERASE_ADV_NEW_CONN_TIMEOUT_MS	30000
#else
 #define ERASE_ADV_NEW_CONN_TIMEOUT_MS	5000
#endif
#define ERASE_ADV_NEW_CONN_TIMEOUT	K_MSEC(ERASE_ADV_NEW_CONN_TIMEOUT_MS)

#define PEER_ID_STORAGE_NAME		"peer_id"
#define BT_ID_LUT_STORAGE_NAME		"bt_lut"

#define ON_START_CLICK(CLICK)		(CLICK + CLICK_COUNT)
#define ON_START_CLICK_UPTIME_MAX	(6 * MSEC_PER_SEC)


enum state {
	STATE_DISABLED,
	STATE_DISABLED_STANDBY,
	STATE_IDLE,
	STATE_STANDBY,
	STATE_ERASE_PEER,
	STATE_ERASE_ADV,
	STATE_SELECT_PEER,
	STATE_DONGLE,
	STATE_DONGLE_STANDBY,
	STATE_DONGLE_ERASE_PEER,
	STATE_DONGLE_ERASE_ADV
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
	 /* State                 Event         Next state               Callback */
#if CONFIG_DESKTOP_BLE_PEER_SELECT
	{STATE_IDLE,              CLICK_SHORT,  STATE_SELECT_PEER,       select_start},
	{STATE_SELECT_PEER,       CLICK_SHORT,  STATE_SELECT_PEER,       select_next},
	{STATE_SELECT_PEER,       CLICK_DOUBLE, STATE_IDLE,              select_confirm},
#endif

#if CONFIG_DESKTOP_BLE_NEW_PEER_SCAN_REQUEST
	{STATE_IDLE,              CLICK_SHORT,  STATE_IDLE,              scan_request},
#endif

#if CONFIG_DESKTOP_BLE_PEER_ERASE
	{STATE_IDLE,              CLICK_LONG,   STATE_ERASE_PEER,        erase_start},
#if CONFIG_DESKTOP_BT_PERIPHERAL
	{STATE_ERASE_PEER,        CLICK_DOUBLE, STATE_ERASE_ADV,         erase_adv_confirm},
#elif CONFIG_DESKTOP_BT_CENTRAL
	{STATE_ERASE_PEER,        CLICK_DOUBLE, STATE_IDLE,              erase_confirm},
#endif

#if CONFIG_DESKTOP_BLE_DONGLE_PEER_ERASE_BOND_BUTTON
	{STATE_DONGLE,            CLICK_LONG,   STATE_DONGLE_ERASE_PEER, erase_start},
	{STATE_DONGLE_ERASE_PEER, CLICK_DOUBLE, STATE_DONGLE_ERASE_ADV,  erase_adv_confirm},
#endif /* CONFIG_DESKTOP_BLE_DONGLE_PEER_ERASE_BOND_BUTTON */
#endif /* CONFIG_DESKTOP_BLE_PEER_ERASE */

#if CONFIG_DESKTOP_BLE_PEER_ERASE_ON_START
	{STATE_IDLE,	ON_START_CLICK(CLICK_LONG), STATE_ERASE_ADV, erase_adv_confirm},
#endif /* CONFIG_DESKTOP_BLE_PEER_ERASE_ON_START */
};

enum ble_bond_opt {
	BLE_BOND_OPT_PEER_ERASE,
	BLE_BOND_OPT_PEER_SEARCH,

	BLE_BOND_OPT_COUNT
};

const static char * const opt_descr[] = {
	[BLE_BOND_OPT_PEER_ERASE] = "peer_erase",
#ifdef CONFIG_DESKTOP_BT_CENTRAL
	[BLE_BOND_OPT_PEER_SEARCH] = "peer_search",
#endif /* CONFIG_DESKTOP_BT_CENTRAL */
};

static enum state state;
static uint8_t cur_ble_peer_id;
static bool cur_peer_id_valid;
static uint8_t tmp_peer_id;
static bool dongle_peer_selected_on_init;
static bool erase_adv_was_extended;


#if CONFIG_DESKTOP_BT_PERIPHERAL
/* nRF Desktop peripheral requires a minimum of three Bluetooth identities:
 * - BT_ID_DEFAULT is not used as it cannot be reset.
 * - one identity is used for bonding with a peer.
 * - one identity is used for the erased advertising operation as a temporary identity.
 * The minimum threshold is increased if the configuration designates an additional identity
 * for the dongle with the CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE Kconfig option.
 */
BUILD_ASSERT(CONFIG_BT_ID_MAX >= (IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE) ? 4 : 3));
#define BT_STACK_ID_LUT_SIZE (CONFIG_BT_ID_MAX - 1)
#elif CONFIG_DESKTOP_BT_CENTRAL
#define BT_STACK_ID_LUT_SIZE 0
#else
#error Device must be Bluetooth peripheral or central.
#endif

/* The Bluetooth Identity look-up table cannot be used in the Bluetooth Central
 * configuration (CONFIG_DESKTOP_BT_CENTRAL), along with the definitions of special
 * peer IDs.
 */
static uint8_t bt_stack_id_lut[BT_STACK_ID_LUT_SIZE];
static bool bt_stack_id_lut_valid;

#define TEMP_PEER_ID (BT_STACK_ID_LUT_SIZE - 1)
#if CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE
#define DONGLE_PEER_ID (TEMP_PEER_ID - 1)
#else
#define DONGLE_PEER_ID (TEMP_PEER_ID)
#endif


static struct k_work_delayable timeout;

static uint8_t get_app_id(void)
{
	if ((state == STATE_DONGLE) || (state == STATE_DONGLE_ERASE_PEER) ||
	    (state == STATE_DONGLE_ERASE_ADV) || (state == STATE_DONGLE_STANDBY)) {
		return DONGLE_PEER_ID;
	} else {
		return cur_ble_peer_id;
	}
}

static int settings_set(const char *key, size_t len_rd,
			settings_read_cb read_cb, void *cb_arg)
{
	ssize_t rc;

	if (!strcmp(key, PEER_ID_STORAGE_NAME)) {
		/* Ignore record when size is improper. */
		if (len_rd != sizeof(cur_ble_peer_id)) {
			cur_peer_id_valid = false;
			return 0;
		}

		rc = read_cb(cb_arg, &cur_ble_peer_id, sizeof(cur_ble_peer_id));

		if (rc == sizeof(cur_ble_peer_id)) {
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

#ifdef CONFIG_DESKTOP_BT_PERIPHERAL
SETTINGS_STATIC_HANDLER_DEFINE(ble_bond, MODULE_NAME, NULL, settings_set, NULL,
			       NULL);
#endif /* CONFIG_DESKTOP_BT_PERIPHERAL */

static uint8_t get_bt_stack_peer_id(uint8_t id)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL)) {
		if ((state == STATE_ERASE_PEER) || (state == STATE_ERASE_ADV) ||
		(state == STATE_DONGLE_ERASE_PEER) || (state == STATE_DONGLE_ERASE_ADV)) {
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
	__ASSERT_NO_MSG((state == STATE_ERASE_ADV) || (state == STATE_DONGLE_ERASE_ADV));

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_USE_DEFAULT_ID)) {
		if ((bt_stack_id_lut[0] == BT_ID_DEFAULT) &&
		    (get_app_id() == 0)) {
			int err = bt_unpair(BT_ID_DEFAULT, NULL);

			if (err) {
				LOG_ERR("Cannot unpair for default id");
				module_set_state(MODULE_STATE_ERROR);
				return;
			}
			bt_stack_id_lut[0] = 1;
		}
	}
	uint8_t temp = bt_stack_id_lut[TEMP_PEER_ID];

	bt_stack_id_lut[TEMP_PEER_ID] = bt_stack_id_lut[get_app_id()];
	bt_stack_id_lut[get_app_id()] = temp;

	int err = store_bt_stack_id_lut();

	if (err) {
		module_set_state(MODULE_STATE_ERROR);
		return;
	}

	err = bt_unpair(bt_stack_id_lut[TEMP_PEER_ID], NULL);
	if (err) {
		LOG_ERR("Cannot unpair for temporary peer Bluetooth identity: %" PRIu8,
			bt_stack_id_lut[TEMP_PEER_ID]);
		module_set_state(MODULE_STATE_ERROR);
	}
}

static int remove_peers(uint8_t identity)
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
	size_t count = CONFIG_BT_ID_MAX;

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
	int err;

	if ((state != STATE_DONGLE) && (state != STATE_DONGLE_STANDBY) &&
	(state != STATE_DONGLE_ERASE_PEER) && (state != STATE_DONGLE_ERASE_ADV)) {
		err = remove_peers(cur_ble_peer_id);
	} else {
		shell_print(shell,
					"Failed to remove peers. Change to other state from dongle state.");
		err = 0;
	}
	return err;
}

static void cancel_operation(void)
{
	LOG_INF("Cancel peer operation");

	/* Cancel cannot fail if executed from another work's context. */
	(void)k_work_cancel_delayable(&timeout);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = ((state == STATE_ERASE_ADV) || (state == STATE_DONGLE_ERASE_ADV)) ?
		(PEER_OPERATION_ERASE_ADV_CANCEL) : (PEER_OPERATION_CANCEL);
	event->bt_app_id = get_app_id();

	/* Updating state to send proper bt_stack_id. */
	if ((state == STATE_DONGLE_ERASE_PEER) || (state == STATE_DONGLE_ERASE_ADV)) {
		state = STATE_DONGLE;
	} else {
		state = STATE_IDLE;
	}
	event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

	APP_EVENT_SUBMIT(event);
}

static uint8_t next_peer_id(uint8_t id)
{
	id++;
	BUILD_ASSERT(TEMP_PEER_ID == (BT_STACK_ID_LUT_SIZE - 1));
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

	tmp_peer_id = next_peer_id(cur_ble_peer_id);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SELECT;
	event->bt_app_id = tmp_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(tmp_peer_id);

	APP_EVENT_SUBMIT(event);
}

static void select_next(void)
{
	LOG_INF("Cycle peers");

	tmp_peer_id = next_peer_id(tmp_peer_id);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SELECT;
	event->bt_app_id = tmp_peer_id;
	event->bt_stack_id = get_bt_stack_peer_id(tmp_peer_id);

	APP_EVENT_SUBMIT(event);
}

static int store_peer_id(uint8_t peer_id)
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

	if (tmp_peer_id != get_app_id()) {
		cur_ble_peer_id = tmp_peer_id;
		op = PEER_OPERATION_SELECTED;

		int err = store_peer_id(get_app_id());

		if (err) {
			module_set_state(MODULE_STATE_ERROR);
			return;
		}
	} else {
		op = PEER_OPERATION_CANCEL;
	}

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = op;
	event->bt_app_id = get_app_id();
	event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

	APP_EVENT_SUBMIT(event);
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
	event->bt_app_id = get_app_id();
	event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

	APP_EVENT_SUBMIT(event);
}

static void erase_start(void)
{
	LOG_INF("Start peer erase");

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_ERASE;
	event->bt_app_id = get_app_id();
	event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

	APP_EVENT_SUBMIT(event);
}

static void erase_confirm(void)
{
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_BT_CENTRAL));

	remove_peers(BT_ID_DEFAULT);

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_ERASED;
	event->bt_app_id = get_app_id();
	event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

	APP_EVENT_SUBMIT(event);
}

static void erase_adv_confirm(void)
{
	__ASSERT_NO_MSG(IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL));

	/* Update state to ensure that proper bt_stack_id will be used. */
	if (state == STATE_IDLE) {
		state = STATE_ERASE_PEER;
	} else if (state == STATE_DONGLE) {
		state = STATE_DONGLE_ERASE_PEER;
	}

	int err = bt_id_reset(get_bt_stack_peer_id(get_app_id()), NULL, NULL);

	if (err < 0) {
		LOG_ERR("Cannot reset id %u (err:%d)",
			get_bt_stack_peer_id(get_app_id()), err);
		module_set_state(MODULE_STATE_ERROR);
		return;
	}
	LOG_INF("Start advertising for peer erase");
	erase_adv_was_extended = false;

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_ERASE_ADV;
	event->bt_app_id = get_app_id();
	event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

	APP_EVENT_SUBMIT(event);
}

static void select_dongle_peer(void)
{
	if (state == STATE_IDLE) {
		state = STATE_DONGLE;
	} else {
		__ASSERT_NO_MSG(state == STATE_STANDBY);
		state = STATE_DONGLE_STANDBY;
	}

	LOG_INF("Selected dongle peer");
	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SELECTED;
	event->bt_app_id = get_app_id();
	event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

	APP_EVENT_SUBMIT(event);
}

static void select_ble_peers(void)
{
	LOG_INF("Selected BLE peers");
	if (state == STATE_DONGLE) {
		state = STATE_IDLE;
	} else if (state == STATE_DONGLE_STANDBY) {
		state = STATE_STANDBY;
	} else {
		__ASSERT_NO_MSG((state == STATE_STANDBY) ||
				(state == STATE_IDLE));
	}

	struct ble_peer_operation_event *event = new_ble_peer_operation_event();

	event->op = PEER_OPERATION_SELECTED;
	event->bt_app_id = get_app_id();
	event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

	APP_EVENT_SUBMIT(event);
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

				if ((state != STATE_IDLE) && (state != STATE_DONGLE)) {
					k_timeout_t work_delay;

					if ((state == STATE_ERASE_ADV) ||
					    (state == STATE_DONGLE_ERASE_ADV)) {
						work_delay = ERASE_ADV_TIMEOUT;
					} else {
						work_delay = CONFIRM_TIMEOUT;
					}

					k_work_reschedule(&timeout,
							      work_delay);
				} else {
					/* Cancel cannot fail if executed from another work's
					 * context.
					 */
					(void)k_work_cancel_delayable(&timeout);
				}

				return;
			}
		}
	}

	if ((state == STATE_ERASE_PEER) ||
	    (state == STATE_ERASE_ADV) ||
	    (state == STATE_DONGLE_ERASE_PEER) ||
	    (state == STATE_DONGLE_ERASE_ADV) ||
	    (state == STATE_SELECT_PEER)) {
		cancel_operation();
	}
}

static void timeout_handler(struct k_work *work)
{
	__ASSERT_NO_MSG((state == STATE_ERASE_PEER) ||
			(state == STATE_ERASE_ADV) ||
			(state == STATE_DONGLE_ERASE_PEER) ||
			(state == STATE_DONGLE_ERASE_ADV) ||
			(state == STATE_SELECT_PEER));

	cancel_operation();
}

static void init_bt_stack_id_lut(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL)) {
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
			LOG_DBG("Identity %zu created", count);
		}
	}
}

static void silence_unused(void)
{
	/* These things will be opt-out by the compiler. */
	if (!IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL)) {
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

	if (cur_ble_peer_id >= DONGLE_PEER_ID) {
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

	cur_ble_peer_id = 0;
	init_bt_stack_id_lut();

	int err = store_peer_id(cur_ble_peer_id);

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

static void broadcast_ble_dongle_id_info(void)
{
	struct ble_dongle_peer_event *event = new_ble_dongle_peer_event();

	event->bt_app_id = DONGLE_PEER_ID;

	APP_EVENT_SUBMIT(event);
}

static int init(void)
{
	silence_unused();

	if (state == STATE_DISABLED) {
		state = STATE_IDLE;
	} else {
		state = STATE_STANDBY;
	}

	if ((IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_CONTROL)) ||
	    (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) &&
	     IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL))) {
		k_work_init_delayable(&timeout, timeout_handler);
	}

	load_identities();

	if (IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL) &&
	    !storage_data_is_valid()) {
		storage_data_overwrite();
		bt_stack_id_lut_valid = true;
		cur_peer_id_valid = true;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL)) {
		/* Ensure that there are no peers bonded to temporary peer Bluetooth identity.
		 * A bonded peer may not be properly removed e.g. in case of spurious power down.
		 */
		int err = bt_unpair(bt_stack_id_lut[TEMP_PEER_ID], NULL);

		if (err) {
			LOG_ERR("Cannot unpair for temporary peer Bluetooth identity: %" PRIu8,
				bt_stack_id_lut[TEMP_PEER_ID]);
			module_set_state(MODULE_STATE_ERROR);
		}
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ID_INFO)) {
		broadcast_ble_dongle_id_info();
	}

	if (dongle_peer_selected_on_init) {
		select_dongle_peer();
	} else {
		select_ble_peers();
	}

	return 0;
}

static bool ble_peer_event_handler(const struct ble_peer_event *event)
{
	if ((state != STATE_ERASE_ADV) && (state != STATE_DONGLE_ERASE_ADV)) {
		return false;
	}

	if (event->state == PEER_STATE_CONNECTED) {
		/* Ensure that connected peer will have time to establish
		 * security.
		 */
		uint32_t remains = k_ticks_to_ms_ceil32(k_work_delayable_remaining_get(&timeout));

		if ((remains < ERASE_ADV_NEW_CONN_TIMEOUT_MS) &&
		    !erase_adv_was_extended) {
			k_work_reschedule(&timeout,
					      ERASE_ADV_NEW_CONN_TIMEOUT);
			erase_adv_was_extended = true;
		}
	} else if (event->state == PEER_STATE_SECURED) {
		/* Ignore secured for previous local id. */
		struct bt_conn_info bt_info;
		int err = bt_conn_get_info(event->id, &bt_info);

		if (err) {
			LOG_ERR("Cannot get conn info");
			module_set_state(MODULE_STATE_ERROR);
			return false;
		}

		if (bt_info.id != get_bt_stack_peer_id(get_app_id())) {
			LOG_INF("Connection for old id - ignored");
			int err = bt_conn_disconnect(event->id,
					BT_HCI_ERR_REMOTE_USER_TERM_CONN);

			if (err && (err != -ENOTCONN)) {
				LOG_ERR("Cannot disconnect peer (err=%d)", err);
				module_set_state(MODULE_STATE_ERROR);
			}

			return false;
		}

		LOG_INF("Erased peer");
		swap_bt_stack_peer_id();

		if (state == STATE_DONGLE_ERASE_ADV) {
			state = STATE_DONGLE;
		} else {
			state = STATE_IDLE;
		}

		struct ble_peer_operation_event *new_event =
			new_ble_peer_operation_event();

		new_event->op = PEER_OPERATION_ERASED;
		new_event->bt_app_id = get_app_id();
		new_event->bt_stack_id = get_bt_stack_peer_id(get_app_id());

		APP_EVENT_SUBMIT(new_event);
		/* Cancel cannot fail if executed from another work's context. */
		(void)k_work_cancel_delayable(&timeout);
	}

	return false;
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
		case STATE_DISABLED_STANDBY:
			dongle_peer_selected_on_init = true;
			break;
		case STATE_DONGLE:
		case STATE_DONGLE_STANDBY:
		case STATE_DONGLE_ERASE_PEER:
		case STATE_DONGLE_ERASE_ADV:
			/* Ignore */
			break;
		case STATE_ERASE_PEER:
		case STATE_ERASE_ADV:
		case STATE_SELECT_PEER:
			cancel_operation();
			/* Fall-through */
		case STATE_IDLE:
		case STATE_STANDBY:
			select_dongle_peer();
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
	} else {
		switch (state) {
		case STATE_DISABLED:
		case STATE_DISABLED_STANDBY:
			dongle_peer_selected_on_init = false;
			break;
		case STATE_IDLE:
		case STATE_STANDBY:
			/* Ignore */
			break;
		case STATE_ERASE_PEER:
		case STATE_ERASE_ADV:
		case STATE_SELECT_PEER:
		case STATE_DONGLE_ERASE_PEER:
		case STATE_DONGLE_ERASE_ADV:
			cancel_operation();
			/* Fall-through */
		case STATE_DONGLE:
		case STATE_DONGLE_STANDBY:
			select_ble_peers();
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
	}
}

static void config_set(const uint8_t opt_id, const uint8_t *data, const size_t size)
{
	ARG_UNUSED(data);
	ARG_UNUSED(size);

	if ((state != STATE_IDLE) && (state != STATE_DONGLE)) {
		LOG_WRN(MODULE_NAME " is busy");
		return;
	}

	switch (opt_id) {
	case BLE_BOND_OPT_PEER_SEARCH:
		if (IS_ENABLED(CONFIG_DESKTOP_BT_CENTRAL)) {
			LOG_INF("Remote scan request");
			scan_request();
		} else {
			LOG_WRN("Peer search not supported");
		}
		break;

	case BLE_BOND_OPT_PEER_ERASE:
		LOG_INF("Remote peer erase request");
		if (IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL)) {
			if ((!IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ERASE_BOND_CONF_CHANNEL)
				&& (state == STATE_DONGLE))) {
				LOG_WRN("Peer erase not supported");
				return;
			}
			erase_adv_confirm();
			if (state == STATE_ERASE_PEER) {
				state = STATE_ERASE_ADV;
			} else if (state == STATE_DONGLE_ERASE_PEER) {
				state = STATE_DONGLE_ERASE_ADV;
			}

			k_work_reschedule(&timeout, ERASE_ADV_TIMEOUT);

		} else if (IS_ENABLED(CONFIG_DESKTOP_BT_CENTRAL)) {
			erase_confirm();
		}
		break;

	default:
		LOG_WRN("Unsupported config event opt id 0x%" PRIx8, opt_id);
		break;
	};
}

static void config_fetch(const uint8_t opt_id, uint8_t *data, size_t *size)
{
	LOG_WRN("Config fetch is not supported");
}

static bool handle_power_down_event(const struct power_down_event *event)
{
	switch (state) {
	case STATE_DISABLED:
		state = STATE_DISABLED_STANDBY;
		break;

	case STATE_ERASE_PEER:
	case STATE_ERASE_ADV:
	case STATE_SELECT_PEER:
		cancel_operation();
		/* Fall-through */

	case STATE_IDLE:
		state = STATE_STANDBY;
		module_set_state(MODULE_STATE_OFF);
		break;

	case STATE_DONGLE_ERASE_PEER:
	case STATE_DONGLE_ERASE_ADV:
		cancel_operation();
		/* Fall-through */

	case STATE_DONGLE:
		state = STATE_DONGLE_STANDBY;
		module_set_state(MODULE_STATE_OFF);
		break;

	case STATE_STANDBY:
		/* Fall-through */
	case STATE_DISABLED_STANDBY:
		/* Fall-through */
	case STATE_DONGLE_STANDBY:
		/* No action. */
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	return false;
}

static bool handle_wake_up_event(const struct wake_up_event *event)
{
	switch (state) {
	case STATE_DISABLED_STANDBY:
		state = STATE_DISABLED;
		break;

	case STATE_STANDBY:
		state = STATE_IDLE;
		module_set_state(MODULE_STATE_READY);
		break;

	case STATE_DONGLE_STANDBY:
		state = STATE_DONGLE;
		module_set_state(MODULE_STATE_READY);
		break;

	case STATE_DISABLED:
		/* Fall-through */
	case STATE_IDLE:
		/* Fall-through */
	case STATE_ERASE_PEER:
		/* Fall-through */
	case STATE_ERASE_ADV:
		/* Fall-through */
	case STATE_SELECT_PEER:
		/* Fall-through */
	case STATE_DONGLE_ERASE_PEER:
		/* Fall-through */
	case STATE_DONGLE_ERASE_ADV:
		/* Fall-through */
	case STATE_DONGLE:
		/* No action. */
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_module_state_event(aeh)) {
		const struct module_state_event *event =
			cast_module_state_event(aeh);

		if (check_state(event, MODULE_ID(settings_loader),
				MODULE_STATE_READY)) {
			__ASSERT_NO_MSG((state == STATE_DISABLED) ||
					(state == STATE_DISABLED_STANDBY));

			int err = init();

			if (!err) {
				module_set_state(MODULE_STATE_READY);
				if (state == STATE_STANDBY) {
					module_set_state(MODULE_STATE_OFF);
				}
			} else {
				module_set_state(MODULE_STATE_ERROR);
			}
		}

		return false;
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL) &&
	    is_ble_peer_event(aeh)) {
		return ble_peer_event_handler(cast_ble_peer_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_CONTROL) &&
	    is_click_event(aeh)) {
		return click_event_handler(cast_click_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE) &&
	    is_selector_event(aeh)) {
		selector_event_handler(cast_selector_event(aeh));
		return false;
	}

	if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER) &&
	    is_power_down_event(aeh)) {
		return handle_power_down_event(cast_power_down_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_POWER_MANAGER) &&
	    is_wake_up_event(aeh)) {
		return handle_wake_up_event(cast_wake_up_event(aeh));
	}

	GEN_CONFIG_EVENT_HANDLERS(STRINGIFY(MODULE), opt_descr, config_set,
				  config_fetch);

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}
APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
#if IS_ENABLED(CONFIG_DESKTOP_BT_PERIPHERAL)
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
#endif
#if IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)
APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
#endif
#if IS_ENABLED(CONFIG_DESKTOP_BLE_PEER_CONTROL)
APP_EVENT_SUBSCRIBE(MODULE, click_event);
#endif
#if IS_ENABLED(CONFIG_DESKTOP_BLE_DONGLE_PEER_ENABLE)
APP_EVENT_SUBSCRIBE(MODULE, selector_event);
#endif
#if IS_ENABLED(CONFIG_CAF_POWER_MANAGER)
APP_EVENT_SUBSCRIBE(MODULE, power_down_event);
APP_EVENT_SUBSCRIBE(MODULE, wake_up_event);
#endif

#if IS_ENABLED(CONFIG_SHELL)
SHELL_STATIC_SUBCMD_SET_CREATE(sub_peers,
	SHELL_CMD_ARG(show, NULL, "Show bonded peers", shell_show_peers, 0, 0),
	SHELL_CMD_ARG(remove, NULL, "Remove bonded devices",
		      shell_remove_peers, 0, 0),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(peers, &sub_peers, "BLE peer commands", NULL);
#endif /* CONFIG_SHELL */
