/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <openthread.h>
#include <openthread/thread.h>
#include <openthread/srp_client.h>

#include <health_monitor.h>

LOG_MODULE_REGISTER(health_monitor, CONFIG_HEALTH_MONITOR_LOG_LEVEL);


#define EVENT_SIZE_UNIT sizeof(struct health_event_data)
#define QUEUE_EVENT_COUNT 10

struct test_api_data {
	bool (*verify_and_notify_handler)(bool notify);
	int64_t required_time;
	int64_t next_timeout;
	bool reported;
};

static bool test_srp(bool notify);
static bool test_singleton_leader(bool notify);

static void post_event(struct health_event_data *event);

static void health_monitor_healthcheck(struct k_work *item);
static void health_monitor_raport(struct k_work *item);
static void health_monitor_process(struct k_work *item);

static void health_check_timeout(struct k_timer *timer_id);
static void health_raport_timeout(struct k_timer *timer_id);

K_TIMER_DEFINE(health_check_timer, health_check_timeout, NULL);
K_TIMER_DEFINE(health_raport_timer, health_raport_timeout, NULL);
K_MSGQ_DEFINE(event_queue, EVENT_SIZE_UNIT, QUEUE_EVENT_COUNT, 4);

K_WORK_DEFINE(health_check_work, health_monitor_healthcheck);
K_WORK_DEFINE(health_raport_work, health_monitor_raport);
K_WORK_DEFINE(health_notify_work, health_monitor_process);

static uint32_t health_raport_period = CONFIG_HEALTH_MONITOR_RAPORT_PERIOD;

static health_monitor_event_callback	event_callback;
static void				*callback_context;

static struct counters_data  counters;
static struct k_work_q      *monitor_workq;

static struct test_api_data periodic_handlers[] = {
	{test_srp, CONFIG_HEALTH_MONITOR_SRP_TIMEOUT},
	{test_singleton_leader, CONFIG_HEALTH_MONITOR_SINGLETON_LEADER_TIMEOUT},
	{NULL}
};

static void test_with_timeout(struct test_api_data *data)
{
	if (data != NULL && data->verify_and_notify_handler != NULL) {
		bool active;
		bool send = false;
		int64_t now = k_uptime_get();

		if (((data->next_timeout > 0) && (data->next_timeout < now))
			|| data->required_time == 0) {

			send = !data->reported;
		}

		active = data->verify_and_notify_handler(send);

		if (active == false) {
			data->reported = false;
			data->next_timeout = 0;
		} else if (send) {
			data->reported = true;
		} else if (data->next_timeout == 0 && data->required_time > 0) {
			data->next_timeout = now + data->required_time;
		}
	}
}

static bool test_srp(bool notify)
{
	bool active = false;

	openthread_mutex_lock();
	if (otSrpClientIsRunning(openthread_get_default_instance())) {
		const otSrpClientService *service =
			otSrpClientGetServices(openthread_get_default_instance());

		while (service != NULL) {
			if (service->mState != OT_SRP_CLIENT_ITEM_STATE_REGISTERED
			    && service->mState != OT_SRP_CLIENT_ITEM_STATE_REMOVED) {
				active = true;
				break;
			}

			service = service->mNext;
		}
	}
	openthread_mutex_unlock();

	if (active && notify) {
		struct health_event_data event = {HEALTH_EVENT_SRP_REG};

		post_event(&event);
	}
	return active;
}

static bool test_singleton_leader(bool notify)
{
	bool active = false;

	openthread_mutex_lock();
	otDeviceRole otRole = otThreadGetDeviceRole(openthread_get_default_instance());

	if (otRole == OT_DEVICE_ROLE_LEADER) {
		otNeighborInfoIterator iterator = OT_NEIGHBOR_INFO_ITERATOR_INIT;
		otNeighborInfo         neighborInfo;
		otError                error;

		error = otThreadGetNextNeighborInfo(openthread_get_default_instance(),
						    &iterator,
						    &neighborInfo);

		if (error == OT_ERROR_NOT_FOUND) {
			active = true;
		}
	}
	openthread_mutex_unlock();

	if (active && notify) {
		struct health_event_data event = {HEALTH_EVENT_NO_NEIGBOURS};

		post_event(&event);
	}
	return active;

}

