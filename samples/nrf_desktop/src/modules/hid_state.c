/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
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
#include "usb_event.h"

#include "hid_keymap.h"

#define MODULE hid_state
#include "module_state_event.h"

#include <logging/log.h>
#define LOG_LEVEL CONFIG_DESKTOP_LOG_HID_STATE_LEVEL
LOG_MODULE_REGISTER(MODULE);


/**@brief Module state. */
enum state {
	STATE_DISCONNECTED,	/**< Not connected. */
	STATE_CONNECTED_IDLE,	/**< Connected, no data exchange. */
	STATE_CONNECTED_BUSY	/**< Connected, report is generated. */
};

#define SUBSCRIBER_COUNT 2

/**@brief HID state item. */
struct item {
	u16_t usage_id;		/**< HID usage ID. */
	s16_t value;		/**< HID value. */
};

/**@brief Structure keeping state for a single target HID report. */
struct items {
	struct item item[CONFIG_DESKTOP_HID_STATE_ITEM_COUNT];
	u8_t item_count;
};

/**@brief Enqueued HID state item. */
struct item_event {
	sys_snode_t node;	/**< Event queue linked list node. */
	struct item item;	/**< HID state item which has been enqueued. */
	u32_t timestamp;	/**< HID event timestamp. */
};

/**@brief Event queue. */
struct eventq {
	sys_slist_t root;
	size_t len;
};

/**@brief Report state. */
struct report_data {
	struct items items;
	struct eventq eventq;
};

struct report_state {
	enum state state;
	unsigned int cnt;
	unsigned int err_cnt;
};

struct subscriber {
	const void *id;
	bool is_usb;
	struct report_state state[TARGET_REPORT_COUNT];
};

/**@brief HID state structure. */
struct hid_state {
	struct report_data report_data[TARGET_REPORT_COUNT];
	struct subscriber subscriber[SUBSCRIBER_COUNT];
	struct subscriber *selected;
	s32_t wheel_acc;
	s16_t last_dx;
	s16_t last_dy;
};


static struct hid_state state;


/**@brief Binary search. Input array must be already sorted.
 *
 * bsearch is also available from newlib libc, but including
 * the library takes around 10K of FLASH.
 */
static void *bsearch(const void *key, const u8_t *base,
			 size_t elem_num, size_t elem_size,
			 int (*compare)(const void *, const void *))
{
	__ASSERT_NO_MSG(base);
	__ASSERT_NO_MSG(compare);
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
					 (u8_t *)hid_keymap,
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

static void eventq_reset(struct eventq *eventq)
{
	struct item_event *event;
	struct item_event *tmp;

	SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&eventq->root, event, tmp, node) {
		sys_slist_remove(&eventq->root, NULL, &event->node);

		k_free(event);
	}

	sys_slist_init(&eventq->root);
	eventq->len = 0;
}

static bool eventq_is_full(const struct eventq *eventq)
{
	return (eventq->len >= CONFIG_DESKTOP_HID_EVENT_QUEUE_SIZE);
}


static bool eventq_is_empty(struct eventq *eventq)
{
	bool empty = sys_slist_is_empty(&eventq->root);

	__ASSERT_NO_MSG(!empty == (eventq->len != 0));

	return empty;
}

static struct item_event *eventq_get(struct eventq *eventq)
{
	sys_snode_t *node = sys_slist_get(&eventq->root);

	if (!node) {
		return NULL;
	}

	eventq->len--;

	return CONTAINER_OF(node, struct item_event, node);
}

static void eventq_append(struct eventq *eventq, u16_t usage_id, s16_t value)
{
	struct item_event *hid_event = k_malloc(sizeof(*hid_event));

	if (!hid_event) {
		LOG_WRN("Failed to allocate HID event");
		return;
	}

	hid_event->item.usage_id = usage_id;
	hid_event->item.value = value;
	hid_event->timestamp = MSEC(z_tick_get());

	/* Add a new event to the queue. */
	sys_slist_append(&eventq->root, &hid_event->node);

	eventq->len++;
}

static void eventq_region_purge(struct eventq *eventq,
				sys_snode_t *last_to_purge)
{
	sys_snode_t *tmp;
	sys_snode_t *tmp_safe;
	size_t cnt = 0;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&eventq->root, tmp, tmp_safe) {
		sys_slist_remove(&eventq->root, NULL, tmp);

		k_free(CONTAINER_OF(tmp, struct item_event, node));
		cnt++;

		if (tmp == last_to_purge) {
			break;
		}
	}

	eventq->len -= cnt;

	LOG_WRN("%u stale events removed from the queue!", cnt);
}


