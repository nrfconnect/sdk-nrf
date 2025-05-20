/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <bluetooth/services/nus.h>

#include "ei_data_forwarder.h"
#include "ei_data_forwarder_event.h"

#include <caf/events/ble_common_event.h>
#include <caf/events/sensor_event.h>
#include <caf/events/sensor_data_aggregator_event.h>
#include "ml_app_mode_event.h"

#define MODULE ei_data_forwarder
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_ML_APP_EI_DATA_FORWARDER_LOG_LEVEL);

#define MIN_CONN_INT		CONFIG_BT_PERIPHERAL_PREF_MIN_INT
#define MAX_CONN_INT		CONFIG_BT_PERIPHERAL_PREF_MAX_INT

#define DATA_BUF_SIZE		CONFIG_ML_APP_EI_DATA_FORWARDER_BUF_SIZE
#define DATA_BUF_COUNT		CONFIG_ML_APP_EI_DATA_FORWARDER_BUF_COUNT
#define PIPELINE_MAX_CNT	CONFIG_ML_APP_EI_DATA_FORWARDER_PIPELINE_COUNT
#define BUF_SLAB_ALIGN		4

#define ML_APP_MODE_CONTROL	IS_ENABLED(CONFIG_ML_APP_MODE_EVENTS)

enum {
	CONN_SECURED			= BIT(0),
	CONN_SUBSCRIBED			= BIT(1),
	CONN_INTERVAL_VALID		= BIT(2),
};

enum state {
	STATE_DISABLED_ACTIVE,
	STATE_DISABLED_SUSPENDED,
	STATE_ACTIVE,
	STATE_SUSPENDED,
	STATE_BLOCKED,
	STATE_ERROR
};

struct ei_data_packet {
	sys_snode_t node;
	size_t size;
	uint8_t buf[DATA_BUF_SIZE];
};

BUILD_ASSERT(ARRAY_SIZE(CONFIG_ML_APP_EI_DATA_FORWARDER_SENSOR_EVENT_DESCR) > 1);
static const char *handled_sensor_event_descr = CONFIG_ML_APP_EI_DATA_FORWARDER_SENSOR_EVENT_DESCR;

static uint8_t conn_state;
static struct bt_conn *nus_conn;

static enum state state = ML_APP_MODE_CONTROL ? STATE_DISABLED_SUSPENDED : STATE_DISABLED_ACTIVE;

BUILD_ASSERT(sizeof(struct ei_data_packet) % BUF_SLAB_ALIGN == 0);
K_MEM_SLAB_DEFINE(buf_slab, sizeof(struct ei_data_packet), DATA_BUF_COUNT, BUF_SLAB_ALIGN);

static sys_slist_t send_queue;
static struct k_work send_queued;
static size_t pipeline_cnt;
static atomic_t sent_cnt;


static void broadcast_ei_data_forwarder_state(enum ei_data_forwarder_state forwarder_state)
{
	__ASSERT_NO_MSG(forwarder_state != EI_DATA_FORWARDER_STATE_DISABLED);

	struct ei_data_forwarder_event *event = new_ei_data_forwarder_event();

	event->state = forwarder_state;
	APP_EVENT_SUBMIT(event);
}

static bool is_nus_conn_valid(struct bt_conn *conn, uint8_t conn_state)
{
	if (!conn) {
		return false;
	}

	if (!(conn_state & CONN_INTERVAL_VALID)) {
		return false;
	}

	if (!(conn_state & CONN_SECURED)) {
		return false;
	}

	if (!(conn_state & CONN_SUBSCRIBED)) {
		return false;
	}

	return true;
}

static void update_state(enum state new_state)
{
	static enum ei_data_forwarder_state forwarder_state = EI_DATA_FORWARDER_STATE_DISABLED;
	enum ei_data_forwarder_state new_forwarder_state;

	switch (new_state) {
	case STATE_DISABLED_ACTIVE:
	case STATE_DISABLED_SUSPENDED:
		new_forwarder_state = EI_DATA_FORWARDER_STATE_DISABLED;
		break;

	case STATE_ACTIVE:
		if (is_nus_conn_valid(nus_conn, conn_state)) {
			new_forwarder_state = EI_DATA_FORWARDER_STATE_TRANSMITTING;
			break;
		}
		/* Fall-through */

	case STATE_SUSPENDED:
	case STATE_BLOCKED:
		if (nus_conn) {
			new_forwarder_state = EI_DATA_FORWARDER_STATE_CONNECTED;
			break;
		}
		/* Fall-through */

	case STATE_ERROR:
	default:
		new_forwarder_state = EI_DATA_FORWARDER_STATE_DISCONNECTED;
		break;
	}

	state = new_state;

	if (new_forwarder_state != forwarder_state) {
		broadcast_ei_data_forwarder_state(new_forwarder_state);
		forwarder_state = new_forwarder_state;
	}
}

