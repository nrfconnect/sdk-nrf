/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: BSD-5-Clause-Nordic
 */

/**
 * @file hid_state.c
 *
 * @brief Module for managing the HID state.
 */

#include <limits.h>
#include <sys/types.h>

#include <zephyr/types.h>
#include <misc/slist.h>
#include <misc/util.h>

#include "button_event.h"
#include "motion_event.h"
#include "wheel_event.h"
#include "power_event.h"
#include "hid_event.h"
#include "ble_event.h"

#include "hid_keymap.h"

#define MODULE hid_state
#include "module_state_event.h"

#define SYS_LOG_DOMAIN	MODULE_NAME
#define SYS_LOG_LEVEL	CONFIG_DESKTOP_SYS_LOG_HID_STATE_LEVEL
#include <logging/sys_log.h>


/**@brief HID state item. */
struct item {
	u16_t usage_id;		/**< HID usage ID. */
	s16_t value;		/**< HID value. */
};

/**@brief Enqueued HID state item. */
struct item_event {
	sys_snode_t node;	/**< Event queue linked list node. */
	struct item item;	/**< HID state item which has been enqueued. */
	enum target_report tr;	/**< HID target report. */
	u32_t timestamp;	/**< HID event timestamp. */
};

/**@brief Module state. */
enum state {
	HID_STATE_DISCONNECTED,		/**< Not connected. */
	HID_STATE_CONNECTED_IDLE,	/**< Connected, no data exchange. */
	HID_STATE_CONNECTED_BUSY	/**< Connected, report is generated. */
};

/**@brief Structure keeping state for a single target HID report. */
struct items {
	struct item item[CONFIG_DESKTOP_HID_STATE_ITEM_COUNT];
	u8_t item_count;
};

/**@brief HID state structure. */
struct hid_state {
	struct items	items[TARGET_REPORT_COUNT];
	sys_slist_t	eventq;
	u8_t		eventq_len;
	enum state	state;
	s32_t		wheel_acc;
};


static struct hid_state state;

static void report_send(enum target_report target_report);


/**@brief Binary search. Input array must be already sorted.
 *
 * bsearch is also available from newlib libc, but including
 * the library takes around 10K of FLASH.
 */
static void *bsearch(const void *key, const void *base,
			 size_t elem_num, size_t elem_size,
			 int (*compare)(const void *, const void *))
{
	__ASSERT_NO_MSG(base != NULL);
	__ASSERT_NO_MSG(compare != NULL);
	__ASSERT_NO_MSG(elem_num <= SSIZE_MAX);

	if (!elem_num) {
		return NULL;
	}

	ssize_t lower = 0;
	ssize_t upper = elem_num - 1;

	while (upper >= lower) {
		ssize_t m = (lower + upper) / 2;
		int cmp = compare(key, base + (elem_size * m));

		if (cmp == 0) {
			return (void *)(base + (elem_size * m));
		} else if (cmp < 0) {
			upper = m - 1;
		} else {
			lower = m + 1;
		}
	}

	/* key not found */
	return NULL;
}

/**@brief Compare Key ID in HID Keymap entries. */
static int hid_keymap_compare(const void *a, const void *b)
{
	const struct hid_keymap *p_a = a;
	const struct hid_keymap *p_b = b;

	return (p_a->key_id - p_b->key_id);
}

/**@brief Translate Key ID to HID Usage ID and target report. */
static struct hid_keymap *hid_keymap_get(u16_t key_id)
{
	struct hid_keymap key = {
		.key_id = key_id
	};

	struct hid_keymap *map = bsearch(&key,
					 hid_keymap,
					 hid_keymap_size,
					 sizeof(key),
					 hid_keymap_compare);

	return map;
}

/**@brief Compare two usage values. */
static int usage_id_compare(const void *a, const void *b)
{
	const struct item *p_a = a;
	const struct item *p_b = b;

	return (p_a->usage_id - p_b->usage_id);
}

static void eventq_reset(void)
{
	struct item_event *event;
	struct item_event *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&state.eventq, event, tmp, node) {
		sys_slist_remove(&state.eventq, NULL, &event->node);

		k_free(event);
	}

	sys_slist_init(&state.eventq);
	state.eventq_len = 0;
}

/**@brief Check if the event queue is full. */
static bool eventq_full(void)
{
	return (state.eventq_len >= CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE);
}

