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
#include "hid_event.h"
#include "ble_event.h"
#include "usb_event.h"

#include "hid_keymap.h"
#include "hid_keymap_def.h"
#include "hid_report_desc.h"

#define MODULE hid_state
#include "module_state_event.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_STATE_LOG_LEVEL);


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
	bool update_needed;
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
};

struct subscriber {
	const void *id;
	bool is_usb;
	struct report_state state[IN_REPORT_COUNT];
};

/**@brief HID state structure. */
struct hid_state {
	struct report_data report_data[IN_REPORT_COUNT];
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
					 ARRAY_SIZE(hid_keymap),
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
	hid_event->timestamp = K_MSEC(z_tick_get());

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

static void sort_by_usage_id(struct item items[], size_t array_size)
{
	for (size_t k = 0; k < array_size; k++) {
		size_t id = k;

		for (size_t l = k + 1; l < array_size; l++) {
			if (items[l].usage_id < items[id].usage_id) {
				id = l;
			}
		}
		if (id != k) {
			struct item tmp = items[k];

			items[k] = items[id];
			items[id] = tmp;
		}
	}
}

static struct subscriber *get_subscriber(const void *subscriber_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (subscriber_id == state.subscriber[i].id) {
			return &state.subscriber[i];
		}
	}
	LOG_WRN("No subscriber %p", subscriber_id);
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
	LOG_WRN("No subscriber of type %s", (is_usb)?("USB"):("BLE"));
	return NULL;
}

static void connect_subscriber(const void *subscriber_id, bool is_usb)
{
	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (!state.subscriber[i].id) {
			state.subscriber[i].id = subscriber_id;
			state.subscriber[i].is_usb = is_usb;
			LOG_INF("Subscriber %p connected", subscriber_id);

			if (!state.selected || is_usb) {
				state.selected = &state.subscriber[i];
				LOG_INF("Active subscriber %p",
					state.selected->id);
			}
			return;
		}
	}
	LOG_WRN("Cannot connect subscriber");
}

static void disconnect_subscriber(const void *subscriber_id)
{
	struct subscriber *s = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (subscriber_id == state.subscriber[i].id) {
			s = &state.subscriber[i];
			break;
		}
	}

	if (s == NULL) {
		LOG_WRN("Cannot disconnect subscriber");
		return;
	}

	if (s == state.selected) {
		enum in_report tr = IN_REPORT_MOUSE;
		struct report_state *rs = &s->state[tr];

		if (rs->state != STATE_DISCONNECTED) {
			/* Clear state if notification was not
			 * disabled before disconnection.
			 */
			struct report_data *rd = &state.report_data[tr];

			LOG_INF("Clear mouse report data");
			memset(&rd->items, 0, sizeof(rd->items));
			eventq_reset(&rd->eventq);
		}
	}

	memset(s, 0, sizeof(*s));

	LOG_INF("Subscriber %p disconnected", subscriber_id);

	if (s == state.selected) {
		state.selected = get_subscriber_by_type(false);

		if (!state.selected) {
			LOG_INF("No active subscriber");
		} else {
			LOG_INF("Active subscriber %p", state.selected->id);
		}
	}
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
		sort_by_usage_id(items->item, ARRAY_SIZE(items->item));
	}

	items->update_needed = items->update_needed || update_needed;

	return update_needed;
}

static void send_report_keyboard(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_KEYBOARD)) {
		struct report_data *rd = &state.report_data[IN_REPORT_KEYBOARD_KEYS];

		const size_t max = ARRAY_SIZE(rd->items.item);

		struct hid_keyboard_event *event = new_hid_keyboard_event();

		event->subscriber = state.selected->id;
		event->modifier_bm = 0;

		size_t cnt = 0;
		for (size_t i = 0;
		     (i < max) && (cnt < ARRAY_SIZE(event->keys));
		     i++) {
			struct item item = rd->items.item[max - i - 1];

			if (item.usage_id) {
				__ASSERT_NO_MSG(item.value > 0);
				if (item.usage_id <= KEYBOARD_REPORT_LAST_KEY) {
					event->keys[cnt] = item.usage_id;
					cnt++;
				} else if ((item.usage_id >= KEYBOARD_REPORT_FIRST_MODIFIER) &&
					   (item.usage_id <= KEYBOARD_REPORT_LAST_MODIFIER)) {
					event->modifier_bm |= BIT(item.usage_id - KEYBOARD_REPORT_FIRST_MODIFIER);
				} else {
					LOG_WRN("Undefined usage 0x%x", item.usage_id);
				}
			} else {
				break;
			}
		}

		/* Fill the rest of report with zeros. */
		for (; cnt < ARRAY_SIZE(event->keys); cnt++) {
			event->keys[cnt] = 0;
		}

		EVENT_SUBMIT(event);

		rd->items.update_needed = false;
	} else {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
	}
}

