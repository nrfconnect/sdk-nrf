/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file hid_state.c
 *
 * @brief Module for managing the HID state.
 */

#include <limits.h>
#include <sys/types.h>

#include <zephyr/types.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/byteorder.h>

#include <caf/events/led_event.h>
#include <caf/events/button_event.h>
#include "motion_event.h"
#include "wheel_event.h"
#include "hid_event.h"

#include CONFIG_DESKTOP_HID_STATE_HID_KEYBOARD_LEDS_DEF_PATH
#include "hid_keymap.h"
#include CONFIG_DESKTOP_HID_STATE_HID_KEYMAP_DEF_PATH
#include "hid_report_desc.h"

#define MODULE hid_state
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_HID_STATE_LOG_LEVEL);


/**@brief Module state. */
enum state {
	STATE_DISCONNECTED,	/**< Not connected. */
	STATE_CONNECTED_IDLE,	/**< Connected, no data exchange. */
	STATE_CONNECTED_BUSY	/**< Connected, report is generated. */
};

#define SUBSCRIBER_COUNT CONFIG_DESKTOP_HID_STATE_SUBSCRIBER_COUNT

#define OUTPUT_REPORT_DATA_MAX_LEN (REPORT_SIZE_KEYBOARD_LEDS)

#define OUTPUT_REPORT_STATE_COUNT (ARRAY_SIZE(output_reports))

#define INPUT_REPORT_DATA_COUNT (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT) +		\
				 IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT) +	\
				 IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT) +	\
				 IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT))

#define INPUT_REPORT_STATE_COUNT (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT) +		\
				  IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT) +	\
				  IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT) +	\
				  IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT) +	\
				  IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE) +		\
				  IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD))

#define ITEM_COUNT MAX(MAX(MOUSE_REPORT_BUTTON_COUNT_MAX,	\
			   KEYBOARD_REPORT_KEY_COUNT_MAX),	\
		       MAX(SYSTEM_CTRL_REPORT_KEY_COUNT_MAX,	\
			   CONSUMER_CTRL_REPORT_KEY_COUNT_MAX))

#define AXIS_COUNT (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT) * MOUSE_REPORT_AXIS_COUNT)

/**@brief HID state item. */
struct item {
	uint16_t usage_id; /**< HID usage ID. */
	int16_t value; /**< HID value. */
};

/**@brief Structure keeping state for a single target HID report. */
struct items {
	uint8_t item_count_max; /**< Maximal numer of items in this set. */
	uint8_t item_count; /**< Current number of items in this set. */
	struct item item[ITEM_COUNT]; /**< Items set. Browse from the end. */
};

/**@brief Enqueued HID state item. */
struct item_event {
	sys_snode_t node; /**< Event queue linked list node. */
	struct item item; /**< HID state item which has been enqueued. */
	uint32_t timestamp; /**< HID event timestamp. */
};

/**@brief Event queue. */
struct eventq {
	sys_slist_t root;
	size_t len;
};

/**@brief Axis data. */
struct axis_data {
	int16_t axis[AXIS_COUNT]; /**< Array of axes. */
	uint8_t axis_count; /**< Number of axes in this array. */
};

struct report_data {
	struct items items;
	struct eventq eventq;
	struct axis_data axes;
	struct report_state *linked_rs;
};

struct report_state {
	enum state state;
	uint8_t cnt;
	uint8_t report_id;
	struct subscriber *subscriber;
	struct report_data *linked_rd;
	bool update_needed;
};

struct output_report_state {
	uint8_t data[OUTPUT_REPORT_DATA_MAX_LEN];
};

struct subscriber {
	const void *id;
	uint8_t priority;
	uint8_t pipeline_size;
	uint8_t report_max;
	uint8_t report_cnt;
	struct output_report_state output_reports[OUTPUT_REPORT_STATE_COUNT];
	struct report_state state[INPUT_REPORT_STATE_COUNT];
};

/**@brief HID state structure. */
struct hid_state {
	struct report_data report_data[INPUT_REPORT_DATA_COUNT];
	struct subscriber subscriber[SUBSCRIBER_COUNT];
};


static const struct report_data empty_rd = {
			.eventq.root = SYS_SLIST_STATIC_INIT(&empty_rd.eventq.root)};

static uint8_t report_data_index[REPORT_ID_COUNT];
static uint8_t report_state_index[REPORT_ID_COUNT];
static struct hid_state state;


static bool report_send(struct report_state *rs,
			struct report_data *rd,
			bool check_state,
			bool send_always);


/**@brief Binary search. Input array must be already sorted.
 *
 * bsearch is also available from newlib libc, but including
 * the library takes around 10K of FLASH.
 */