static sys_snode_t *eventq_region_find(sys_snode_t *first_valid, sys_snode_t *current)
{
	sys_snode_t *maxfound = sys_slist_peek_head(&state.eventq);
	const struct item current_item = (CONTAINER_OF(current, struct item_event, node)->item);

	if (current_item.value > 0) {
		/* Every key down must be paired with key up.
		 * Set hit count to value as we just detected
		 * first key down for this usage.
		 */
		unsigned int hit_count = current_item.value;
		sys_snode_t *j = sys_slist_peek_next(current);

		SYS_SLIST_ITERATE_FROM_NODE(&state.eventq, j) {
			if (j == first_valid) {
				break;
			}

			if (current_item.usage_id ==
			    CONTAINER_OF(j, struct item_event, node)->item.usage_id) {
				hit_count += CONTAINER_OF(j, struct item_event, node)->item.value;

				if (hit_count == 0) {
					/* When the hit count reaches zero, all
					 * events with this usage are paired.
					 */
					break;
				}
			}
		}

		if (j == first_valid) {
			/* Pair not found. */
			return maxfound;
		}

		sys_snode_t *tmp = maxfound;
		SYS_SLIST_ITERATE_FROM_NODE(&state.eventq, tmp) {
			if (!j) {
				break;
			}

			if (tmp == j) {
				/* We can remove events up to this point. */
				maxfound = j;
				break;
			}
		}
	}

	return maxfound;
}

static void eventq_region_purge(sys_snode_t *last_to_purge)
{
	sys_snode_t *tmp;
	sys_snode_t *tmp_safe;
	size_t cnt = 0;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&state.eventq, tmp, tmp_safe) {
		sys_slist_remove(&state.eventq, NULL, tmp);

		k_free(CONTAINER_OF(tmp, struct item_event, node));
		cnt++;

		if (tmp == last_to_purge) {
			break;
		}
	}

	state.eventq_len -= cnt;

	SYS_LOG_WRN("%u stale events removed from the queue!", cnt);
}


/**@brief Remove stale events from the event queue. */
static void eventq_cleanup(u32_t timestamp)
{
	/* Find timed out events. */
	sys_snode_t *first_valid;

	SYS_SLIST_FOR_EACH_NODE(&state.eventq, first_valid) {
		u32_t diff = timestamp - CONTAINER_OF(
			first_valid, struct item_event, node)->timestamp;

		if (diff < CONFIG_DESKTOP_HID_REPORT_EXPIRATION) {
			break;
		}
	}

	/* Remove events but only if key up was generated for each removed
	 * key down.
	 */
	sys_snode_t *i;
	sys_snode_t *tmp_safe;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&state.eventq, i, tmp_safe) {
		if (i == first_valid) {
			break;
		}

		sys_snode_t *maxfound = eventq_region_find(first_valid, i);

		if (i == maxfound) {
			/* All events up to this point have pairs and can
			 * be deleted.
			 */
			eventq_region_purge(maxfound);
		}
	}
}

/**@brief Update value linked with given usage. */
static bool value_set(struct items *items, u16_t usage_id, s16_t report)
{
	const u8_t prev_item_count = items->item_count;

	bool update_needed = false;
	struct item *p_item;

	__ASSERT_NO_MSG(usage_id != 0);

	/* Report equal to zero brings no change. This should never happen. */
	__ASSERT_NO_MSG(report != 0);

	p_item = bsearch(&usage_id,
			 items->item,
			 ARRAY_SIZE(items->item),
			 sizeof(items->item[0]),
			 usage_id_compare);

	if (p_item != NULL) {
		/* Item is present in the array - update its value. */
		p_item->value += report;
		if (p_item->value == 0) {
			__ASSERT_NO_MSG(items->item_count != 0);
			items->item_count -= 1;
			p_item->usage_id = 0;
		}

		update_needed = true;
	} else if (report < 0) {
		/* For items with absolute value, the value is used as
		 * a reference counter and must not fall below zero. This
		 * could happen if a key up event is lost and the state
		 * receives an unpaired key down event.
		 */
	} else if (prev_item_count >= ARRAY_SIZE(items->item)) {
		/* Configuration should allow the HID module to hold data
		 * about the maximum number of simultaneously pressed keys.
		 * Generate a warning if an item cannot be recorded.
		 */
		SYS_LOG_WRN("No place on the list to store HID item!");
	} else {
		/* After sort operation, free slots (zeros) are stored
		 * at the beginning of the array.
		 */
		size_t const idx = ARRAY_SIZE(items->item) - prev_item_count - 1;

		__ASSERT_NO_MSG(items->item[idx].usage_id == 0);

		/* Record this value change. */
		items->item[idx].usage_id = usage_id;
		items->item[idx].value = report;
		items->item_count += 1;

		update_needed = true;
	}

	if (prev_item_count != items->item_count) {
		/* Sort elements on the list. Use simple algorithm
		 * with small footprint.
		 */
		for (size_t k = 0; k < ARRAY_SIZE(items->item); k++) {
			size_t id = k;

			for (size_t l = k + 1; l < ARRAY_SIZE(items->item); l++) {
				if (items->item[l].usage_id < items->item[id].usage_id) {
					id = l;
				}
			}
			if (id != k) {
				struct item tmp = items->item[k];

				items->item[k] = items->item[id];
				items->item[id] = tmp;
			}
		}
	}

	return update_needed;
}

