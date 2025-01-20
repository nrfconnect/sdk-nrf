/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <zephyr/kernel.h>
#include <zephyr/init.h>

#include <dk_buttons_and_leds.h>

#include "dect_phy_common.h"

#include "dect_app_time.h"
#include "dect_common_settings.h"
#include "dect_phy_common_rx.h"

#include "dect_phy_ctrl.h"

#include "dect_phy_api_scheduler_integration.h"
#include "dect_phy_api_scheduler.h"

/* Internals */

/* Internal events to scheduler thread */
#define DECT_PHY_API_EVENT_SCHEDULER_NEXT_FRAME			1
#define DECT_PHY_API_EVENT_SCHEDULER_OP_COMPLETED		2
#define DECT_PHY_API_EVENT_SCHEDULER_OP_PDC_DATA_RECEIVED	3
#define DECT_PHY_API_EVENT_SCHEDULER_OP_LIST_PURGE		4
#define DECT_PHY_API_EVENT_SCHEDULER_OP_LIST_ITEM_RM_DEALLOC	5
#define DECT_PHY_API_EVENT_SCHEDULER_LED_TX_ON			6
#define DECT_PHY_API_EVENT_SCHEDULER_LED_TX_OFF			7
#define DECT_PHY_API_EVENT_SCHEDULER_OP_SUSPEND			8
#define DECT_PHY_API_EVENT_SCHEDULER_OP_RESUME			9
#define DECT_PHY_API_EVENT_SCHEDULER_OP_RECONFIG_ON		10
#define DECT_PHY_API_EVENT_SCHEDULER_OP_RECONFIG_OFF		11

/* Scheduler states */
enum dect_phy_api_scheduler_state {
	SCHEDULER_STATE_NORMAL = 0,
	SCHEDULER_STATE_SUSPENDED,
};

/* Scheduler data */
static struct dect_phy_api_scheduler_data {
	enum dect_phy_api_scheduler_state state;
} scheduler_data;

/* Scheduler lists */
static sys_dlist_t to_be_sheduled_list;
static sys_dlist_t done_sheduled_list;

K_MUTEX_DEFINE(to_be_sheduled_list_mutex);
K_MSGQ_DEFINE(
	dect_phy_api_scheduler_op_event_msgq, sizeof(struct dect_phy_op_event_msgq_item), 300, 4);

/* Private function prototypes */

static int dect_phy_api_scheduler_raise_event(uint16_t event_id);
static int dect_phy_api_scheduler_raise_event_with_data(uint16_t event_id, void *data,
							size_t data_size);

static void dect_phy_api_scheduler_timer_handler(struct k_timer *timer_id);
#if defined(CONFIG_DK_LIBRARY)
static void dect_phy_api_scheduler_led_off_timer_handler(struct k_timer *timer_id);
#endif

static bool dect_phy_api_scheduler_list_remove_from_tail(void);
static void dect_phy_api_scheduler_list_purge(void);

static struct dect_phy_api_scheduler_list_item *
dect_phy_api_scheduler_done_list_item_add(struct dect_phy_api_scheduler_list_item *new_list_item);

static struct dect_phy_api_scheduler_list_item *
dect_phy_api_scheduler_done_list_item_find_by_phy_handle(uint32_t handle);
static void dect_phy_api_scheduler_done_list_item_remove_n_dealloc_by_item(
	struct dect_phy_api_scheduler_list_item *list_item);
static void dect_phy_api_scheduler_done_list_purge(void);

static void dect_phy_api_scheduler_th_handler(void);
static void dect_phy_api_scheduler_core_tick_th_schedule_next_frame(void);
static int
dect_phy_api_scheduler_core_mdm_phy_op(struct dect_phy_api_scheduler_list_item *list_item,
				       uint64_t op_start_time);

/**************************************************************************************************/

static int dect_phy_api_scheduler_raise_event(uint16_t event_id)
{
	int ret = 0;
	struct dect_phy_op_event_msgq_item event;

	event.id = event_id;
	event.data = NULL;

	ret = k_msgq_put(&dect_phy_api_scheduler_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		return -ENOBUFS;
	}
	return 0;
}

static int dect_phy_api_scheduler_raise_event_with_data(uint16_t event_id, void *data,
							size_t data_size)
{
	int ret = 0;
	struct dect_phy_op_event_msgq_item event;

	event.data = k_malloc(data_size);
	if (event.data == NULL) {
		return -ENOMEM;
	}
	memcpy(event.data, data, data_size);
	event.id = event_id;

	ret = k_msgq_put(&dect_phy_api_scheduler_op_event_msgq, &event, K_NO_WAIT);
	if (ret) {
		k_free(event.data);
		return -ENOBUFS;
	}
	return 0;
}

/**************************************************************************************************/

K_TIMER_DEFINE(scheduler_timer, dect_phy_api_scheduler_timer_handler, NULL);

static void dect_phy_api_scheduler_timer_handler(struct k_timer *timer_id)
{
	dect_phy_api_scheduler_raise_event(DECT_PHY_API_EVENT_SCHEDULER_NEXT_FRAME);
}

/**************************************************************************************************/

#if defined(CONFIG_DK_LIBRARY)
static void dect_phy_api_scheduler_led_off_timer_handler(struct k_timer *timer_id);

K_TIMER_DEFINE(scheduler_led_off_timer, dect_phy_api_scheduler_led_off_timer_handler, NULL);

static void dect_phy_api_scheduler_led_off_timer_handler(struct k_timer *timer_id)
{
	dect_phy_api_scheduler_raise_event(DECT_PHY_API_EVENT_SCHEDULER_LED_TX_OFF);
}
#endif

/**************************************************************************************************/

/* Scheduler list */

struct dect_phy_api_scheduler_list_item *dect_phy_api_scheduler_list_item_create_new_copy(
	struct dect_phy_api_scheduler_list_item *item_to_be_copied, uint64_t new_frame_time,
	bool also_tx_data)
{
	struct dect_phy_api_scheduler_list_item *new_item =
		k_calloc(1, sizeof(struct dect_phy_api_scheduler_list_item));
	uint8_t *payload = NULL;

	if (!new_item) {
		return NULL;
	}

	new_item->sched_config.tx.encoded_payload_pdu = NULL;
	if (also_tx_data && DECT_PHY_API_SCHEDULER_PRIORITY_IS_TX(item_to_be_copied->priority)) {
		payload = k_calloc(DECT_DATA_MAX_LEN, sizeof(uint8_t));
		if (!payload) {
			k_free(new_item);
			return NULL;
		}
	}

	new_item->sched_config.tx.encoded_payload_pdu = payload;
	dect_phy_api_scheduler_list_item_mem_copy(new_item, item_to_be_copied);
	new_item->sched_config.frame_time = new_frame_time;

	return new_item;
}

void dect_phy_api_scheduler_list_item_mem_copy(struct dect_phy_api_scheduler_list_item *target,
					       struct dect_phy_api_scheduler_list_item *source)
{
	uint8_t *temp_ptr = target->sched_config.tx.encoded_payload_pdu;