static void send_report_mouse(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_MOUSE)) {
		struct report_data *rd = &state.report_data[IN_REPORT_MOUSE];

		struct hid_mouse_event *event = new_hid_mouse_event();

		event->subscriber = state.selected->id;

		event->dx        = state.last_dx;
		event->dy        = -state.last_dy;
		event->wheel     = state.wheel_acc / 2;
		event->button_bm = 0;

		state.last_dx   = 0;
		state.last_dy   = 0;
		state.wheel_acc -= event->wheel * 2;

		/* Traverse pressed keys and build mouse buttons report */
		for (size_t i = 0; i < ARRAY_SIZE(rd->items.item); i++) {
			struct item item = rd->items.item[i];

			if (item.usage_id) {
				__ASSERT_NO_MSG(item.usage_id <= 8);
				__ASSERT_NO_MSG(item.value > 0);

				u8_t mask = 1 << (item.usage_id - 1);

				event->button_bm |= mask;
			}
		}

		EVENT_SUBMIT(event);

		rd->items.update_needed = false;
	} else {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
	}
}

static bool consumer_ctrl_process_item(struct item *item, u16_t *bitmask)
{
	if (!item->usage_id) {
		return false;
	}
	__ASSERT_NO_MSG(item->value > 0);

	bool remove_item = true;

	switch (item->usage_id) {
	case 0x00EA:
		*bitmask |= BIT(0);
		remove_item = false;
		break;

	case 0x00E9:
		*bitmask |= BIT(1);
		remove_item = false;
		break;

	case 0x00E2:
		*bitmask |= BIT(2);
		break;

	case 0x00CD:
		*bitmask |= BIT(3);
		break;

	case 0x00B5:
		*bitmask |= BIT(4);
		break;

	case 0x00B6:
		*bitmask |= BIT(5);
		break;

	case 0x0032:
		*bitmask |= BIT(6);
		break;

	case 0x021F:
		*bitmask |= BIT(7);
		break;

	case 0x0192:
		*bitmask |= BIT(8);
		break;

	case 0x018A:
		*bitmask |= BIT(9);
		break;

	case 0x0196:
		*bitmask |= BIT(10);
		break;

	default:
		/* Unknown usage_id. */
		__ASSERT_NO_MSG(false);

	}

	return remove_item;
}

static void send_report_consumer_ctrl(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_CONSUMER_CTRL)) {
		struct report_data *rd =
			&state.report_data[IN_REPORT_CONSUMER_CTRL];
		struct hid_consumer_ctrl_event *event =
			new_hid_consumer_ctrl_event();

		event->subscriber = state.selected->id;
		event->bitmask = 0;

		/* Traverse pressed keys and build consumer control report */
		for (size_t i = 0; i < ARRAY_SIZE(rd->items.item); i++) {
			struct item *item = &rd->items.item[i];

			if (consumer_ctrl_process_item(item, &event->bitmask)) {
				__ASSERT_NO_MSG(rd->items.item_count != 0);
				rd->items.item_count--;
				item->value = 0;
				item->usage_id = 0;
			}
		}

		sort_by_usage_id(rd->items.item,
				 ARRAY_SIZE(rd->items.item));

		EVENT_SUBMIT(event);

		rd->items.update_needed = false;
	}  else {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
	}
}

static bool update_report(enum in_report tr)
{
	struct report_data *rd = &state.report_data[tr];
	bool update_needed = false;

	while (!update_needed && !eventq_is_empty(&rd->eventq)) {
		/* There are enqueued events to handle. */
		struct item_event *event = eventq_get(&rd->eventq);

		__ASSERT_NO_MSG(event);

		update_needed = key_value_set(&rd->items,
					      event->item.usage_id,
					      event->item.value);

		k_free(event);

		/* If no item was changed, try next event. */
	}

	if (tr == IN_REPORT_MOUSE) {
		if ((state.last_dx != 0) ||
		    (state.last_dy != 0) ||
		    (state.wheel_acc != 0)) {
			update_needed = true;
		}
	}

	if (rd->items.update_needed) {
		update_needed = true;
	}

	return update_needed;
}