static void report_error(void)
{
	update_state(STATE_ERROR);
	module_set_state(MODULE_STATE_ERROR);
}

static void clean_buffered_data(void)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(&send_queue))) {
		struct ei_data_packet *packet = CONTAINER_OF(node, __typeof__(*packet), node);

		k_mem_slab_free(&buf_slab, (void *)packet);
	}
}

static int send_packet(struct bt_conn *conn, uint8_t *buf, size_t size)
{
	int err = 0;
	uint32_t mtu = bt_nus_get_mtu(conn);

	if (mtu < size) {
		LOG_WRN("GATT MTU too small: %zu > %u", size, mtu);
		err = -ENOBUFS;
	} else {
		__ASSERT_NO_MSG(size <= UINT16_MAX);

		err = bt_nus_send(nus_conn, buf, size);

		if (err) {
			LOG_WRN("bt_nus_tx error: %d", err);
		}
	}

	return err;
}

static void send_queued_fn(struct k_work *w)
{
	atomic_dec(&sent_cnt);

	if (atomic_get(&sent_cnt)) {
		k_work_submit(&send_queued);
	}

	__ASSERT_NO_MSG(pipeline_cnt > 0);
	pipeline_cnt--;

	__ASSERT_NO_MSG(nus_conn);

	if (state != STATE_ACTIVE) {
		return;
	}

	sys_snode_t *node = sys_slist_get(&send_queue);

	if (node) {
		struct ei_data_packet *packet = CONTAINER_OF(node, __typeof__(*packet), node);

		if (send_packet(nus_conn, packet->buf, packet->size)) {
			update_state(STATE_BLOCKED);
		} else {
			pipeline_cnt++;
		}

		k_mem_slab_free(&buf_slab, (void *)packet);
	}
}

static bool send_data_block(const struct sensor_value *data_ptr, size_t data_cnt)
{
	static uint8_t buf[DATA_BUF_SIZE];
	int pos = ei_data_forwarder_parse_data(data_ptr, data_cnt, (char *)buf, sizeof(buf));

	if (pos < 0) {
		LOG_ERR("EI data forwader parsing error: %d", pos);
		report_error();
		return false;
	}

	if (pipeline_cnt < PIPELINE_MAX_CNT) {
		if (send_packet(nus_conn, buf, pos)) {
			update_state(STATE_BLOCKED);
		} else {
			pipeline_cnt++;
		}
	} else {
		struct ei_data_packet *packet;

		if (k_mem_slab_alloc(&buf_slab, (void **)&packet, K_NO_WAIT)) {
			LOG_WRN("No space to buffer data");
			LOG_WRN("Sampling frequency is too high");
			update_state(STATE_BLOCKED);
		} else {
			memcpy(packet->buf, buf, pos);
			packet->size = pos;
			sys_slist_append(&send_queue, &packet->node);
		}
	}

	return false;
}

static bool handle_sensor_event(const struct sensor_event *event)
{
	if ((state != STATE_ACTIVE) || !is_nus_conn_valid(nus_conn, conn_state)) {
		return false;
	}

	if ((event->descr != handled_sensor_event_descr) &&
	    strcmp(event->descr, handled_sensor_event_descr)) {
		return false;
	}

	__ASSERT_NO_MSG(sensor_event_get_data_cnt(event) > 0);

	return send_data_block(sensor_event_get_data_ptr(event), sensor_event_get_data_cnt(event));
}

static bool handle_sensor_data_aggregator_event(const struct sensor_data_aggregator_event *event)
{
	if ((state != STATE_ACTIVE) || !is_nus_conn_valid(nus_conn, conn_state)) {
		return false;
	}

	if ((event->sensor_descr != handled_sensor_event_descr) &&
	    strcmp(event->sensor_descr, handled_sensor_event_descr)) {
		return false;
	}

	__ASSERT_NO_MSG(event->sample_cnt > 0);
	__ASSERT_NO_MSG(event->values_in_sample > 0);

	for (size_t i = 0; i < event->sample_cnt; ++i) {
		size_t pos = i * event->values_in_sample;
		(void)send_data_block(event->samples + pos, event->values_in_sample);
	}

	return false;
}

static bool handle_ml_app_mode_event(const struct ml_app_mode_event *event)
{
	if (state == STATE_ERROR) {
		return false;
	}

	bool disabled = ((state == STATE_DISABLED_ACTIVE) || (state == STATE_DISABLED_SUSPENDED));
	enum state new_state;

	if (event->mode == ML_APP_MODE_DATA_FORWARDING) {
		new_state = disabled ? STATE_DISABLED_ACTIVE : STATE_ACTIVE;

		clean_buffered_data();
	} else {
		new_state = disabled ? STATE_DISABLED_SUSPENDED : STATE_SUSPENDED;
	}

	update_state(new_state);

	return false;
}

static void bt_nus_sent_cb(struct bt_conn *conn)
{
	atomic_inc(&sent_cnt);
	k_work_submit(&send_queued);
}