static void *bsearch(const void *key, const uint8_t *base,
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
static struct hid_keymap *hid_keymap_get(uint16_t key_id)
{
	struct hid_keymap key = {
		.key_id = key_id
	};

	struct hid_keymap *map = bsearch(&key,
					 (uint8_t *)hid_keymap,
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

static void eventq_append(struct eventq *eventq, uint16_t usage_id, int16_t value)
{
	struct item_event *hid_event = k_malloc(sizeof(*hid_event));

	if (!hid_event) {
		LOG_ERR("Failed to allocate HID event");
		/* Should never happen. */
		__ASSERT_NO_MSG(false);
		return;
	}

	hid_event->item.usage_id = usage_id;
	hid_event->item.value = value;
	hid_event->timestamp = k_uptime_get_32();

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


static void eventq_cleanup(struct eventq *eventq, uint32_t timestamp)
{
	/* Find timed out events. */

	sys_snode_t *first_valid;

	SYS_SLIST_FOR_EACH_NODE(&eventq->root, first_valid) {
		uint32_t diff = timestamp - CONTAINER_OF(
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

static void clear_items(struct items *items)
{
	memset(items->item, 0, sizeof(items->item));
	items->item_count = 0;
}

static void clear_axes(struct axis_data *axes)
{
	memset(axes->axis, 0, sizeof(axes->axis));
}

static void clear_report_data(struct report_data *rd)
{
	LOG_INF("Clear report data (%p)", (void *)rd);

	clear_axes(&rd->axes);
	clear_items(&rd->items);
	eventq_reset(&rd->eventq);
}

static struct report_state *get_report_state(struct subscriber *subscriber,
					     uint8_t report_id)
{
	__ASSERT_NO_MSG(subscriber);

	size_t pos = report_state_index[report_id];

	if (pos < ARRAY_SIZE(subscriber->state)) {
		return &subscriber->state[pos];
	}

	return NULL;
}

static struct report_data *get_report_data(uint8_t report_id)
{
	size_t pos = report_data_index[report_id];
	if (pos >= ARRAY_SIZE(state.report_data)) {
		return NULL;
	}

	return &state.report_data[pos];
}

static size_t get_output_report_idx(uint8_t report_id)
{
	for (size_t i = 0; i < ARRAY_SIZE(output_reports); i++) {
		if (output_reports[i] == report_id) {
			return i;
		}
	}

	/* Should not happen. */
	__ASSERT_NO_MSG(false);
	return 0;
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

static struct subscriber *get_linked_subscriber(uint8_t report_id)
{
	struct report_data *rd = get_report_data(report_id);
	if (!rd) {
		LOG_WRN("No report data %d", report_id);
		return NULL;
	}
	struct report_state *rs = rd->linked_rs;

	return rs ? rs->subscriber : NULL;
}

static bool key_value_set(struct items *items, uint16_t usage_id, int16_t value)
{
	const uint8_t prev_item_count = items->item_count;

	bool update_needed = false;
	struct item *p_item;

	__ASSERT_NO_MSG(usage_id != 0);
	__ASSERT_NO_MSG(items->item_count_max > 0);

	/* Report equal to zero brings no change. This should never happen. */
	__ASSERT_NO_MSG(value != 0);

	struct item i = {
		.usage_id = usage_id,
	};

	p_item = bsearch(&i,
			 (uint8_t *)items->item,
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
	} else if (prev_item_count >= items->item_count_max) {
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

	return update_needed;
}

static void send_report_keyboard(struct report_state *rs, struct report_data *rd)
{
	__ASSERT_NO_MSG((IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT) &&
			 (rs->report_id == REPORT_ID_KEYBOARD_KEYS)) ||
			(IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD) &&
			 (rs->report_id == REPORT_ID_BOOT_KEYBOARD)));
	/* Both normal and boot protocol reports use the same formatting. */

	if (!IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
		return;
	}

	/* Keyboard report should contain keys plus one byte for modifier
	 * and one reserved byte.
	 */
	BUILD_ASSERT(REPORT_SIZE_KEYBOARD_KEYS == KEYBOARD_REPORT_KEY_COUNT_MAX + 2,
			 "Incorrect keyboard report size");

	/* Encode report. */

	struct hid_report_event *event = new_hid_report_event(sizeof(rs->report_id)
							+ REPORT_SIZE_KEYBOARD_KEYS);
	event->source = &state;
	event->subscriber = rs->subscriber->id;

	event->dyndata.data[0] = rs->report_id;
	event->dyndata.data[2] = 0; /* Reserved byte */

	uint8_t modifier_bm = 0;
	uint8_t *keys = &event->dyndata.data[3];

	const size_t max = ARRAY_SIZE(rd->items.item);
	size_t cnt = 0;
	for (size_t i = 0; (i < max) && (cnt < KEYBOARD_REPORT_KEY_COUNT_MAX); i++) {
		struct item item = rd->items.item[max - i - 1];

		if (item.usage_id) {
			__ASSERT_NO_MSG(item.value > 0);
			if (item.usage_id <= KEYBOARD_REPORT_LAST_KEY) {
				__ASSERT_NO_MSG(item.usage_id <= UINT8_MAX);
				keys[cnt] = item.usage_id;
				cnt++;
			} else if ((item.usage_id >= KEYBOARD_REPORT_FIRST_MODIFIER) &&
				   (item.usage_id <= KEYBOARD_REPORT_LAST_MODIFIER)) {
				/* Make sure any key bitmask will fit into modifiers. */
				BUILD_ASSERT(KEYBOARD_REPORT_LAST_MODIFIER - KEYBOARD_REPORT_FIRST_MODIFIER < 8);
				modifier_bm |= BIT(item.usage_id - KEYBOARD_REPORT_FIRST_MODIFIER);
			} else {
				LOG_WRN("Undefined usage 0x%x", item.usage_id);
			}
		} else {
			break;
		}
	}

	/* Fill the rest of report with zeros. */
	for (; cnt < KEYBOARD_REPORT_KEY_COUNT_MAX; cnt++) {
		keys[cnt] = 0;
	}

	event->dyndata.data[1] = modifier_bm;

	APP_EVENT_SUBMIT(event);

	rs->update_needed = false;
}

static void send_report_mouse(struct report_state *rs, struct report_data *rd)
{
	__ASSERT_NO_MSG(rs->report_id == REPORT_ID_MOUSE);

	if (!IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
		return;
	}

	/* X/Y axis */
	int16_t dx = MAX(MIN(rd->axes.axis[MOUSE_REPORT_AXIS_X], MOUSE_REPORT_XY_MAX),
		       MOUSE_REPORT_XY_MIN);
	int16_t dy = MAX(MIN(-rd->axes.axis[MOUSE_REPORT_AXIS_Y], MOUSE_REPORT_XY_MAX),
		       MOUSE_REPORT_XY_MIN);
	if (dx) {
		rd->axes.axis[MOUSE_REPORT_AXIS_X] -= dx;
	}
	if (dy) {
		rd->axes.axis[MOUSE_REPORT_AXIS_Y] += dy;
	}

	/* Wheel */
	int16_t wheel = MAX(MIN(rd->axes.axis[MOUSE_REPORT_AXIS_WHEEL] / 2, MOUSE_REPORT_WHEEL_MAX),
			  MOUSE_REPORT_WHEEL_MIN);
	if (wheel) {
		rd->axes.axis[MOUSE_REPORT_AXIS_WHEEL] -= wheel * 2;
	}

	/* Traverse pressed keys and build mouse buttons bitmask */
	uint8_t button_bm = 0;
	for (size_t i = 0; i < ARRAY_SIZE(rd->items.item); i++) {
		struct item item = rd->items.item[i];

		if (item.usage_id) {
			__ASSERT_NO_MSG(item.usage_id <= 8);
			__ASSERT_NO_MSG(item.value > 0);

			uint8_t mask = 1 << (item.usage_id - 1);

			button_bm |= mask;
		}
	}


	/* Encode report. */
	BUILD_ASSERT(REPORT_SIZE_MOUSE == 5, "Invalid report size");

	struct hid_report_event *event = new_hid_report_event(sizeof(rs->report_id)
							+ REPORT_SIZE_MOUSE);

	event->source = &state;
	event->subscriber = rs->subscriber->id;

	/* Convert to little-endian. */
	uint8_t x_buff[sizeof(dx)];
	uint8_t y_buff[sizeof(dy)];
	sys_put_le16(dx, x_buff);
	sys_put_le16(dy, y_buff);


	event->dyndata.data[0] = rs->report_id;
	event->dyndata.data[1] = button_bm;
	event->dyndata.data[2] = wheel;
	event->dyndata.data[3] = x_buff[0];
	event->dyndata.data[4] = (y_buff[0] << 4) | (x_buff[1] & 0x0f);
	event->dyndata.data[5] = (y_buff[1] << 4) | (y_buff[0] >> 4);

	APP_EVENT_SUBMIT(event);

	if ((rd->axes.axis[MOUSE_REPORT_AXIS_X] != 0) ||
	    (rd->axes.axis[MOUSE_REPORT_AXIS_Y] != 0) ||
	    (rd->axes.axis[MOUSE_REPORT_AXIS_WHEEL]  < -1) ||
	    (rd->axes.axis[MOUSE_REPORT_AXIS_WHEEL]  > 1)) {
		/* If there is some axis data to send, request report update. */
		rs->update_needed = true;
	} else {
		rs->update_needed = false;
	}
}

static void send_report_boot_mouse(struct report_state *rs, struct report_data *rd)
{
	__ASSERT_NO_MSG(rs->report_id == REPORT_ID_BOOT_MOUSE);

	if (!IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE)) {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
		return;
	}

	/* X/Y axis */
	int8_t dx = MAX(MIN(rd->axes.axis[MOUSE_REPORT_AXIS_X], INT8_MAX), INT8_MIN);
	int8_t dy = MAX(MIN(-rd->axes.axis[MOUSE_REPORT_AXIS_Y], INT8_MAX), INT8_MIN);
	int16_t wheel = rd->axes.axis[MOUSE_REPORT_AXIS_WHEEL];

	if (dx) {
		rd->axes.axis[MOUSE_REPORT_AXIS_X] -= dx;
	}
	if (dy) {
		rd->axes.axis[MOUSE_REPORT_AXIS_Y] += dy;
	}
	if (wheel) {
		rd->axes.axis[MOUSE_REPORT_AXIS_WHEEL] = 0;
	}
	/* Traverse pressed keys and build mouse buttons bitmask */
	uint8_t button_bm = 0;
	for (size_t i = 0; i < ARRAY_SIZE(rd->items.item); i++) {
		struct item item = rd->items.item[i];

		if (item.usage_id) {
			__ASSERT_NO_MSG(item.usage_id <= 8);
			__ASSERT_NO_MSG(item.value > 0);

			uint8_t mask = 1 << (item.usage_id - 1);

			button_bm |= mask;
		}
	}


	size_t report_size = sizeof(rs->report_id) + sizeof(dx) + sizeof(dy) +
			     sizeof(button_bm);
	struct hid_report_event *event = new_hid_report_event(report_size);

	event->source = &state;
	event->subscriber = rs->subscriber->id;

	event->dyndata.data[0] = rs->report_id;
	event->dyndata.data[1] = button_bm;
	event->dyndata.data[2] = dx;
	event->dyndata.data[3] = dy;

	APP_EVENT_SUBMIT(event);

	if ((rd->axes.axis[MOUSE_REPORT_AXIS_X] != 0) ||
	    (rd->axes.axis[MOUSE_REPORT_AXIS_Y] != 0)) {
		/* If there is some axis data to send, request report update. */
		rs->update_needed = true;
	} else {
		rs->update_needed = false;
	}
}

static void send_report_ctrl(struct report_state *rs, struct report_data *rd)
{
	size_t report_size = sizeof(rs->report_id);

	if ((rs->report_id == REPORT_ID_SYSTEM_CTRL) &&
	    IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT)) {
		report_size += REPORT_SIZE_SYSTEM_CTRL;
	} else if ((rs->report_id == REPORT_ID_CONSUMER_CTRL) &&
		   IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT)) {
		report_size += REPORT_SIZE_CONSUMER_CTRL;
	} else {
		/* Not supported. */
		__ASSERT_NO_MSG(false);
		return;
	}

	struct hid_report_event *event = new_hid_report_event(report_size);

	event->source = &state;
	event->subscriber = rs->subscriber->id;

	/* Only one item can fit in the consumer control report. */
	__ASSERT_NO_MSG(report_size == sizeof(rs->report_id) +
				       sizeof(rd->items.item[0].usage_id));
	event->dyndata.data[0] = rs->report_id;

	const size_t idx = ARRAY_SIZE(rd->items.item) - 1;

	sys_put_le16(rd->items.item[idx].usage_id,
		     &event->dyndata.data[sizeof(rs->report_id)]);

	APP_EVENT_SUBMIT(event);

	rs->update_needed = false;
}

static bool update_report(struct report_data *rd)
{
	bool update_needed = false;

	while (!update_needed && !eventq_is_empty(&rd->eventq)) {
		/* There are enqueued events to handle. */
		struct item_event *event = eventq_get(&rd->eventq);

		__ASSERT_NO_MSG(event);

		update_needed = key_value_set(&rd->items,
					      event->item.usage_id,
					      event->item.value);

		rd->linked_rs->update_needed = rd->linked_rs->update_needed || update_needed;


		k_free(event);

		/* If no item was changed, try next event. */
	}

	return rd->linked_rs->update_needed;
}

static bool report_send(struct report_state *rs,
			struct report_data *rd,
			bool check_state,
			bool send_always)
{
	bool report_sent = false;
	if (!rs && rd) {
		rs = rd->linked_rs;
	}
	if (!rd && rs) {
		rd = rs->linked_rd;
	}
	if (!rs || !rd) {
		LOG_WRN("No linked report sink");
		return report_sent;
	}

	if (!check_state || (rs->state != STATE_DISCONNECTED)) {
		unsigned int pipeline_depth = rs->subscriber->pipeline_size;

		if ((rs->report_id == REPORT_ID_CONSUMER_CTRL) ||
		    (rs->report_id == REPORT_ID_SYSTEM_CTRL))  {
			pipeline_depth = 1;
		}

		while ((rs->cnt < pipeline_depth) &&
		       (rs->subscriber->report_cnt < rs->subscriber->report_max) &&
		       (update_report(rd) || rs->update_needed || send_always)) {

			switch (rs->report_id) {
			case REPORT_ID_KEYBOARD_KEYS:
				send_report_keyboard(rs, rd);
				break;

			case REPORT_ID_MOUSE:
				send_report_mouse(rs, rd);
				break;

			case REPORT_ID_SYSTEM_CTRL:
			case REPORT_ID_CONSUMER_CTRL:
				send_report_ctrl(rs, rd);
				break;

			case REPORT_ID_BOOT_KEYBOARD:
				send_report_keyboard(rs, rd);
				break;

			case REPORT_ID_BOOT_MOUSE:
				send_report_boot_mouse(rs, rd);
				break;

			default:
				/* Unhandled HID report type. */
				__ASSERT_NO_MSG(false);
				break;
			}

			__ASSERT_NO_MSG(rs->cnt < UINT8_MAX);
			rs->cnt++;
			rs->subscriber->report_cnt++;
			report_sent = true;

			/* To make sure report is sampled on every connection
			 * event, add one additional report to the pipeline.
			 */
		}

		if (rs->cnt != 0) {
			rs->state = STATE_CONNECTED_BUSY;
		}
	}

	/* Ensure that report marked as send_always will be sent. */
	if (send_always && !report_sent) {
		rs->update_needed = true;
	}

	return report_sent;
}

static void report_issued(const void *subscriber_id, uint8_t report_id, bool error)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	bool subscriber_unblocked =
		(subscriber->report_cnt == subscriber->report_max);
	subscriber->report_cnt--;

	struct report_state *rs = get_report_state(subscriber, report_id);
	__ASSERT_NO_MSG(rs);

	if (rs->state != STATE_DISCONNECTED) {
		__ASSERT_NO_MSG(rs->cnt > 0);
		rs->cnt--;

		if (rs->cnt == 0) {
			rs->state = STATE_CONNECTED_IDLE;
		}

		if (error &&
		    (rs->linked_rd->linked_rs == rs)) {
			/* To maintain the sanity of HID state, clear
			 * all recorded events and items.
			 */
			LOG_ERR("Error while sending report");

			clear_report_data(rs->linked_rd);

			return;
		}
	}

	/* Pick next report to send. */
	struct report_state *next_rs;

	if (!subscriber_unblocked) {
		next_rs = rs;
	} else {
		/* Subscriber was blocked. Let's see if there are some other
		 * reports waiting to be sent.
		 */
		next_rs = rs + 1;
		if (next_rs == &subscriber->state[ARRAY_SIZE(subscriber->state)]) {
			next_rs = &subscriber->state[0];
		}
	}

	while (true) {
		if ((next_rs->state != STATE_DISCONNECTED) &&
		    (next_rs->cnt == 0) &&
		    ((next_rs->linked_rd->linked_rs == next_rs) ||
		     (next_rs->linked_rd->linked_rs == NULL))) {
			if (report_send(next_rs, NULL, false, false)) {
				break;
			}
		}

		if (next_rs == rs) {
			/* No report has data to be sent. */
			break;
		}

		next_rs = next_rs + 1;
		if (next_rs == &subscriber->state[ARRAY_SIZE(subscriber->state)]) {
			next_rs = &subscriber->state[0];
		}
	}
}

static struct report_data *get_used_rd(uint8_t report_id)
{
	/* Connect with appropriate report data. */
	uint8_t report_data_id = report_id;

	switch (report_id) {
	case REPORT_ID_BOOT_MOUSE:
		report_data_id = REPORT_ID_MOUSE;
		break;
	case REPORT_ID_BOOT_KEYBOARD:
		report_data_id = REPORT_ID_KEYBOARD_KEYS;
		break;
	default:
		/* Use the same report id. */
		break;
	}
	return get_report_data(report_data_id);
}

static struct report_state *find_next_report_state(const struct report_data *rd,
						   struct report_state *former_rs)
{
	struct report_state *rs_result = NULL;
	uint8_t sub_prio_result = 0;

	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (!state.subscriber[i].id) {
			continue;
		}

		struct subscriber *sub = &state.subscriber[i];
		uint8_t sub_prio = sub->priority;

		for (size_t j = 0; j < ARRAY_SIZE(sub->state); j++) {
			struct report_state *rs = &sub->state[j];

			if (((rs->linked_rd == rd) ||
			     ((rs->linked_rd == &empty_rd) &&
			      (get_used_rd(rs->report_id) == rd))) &&
			    (sub_prio_result <= sub_prio) &&
			    (rs != former_rs)) {
				__ASSERT_NO_MSG(sub_prio_result != sub_prio);
				rs_result = rs;
				sub_prio_result = sub_prio;
			}
		}
	}

	return rs_result;
}

static void update_led(uint8_t led_id, const struct led_effect *effect)
{
	if (led_id == LED_UNAVAILABLE) {
		return;
	}

	struct led_event *event = new_led_event();

	event->led_id = led_id;
	event->led_effect = effect;
	APP_EVENT_SUBMIT(event);
}

static void broadcast_keyboard_leds(void)
{
	BUILD_ASSERT(HID_KEYBOARD_LEDS_COUNT <= CHAR_BIT);
	BUILD_ASSERT(REPORT_SIZE_KEYBOARD_LEDS == 1);

	const struct subscriber *sub = get_linked_subscriber(REPORT_ID_KEYBOARD_KEYS);
	size_t idx = get_output_report_idx(REPORT_ID_KEYBOARD_LEDS);

	static uint8_t displayed_leds_state;
	uint8_t new_leds_state = sub ? sub->output_reports[idx].data[0] : 0;
	uint8_t update_needed = (displayed_leds_state ^ new_leds_state);

	for (size_t i = 0; i < ARRAY_SIZE(keyboard_led_map); i++) {
		if (update_needed & BIT(i)) {
			update_led(keyboard_led_map[i],
				((new_leds_state & BIT(i)) ? &keyboard_led_on : &keyboard_led_off));
		}
	}

	displayed_leds_state = new_leds_state;
}

static void update_output_report_state(void)
{
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		broadcast_keyboard_leds();
	}
}

static void connect(const void *subscriber_id, uint8_t report_id)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	struct report_state *rs = get_report_state(subscriber, report_id);
	__ASSERT_NO_MSG(rs);

	rs->subscriber = subscriber;
	rs->state = STATE_CONNECTED_IDLE;
	rs->report_id = report_id;

	struct report_data *rd = get_used_rd(report_id);

	__ASSERT_NO_MSG(rd);

	rs->linked_rd = rd;

	if (rd->linked_rs) {
		__ASSERT_NO_MSG(rd->linked_rs != rs);

		uint8_t cur_sub_prio = rd->linked_rs->subscriber->priority;
		uint8_t new_sub_prio = subscriber->priority;

		if (cur_sub_prio < new_sub_prio) {
			LOG_WRN("Force report data unlink");
			struct report_state *former_rs = rd->linked_rs;
			rd->linked_rs = NULL;
			clear_report_data(rd);

			/* Unlink report data from former report sink.
			 * Instead link an empty report data to it so
			 * host get a clear report releasing any buttons.
			 */
			former_rs->update_needed = true;
			former_rs->linked_rd = (struct report_data *)&empty_rd;
			report_send(former_rs, NULL, true, false);
		}
	}

	if (!rd->linked_rs) {
		rd->linked_rs = rs;

		if (!eventq_is_empty(&rd->eventq)) {
			/* Remove all stale events from the queue. */
			eventq_cleanup(&rd->eventq, k_uptime_get_32());
		}

		clear_axes(&rd->axes);

		report_send(NULL, rd, false, true);
	}

	update_output_report_state();
}