static void eventq_cleanup(struct eventq *eventq, u32_t timestamp)
{
	/* Find timed out events. */

	sys_snode_t *first_valid;

	SYS_SLIST_FOR_EACH_NODE(&eventq->root, first_valid) {
		u32_t diff = timestamp - CONTAINER_OF(
			first_valid, struct item_event, node)->timestamp;

		if (diff < CONFIG_DESKTOP_HID_REPORT_EXPIRATION) {
			break;
		}
	}

	/* Remove events but only if key up was generated for each removed
	 * key down.
	 */

	sys_snode_t *maxfound = sys_slist_peek_head(&eventq->root);
	size_t maxfound_pos = 0;

	sys_snode_t *cur;
	size_t cur_pos = 0;

	sys_snode_t *tmp_safe;

	SYS_SLIST_FOR_EACH_NODE_SAFE(&eventq->root, cur, tmp_safe) {
		const struct item cur_item =
			CONTAINER_OF(cur, struct item_event, node)->item;

		if (cur_item.value > 0) {
			/* Every key down must be paired with key up.
			 * Set hit count to value as we just detected
			 * first key down for this usage.
			 */

			unsigned int hit_count = cur_item.value;
			sys_snode_t *j = cur;
			size_t j_pos = cur_pos;

			SYS_SLIST_ITERATE_FROM_NODE(&eventq->root, j) {
				j_pos++;
				if (j == first_valid) {
					break;
				}

				const struct item item =
					CONTAINER_OF(j,
						     struct item_event,
						     node)->item;

				if (cur_item.usage_id == item.usage_id) {
					hit_count += item.value;

					if (hit_count == 0) {
						/* All events with this usage
						 * are paired.
						 */
						break;
					}
				}
			}

			if (j == first_valid) {
				/* Pair not found. */
				break;
			}

			if (j_pos > maxfound_pos) {
				maxfound = j;
				maxfound_pos = j_pos;
			}
		}


		if (cur == first_valid) {
			break;
		}

		if (cur == maxfound) {
			/* All events up to this point have pairs and can
			 * be deleted.
			 */
			eventq_region_purge(eventq, maxfound);
		}

		cur_pos++;
	}
}

static struct subscriber *get_subscriber(const void *subscriber_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (subscriber_id == state.subscriber[i].id) {
			return &state.subscriber[i];
		}
	}
	LOG_WRN("no subscriber %p\n", subscriber_id);
	return NULL;
}

static struct subscriber *get_subscriber_by_type(bool is_usb)
{
	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (state.subscriber[i].id &&
		    (state.subscriber[i].is_usb == is_usb)) {
			return &state.subscriber[i];
		}
	}
	LOG_WRN("no subscriber of type %s\n", (is_usb)?("USB"):("BLE"));
	return NULL;
}

static void connect_subscriber(const void *subscriber_id, bool is_usb)
{
	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (!state.subscriber[i].id) {
			state.subscriber[i].id = subscriber_id;
			state.subscriber[i].is_usb = is_usb;
			LOG_INF("subscriber %p connected", subscriber_id);

			if (!state.selected || is_usb) {
				state.selected = &state.subscriber[i];
				LOG_INF("Active subscriber %p",
					state.selected->id);
			}
			return;
		}
	}
	LOG_WRN("cannot connect subscriber");
}

static void disconnect_subscriber(const void *subscriber_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (subscriber_id == state.subscriber[i].id) {
			bool is_usb = state.subscriber[i].is_usb;

			memset(&state.subscriber[i],
			       0,
			       sizeof(state.subscriber[i]));
			LOG_INF("subscriber %p disconnected", subscriber_id);

			if (&state.subscriber[i] == state.selected) {
				state.selected = NULL;
				if (is_usb) {
					state.selected =
						get_subscriber_by_type(false);
				}

				if (!state.selected) {
					LOG_INF("no active subscriber");
				} else {
					LOG_INF("Active subscriber %p",
						state.selected->id);
				}
			}
			return;
		}
	}
	LOG_WRN("cannot disconnect subscriber");
}