	memcpy(target, source, sizeof(struct dect_phy_api_scheduler_list_item));
	target->sched_config.tx.encoded_payload_pdu = temp_ptr;
	if (temp_ptr != NULL) {
		memcpy(target->sched_config.tx.encoded_payload_pdu,
		       source->sched_config.tx.encoded_payload_pdu,
		       source->sched_config.tx.encoded_payload_pdu_size);
	} else {
		target->sched_config.tx.encoded_payload_pdu_size = 0;
	}
}

static bool dect_phy_api_scheduler_list_remove_from_tail(void)
{
	sys_dnode_t *node; /* list item */
	struct dect_phy_api_scheduler_list_item *list_item;
	bool return_value = false;

	if (sys_dlist_is_empty(&to_be_sheduled_list)) {
		return_value = false;
		goto exit;
	}

	return_value = true;
	node = sys_dlist_peek_tail(&to_be_sheduled_list);

	list_item = CONTAINER_OF(node, struct dect_phy_api_scheduler_list_item, dnode);
	__ASSERT_NO_MSG(node != NULL && list_item != NULL);
	sys_dlist_remove(node);
	dect_phy_api_scheduler_list_item_dealloc(list_item);

exit:
	return return_value;
}

bool dect_phy_api_scheduler_list_is_empty(void)
{
	return sys_dlist_is_empty(&to_be_sheduled_list);
}

static void dect_phy_api_scheduler_list_purge(void)
{
	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	while (dect_phy_api_scheduler_list_remove_from_tail()) {
	}
	sys_dlist_init(&to_be_sheduled_list);
	__ASSERT_NO_MSG(sys_dlist_is_empty(&to_be_sheduled_list));

	k_mutex_unlock(&to_be_sheduled_list_mutex);
}

void dect_phy_api_scheduler_list_delete_all_items(void)
{
	dect_phy_api_scheduler_raise_event(DECT_PHY_API_EVENT_SCHEDULER_OP_LIST_PURGE);
}

bool dect_phy_api_scheduler_list_item_remove_by_item(struct dect_phy_api_scheduler_list_item *item)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	bool found = false;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		if (iterator == item) {
			found = true;
			break;
		}
	}

	if (!found) {
		k_mutex_unlock(&to_be_sheduled_list_mutex);
		return false;
	}
	sys_dlist_remove(&item->dnode);
	k_mutex_unlock(&to_be_sheduled_list_mutex);

	return true;
}

struct dect_phy_api_scheduler_list_item *
dect_phy_api_scheduler_list_item_remove_by_phy_op_handle(uint32_t handle)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	bool found = false;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle == handle) {
			found = true;
			break;
		}
	}

	if (found) {
		sys_dlist_remove(&iterator->dnode);
	} else {
		iterator = NULL;
	}
	k_mutex_unlock(&to_be_sheduled_list_mutex);
	return iterator;
}

bool dect_phy_api_scheduler_list_item_running_by_phy_op_handle(uint32_t handle)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	bool found = false;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle == handle) {
			found = true;
			break;
		}
	}

	k_mutex_unlock(&to_be_sheduled_list_mutex);
	return found;
}

void dect_phy_api_scheduler_th_list_item_remove_dealloc_by_phy_op_handle(uint32_t handle)
{
	(void)dect_phy_api_scheduler_raise_event_with_data(
		DECT_PHY_API_EVENT_SCHEDULER_OP_LIST_ITEM_RM_DEALLOC, (void *)&handle,
		sizeof(uint32_t));
}

void dect_phy_api_scheduler_list_item_remove_dealloc_by_phy_op_handle(uint32_t handle)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;

	iterator = dect_phy_api_scheduler_list_item_remove_by_phy_op_handle(handle);
	dect_phy_api_scheduler_list_item_dealloc(iterator);
}

void dect_phy_api_scheduler_list_item_remove_dealloc_by_phy_op_handle_range(uint32_t range_start,
									    uint32_t range_end)
{
	for (int i = range_start; i <= range_end; i++) {
		dect_phy_api_scheduler_list_item_remove_dealloc_by_phy_op_handle(i);
	}
}

void dect_phy_api_scheduler_list_item_pdu_payload_update_by_phy_handle(
	uint32_t handle, uint8_t *new_encoded_payload_pdu, uint16_t size)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	bool found = false;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle == handle) {
			found = true;
			break;
		}
	}

	if (found) {
		if (size <= DECT_DATA_MAX_LEN &&
		    iterator->sched_config.tx.encoded_payload_pdu != NULL) {
			memcpy(iterator->sched_config.tx.encoded_payload_pdu,
			       new_encoded_payload_pdu, size);
		} else {
			desh_error("(%s): cannot update pdu: input size too big: %d (max %d)",
				   (__func__), size, DECT_DATA_MAX_LEN);
		}
	} else {
		desh_error("(%s): cannot update pdu: item handle id %d not found", (__func__),
			   handle);
	}
	k_mutex_unlock(&to_be_sheduled_list_mutex);
}

void dect_phy_api_scheduler_list_item_tx_phy_header_update_by_phy_handle(
	uint32_t handle, union nrf_modem_dect_phy_hdr *phy_header,
	dect_phy_header_type_t header_type)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	bool found = false;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle == handle) {
			found = true;
			break;
		}
	}

	if (found) {
		union nrf_modem_dect_phy_hdr tmp_phy_header;

		if (header_type == DECT_PHY_HEADER_TYPE1) {
			memcpy(&iterator->sched_config.tx.phy_header.type_1, &(phy_header->type_1),
			       sizeof(tmp_phy_header.type_1));
		} else {
			__ASSERT_NO_MSG(header_type == DECT_PHY_HEADER_TYPE2);

			memcpy(&iterator->sched_config.tx.phy_header.type_2, &(phy_header->type_2),
			       sizeof(tmp_phy_header.type_2));
		}
	}
	k_mutex_unlock(&to_be_sheduled_list_mutex);
}

void dect_phy_api_scheduler_list_item_beacon_tx_sched_config_update_by_phy_op_handle(
	uint32_t handle, struct dect_phy_api_scheduler_list_item_config *tx_conf)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	bool found = false;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle == handle) {
			found = true;
			break;
		}
	}

	if (found) {
		union nrf_modem_dect_phy_hdr phy_header;

		/* Update only certain items that could be changed from settings */
		iterator->sched_config.channel = tx_conf->channel;

		iterator->sched_config.subslot_used = tx_conf->subslot_used;
		iterator->sched_config.start_slot = tx_conf->start_slot;
		iterator->sched_config.start_subslot = tx_conf->start_subslot;
		iterator->sched_config.interval_mdm_ticks = tx_conf->interval_mdm_ticks;
		iterator->sched_config.interval_count_left = tx_conf->interval_count_left;
		iterator->sched_config.interval_count_left = tx_conf->interval_count_left;

		iterator->sched_config.tx.phy_lbt_period = tx_conf->tx.phy_lbt_period;
		iterator->sched_config.tx.phy_lbt_rssi_threshold_max =
			tx_conf->tx.phy_lbt_rssi_threshold_max;

		/* Beacon: type 1 */
		memcpy(&iterator->sched_config.tx.phy_header.type_1,
		       &(tx_conf->tx.phy_header.type_1), sizeof(phy_header.type_1));
	}
	k_mutex_unlock(&to_be_sheduled_list_mutex);
}