static void send_report_keyboard(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
		struct hid_keyboard_event *event = new_hid_keyboard_event();

		if (!event) {
			SYS_LOG_WRN("Failed to allocate an event");
			return;
		}

		size_t cnt = 0;
		const size_t max = ARRAY_SIZE(state.items[TARGET_REPORT_KEYBOARD].item);

		for (size_t i = 0;
		     (i < max) && (cnt < ARRAY_SIZE(event->keys));
		     i++, cnt++) {
			if (state.items[TARGET_REPORT_KEYBOARD].item[max - i - 1].value) {
				event->keys[cnt] = state.items[TARGET_REPORT_KEYBOARD].item[max - i - 1].usage_id;
			} else {
				break;
			}
		}

		/* Fill the rest of report with zeros. */
		for (; cnt < ARRAY_SIZE(event->keys); cnt++) {
			event->keys[cnt] = 0;
		}

		event->modifier_bm = 0;

		EVENT_SUBMIT(event);
	}
}

static void send_report_mouse_buttons(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		struct hid_mouse_button_event *event =
			new_hid_mouse_button_event();

		if (!event) {
			SYS_LOG_WRN("Failed to allocate an event");
			return;
		}

		event->button_bm = 0;

		/* Traverse pressed keys and build mouse buttons report */
		for (size_t i = 0;
		     i < ARRAY_SIZE(state.items[TARGET_REPORT_MOUSE_BUTTON].item);
		     i++) {
			struct item item =
				state.items[TARGET_REPORT_MOUSE_BUTTON].item[i];

			if (item.value) {
				__ASSERT_NO_MSG(item.usage_id != 0);
				__ASSERT_NO_MSG(item.usage_id <= 8);

				u8_t mask = 1 << (item.usage_id - 1);

				event->button_bm |= mask;
			}
		}

		EVENT_SUBMIT(event);
	}
}

static void send_report_mouse_xy(s16_t dx, s16_t dy)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		struct hid_mouse_xy_event *event = new_hid_mouse_xy_event();

		if (!event) {
			SYS_LOG_WRN("Failed to allocate an event");
			return;
		}

		event->dx = dx;
		event->dy = dy;

		EVENT_SUBMIT(event);

		state.state = HID_STATE_CONNECTED_BUSY;
	}
}

static void send_report_mouse_wheel(s32_t wheel)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		if (wheel == 0) {
			/* There is nothing to send. */
			return;
		}

		struct hid_mouse_wp_event *event = new_hid_mouse_wp_event();

		if (!event) {
			SYS_LOG_WRN("Failed to allocate an event");
			return;
		}

		event->wheel = wheel;
		event->pan   = 0;

		EVENT_SUBMIT(event);

		state.state = HID_STATE_CONNECTED_BUSY;
	}
}