static void disconnect(const void *subscriber_id, uint8_t report_id)
{
	struct subscriber *subscriber = get_subscriber(subscriber_id);

	if (!subscriber) {
		LOG_WRN("No subscriber %p", subscriber_id);
		return;
	}

	struct report_state *rs = get_report_state(subscriber, report_id);
	__ASSERT_NO_MSG(rs);

	rs->subscriber = NULL;
	rs->state = STATE_DISCONNECTED;
	rs->cnt = 0;

	struct report_data *rd = rs->linked_rd;

	rs->linked_rd = NULL;

	if (rd->linked_rs == rs) {
		clear_report_data(rd);
		rd->linked_rs = find_next_report_state(rd, rs);
		__ASSERT_NO_MSG(rd->linked_rs != rs);

		if (rd->linked_rs) {
			rd->linked_rs->linked_rd = rd;
			/* Refresh state of the newly routed subscriber. */
			report_send(NULL, rd, true, true);
		}
	}

	update_output_report_state();
}

static void connect_subscriber(const void *subscriber_id, uint8_t priority,
			       uint8_t pipeline_size, uint8_t report_max)
{
	__ASSERT_NO_MSG(priority > 0);
	__ASSERT_NO_MSG(pipeline_size > 0);
	__ASSERT_NO_MSG(report_max > 0);

	for (size_t i = 0; i < ARRAY_SIZE(state.subscriber); i++) {
		if (!state.subscriber[i].id) {
			state.subscriber[i].id = subscriber_id;
			state.subscriber[i].priority = priority;
			state.subscriber[i].pipeline_size = pipeline_size;
			state.subscriber[i].report_max = report_max;
			state.subscriber[i].report_cnt = 0;
			update_output_report_state();
			LOG_INF("Subscriber %p connected", subscriber_id);
			return;
		}
	}
	LOG_WRN("Cannot connect subscriber");
	/* Should not happen. */
	__ASSERT_NO_MSG(false);
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
		LOG_WRN("Subscriber %p was not connected", subscriber_id);
		return;
	}

	for (size_t i = 0; i < ARRAY_SIZE(s->state); i++) {
		struct report_state *rs = &s->state[i];

		/* Disconnect report data from report state. */
		if (rs->linked_rd) {
			struct report_data *rd = rs->linked_rd;

			__ASSERT_NO_MSG((rs->report_id > 0) &&
					(rs->report_id < REPORT_ID_COUNT));
			if (rd->linked_rs == rs) {
				clear_report_data(rd);
				rs->linked_rd = NULL;

				rd->linked_rs = find_next_report_state(rd, rs);
				__ASSERT_NO_MSG(rd->linked_rs != rs);

				if (rd->linked_rs) {
					rd->linked_rs->linked_rd = rd;
					/* Refresh state of the newly routed subscriber. */
					report_send(NULL, rd, true, true);
				}
			}
		}
	}

	memset(s, 0, sizeof(*s));

	update_output_report_state();

	LOG_INF("Subscriber %p disconnected", subscriber_id);
}