static bool key_value_set(struct items *items, u16_t usage_id, s16_t value)
{
	const u8_t prev_item_count = items->item_count;

	bool update_needed = false;
	struct item *p_item;

	__ASSERT_NO_MSG(usage_id != 0);

	/* Report equal to zero brings no change. This should never happen. */
	__ASSERT_NO_MSG(value != 0);

	p_item = bsearch(&usage_id,
			 (u8_t *)items->item,
			 ARRAY_SIZE(items->item),
			 sizeof(items->item[0]),
			 usage_id_compare);

	if (p_item) {
		/* Item is present in the array - update its value. */
		p_item->value += value;
		if (p_item->value == 0) {
			__ASSERT_NO_MSG(items->item_count != 0);
			items->item_count -= 1;
			p_item->usage_id = 0;
		}

		update_needed = true;
	} else if (value < 0) {
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
		LOG_WRN("No place on the list to store HID item!");
	} else {
		/* After sort operation, free slots (zeros) are stored
		 * at the beginning of the array.
		 */
		size_t const idx = ARRAY_SIZE(items->item) - prev_item_count - 1;

		__ASSERT_NO_MSG(items->item[idx].usage_id == 0);

		/* Record this value change. */
		items->item[idx].usage_id = usage_id;
		items->item[idx].value = value;
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
		struct report_data *rd = &state.report_data[TARGET_REPORT_KEYBOARD];

		const size_t max = ARRAY_SIZE(rd->items.item);

		struct hid_keyboard_event *event = new_hid_keyboard_event();

		event->subscriber = state.selected->id;

		size_t cnt = 0;
		for (size_t i = 0;
		     (i < max) && (cnt < ARRAY_SIZE(event->keys));
		     i++, cnt++) {
			struct item item = rd->items.item[max - i - 1];

			if (item.value) {
				event->keys[cnt] = item.usage_id;
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
	} else {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
	}
}

static void send_report_mouse(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		struct report_data *rd = &state.report_data[TARGET_REPORT_MOUSE];

		struct hid_mouse_event *event = new_hid_mouse_event();

		event->subscriber = state.selected->id;

		event->dx        = state.last_dx;
		event->dy        = state.last_dy;
		event->wheel     = state.wheel_acc;
		event->button_bm = 0;

		state.last_dx   = 0;
		state.last_dy   = 0;
		state.wheel_acc = 0;

		/* Traverse pressed keys and build mouse buttons report */
		for (size_t i = 0; i < ARRAY_SIZE(rd->items.item); i++) {
			struct item item = rd->items.item[i];

			if (item.value) {
				__ASSERT_NO_MSG(item.usage_id != 0);
				__ASSERT_NO_MSG(item.usage_id <= 8);

				u8_t mask = 1 << (item.usage_id - 1);

				event->button_bm |= mask;
			}
		}

		EVENT_SUBMIT(event);
	} else {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
	}
}

static void report_send(enum target_report tr, bool check_state)
{
	if (!state.selected) {
		LOG_WRN("No active subscriber");
		return;
	}

	struct report_state *rs = &state.selected->state[tr];

	if (!check_state || (rs->state == STATE_CONNECTED_IDLE)) {
		do {
			switch (tr) {
			case TARGET_REPORT_KEYBOARD:
				send_report_keyboard();
				break;

			case TARGET_REPORT_MOUSE:
				send_report_mouse();
				break;

			case TARGET_REPORT_MPLAYER:
				/* Not supported. */
				__ASSERT_NO_MSG(false);
				break;

			default:
				/* Unhandled HID report type. */
				__ASSERT_NO_MSG(false);
				break;
			}

			rs->cnt++;

			/* To make sure report is sampled on every connection
			 * event, add one additional report to the pipeline.
			 */
		} while ((rs->cnt == 1) && !state.selected->is_usb);

		rs->state = STATE_CONNECTED_BUSY;
	}
}

static void report_issued(const void *subscriber_id, enum target_report tr,
			  bool error)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("no subscriber %p", subscriber_id);
		return;
	}

	if (state.selected != subscriber) {
		LOG_INF("subscriber %p not active", subscriber_id);
		return;
	}

	struct report_state *rs = &subscriber->state[tr];
	struct report_data *rd = &state.report_data[tr];

	__ASSERT_NO_MSG(rs->cnt > 0);
	rs->cnt--;

	if (error) {
		rs->err_cnt++;
		if (rs->err_cnt > 1) {
			LOG_ERR("Error while sending report");
			if (rs->cnt == 0) {
				rs->state = STATE_CONNECTED_IDLE;
			}
			return;
		}
	} else {
		rs->err_cnt = 0;
	}

	/* If there was an error try sending the state again. */
	bool update_needed = error;

	while (!update_needed) {
		if (eventq_is_empty(&rd->eventq)) {
			/* Module is connected but there are no events to
			 * dequeue. If that was the last report switch to idle
			 * state.
			 */
			if (rs->cnt == 0) {
				rs->state = STATE_CONNECTED_IDLE;
			}
			break;
		}

		/* There are enqueued events to handle. */
		struct item_event *event = eventq_get(&rd->eventq);

		__ASSERT_NO_MSG(event);

		update_needed = key_value_set(&rd->items,
					      event->item.usage_id,
					      event->item.value);

		k_free(event);

		/* If no item was changed, try next event. */
	}

	if (!update_needed && (tr == TARGET_REPORT_MOUSE)) {
		if ((state.last_dx != 0) ||
		    (state.last_dy != 0) ||
		    (state.wheel_acc != 0)) {
			update_needed = true;
		}
	}

	if (update_needed) {
		/* Something was updated. Report must be issued. */
		report_send(tr, false);
	}
}