/**@brief Callback used when report was generated. */
static bool report_issued(void)
{
	__ASSERT_NO_MSG(state.state == HID_STATE_CONNECTED_BUSY);
	bool update_needed;

	if (state.wheel_acc != 0) {
		send_report_mouse_wheel(state.wheel_acc);
		state.wheel_acc = 0;
		return true;
	}

	do {
		if (sys_slist_is_empty(&state.eventq)) {
			/* Module is connected but there are no events to
			 * dequeue. Switch to idle state.
			 */
			state.state = HID_STATE_CONNECTED_IDLE;
			break;
		}

		/* There are enqueued events to handle. */
		sys_snode_t *node = sys_slist_get(&state.eventq);

		state.eventq_len--;

		struct item_event *event = CONTAINER_OF(node,
							struct item_event,
							node);

		update_needed = value_set(&(state.items[event->tr]),
					       event->item.usage_id,
					       event->item.value);

		if (update_needed) {
			/* Some item was updated. Report must be issued. */
			report_send(event->tr);
		}

		k_free(event);

		/* No item was changed. Try next event. */
	} while (!update_needed);

	return update_needed;
}

/**@brief Request report sending. */
static void report_send(enum target_report target_report)
{
	switch (target_report) {
	case TARGET_REPORT_KEYBOARD:
		send_report_keyboard();
		break;

	case TARGET_REPORT_MOUSE_BUTTON:
		send_report_mouse_buttons();
		break;

	case TARGET_REPORT_MPLAYER:
		/* TODO: create consumer control report */
		break;

	case TARGET_REPORT_MOUSE_WHEEL:
	case TARGET_REPORT_MOUSE_PAN:
	default:
		/* Unhandled HID report type. */
		__ASSERT_NO_MSG(false);
	}

	state.state = HID_STATE_CONNECTED_BUSY;
}


static void connect(void)
{
	if (!sys_slist_is_empty(&state.eventq)) {
		/* Remove all stale events from the queue. */
		eventq_cleanup(MSEC(_sys_clock_tick_count));
	}

	if (sys_slist_is_empty(&state.eventq)) {
		/* No events left on the queue - connect but stay idle. */
		state.state = HID_STATE_CONNECTED_IDLE;
	} else {
		/* There are some collected events,
		 * start event draining procedure.
		 */
		state.state = HID_STATE_CONNECTED_BUSY;
		report_issued();
	}
}

static void disconnect(void)
{
	/* Check if module is connected. Disconnect request can happen
	 * if Bluetooth connection attempt failed.
	 */
	if (state.state != HID_STATE_DISCONNECTED) {
		/* Disconnection starts a new state session. Queue is cleared
		 * and event collection is started. When a connection happens,
		 * the same queue is used until all collected
		 * events are drained.
		 */
		state.state = HID_STATE_DISCONNECTED;

		/* Clear state and queue. */
		memset(state.items, 0, sizeof(state.items));
		eventq_reset();
	}
}

static void keep_device_active(void)
{
	struct keep_active_event *event = new_keep_active_event();

	if (event) {
		event->module_name = MODULE_NAME;
		EVENT_SUBMIT(event);
	}
}

/**@brief Enqueue event that updates a given usage. */
static void enqueue(enum target_report tr, u16_t usage_id, s16_t report)
{
	eventq_cleanup(MSEC(_sys_clock_tick_count));

	if (eventq_full()) {
		if (state.state == HID_STATE_DISCONNECTED) {
			/* In disconnected state no items are recorded yet.
			 * Try to remove queued items starting from the
			 * oldest one.
			 */
			sys_snode_t *i;

			SYS_SLIST_FOR_EACH_NODE(&state.eventq, i) {
				/* Initial cleanup was done above. Queue will
				 * not contain events with expired timestamp.
				 */
				u32_t timestamp = (CONTAINER_OF(i, struct item_event, node)->timestamp + CONFIG_DESKTOP_HID_REPORT_EXPIRATION);

				eventq_cleanup(timestamp);
				if (!eventq_full()) {
					/* At least one element was removed
					 * from the queue. Do not continue
					 * list traverse, content was modified!
					 */

					break;
				}
			}
		}
		if (eventq_full()) {
			/* To maintain the sanity of HID state, clear
			 * all recorded events and items.
			 */
			SYS_LOG_WRN("Queue is full, all events are dropped!");
			memset(state.items, 0, sizeof(state.items));
			eventq_reset();
		}
	}

	struct item_event *event = k_malloc(sizeof(struct item_event));

	if (!event) {
		SYS_LOG_WRN("Failed to allocate HID event");
		return;
	}

	event->item.usage_id = usage_id;
	event->item.value = report;
	event->tr = tr;
	event->timestamp = MSEC(_sys_clock_tick_count);

	/* Add a new event to the queue. */
	sys_slist_append(&state.eventq, &event->node);

	state.eventq_len++;
}