/**@brief Enqueue event that updates a given usage. */
static void enqueue(struct report_data *rd, uint16_t usage_id, int16_t value,
		    bool connected)
{
	eventq_cleanup(&rd->eventq, k_uptime_get_32());

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
				uint32_t timestamp =
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
			clear_report_data(rd);
		}
	}

	eventq_append(&rd->eventq, usage_id, value);
}

/**@brief Function for updating the value linked to the HID usage. */
static void update_key(const struct hid_keymap *map, int16_t value)
{
	uint8_t report_id = map->report_id;

	struct report_data *rd = get_report_data(report_id);
	__ASSERT_NO_MSG(rd != NULL);
	struct report_state *rs = rd->linked_rs;

	bool connected = false;

	if (rs) {
		connected = (rs->state != STATE_DISCONNECTED);
	}

	if (!connected || !eventq_is_empty(&rd->eventq)) {
		/* Report cannot be sent yet - enqueue this HID event. */
		enqueue(rd, map->usage_id, value, connected);
	} else {
		/* Update state and issue report generation event. */
		if (key_value_set(&rd->items, map->usage_id, value)) {
			report_send(NULL, rd, false, true);
		}
	}
}

static void init(void)
{
	if (IS_ENABLED(CONFIG_ASSERT)) {
		/* Validate the order of key IDs on the key map array. */
		for (size_t i = 1; i < ARRAY_SIZE(hid_keymap); i++) {
			__ASSERT(hid_keymap[i - 1].key_id < hid_keymap[i].key_id,
				 "The hid_keymap array must be sorted by key_id!");
		}
		/* Validate if report IDs are correct. */
		for (size_t i = 0; i < ARRAY_SIZE(hid_keymap); i++) {
			__ASSERT((hid_keymap[i].report_id != REPORT_ID_RESERVED) &&
				 (hid_keymap[i].report_id < REPORT_ID_COUNT),
				 "Invalid report ID used in hid_keymap!");
		}
	}

	/* Mark unused report IDs. */
	for (size_t i = 0; i < ARRAY_SIZE(report_data_index); i++) {
		report_data_index[i] = INPUT_REPORT_DATA_COUNT;
	}
	for (size_t i = 0; i < ARRAY_SIZE(report_state_index); i++) {
		report_state_index[i] = INPUT_REPORT_STATE_COUNT;
	}

	size_t data_id = 0;
	size_t state_id = 0;

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		report_data_index[REPORT_ID_MOUSE] = data_id;
		report_state_index[REPORT_ID_MOUSE] = state_id;

		state.report_data[data_id].items.item_count_max = MOUSE_REPORT_BUTTON_COUNT_MAX;
		state.report_data[data_id].axes.axis_count = MOUSE_REPORT_AXIS_COUNT;

		data_id++;
		state_id++;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT)) {
		report_data_index[REPORT_ID_KEYBOARD_KEYS] = data_id;
		report_state_index[REPORT_ID_KEYBOARD_KEYS] = state_id;

		state.report_data[data_id].items.item_count_max = KEYBOARD_REPORT_KEY_COUNT_MAX;

		data_id++;
		state_id++;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_SYSTEM_CTRL_SUPPORT)) {
		report_data_index[REPORT_ID_SYSTEM_CTRL] = data_id;
		report_state_index[REPORT_ID_SYSTEM_CTRL] = state_id;

		state.report_data[data_id].items.item_count_max = SYSTEM_CTRL_REPORT_KEY_COUNT_MAX;

		data_id++;
		state_id++;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_CONSUMER_CTRL_SUPPORT)) {
		report_data_index[REPORT_ID_CONSUMER_CTRL] = data_id;
		report_state_index[REPORT_ID_CONSUMER_CTRL] = state_id;

		state.report_data[data_id].items.item_count_max = CONSUMER_CTRL_REPORT_KEY_COUNT_MAX;

		data_id++;
		state_id++;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_MOUSE)) {
		report_state_index[REPORT_ID_BOOT_MOUSE] = state_id;

		state_id++;
	}
	if (IS_ENABLED(CONFIG_DESKTOP_HID_BOOT_INTERFACE_KEYBOARD)) {
		report_state_index[REPORT_ID_BOOT_KEYBOARD] = state_id;

		state_id++;
	}

	__ASSERT_NO_MSG(data_id == INPUT_REPORT_DATA_COUNT);
	__ASSERT_NO_MSG(state_id == INPUT_REPORT_STATE_COUNT);
}