static void connect(const void *subscriber_id, enum target_report tr)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("no subscriber %p", subscriber_id);
		return;
	}

	subscriber->state[tr].state = STATE_CONNECTED_IDLE;

	if (state.selected == subscriber) {
		switch (tr) {
		case TARGET_REPORT_MOUSE:
			state.last_dx   = 0;
			state.last_dy   = 0;
			state.wheel_acc = 0;
			break;
		case TARGET_REPORT_KEYBOARD:
		case TARGET_REPORT_MPLAYER:
			break;
		default:
			break;
		}

		struct report_data *rd = &state.report_data[tr];

		if (!eventq_is_empty(&rd->eventq)) {
			/* Remove all stale events from the queue. */
			eventq_cleanup(&rd->eventq, MSEC(z_tick_get()));
		}

		report_send(tr, false);
	}
}

static void disconnect(const void *subscriber_id, enum target_report tr)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("no subscriber %p", subscriber_id);
		return;
	}

	subscriber->state[tr].state = STATE_DISCONNECTED;
}

static void keep_device_active(void)
{
	struct keep_active_event *event = new_keep_active_event();
	event->module_name = MODULE_NAME;
	EVENT_SUBMIT(event);
}

/**@brief Enqueue event that updates a given usage. */
static void enqueue(enum target_report tr, u16_t usage_id, s16_t value,
		    bool connected)
{
	struct report_data *rd = &state.report_data[tr];

	eventq_cleanup(&rd->eventq, MSEC(z_tick_get()));

	if (eventq_is_full(&rd->eventq)) {
		if (!connected) {
			/* In disconnected state no items are recorded yet.
			 * Try to remove queued items starting from the
			 * oldest one.
			 */
			sys_snode_t *i;

			SYS_SLIST_FOR_EACH_NODE(&rd->eventq.root, i) {
				/* Initial cleanup was done above. Queue will
				 * not contain events with expired timestamp.
				 */
				u32_t timestamp =
					CONTAINER_OF(i,
						     struct item_event,
						     node)->timestamp +
					CONFIG_DESKTOP_HID_REPORT_EXPIRATION;

				eventq_cleanup(&rd->eventq, timestamp);

				if (!eventq_is_full(&rd->eventq)) {
					/* At least one element was removed
					 * from the queue. Do not continue
					 * list traverse, content was modified!
					 */
					break;
				}
			}
		}

		if (eventq_is_full(&rd->eventq)) {
			/* To maintain the sanity of HID state, clear
			 * all recorded events and items.
			 */
			LOG_WRN("Queue is full, all events are dropped!");
			memset(&rd->items, 0, sizeof(rd->items));
			eventq_reset(&rd->eventq);
		}
	}

	eventq_append(&rd->eventq, usage_id, value);
}

/**@brief Function for updating the value linked to the HID usage. */
static void update_key(const struct hid_keymap *map, s16_t value)
{
	enum target_report tr = map->target_report;

	if (!state.selected ||
	    (state.selected->state[tr].state != STATE_CONNECTED_IDLE)) {
		/* Report cannot be sent yet - enqueue this HID event. */
		enqueue(map->target_report, map->usage_id, value,
			state.selected->state[tr].state != STATE_DISCONNECTED);
	} else {
		/* Update state and issue report generation event. */
		struct report_data *rd = &state.report_data[tr];
		if (key_value_set(&rd->items, map->usage_id, value)) {
			report_send(tr, false);
		}
	}
}