void dect_phy_api_scheduler_list_item_sched_config_frame_time_update_by_phy_op_handle(
	uint32_t handle, int64_t frame_time_diff)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	bool found = false;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle == handle) {
			found = true;
			break;
		}
	}

	if (found) {
		iterator->sched_config.frame_time += (frame_time_diff);
	}
	k_mutex_unlock(&to_be_sheduled_list_mutex);
}

void dect_phy_api_scheduler_list_item_beacon_rx_sched_config_update_by_phy_op_handle_range(
	uint16_t range_start, uint16_t range_end,
	struct dect_phy_api_scheduler_list_item_config *rx_conf)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);
	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle >= range_start &&
		    iterator->phy_op_handle <= range_end) {
			/* Update only certain items that could be changed from settings */
			iterator->sched_config.channel = rx_conf->channel;
			iterator->sched_config.subslot_used = rx_conf->subslot_used;
			iterator->sched_config.start_slot = rx_conf->start_slot;
			iterator->sched_config.start_subslot = rx_conf->start_subslot;
			iterator->sched_config.length_slots = rx_conf->length_slots;
			iterator->sched_config.length_subslots = rx_conf->length_subslots;
			iterator->sched_config.interval_mdm_ticks = rx_conf->interval_mdm_ticks;
			iterator->sched_config.interval_count_left = rx_conf->interval_count_left;

			iterator->sched_config.rx.expected_rssi_level =
				rx_conf->rx.expected_rssi_level;

			iterator->sched_config.rx.filter.short_network_id =
				rx_conf->rx.filter.short_network_id;
			iterator->sched_config.rx.filter.receiver_identity =
				rx_conf->rx.filter.receiver_identity;
			iterator->sched_config.rx.mode = rx_conf->rx.mode;
		}
	}

	k_mutex_unlock(&to_be_sheduled_list_mutex);
}

struct dect_phy_api_scheduler_list_item *
dect_phy_api_scheduler_list_item_add(struct dect_phy_api_scheduler_list_item *new_list_item)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	uint32_t scheduler_offset = dect_phy_ctrl_modem_latency_min_margin_between_ops_get();

	if (new_list_item == NULL) {
		return NULL;
	}
	const uint64_t new_frame_time = new_list_item->sched_config.frame_time;
	const uint64_t next_frame_time = new_frame_time + DECT_RADIO_FRAME_DURATION_IN_MODEM_TICKS;
	const uint64_t new_length_mdm_ticks =
		(new_list_item->sched_config.subslot_used)
			? (new_list_item->sched_config.length_subslots *
			   DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS)
			: (new_list_item->sched_config.length_slots *
			   DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS);
	const uint64_t new_duration = (new_list_item->sched_config.rx.duration > 0)
					      ? new_list_item->sched_config.rx.duration
					      : new_length_mdm_ticks;
	const uint64_t new_op_start_offset = new_list_item->sched_config.subslot_used
						     ? (new_list_item->sched_config.start_subslot *
							DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS)
						     : (new_list_item->sched_config.start_slot *
							DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS);
	const uint64_t new_start_time = new_frame_time + new_op_start_offset;
	const dect_phy_api_scheduling_priority_t entry_prio = new_list_item->priority;
	const uint64_t new_end_time = new_start_time + new_duration + scheduler_offset;

	if (new_list_item->priority != DECT_PRIORITY0_FORCE_TX) {
		/* RX duration can be longer than a frame */
		if (!DECT_PHY_API_SCHEDULER_PRIORITY_IS_RX(new_list_item->priority) &&
		    new_end_time >= next_frame_time) {
			/* Note: this restriction is a DeSH specific.
			 * According to MAC spec:
			 * TX to RACH resource can be over frame boundaries,
			 * they just need to fit into the RACH resource.
			 */
			desh_error("(%s): new list item (handle %d, end_time %llu) cannot fit into "
				   "a frame (next_frame_time %llu)",
				   (__func__), new_list_item->phy_op_handle, new_end_time,
				   next_frame_time);
			return NULL;
		}
	}

	if (k_mutex_lock(&to_be_sheduled_list_mutex, K_SECONDS(1)) == 0) {
	} else {
		desh_warn("warn: to_be_sheduled_list_mutex was locked?!\n");
	}

	bool list_was_empty_at_entry = dect_phy_api_scheduler_list_is_empty();

	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		const uint64_t list_frame_time = iterator->sched_config.frame_time;
		const uint64_t list_op_start_offset =
			iterator->sched_config.subslot_used
				? (iterator->sched_config.start_subslot *
				   DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS)
				: (iterator->sched_config.start_slot *
				   DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS);

		const uint64_t list_start_time = list_frame_time + list_op_start_offset;
		const uint64_t list_length_mdm_ticks =
			(iterator->sched_config.subslot_used)
				? (iterator->sched_config.length_subslots *
				   DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS)
				: (iterator->sched_config.length_slots *
				   DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS);
		const uint64_t list_duration = (iterator->sched_config.rx.duration > 0)
						       ? iterator->sched_config.rx.duration
						       : list_length_mdm_ticks;
		const uint64_t list_end_time = list_frame_time + list_duration + scheduler_offset;

		if (new_frame_time < list_frame_time) {
			/* Not starting in same frame */
			if (list_start_time <= new_end_time && list_end_time >= new_start_time) {
				/* Not starting in same frame but with RX we could have over frame
				 * duration and this seems to be overlapping -> insert new one only
				 * if prio allows and if allows, probably either one id failing in
				 * mdm.
				 */
				if (entry_prio >= iterator->priority) {
					desh_warn("(%s) 1: cannot schedule: new item (handle %d, "
						  "start_time %llu, end_time %llu) is overlapping "
						  "with current one (handle %d, start_time %llu, "
						  "end_time %llu)",
						  (__func__), new_list_item->phy_op_handle,
						  new_start_time, new_end_time,
						  iterator->phy_op_handle, list_start_time,
						  list_end_time);
					new_list_item = NULL;
					goto exit;
				} else {
					/* Do not print warnings if wanted as we know that there
					 * most probably are collisions
					 */
					if (!new_list_item->silent_fail) {
						desh_warn("(%s) prio insert 1: new item (handle "
							  "%d, start_time %llu, end_time %llu) is "
							  "overlapping with current one (handle "
							  "%d, start_time %llu, end_time %llu)",
							  (__func__), new_list_item->phy_op_handle,
							  new_start_time, new_end_time,
							  iterator->phy_op_handle, list_start_time,
							  list_end_time);
					}
				}
			}
			sys_dlist_insert(&iterator->dnode, &new_list_item->dnode);
			goto exit;
		} else if (new_frame_time == list_frame_time) {
			/* Trying to fit in same frame, check if overlapping */
			if (list_start_time <= new_end_time && list_end_time >= new_start_time) {
				/* Overlapping */
				if (entry_prio >= iterator->priority) {
					desh_warn("(%s) 2: cannot schedule: new item (handle %d, "
						  "start_time %llu, end_time %llu) is overlapping "
						  "with current one (handle %d, start_time %llu, "
						  "end_time %llu)",
						  (__func__), new_list_item->phy_op_handle,
						  new_start_time, new_end_time,
						  iterator->phy_op_handle, list_start_time,
						  list_end_time);
					new_list_item = NULL;
					goto exit;
				} else {
					/* Do not print warnings if wanted as we know that there
					 * most probably are collisions
					 */
					if (!new_list_item->silent_fail) {
						desh_warn("(%s) prio insert 2: new item (handle "
							  "%d, start_time %llu, end_time %llu) is "
							  "overlapping with current one (handle "
							  "%d, start_time %llu, end_time %llu)",
							  (__func__), new_list_item->phy_op_handle,
							  new_start_time, new_end_time,
							  iterator->phy_op_handle, list_start_time,
							  list_end_time);
					}
					/* Overlapping and priority item: insert before the list
					 * item
					 */
					sys_dlist_insert(&iterator->dnode, &new_list_item->dnode);
					goto exit;
				}
			}

			/* Check where to insert according to start_time */
			if (new_start_time < list_start_time) {
				/* Insert before the current list item */
				sys_dlist_insert(&iterator->dnode, &new_list_item->dnode);
				goto exit;
			} else {
				/* Insert after the current list item */
				sys_dnode_t *next =
					sys_dlist_peek_next(&to_be_sheduled_list, &iterator->dnode);

				if (next) {
					sys_dlist_insert(next, &new_list_item->dnode);
					goto exit;
				} else {
					/* append() */
					break;
				}
			}
		} else if (new_frame_time > list_frame_time) {
			/* Not starting in same frame: check still overlappings */
			if (list_start_time <= new_end_time && list_end_time >= new_start_time) {
				/* Overlapping
				 * -> insert new one only if prio allows and if allows, probably
				 * either one id failing in mdm.
				 */
				if (entry_prio >= iterator->priority) {
					desh_warn("(%s) 3: cannot schedule: new item (handle %d, "
						  "start_time %llu, end_time %llu) is overlapping "
						  "with current one (handle %d, start_time %llu, "
						  "end_time %llu)",
						  (__func__), new_list_item->phy_op_handle,
						  new_start_time, new_end_time,
						  iterator->phy_op_handle, list_start_time,
						  list_end_time);
					new_list_item = NULL;
					goto exit;
				} else {
					/* Do not print warnings if wanted as we know that there
					 * most probably are collisions
					 */
					if (!new_list_item->silent_fail) {
						desh_warn("(%s) prio insert 3: new item (handle "
							  "%d, start_time %llu, end_time %llu) is "
							  "overlapping with current one (handle "
							  "%d, start_time %llu, end_time %llu)",
							  (__func__), new_list_item->phy_op_handle,
							  new_start_time, new_end_time,
							  iterator->phy_op_handle, list_start_time,
							  list_end_time);
					}
				}
				sys_dlist_insert(&iterator->dnode, &new_list_item->dnode);
				goto exit;
			}
		}
	}
	sys_dlist_append(&to_be_sheduled_list, &new_list_item->dnode);