static bool handle_motion_event(const struct motion_event *event)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		return false;
	}

	struct report_data *rd = get_report_data(REPORT_ID_MOUSE);
	__ASSERT_NO_MSG(rd != NULL);

	rd->axes.axis[MOUSE_REPORT_AXIS_X] += event->dx;
	rd->axes.axis[MOUSE_REPORT_AXIS_Y] += event->dy;

	report_send(NULL, rd, true, true);

	return false;
}

static bool handle_wheel_event(const struct wheel_event *event)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_MOUSE_SUPPORT)) {
		return false;
	}

	struct report_data *rd = get_report_data(REPORT_ID_MOUSE);
	__ASSERT_NO_MSG(rd != NULL);

	rd->axes.axis[MOUSE_REPORT_AXIS_WHEEL] += event->wheel;

	report_send(NULL, rd, true, true);

	return false;
}

static bool handle_button_event(const struct button_event *event)
{
	/* Get usage ID and target report from HID Keymap */
	struct hid_keymap *map = hid_keymap_get(event->key_id);

	if (!map || !map->usage_id) {
		LOG_DBG("No mapping, button ignored");
	} else {
		/* Keydown increases ref counter, keyup decreases it. */
		int16_t value = (event->pressed != false) ? (1) : (-1);
		update_key(map, value);
	}

	return false;
}