static void init(void)
{
	if (IS_ENABLED(CONFIG_ASSERT) &&
	    !IS_ENABLED(CONFIG_DESKTOP_BUTTONS_NONE)) {
		/* Validate the order of key IDs on the key map array. */
		for (size_t i = 1; i < hid_keymap_size; i++) {
			if (hid_keymap[i - 1].key_id >= hid_keymap[i].key_id) {
				__ASSERT(false, "The hid_keymap array must be "
						"sorted by key_id!");
			}
		}
	}
}

static bool event_handler(const struct event_header *eh)
{
	if (is_motion_event(eh)) {
		const struct motion_event *event = cast_motion_event(eh);

		/* Do not accumulate mouse motion data */
		state.last_dx = event->dx;
		state.last_dy = event->dy;

		report_send(TARGET_REPORT_MOUSE, true);

		keep_device_active();

		return false;
	}

	if (is_hid_report_sent_event(eh)) {
		const struct hid_report_sent_event *event =
			cast_hid_report_sent_event(eh);

		report_issued(event->subscriber, event->report_type,
			      event->error);

		return false;
	}

	if (is_wheel_event(eh)) {
		const struct wheel_event *event = cast_wheel_event(eh);

		state.wheel_acc += event->wheel;

		report_send(TARGET_REPORT_MOUSE, true);

		keep_device_active();

		return false;
	}

	if (!IS_ENABLED(CONFIG_DESKTOP_BUTTONS_NONE)) {
		if (is_button_event(eh)) {
			const struct button_event *event =
				cast_button_event(eh);

			/* Get usage ID and target report from HID Keymap */
			struct hid_keymap *map = hid_keymap_get(event->key_id);
			if (!map || !map->usage_id) {
				LOG_WRN("No mapping, button ignored.");
				return false;
			}

			/* Keydown increases ref counter, keyup decreases it. */
			s16_t value = (event->pressed != false) ? (1) : (-1);
			update_key(map, value);

			keep_device_active();

			return false;
		}
	}

	if (is_hid_report_subscription_event(eh)) {
		const struct hid_report_subscription_event *event =
			cast_hid_report_subscription_event(eh);

		if (event->enabled) {
			connect(event->subscriber, event->report_type);
		} else {
			disconnect(event->subscriber, event->report_type);
		}

		return false;
	}

	if (is_ble_peer_event(eh)) {
		const struct ble_peer_event *event =
			cast_ble_peer_event(eh);

		switch (event->state) {
		case PEER_STATE_CONNECTED:
			connect_subscriber(event->id, false);
			break;
		case PEER_STATE_DISCONNECTED:
			disconnect_subscriber(event->id);
			break;
		case PEER_STATE_SECURED:
			/* Ignore */
			break;
		default:
			__ASSERT_NO_MSG(false);
			break;
		}
		return false;
	}


	if (IS_ENABLED(CONFIG_DESKTOP_USB_ENABLE)) {
		if (is_usb_state_event(eh)) {
			const struct usb_state_event *event =
				cast_usb_state_event(eh);

			switch (event->state) {
			case USB_STATE_POWERED:
				connect_subscriber(event->id, true);
				break;
			case USB_STATE_DISCONNECTED:
				disconnect_subscriber(event->id);
				break;
			default:
				/* Ignore */
				break;
			}
			return false;
		}
	}

	if (is_module_state_event(eh)) {
		const struct module_state_event *event =
			cast_module_state_event(eh);

		if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
			static bool initialized;

			__ASSERT_NO_MSG(!initialized);
			initialized = true;

			LOG_INF("Init HID state!");
			init();
		}
		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

EVENT_LISTENER(MODULE, event_handler);
EVENT_SUBSCRIBE(MODULE, ble_peer_event);
EVENT_SUBSCRIBE(MODULE, usb_state_event);
EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
EVENT_SUBSCRIBE(MODULE, module_state_event);
EVENT_SUBSCRIBE(MODULE, button_event);
EVENT_SUBSCRIBE(MODULE, motion_event);
EVENT_SUBSCRIBE(MODULE, wheel_event);