exit:
	k_mutex_unlock(&to_be_sheduled_list_mutex);

	if (list_was_empty_at_entry) {
		dect_phy_api_scheduler_next_frame();
	} else {
		k_timer_start(&scheduler_timer, K_MSEC(5), K_NO_WAIT);
	}

	return new_list_item;
}

struct dect_phy_api_scheduler_list_item *dect_phy_api_scheduler_list_item_alloc_tx_element(
	struct dect_phy_api_scheduler_list_item_config **item_conf)
{
	struct dect_phy_api_scheduler_list_item *p_elem =
		k_calloc(1, sizeof(struct dect_phy_api_scheduler_list_item));

	if (p_elem == NULL) {
		return NULL;
	}
	uint8_t *payload = k_calloc(DECT_DATA_MAX_LEN, sizeof(uint8_t));

	if (!payload) {
		k_free(p_elem);
		return NULL;
	}

	p_elem->priority = DECT_PRIORITY1_TX;
	p_elem->silent_fail = false;
	p_elem->sched_config.tx.encoded_payload_pdu = payload;
	*item_conf = &p_elem->sched_config;

	return p_elem;
}

struct dect_phy_api_scheduler_list_item *dect_phy_api_scheduler_list_item_alloc_rx_element(
	struct dect_phy_api_scheduler_list_item_config **item_conf)
{
	struct dect_phy_api_scheduler_list_item *p_elem =
		k_calloc(1, sizeof(struct dect_phy_api_scheduler_list_item));

	if (p_elem == NULL) {
		printk("%s: cannot allocate memory for scheduler list item\n", (__func__));
		return NULL;
	}
	p_elem->priority = DECT_PRIORITY_LOWEST_RX;
	p_elem->sched_config.tx.encoded_payload_pdu = NULL;
	p_elem->silent_fail = false;
	*item_conf = &p_elem->sched_config;

	return p_elem;
}

struct dect_phy_api_scheduler_list_item *dect_phy_api_scheduler_list_item_alloc_rssi_element(
	struct dect_phy_api_scheduler_list_item_config **item_conf)
{
	struct dect_phy_api_scheduler_list_item *p_elem =
		k_calloc(1, sizeof(struct dect_phy_api_scheduler_list_item));

	if (p_elem == NULL) {
		printk("%s: cannot allocate memory for scheduler list item\n", (__func__));
		return NULL;
	}
	p_elem->priority = DECT_PRIORITY1_RX_RSSI;
	p_elem->sched_config.tx.encoded_payload_pdu = NULL;
	p_elem->silent_fail = false;
	*item_conf = &p_elem->sched_config;

	return p_elem;
}

void dect_phy_api_scheduler_list_item_dealloc(struct dect_phy_api_scheduler_list_item *list_item)
{
	if (list_item != NULL) {
		if (list_item->sched_config.tx.encoded_payload_pdu != NULL) {
			k_free(list_item->sched_config.tx.encoded_payload_pdu);
		}
		k_free(list_item);
	}
}

uint64_t dect_phy_api_scheduler_list_item_last_scheduled_modem_frame_time_get(void)
{
	sys_dnode_t *node;
	struct dect_phy_api_scheduler_list_item *list_item;
	uint64_t ret_value = 0;

	if (scheduler_data.state != SCHEDULER_STATE_SUSPENDED) {
		node = sys_dlist_peek_tail(&to_be_sheduled_list);
		if (node) {
			list_item =
				CONTAINER_OF(node, struct dect_phy_api_scheduler_list_item, dnode);
			if (list_item) {
				ret_value = list_item->sched_config.frame_time;
			}
		}
	}
	return ret_value;
}