static bool handle_hid_report_sent_event(
		const struct hid_report_sent_event *event)
{
	report_issued(event->subscriber, event->report_id, event->error);

	return false;
}

static void handle_keyboard_leds_report(struct subscriber *sub,
					const uint8_t *data, size_t len)
{
	BUILD_ASSERT(REPORT_SIZE_KEYBOARD_LEDS == 1);
	BUILD_ASSERT(REPORT_SIZE_KEYBOARD_LEDS <= OUTPUT_REPORT_DATA_MAX_LEN);

	if (len != REPORT_SIZE_KEYBOARD_LEDS) {
		LOG_WRN("Improper keyboard LED report size");
		return;
	}

	size_t idx = get_output_report_idx(REPORT_ID_KEYBOARD_LEDS);

	memcpy(sub->output_reports[idx].data, data, len);
	broadcast_keyboard_leds();
}

static void handle_hid_output_report(struct subscriber *sub, uint8_t report_id,
				     const uint8_t *data, size_t len)
{
	__ASSERT_NO_MSG(sub);

	switch (report_id) {
	case REPORT_ID_KEYBOARD_LEDS:
		handle_keyboard_leds_report(sub, data, len);
		break;
	default:
		/* Ignore unknown report. */
		break;
	}
}

static bool handle_hid_report_event(const struct hid_report_event *event)
{
	/* Ignore HID input reports. */
	if (event->subscriber) {
		return false;
	}

	__ASSERT_NO_MSG(event->dyndata.size > 0);

	handle_hid_output_report(get_subscriber(event->source), event->dyndata.data[0],
				 &event->dyndata.data[1], event->dyndata.size - 1);

	return false;
}