static void report_send(enum in_report tr, bool check_state,
			bool send_always)
{
	if (!state.selected) {
		LOG_WRN("No active subscriber");
		return;
	}

	struct report_state *rs = &state.selected->state[tr];

	if (!check_state || (rs->state != STATE_DISCONNECTED)) {
		unsigned int pipeline_depth;

		if (state.selected->is_usb) {
			pipeline_depth = 1;
		} else {
			pipeline_depth = 2;
		}

		while ((rs->cnt < pipeline_depth) &&
		       (update_report(tr) || send_always)) {

			switch (tr) {
			case IN_REPORT_KEYBOARD_KEYS:
				send_report_keyboard();
				break;

			case IN_REPORT_MOUSE:
				send_report_mouse();
				break;

			case IN_REPORT_CONSUMER_CTRL:
				send_report_consumer_ctrl();
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
		}

		if (rs->cnt == 0) {
			rs->state = STATE_CONNECTED_IDLE;
		} else {
			rs->state = STATE_CONNECTED_BUSY;
		}
	}
}

static void report_issued(const void *subscriber_id, enum in_report tr,
			  bool error)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	struct report_state *rs = &subscriber->state[tr];

	__ASSERT_NO_MSG(rs->cnt > 0);
	rs->cnt--;

	if (state.selected != subscriber) {
		LOG_INF("Subscriber %p not active", subscriber_id);
		if (rs->cnt == 0) {
			rs->state = STATE_CONNECTED_IDLE;
		}
		return;
	}

	struct report_data *rd = &state.report_data[tr];

	if (error) {
		/* To maintain the sanity of HID state, clear
		 * all recorded events and items.
		 */
		LOG_ERR("Error while sending report");
		memset(&rd->items, 0, sizeof(rd->items));
		eventq_reset(&rd->eventq);
		if (tr == IN_REPORT_MOUSE) {
			state.last_dx = 0;
			state.last_dy = 0;
			state.wheel_acc = 0;
		}

		if (rs->cnt == 0) {
			rs->state = STATE_CONNECTED_IDLE;
		}

		return;
	}

	if (rs->cnt == 0) {
		report_send(tr, false, false);
	}
}

static void connect(const void *subscriber_id, enum in_report tr)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	subscriber->state[tr].state = STATE_CONNECTED_IDLE;

	if (state.selected == subscriber) {
		switch (tr) {
		case IN_REPORT_MOUSE:
			state.last_dx   = 0;
			state.last_dy   = 0;
			state.wheel_acc = 0;
			break;
		case IN_REPORT_KEYBOARD_KEYS:
		case IN_REPORT_CONSUMER_CTRL:
			break;
		default:
			break;
		}

		struct report_data *rd = &state.report_data[tr];

		if (!eventq_is_empty(&rd->eventq)) {
			/* Remove all stale events from the queue. */
			eventq_cleanup(&rd->eventq, K_MSEC(z_tick_get()));
		}

		report_send(tr, false, true);
	}
}

static void disconnect(const void *subscriber_id, enum in_report tr)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	subscriber->state[tr].state = STATE_DISCONNECTED;

	struct report_data *rd = &state.report_data[tr];

	LOG_INF("Clear report data (%d)", tr);
	memset(&rd->items, 0, sizeof(rd->items));
	eventq_reset(&rd->eventq);
}

/**@brief Enqueue event that updates a given usage. */
static void enqueue(enum in_report tr, u16_t usage_id, s16_t value,
		    bool connected)
{
	struct report_data *rd = &state.report_data[tr];

	eventq_cleanup(&rd->eventq, K_MSEC(z_tick_get()));

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
	enum in_report tr = map->in_report;

	if (!state.selected ||
	    (state.selected->state[tr].state == STATE_DISCONNECTED)) {
		/* Report cannot be sent yet - enqueue this HID event. */
		enqueue(map->in_report, map->usage_id, value, false);
	} else {
		/* Update state and issue report generation event. */
		struct report_data *rd = &state.report_data[tr];
		if (key_value_set(&rd->items, map->usage_id, value)) {
			report_send(tr, false, true);
		}
	}
}