/**
 * @brief Function for updating the value linked to the HID usage.
 *
 * The function updates the HID state and sends a report if BLE is connected.
 * If a connection was not made yet, information about usage change may be
 * stored in a queue if usage is queueable.
 *
 * @param[in] map	HID keymap containing the usage to update.
 * @param[in] report	Value linked with the usage.
 */
static void update(struct hid_keymap *map, s16_t report)
{
	switch (state.state) {
	case HID_STATE_CONNECTED_IDLE:
		/* Update state and issue report generation event. */
		if (value_set(&state.items[map->target_report],
		    map->usage_id, report)) {
			report_send(map->target_report);
		}
		break;

	case HID_STATE_DISCONNECTED:
		/* Report cannot be sent yet - enqueue this HID event. */
	case HID_STATE_CONNECTED_BUSY:
		/* Sequence is important - enqueue this HID event. */
		enqueue(map->target_report, map->usage_id, report);
		break;

	default:
		__ASSERT_NO_MSG(false);
	}
}

/* @brief Initialize HID state. */
static void init(void)
{
	if (IS_ENABLED(CONFIG_ASSERT)) {
		/* Validate the order of key IDs on the key map array. */
		for (size_t i = 1; i < hid_keymap_size; i++) {
			if (hid_keymap[i - 1].key_id >= hid_keymap[i].key_id) {
				__ASSERT(false, "The hid_keymap array must be "
						"sorted by key_id!");
			}
		}
	}

	eventq_reset();
}

static bool event_handler(const struct event_header *eh)
{
	if (is_motion_event(eh)) {
		struct motion_event *event = cast_motion_event(eh);

		SYS_LOG_INF("motion event");

		/* Do not accumulate mouse motion data */
		if (state.state == HID_STATE_CONNECTED_IDLE) {
			send_report_mouse_xy(event->dx, event->dy);
		} else if (state.state != HID_STATE_DISCONNECTED) {
			SYS_LOG_WRN("Motion sensed while busy");
		}

		keep_device_active();

		return false;
	}

	if (is_ble_interval_event(eh)) {
		SYS_LOG_INF("report issued");

		/* Drain internal queue before sensor read trigger */
		return report_issued();
	}

	if (is_wheel_event(eh)) {
		struct wheel_event *event = cast_wheel_event(eh);

		SYS_LOG_INF("wheel event");

		/* Do not accumulate mouse wheel data */
		if (state.state == HID_STATE_CONNECTED_IDLE) {
			send_report_mouse_wheel(event->wheel);
		} else if (state.state != HID_STATE_DISCONNECTED) {
			SYS_LOG_INF("Wheel sensed while busy");
			state.wheel_acc += event->wheel;
		}

		keep_device_active();

		return false;
	}

	if (is_button_event(eh)) {
		struct button_event *event = cast_button_event(eh);

		SYS_LOG_INF("button event");

		/* Get usage ID and target report from HID Keymap */
		struct hid_keymap *map = hid_keymap_get(event->key_id);
		if (!map || !map->usage_id) {
			SYS_LOG_WRN("No translation found, button ignored.");
			return false;
		}

		/* Key down increases key ref counter, key up decreases it. */
		s16_t report = (event->pressed != false) ? (1) : (-1);
		update(map, report);

		keep_device_active();

		return false;
	}

	if (is_ble_peer_event(eh)) {
		struct ble_peer_event *event = cast_ble_peer_event(eh);

		SYS_LOG_INF("peer %sconnected",
			    (event->connected)?(""):("dis"));

		if (event->state == PEER_SECURED) {
			connect();

			/* TODO: Send enqueued keys and mouse buttons as soon
			 * as the peer subscribes to notifications using CCC
			 *
			 * This needs modification of HIDS service.
			 */
		} else if (event->state == PEER_DISCONNECTED) {
			disconnect();
		} else {
			/* No action */
		}

		return false;
	}

	if (is_module_state_event(eh)) {
		struct module_state_event *event = cast_module_state_event(eh);

		if (check_state(event, "main", "ready")) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			SYS_LOG_INF("Init HID state!");
			init();
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE_EARLY(MODULE, ble_interval_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, button_event);
EVENT_SUBSCRIBE(MODULE, motion_event);
EVENT_SUBSCRIBE(MODULE, wheel_event);