void dect_phy_api_scheduler_list_status_print(void)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	uint32_t count = 0;
	uint32_t done_count = 0;

	SYS_DLIST_FOR_EACH_CONTAINER(&to_be_sheduled_list, iterator, dnode) {
		count++;
	}
	SYS_DLIST_FOR_EACH_CONTAINER(&done_sheduled_list, iterator, dnode) {
		done_count++;
		desh_print("  Done list item phy handle: %d", iterator->phy_op_handle);
	}

	desh_print("Scheduler list status:");
	desh_print("  List item count: %d", count);
	desh_print("Scheduler done list status:");
	desh_print("  List item count: %d", done_count);
}

/**************************************************************************************************/

/* Scheduler done list (i.e. the items that are sent to modem but not completed there yet) */
static struct dect_phy_api_scheduler_list_item *
dect_phy_api_scheduler_done_list_item_add(struct dect_phy_api_scheduler_list_item *new_list_item)
{

	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	bool found = false;

	SYS_DLIST_FOR_EACH_CONTAINER(&done_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle == new_list_item->phy_op_handle) {
			found = true;
			break;
		}
	}

	if (found) {
		desh_warn("(%s): same phy op handle than was in list already: %d -- continue ",
			  (__func__), new_list_item->phy_op_handle);
	}

	sys_dlist_append(&done_sheduled_list, &new_list_item->dnode);

	return new_list_item;
}

static void dect_phy_api_scheduler_done_list_item_remove_n_dealloc_by_item(
	struct dect_phy_api_scheduler_list_item *list_item)
{
	if (list_item != NULL) {
		sys_dlist_remove(&list_item->dnode);
		dect_phy_api_scheduler_list_item_dealloc(list_item);
	}
}

static bool dect_phy_api_scheduler_done_list_remove_from_tail(void)
{
	sys_dnode_t *node; /* list item */
	struct dect_phy_api_scheduler_list_item *list_item;
	bool return_value = false;

	if (sys_dlist_is_empty(&done_sheduled_list)) {
		return_value = false;
		goto exit;
	}

	return_value = true;
	node = sys_dlist_peek_tail(&done_sheduled_list);

	list_item = CONTAINER_OF(node, struct dect_phy_api_scheduler_list_item, dnode);
	__ASSERT_NO_MSG(node != NULL && list_item != NULL);
	sys_dlist_remove(node);
	dect_phy_api_scheduler_list_item_dealloc(list_item);

exit:
	return return_value;
}

static void dect_phy_api_scheduler_done_list_purge(void)
{
	while (dect_phy_api_scheduler_done_list_remove_from_tail()) {
	}
	sys_dlist_init(&done_sheduled_list);
	__ASSERT_NO_MSG(sys_dlist_is_empty(&done_sheduled_list));
}

bool dect_phy_api_scheduler_done_list_is_empty(void)
{
	return sys_dlist_is_empty(&done_sheduled_list);
}

static struct dect_phy_api_scheduler_list_item *
dect_phy_api_scheduler_done_list_item_find_by_phy_handle(uint32_t handle)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;

	if (dect_phy_api_scheduler_done_list_is_empty()) {
		return NULL;
	}

	SYS_DLIST_FOR_EACH_CONTAINER(&done_sheduled_list, iterator, dnode) {
		if (iterator->phy_op_handle == handle) {
			break;
		}
	}
	return iterator;
}

static void dect_phy_api_scheduler_done_list_mdm_op_complete(
	struct dect_phy_common_op_completed_params *params,
	struct dect_phy_api_scheduler_list_item *list_item)
{
	struct dect_phy_api_scheduler_list_item *found = list_item;

	if (!list_item) {
		found = dect_phy_api_scheduler_done_list_item_find_by_phy_handle(params->handle);
	}

	if (found) {
		if (DECT_PHY_API_SCHEDULER_PRIORITY_IS_TX(found->priority) &&
		    found->sched_config.tx.cb_harq_tx_store) {
			/* Store data for possible retransmission based on HARQ
			 * feedback NACK
			 */
			struct dect_harq_tx_payload_data harq_tx_data;
			struct dect_phy_api_scheduler_list_item *new_list_item =
				dect_phy_api_scheduler_list_item_create_new_copy(
					found, found->sched_config.frame_time, true);

			if (!new_list_item) {
				desh_error("No memory for storing data for possible HARQ "
					   "reTX");
			} else {
				harq_tx_data.harq_user = found->sched_config.tx.harq_user;
				harq_tx_data.sche_list_item = new_list_item;

				found->sched_config.tx.cb_harq_tx_store(&harq_tx_data);
			}
		}
		if (found->sched_config.cb_op_completed) {
			found->sched_config.cb_op_completed(
				params,
				found->sched_config.frame_time);
		}
		if (found->sched_config.cb_op_completed_with_count) {
			if (found->sched_config.interval_count_left == 0) {
				found->sched_config.cb_op_completed_with_count(params->handle);
			}
		}
	}
	if (!list_item) {
		dect_phy_api_scheduler_done_list_item_remove_n_dealloc_by_item(found);
	}
}

/**************************************************************************************************/

int dect_phy_api_scheduler_suspend(void)
{
	return dect_phy_api_scheduler_raise_event(DECT_PHY_API_EVENT_SCHEDULER_OP_SUSPEND);
}

int dect_phy_api_scheduler_resume(void)
{
	return dect_phy_api_scheduler_raise_event(DECT_PHY_API_EVENT_SCHEDULER_OP_RESUME);
}

int dect_phy_api_scheduler_next_frame(void)
{
	return dect_phy_api_scheduler_raise_event(DECT_PHY_API_EVENT_SCHEDULER_NEXT_FRAME);
}

/**************************************************************************************************/

int dect_phy_api_scheduler_mdm_op_completed(
	struct dect_phy_common_op_completed_params *params)
{
	int ret = dect_phy_api_scheduler_raise_event_with_data(
		DECT_PHY_API_EVENT_SCHEDULER_OP_COMPLETED, (void *)params,
		sizeof(struct dect_phy_common_op_completed_params));
	return ret;
}

int dect_phy_api_scheduler_mdm_pdc_data_recv(
	struct dect_phy_api_scheduler_op_pdc_type_rcvd_params *rcv_data)
{
	int ret = dect_phy_api_scheduler_raise_event_with_data(
		DECT_PHY_API_EVENT_SCHEDULER_OP_PDC_DATA_RECEIVED, (void *)rcv_data,
		sizeof(struct dect_phy_api_scheduler_op_pdc_type_rcvd_params));

	return ret;
}

/**************************************************************************************************/

/* Scheduler core logics */