static void init(void)
{
	if (IS_ENABLED(CONFIG_ASSERT) &&
	    !IS_ENABLED(CONFIG_DESKTOP_BUTTONS_NONE)) {
		/* Validate the order of key IDs on the key map array. */
		for (size_t i = 1; i < ARRAY_SIZE(hid_keymap); i++) {
			if (hid_keymap[i - 1].key_id >= hid_keymap[i].key_id) {
				__ASSERT(false, "The hid_keymap array must be "
						"sorted by key_id!");
			}
		}
	}
}

static bool handle_motion_event(const struct motion_event *event)
{
	/* Do not accumulate mouse motion data */
	state.last_dx = event->dx;
	state.last_dy = event->dy;

	report_send(IN_REPORT_MOUSE, true, true);

	return false;
}

static bool handle_wheel_event(const struct wheel_event *event)
{
	state.wheel_acc += event->wheel;

	report_send(IN_REPORT_MOUSE, true, true);

	return false;
}

static bool handle_button_event(const struct button_event *event)
{
	/* Get usage ID and target report from HID Keymap */
	struct hid_keymap *map = hid_keymap_get(event->key_id);

	if (!map || !map->usage_id) {
		LOG_WRN("No mapping, button ignored");
	} else {
		/* Keydown increases ref counter, keyup decreases it. */
		s16_t value = (event->pressed != false) ? (1) : (-1);
		update_key(map, value);
	}

	return false;
}

static bool handle_hid_report_sent_event(
		const struct hid_report_sent_event *event)
{
	report_issued(event->subscriber, event->report_type, event->error);

	return false;
}

static bool handle_hid_report_subscription_event(
		const struct hid_report_subscription_event *event)
{
	if (event->enabled) {
		connect(event->subscriber, event->report_type);
	} else {
		disconnect(event->subscriber, event->report_type);
	}

	return false;
}

static bool handle_ble_peer_event(const struct ble_peer_event *event)
{
	switch (event->state) {
	case PEER_STATE_CONNECTED:
		connect_subscriber(event->id, false);
		break;

	case PEER_STATE_DISCONNECTED:
		disconnect_subscriber(event->id);

		/* Refresh state of newly routed subscriber if any. */
		for (size_t tr = 0; tr < IN_REPORT_COUNT; tr++) {
			report_send(tr, true, true);
		}
		break;

	case PEER_STATE_SECURED:
	case PEER_STATE_CONN_FAILED:
		/* Ignore */
		break;

	default:
		__ASSERT_NO_MSG(false);
		break;
	}

	return false;
}

static bool handle_usb_state_event(const struct usb_state_event *event)
{
	switch (event->state) {
	case USB_STATE_POWERED:
		if (!get_subscriber_by_type(true)) {
			connect_subscriber(event->id, true);
		}
		break;

	case USB_STATE_DISCONNECTED:
		disconnect_subscriber(event->id);

		/* Refresh state of newly routed subscriber if any. */
		for (size_t tr = 0; tr < IN_REPORT_COUNT; tr++) {
			report_send(tr, true, true);
		}
		break;

	default:
		/* Ignore */
		break;
	}

	return false;
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(main), MODULE_STATE_READY)) {
		static bool initialized;

		__ASSERT_NO_MSG(!initialized);
		initialized = true;

		LOG_INF("Init HID state!");
		init();
	}

	return false;
}

static bool event_handler(const struct event_header *eh)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_MOTION_NONE) &&
	    is_motion_event(eh)) {
		return handle_motion_event(cast_motion_event(eh));
	}

	if (is_hid_report_sent_event(eh)) {
		return handle_hid_report_sent_event(
				cast_hid_report_sent_event(eh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_WHEEL_ENABLE) &&
	    is_wheel_event(eh)) {
		return handle_wheel_event(cast_wheel_event(eh));
	}

	if (!IS_ENABLED(CONFIG_DESKTOP_BUTTONS_NONE) &&
	    is_button_event(eh)) {
		return handle_button_event(cast_button_event(eh));
	}

	if (is_hid_report_subscription_event(eh)) {
		return handle_hid_report_subscription_event(
				cast_hid_report_subscription_event(eh));
	}

	if (is_ble_peer_event(eh)) {
		return handle_ble_peer_event(cast_ble_peer_event(eh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_USB_ENABLE) &&
	    is_usb_state_event(eh)) {
		return handle_usb_state_event(cast_usb_state_event(eh));
	}

	if (is_module_state_event(eh)) {
		return handle_module_state_event(cast_module_state_event(eh));
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