static bool handle_hid_report_subscriber_event(const struct hid_report_subscriber_event *event)
{
	if (event->connected) {
		connect_subscriber(event->subscriber, event->params.priority,
				   event->params.pipeline_size, event->params.report_max);
	} else {
		disconnect_subscriber(event->subscriber);
	}

	return false;
}

static bool handle_hid_report_subscription_event(
		const struct hid_report_subscription_event *event)
{
	if (event->enabled) {
		connect(event->subscriber, event->report_id);
	} else {
		disconnect(event->subscriber, event->report_id);
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

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (!IS_ENABLED(CONFIG_DESKTOP_MOTION_NONE) &&
	    is_motion_event(aeh)) {
		return handle_motion_event(cast_motion_event(aeh));
	}

	if (is_hid_report_sent_event(aeh)) {
		return handle_hid_report_sent_event(
				cast_hid_report_sent_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_WHEEL_ENABLE) &&
	    is_wheel_event(aeh)) {
		return handle_wheel_event(cast_wheel_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_BUTTON_EVENTS) &&
	    is_button_event(aeh)) {
		return handle_button_event(cast_button_event(aeh));
	}

	if (IS_ENABLED(CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT) &&
	    is_hid_report_event(aeh)) {
		return handle_hid_report_event(cast_hid_report_event(aeh));
	}

	if (is_hid_report_subscriber_event(aeh)) {
		return handle_hid_report_subscriber_event(cast_hid_report_subscriber_event(aeh));
	}

	if (is_hid_report_subscription_event(aeh)) {
		return handle_hid_report_subscription_event(
				cast_hid_report_subscription_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
#ifdef CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT
APP_EVENT_SUBSCRIBE(MODULE, hid_report_event);
#endif /* CONFIG_DESKTOP_HID_REPORT_KEYBOARD_SUPPORT */
APP_EVENT_SUBSCRIBE(MODULE, hid_report_sent_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_subscription_event);
APP_EVENT_SUBSCRIBE(MODULE, hid_report_subscriber_event);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE_FINAL(MODULE, button_event);
APP_EVENT_SUBSCRIBE(MODULE, motion_event);
APP_EVENT_SUBSCRIBE(MODULE, wheel_event);