static int
dect_phy_api_scheduler_core_mdm_phy_op(struct dect_phy_api_scheduler_list_item *list_item,
				       uint64_t op_start_time)
{
	int err = 0;

	if (scheduler_data.state == SCHEDULER_STATE_SUSPENDED) {
		/* Don't schedule to modem when suspended. Complete immediately. */
		err = -EAGAIN; /* We use this error value for this purpose */
	} else {
		if (DECT_PHY_API_SCHEDULER_PRIORITY_IS_TX(list_item->priority)) {
			union nrf_modem_dect_phy_hdr phy_header;
			static struct nrf_modem_dect_phy_tx_params tx_op;

			tx_op.bs_cqi = NRF_MODEM_DECT_PHY_BS_CQI_NOT_USED;
			tx_op.carrier = list_item->sched_config.channel;
			tx_op.handle = list_item->phy_op_handle;
			tx_op.network_id = list_item->sched_config.address_info.network_id;
			tx_op.start_time = op_start_time;
			if (list_item->sched_config.tx.header_type == DECT_PHY_HEADER_TYPE1) {
				tx_op.phy_type = DECT_PHY_HEADER_TYPE1;
				memcpy(&phy_header.type_1, &list_item->sched_config.tx.phy_header,
				       sizeof(phy_header.type_1));
			} else {
				tx_op.phy_type = DECT_PHY_HEADER_TYPE2;
				memcpy(&phy_header.type_2, &list_item->sched_config.tx.phy_header,
				       sizeof(phy_header.type_2));
			}
			tx_op.phy_header = &phy_header;

			__ASSERT_NO_MSG(list_item->sched_config.tx.encoded_payload_pdu != NULL);

			tx_op.data_size = list_item->sched_config.tx.encoded_payload_pdu_size;
			tx_op.data = list_item->sched_config.tx.encoded_payload_pdu;
			tx_op.lbt_period = list_item->sched_config.tx.phy_lbt_period;
			tx_op.lbt_rssi_threshold_max =
				list_item->sched_config.tx.phy_lbt_rssi_threshold_max;

			/* Also HARQ feedback RX is using combined tx_rx */
			if (list_item->sched_config.tx.combined_tx_rx_use ||
			    list_item->sched_config.tx.harq_feedback_requested) {
				struct nrf_modem_dect_phy_tx_rx_params tx_rx_param;
				struct nrf_modem_dect_phy_rx_params rx_op = {
					.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
					.filter = {0},
				};

				/* HARQ */
				if (list_item->sched_config.tx.harq_feedback_requested &&
				    !list_item->sched_config.tx.combined_tx_rx_use) {
					/* For catching the HARQ feedback response from the peer:
					 * open the RX after sending like instructed in settings.
					 */
					struct dect_phy_settings *current_settings =
						dect_common_settings_ref_get();
					int32_t harq_rx_duration =
						current_settings->harq
							.harq_feedback_rx_subslot_count *
						DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;
					uint64_t harq_rx_start_time =
						current_settings->harq
							.harq_feedback_rx_delay_subslot_count *
						DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS;

					rx_op.network_id =
						list_item->sched_config.address_info.network_id;
					rx_op.filter.is_short_network_id_used = true;
					rx_op.filter.short_network_id =
						(uint8_t)(list_item->sched_config.address_info
								  .network_id &
							  0xFF);
					rx_op.filter.receiver_identity =
						list_item->sched_config.address_info
							.transmitter_long_rd_id;

					rx_op.carrier = tx_op.carrier;
					rx_op.duration = harq_rx_duration;
					rx_op.handle = DECT_HARQ_FEEDBACK_RX_HANDLE;
					rx_op.mode = NRF_MODEM_DECT_PHY_RX_MODE_SINGLE_SHOT;
					rx_op.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED;
					rx_op.rssi_level = 0;
					rx_op.start_time = harq_rx_start_time;

				} else {
					__ASSERT_NO_MSG(
						list_item->sched_config.tx.combined_tx_rx_use ==
						true);
					rx_op = list_item->sched_config.tx.combined_rx_op;
				}

				tx_rx_param.tx = tx_op;
				tx_rx_param.rx = rx_op;

				/* Use combined tx_rx */
				err = nrf_modem_dect_phy_tx_rx(&tx_rx_param);
			} else {
				err = nrf_modem_dect_phy_tx(&tx_op);
			}
			if (err) {
				printk("(%s): nrf_modem_dect_phy_tx failed: err %d, handle %d\n",
				       (__func__), err, tx_op.handle);
				goto exit;
			} else {
#if defined(CONFIG_DK_LIBRARY)
				dect_phy_api_scheduler_raise_event(
					DECT_PHY_API_EVENT_SCHEDULER_LED_TX_ON);
#endif
			}
		} else if (list_item->priority == DECT_PRIORITY1_RX_RSSI) {
			struct nrf_modem_dect_phy_rssi_params rssi_params =
				list_item->sched_config.rssi.rssi_op_params;

			err = nrf_modem_dect_phy_rssi(&rssi_params);
			if (err) {
				desh_error("(%s): nrf_modem_dect_phy_rssi failed %d",
					(__func__), err);
			}
		} else {
			struct nrf_modem_dect_phy_rx_params rx_op = {
				.rssi_interval = NRF_MODEM_DECT_PHY_RSSI_INTERVAL_OFF,
				.filter = {0},
			};
			const uint64_t length_mdm_ticks =
				(list_item->sched_config.subslot_used)
					? (list_item->sched_config.length_subslots *
					   DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS)
					: (list_item->sched_config.length_slots *
					   DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS);
			int32_t duration = (list_item->sched_config.rx.duration > 0)
						   ? list_item->sched_config.rx.duration
						   : length_mdm_ticks;

			rx_op.network_id = list_item->sched_config.rx.network_id;
			rx_op.filter = list_item->sched_config.rx.filter;
			rx_op.carrier = list_item->sched_config.channel;
			rx_op.duration = duration;
			rx_op.handle = list_item->phy_op_handle;
			rx_op.mode = list_item->sched_config.rx.mode;
			rx_op.link_id = NRF_MODEM_DECT_PHY_LINK_UNSPECIFIED;
			rx_op.rssi_level = list_item->sched_config.rx.expected_rssi_level;
			rx_op.start_time = op_start_time;

			err = dect_phy_common_rx_op(&rx_op);
			if (err) {
				desh_error("nrf_modem_dect_phy_rx failed: err %d, handle %d", err,
					   rx_op.handle);
			}
		}
	}
exit:
	return err;
}