static void send_enabled(enum bt_nus_send_status status)
{
	/* Make sure callback and Application Event Manager event handler will not be preempted. */
	BUILD_ASSERT(CONFIG_SYSTEM_WORKQUEUE_PRIORITY < 0);
	__ASSERT_NO_MSG(!k_is_in_isr());
	__ASSERT_NO_MSG(!k_is_preempt_thread());

	if (status == BT_NUS_SEND_STATUS_ENABLED) {
		conn_state |= CONN_SUBSCRIBED;
	} else {
		conn_state &= ~CONN_SUBSCRIBED;
		__ASSERT_NO_MSG(status == BT_NUS_SEND_STATUS_DISABLED);
	}

	update_state(state);

	LOG_INF("Notifications %sabled", (status == BT_NUS_SEND_STATUS_ENABLED) ? "en" : "dis");
}

static int init_nus(void)
{
	static struct bt_nus_cb nus_cb = {
		.sent = bt_nus_sent_cb,
		.send_enabled = send_enabled,
	};

	int err = bt_nus_init(&nus_cb);

	if (err) {
		LOG_ERR("Cannot initialize NUS (err %d)", err);
	}

	return err;
}

static void init(void)
{
	__ASSERT_NO_MSG((state == STATE_DISABLED_ACTIVE) || (state == STATE_DISABLED_SUSPENDED));

	k_work_init(&send_queued, send_queued_fn);
	sys_slist_init(&send_queue);

	int err = init_nus();

	if (!err) {
		enum state new_state = (state == STATE_DISABLED_ACTIVE) ?
				       STATE_ACTIVE : STATE_SUSPENDED;

		update_state(new_state);
		module_set_state(MODULE_STATE_READY);
	} else {
		report_error();
	}
}

static bool handle_module_state_event(const struct module_state_event *event)
{
	if (check_state(event, MODULE_ID(ble_state), MODULE_STATE_READY)) {
		init();
	}

	return false;
}

static bool handle_ble_peer_event(const struct ble_peer_event *event)
{
	switch (event->state) {
	case PEER_STATE_CONNECTED:
		clean_buffered_data();
		pipeline_cnt = 0;
		atomic_set(&sent_cnt, 0);
		nus_conn = event->id;
		break;

	case PEER_STATE_SECURED:
		__ASSERT_NO_MSG(nus_conn);
		conn_state |= CONN_SECURED;
		break;

	case PEER_STATE_DISCONNECTED:
		k_work_cancel(&send_queued);
		/* Fall-through */

	case PEER_STATE_DISCONNECTING:
		/* Clear flags representing connection state. */
		conn_state = 0;
		nus_conn = NULL;
		break;

	default:
		break;
	}

	update_state(state);

	return false;
}

static bool handle_ble_peer_conn_params_event(const struct ble_peer_conn_params_event *event)
{
	if (!event->updated || (event->id != nus_conn)) {
		return false;
	}

	__ASSERT_NO_MSG(event->interval_min == event->interval_max);

	uint16_t interval = event->interval_min;

	if ((interval >= MIN_CONN_INT) && (interval <= MAX_CONN_INT)) {
		conn_state |= CONN_INTERVAL_VALID;
	} else {
		if ((state == STATE_ACTIVE) && is_nus_conn_valid(nus_conn, conn_state)) {
			state = STATE_BLOCKED;
		}
		conn_state &= ~CONN_INTERVAL_VALID;
	}

	update_state(state);

	return false;
}

static bool app_event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_event(aeh)) {
		return handle_sensor_event(cast_sensor_event(aeh));
	}

	if (IS_ENABLED(CONFIG_CAF_SENSOR_DATA_AGGREGATOR_EVENTS) &&
	    is_sensor_data_aggregator_event(aeh)) {
		return handle_sensor_data_aggregator_event(cast_sensor_data_aggregator_event(aeh));
	}

	if (ML_APP_MODE_CONTROL &&
	    is_ml_app_mode_event(aeh)) {
		return handle_ml_app_mode_event(cast_ml_app_mode_event(aeh));
	}

	if (is_module_state_event(aeh)) {
		return handle_module_state_event(cast_module_state_event(aeh));
	}

	if (is_ble_peer_event(aeh)) {
		return handle_ble_peer_event(cast_ble_peer_event(aeh));
	}

	if (is_ble_peer_conn_params_event(aeh)) {
		return handle_ble_peer_conn_params_event(cast_ble_peer_conn_params_event(aeh));
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, app_event_handler);
APP_EVENT_SUBSCRIBE(MODULE, module_state_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_data_aggregator_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_event);
APP_EVENT_SUBSCRIBE(MODULE, ble_peer_conn_params_event);
#if ML_APP_MODE_CONTROL
APP_EVENT_SUBSCRIBE(MODULE, ml_app_mode_event);
#endif /* ML_APP_MODE_CONTROL */