static void ot_state_changed(otChangedFlags aFlags, void *aContext)
{
	if (aFlags & OT_CHANGED_THREAD_ROLE) {
		counters.state_changes++;
	}

	if (aFlags & OT_CHANGED_THREAD_CHILD_ADDED) {
		counters.child_added++;
	}

	if (aFlags & OT_CHANGED_THREAD_CHILD_REMOVED) {
		counters.child_removed++;
	}

	if (aFlags & OT_CHANGED_THREAD_PARTITION_ID) {
		counters.partition_id_changes++;
	}

	if (aFlags & OT_CHANGED_THREAD_KEY_SEQUENCE_COUNTER) {
		counters.key_sequence_changes++;
	}

	if (aFlags & OT_CHANGED_THREAD_NETDATA) {
		counters.network_data_changes++;
	}

	if (aFlags & OT_CHANGED_ACTIVE_DATASET) {
		counters.active_dataset_changes++;
	}

	if (aFlags & OT_CHANGED_PENDING_DATASET) {
		counters.pending_dataset_changes++;
	}
}


static void health_monitor_healthcheck(struct k_work *item)
{
	for (int i = 0; periodic_handlers[i].verify_and_notify_handler != NULL; i++) {
		test_with_timeout(&periodic_handlers[i]);
	}
}

static void health_check_timeout(struct k_timer *timer_id)
{
	k_work_submit_to_queue(monitor_workq, &health_check_work);
}

static void health_monitor_raport(struct k_work *item)
{
	struct health_event_data event;

	event.event_type = HEALTH_EVENT_STATS;
	event.counters = &counters;

	post_event(&event);
}

static void health_raport_timeout(struct k_timer *timer_id)
{
	k_work_submit_to_queue(monitor_workq, &health_raport_work);
}

static void post_event(struct health_event_data *event)
{
	int error;

	error = k_msgq_put(&event_queue, event, K_NO_WAIT);
	if (error == 0) {
		error = k_work_submit_to_queue(monitor_workq, &health_notify_work);
	}

	if (error < 0) {
		LOG_WRN("Unable to post event: %d", error);
	}
}

static void health_monitor_process(struct k_work *item)
{
	struct health_event_data event;

	while (k_msgq_get(&event_queue, &event, K_NO_WAIT) == 0) {
		if (event_callback) {
			event_callback(&event, callback_context);
		}
	}
}

void health_monitor_init(struct k_work_q *workqueue)
{
	otError error;

	monitor_workq = workqueue;

	LOG_INF("Initializing Health Monitor:\n\thealthcheck period %ums\n\t raport period %u",
		CONFIG_HEALTH_MONITOR_HEALTHCHECK_PERIOD,
		health_raport_period/1000);
	openthread_mutex_lock();
	error = otSetStateChangedCallback(openthread_get_default_instance(),
					  ot_state_changed,
					  NULL);
	openthread_mutex_unlock();

	if (error != OT_ERROR_NONE) {
		LOG_WRN("Failed to register for OpenThread notifications: %d", error);
	}

	k_timer_start(&health_check_timer,
		       K_MSEC(CONFIG_HEALTH_MONITOR_HEALTHCHECK_PERIOD),
		       K_MSEC(CONFIG_HEALTH_MONITOR_HEALTHCHECK_PERIOD));

	if (health_raport_period > 0) {
		k_timer_start(&health_raport_timer,
			       K_MSEC(health_raport_period),
			       K_MSEC(health_raport_period));
	}

}

void health_monitor_set_raport_interval(uint32_t seconds)
{
	health_raport_period = seconds*1000;

	if (health_raport_period > 0) {
		k_timer_start(&health_raport_timer,
			       K_MSEC(health_raport_period),
			       K_MSEC(health_raport_period));
	} else {
		k_timer_stop(&health_raport_timer);
	}
}

void health_monitor_set_event_callback(health_monitor_event_callback callback, void *context)
{
	event_callback = callback;
	callback_context = context;
}