static void dect_phy_api_scheduler_core_tick_th_schedule_next_frame(void)
{
	struct dect_phy_api_scheduler_list_item *iterator = NULL;
	struct dect_phy_api_scheduler_list_item *safe = NULL;
	struct dect_phy_api_scheduler_list_item *scheduled_list_item = NULL;
	int ret = 0;
	bool remove_from_list = true;
	bool dealloc_list_item = true;
	bool add_item_to_done_list = false;
	bool we_need_tx_data_in_done_list = false;
	uint16_t op_count_trials_to_mdm = 0;
	struct dect_phy_common_op_completed_params sched_op_completed_params;

	k_mutex_lock(&to_be_sheduled_list_mutex, K_FOREVER);

	SYS_DLIST_FOR_EACH_CONTAINER_SAFE(&to_be_sheduled_list, iterator, safe, dnode) {
		uint64_t time_now = dect_app_modem_time_now();
		uint64_t frame_time = iterator->sched_config.frame_time;
		int64_t time_to_frame = frame_time - time_now;

		if (time_to_frame < 0) {

			ret = DECT_SCHEDULER_DELAYED_ERROR;
			sched_op_completed_params.handle = iterator->phy_op_handle;
			sched_op_completed_params.time = time_now;
			sched_op_completed_params.temperature =
				NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED;
			sched_op_completed_params.status = ret;

			/* Complete item */
			if (iterator->sched_config.cb_op_to_mdm) {
				iterator->sched_config.cb_op_to_mdm(
					&sched_op_completed_params,
					iterator->sched_config.frame_time);
			}

			if (iterator->sched_config.cb_op_completed) {
				/* Trigger callback also */
				dect_phy_api_scheduler_done_list_mdm_op_complete(
					&sched_op_completed_params, iterator);
			}
			dect_phy_api_scheduler_mdm_op_req_failed_evt_send(
				&sched_op_completed_params);

			if (iterator->sched_config.interval_count_left > 0) {
				iterator->sched_config.interval_count_left--;
				if (iterator->sched_config.interval_count_left == 0) {
					/* We have done needed count. Disable intervals. */
					iterator->sched_config.interval_mdm_ticks = 0;

					if (iterator->sched_config.cb_op_completed_with_count) {
						dect_phy_api_scheduler_done_list_mdm_op_complete(
							&sched_op_completed_params, iterator);
					}
					if (iterator->sched_config
						    .cb_op_to_mdm_with_interval_count_completed) {
						iterator->sched_config
							.cb_op_to_mdm_with_interval_count_completed(
								iterator->phy_op_handle);
					}
				}
			}

			if (iterator->sched_config.interval_mdm_ticks) {
				/* Remove from list and modify frame time and move back to scheduler
				 * list
				 */
				uint64_t new_frame_time =
					frame_time + iterator->sched_config.interval_mdm_ticks;

				dect_phy_api_scheduler_list_item_remove_by_item(iterator);
				iterator->sched_config.frame_time = new_frame_time;
				if (iterator->sched_config.phy_op_handle_range_used) {
					uint32_t next_handle = iterator->phy_op_handle;

					next_handle++;
					if (next_handle >
					    iterator->sched_config.phy_op_handle_range_end) {
						next_handle = iterator->sched_config
								      .phy_op_handle_range_start;
					}
					iterator->phy_op_handle = next_handle;
				}
				if (iterator->priority == DECT_PRIORITY1_RX_RSSI) {
					iterator->sched_config.rssi.rssi_op_params.start_time =
						new_frame_time;
				}

				if (!dect_phy_api_scheduler_list_item_add(iterator)) {
					printk("(%s)/1: dect_phy_api_scheduler_list_item_add for "
					       "failed\n",
					       (__func__));
				} else {
					remove_from_list = false;
					dealloc_list_item = false;
				}
			}
			if (remove_from_list) {
				dect_phy_api_scheduler_list_item_remove_by_item(iterator);
			}
			if (dealloc_list_item) {
				dect_phy_api_scheduler_list_item_dealloc(iterator);
			}
		} else if (op_count_trials_to_mdm >= DECT_PHY_API_SCHEDULER_OP_MAX_COUNT) {
			/* We have sent already quite bunch of operations. Let's come back later */
			k_timer_start(&scheduler_timer, K_MSEC(10), K_NO_WAIT);
			goto exit;
		} else if (MODEM_TICKS_TO_MS(time_to_frame) >
			   DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS) {
			/* frame_time much later. Let's come back later */
			k_timer_start(&scheduler_timer,
				      K_MSEC(DECT_PHY_API_SCHEDULER_OP_TIME_WINDOW_MS / 5),
				      K_NO_WAIT);
			goto exit;
		} else {
			const uint64_t op_start_offset =
				iterator->sched_config.subslot_used
					? (iterator->sched_config.start_subslot *
					   DECT_RADIO_SUBSLOT_DURATION_IN_MODEM_TICKS)
					: (iterator->sched_config.start_slot *
					   DECT_RADIO_SLOT_DURATION_IN_MODEM_TICKS);
			uint64_t start_time = frame_time + op_start_offset;

			ret = dect_phy_api_scheduler_core_mdm_phy_op(iterator, start_time);

			/**************************************************************************/

			if (iterator->sched_config.cb_op_to_mdm) {
				sched_op_completed_params.handle = iterator->phy_op_handle;
				sched_op_completed_params.time = start_time;
				sched_op_completed_params.temperature =
					NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED;
				sched_op_completed_params.status = ret;

				iterator->sched_config.cb_op_to_mdm(
					&sched_op_completed_params,
					iterator->sched_config.frame_time);
			}

			/**************************************************************************/

			if (iterator->sched_config.cb_op_completed ||
			    iterator->sched_config.cb_pdc_received ||
			    DECT_PHY_API_SCHEDULER_PRIORITY_IS_WITH_FORCE(iterator->priority)) {
				add_item_to_done_list = true;
			}
			if (DECT_PHY_API_SCHEDULER_PRIORITY_IS_TX(iterator->priority) &&
			    iterator->sched_config.tx.cb_harq_tx_store) {
				add_item_to_done_list = true;
				we_need_tx_data_in_done_list = true;
			}

			if (iterator->sched_config.interval_count_left > 0) {
				iterator->sched_config.interval_count_left--;
				if (iterator->sched_config.interval_count_left == 0) {
					/* We have done needed count. Disable intervals. */
					iterator->sched_config.interval_mdm_ticks = 0;

					if (iterator->sched_config.cb_op_completed_with_count) {
						add_item_to_done_list = true;
					}
					if (iterator->sched_config
						    .cb_op_to_mdm_with_interval_count_completed) {
						iterator->sched_config
							.cb_op_to_mdm_with_interval_count_completed(
								iterator->phy_op_handle);
					}
				}
			}
			if (add_item_to_done_list) {
				/* We need item still later, let's copy it to done_list
				 * ...but we don't need tx data except with HARQ.
				 */
				scheduled_list_item =
					dect_phy_api_scheduler_list_item_create_new_copy(
						iterator, frame_time, we_need_tx_data_in_done_list);
				we_need_tx_data_in_done_list = false;
				if (!scheduled_list_item) {
					desh_error("(%s): cannot copy list item", (__func__));
				} else {
					if (dect_phy_api_scheduler_done_list_item_add(
						    scheduled_list_item) == NULL) {
						desh_error("(%s): 2: cannot add copied list item "
							   "to done list (handle %d)",
							   (__func__), iterator->phy_op_handle);
						dect_phy_api_scheduler_list_item_dealloc(
							scheduled_list_item);
					}
				}
			}

			/**************************************************************************/

			if (iterator->sched_config.interval_mdm_ticks) {
				uint64_t new_frame_time = iterator->sched_config.frame_time +
							  iterator->sched_config.interval_mdm_ticks;

				/* Remove from list and modify frame_time and add back to scheduler
				 * list
				 */
				dect_phy_api_scheduler_list_item_remove_by_item(iterator);
				iterator->sched_config.frame_time = new_frame_time;
				if (iterator->sched_config.phy_op_handle_range_used) {
					uint32_t next_handle = iterator->phy_op_handle;

					next_handle++;
					if (next_handle >
					    iterator->sched_config.phy_op_handle_range_end) {
						next_handle = iterator->sched_config
								      .phy_op_handle_range_start;
					}
					iterator->phy_op_handle = next_handle;
				}
				if (iterator->priority == DECT_PRIORITY1_RX_RSSI) {
					iterator->sched_config.rssi.rssi_op_params.start_time =
						new_frame_time;
				}

				if (!dect_phy_api_scheduler_list_item_add(iterator)) {
					desh_error("(%s): dect_phy_api_scheduler_list_item_add "
						   "failed - op with interval will be concluded "
						   "(phy handle %d)",
						   (__func__), iterator->phy_op_handle);

					/* If repeateable item fails,
					 * we need to inform users by using dedicated error
					 */
					ret = DECT_SCHEDULER_SCHEDULER_FATAL_MEM_ALLOC_ERROR;

					if (iterator->sched_config
						    .cb_op_to_mdm_with_interval_count_completed) {
						iterator->sched_config
							.cb_op_to_mdm_with_interval_count_completed(
								iterator->phy_op_handle);
					}
					if (iterator->sched_config.cb_op_completed_with_count) {
						iterator->sched_config.cb_op_completed_with_count(
							iterator->phy_op_handle);
					}
				} else {
					remove_from_list = false;
					dealloc_list_item = false;
				}
			}

			/**************************************************************************/

			if (ret) {
				sched_op_completed_params.handle = iterator->phy_op_handle;
				sched_op_completed_params.status = 0;
				sched_op_completed_params.time = time_now;
				sched_op_completed_params.temperature =
					NRF_MODEM_DECT_PHY_TEMP_NOT_MEASURED;

				if (ret != -EAGAIN) { /* EAGAIN is when scheduler suspended */
					/* phy op failed, complete right away */
					sched_op_completed_params.status = ret;
				}
				dect_phy_api_scheduler_mdm_op_completed(&sched_op_completed_params);
				dect_phy_api_scheduler_mdm_op_req_failed_evt_send(
					&sched_op_completed_params);

				/* Something went wrong already when trying to send operation
				 * to modem. So, let's break out for this time and come back later.
				 */
				op_count_trials_to_mdm = DECT_PHY_API_SCHEDULER_OP_MAX_COUNT;
			} else {
				op_count_trials_to_mdm++;
			}

			if (remove_from_list) {
				dect_phy_api_scheduler_list_item_remove_by_item(iterator);
			} else {
				remove_from_list = true;
			}
			if (dealloc_list_item) {
				dect_phy_api_scheduler_list_item_dealloc(iterator);
			} else {
				dealloc_list_item = true;
			}
		}
	}
exit:
	k_mutex_unlock(&to_be_sheduled_list_mutex);
}

/**************************************************************************************************/

static int dect_phy_api_scheduler_init(void)
{
	sys_dlist_init(&to_be_sheduled_list);
	sys_dlist_init(&done_sheduled_list);
	scheduler_data.state = SCHEDULER_STATE_NORMAL;

	return 0;
}

SYS_INIT(dect_phy_api_scheduler_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

static void dect_phy_api_scheduler_th_handler(void)
{
	struct dect_phy_op_event_msgq_item event;

	while (true) {
		k_msgq_get(&dect_phy_api_scheduler_op_event_msgq, &event, K_FOREVER);
		switch (event.id) {
		case DECT_PHY_API_EVENT_SCHEDULER_NEXT_FRAME: {
			dect_phy_api_scheduler_core_tick_th_schedule_next_frame();
			break;
		}

#if defined(CONFIG_DK_LIBRARY)
		case DECT_PHY_API_EVENT_SCHEDULER_LED_TX_ON: {
			dk_set_led_on(DECT_TX_STATUS_LED);
			k_timer_start(&scheduler_led_off_timer, K_MSEC(300), K_NO_WAIT);
			break;
		}
		case DECT_PHY_API_EVENT_SCHEDULER_LED_TX_OFF: {
			dk_set_led_off(DECT_TX_STATUS_LED);
			break;
		}
#endif
		case DECT_PHY_API_EVENT_SCHEDULER_OP_COMPLETED: {
			struct dect_phy_common_op_completed_params *params =
				(struct dect_phy_common_op_completed_params *)event.data;

			dect_phy_api_scheduler_done_list_mdm_op_complete(params, NULL);
			break;
		}
		case DECT_PHY_API_EVENT_SCHEDULER_OP_PDC_DATA_RECEIVED: {
			struct dect_phy_api_scheduler_op_pdc_type_rcvd_params *params =
				(struct dect_phy_api_scheduler_op_pdc_type_rcvd_params *)event.data;
			struct dect_phy_api_scheduler_list_item *iterator;

			SYS_DLIST_FOR_EACH_CONTAINER(&done_sheduled_list, iterator, dnode) {
				if (iterator->sched_config.cb_pdc_received) {
					iterator->sched_config.cb_pdc_received(
						params->time, params->data, params->data_length,
						params->rx_rssi_dbm, params->rx_pwr_dbm);
				}
			}
			break;
		}
		case DECT_PHY_API_EVENT_SCHEDULER_OP_LIST_PURGE: {
			dect_phy_api_scheduler_list_purge();
			dect_phy_api_scheduler_done_list_purge();
			break;
		}
		case DECT_PHY_API_EVENT_SCHEDULER_OP_LIST_ITEM_RM_DEALLOC: {
			uint32_t *phy_handle = (uint32_t *)event.data;

			dect_phy_api_scheduler_list_item_remove_dealloc_by_phy_op_handle(
				*phy_handle);
			break;
		}
		case DECT_PHY_API_EVENT_SCHEDULER_OP_SUSPEND: {
			scheduler_data.state = SCHEDULER_STATE_SUSPENDED;
			dect_phy_api_scheduler_suspended_evt_send();
			break;
		}
		case DECT_PHY_API_EVENT_SCHEDULER_OP_RESUME: {
			scheduler_data.state = SCHEDULER_STATE_NORMAL;
			dect_phy_api_scheduler_resumed_evt_send();
			break;
		}
		default:
			desh_warn("DECT SCHEDULER TH: Unknown event %d received", event.id);
			break;
		}
		if (event.data != NULL) {
			k_free(event.data);
		}
	}
}

#define DECT_PHY_API_SCHEDULER_TICK_THREAD_STACK_SIZE 4096
#define DECT_PHY_API_SCHEDULER_TICK_THREAD_PRIORITY   K_PRIO_COOP(5) /* -11 */

K_THREAD_DEFINE(dect_phy_api_scheduler_th, DECT_PHY_API_SCHEDULER_TICK_THREAD_STACK_SIZE,
		dect_phy_api_scheduler_th_handler, NULL, NULL, NULL,
		DECT_PHY_API_SCHEDULER_TICK_THREAD_PRIORITY, 0, 0);
