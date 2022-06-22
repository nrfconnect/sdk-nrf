/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>
#include <app_event_manager.h>

#include <caf/events/sensor_event.h>
#include <caf/events/sensor_data_aggregator_event.h>
#include <caf/sensor_manager.h>

#define MODULE sensor_data_aggregator
#include <caf/events/module_state_event.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_CAF_SENSOR_DATA_AGGREGATOR_LOG_LEVEL);

#define DT_DRV_COMPAT caf_aggregator

#define __DEFINE_DATA(i, agg_id, size)							\
	static uint8_t agg_ ## agg_id ## _buff_ ## i ## _data[size] __aligned(4)

#if (DT_INST_NODE_HAS_PROP(agg_id, memory_region))

#define __INITIALIZE_BUF(i, agg_id)							\
	{										\
		(uint8_t *) DT_REG_ADDR(DT_INST_PHANDLE(agg_id, memory_region)) +	\
		i * DT_INST_PROP(agg_id, buf_data_length)				\
	},



/* buf_len has to be equal to n*sensor_data_size. */
#define __DEFINE_BUF_DATA(i)									\
	static struct aggregator_buffer agg_ ## i ## _bufs[] = {				\
		UTIL_LISTIFY(DT_INST_PROP(i, buf_count), __INITIALIZE_BUF, i)			\
	};											\
	BUILD_ASSERT((DT_INST_PROP(i, buf_data_length) % DT_INST_PROP(i, sensor_data_size)) == 0,\
		"Wrong sensor data or buffer size");

#else
#define __INITIALIZE_BUF(i, agg_id)							\
	{(uint8_t *) &agg_ ## agg_id ## _buff_ ## i ## _data}

/* buf_len has to be equal to n*sensor_data_size. */
#define __DEFINE_BUF_DATA(i)									\
	LISTIFY(DT_INST_PROP(i, buf_count), __DEFINE_DATA, (;), i,				\
		DT_INST_PROP(i, buf_data_length));						\
	static struct aggregator_buffer agg_ ## i ## _bufs[] = {				\
		LISTIFY(DT_INST_PROP(i, buf_count), __INITIALIZE_BUF, (,), i)			\
	};											\
	BUILD_ASSERT((DT_INST_PROP(i, buf_data_length) % DT_INST_PROP(i, sensor_data_size)) == 0,\
		"Wrong sensor data or buffer size");

#endif

#define __DEFINE_AGGREGATOR(i)								\
	[i].sensor_descr = DT_INST_PROP(i, sensor_descr),				\
	[i].sensor_data_size = DT_INST_PROP(i, sensor_data_size),			\
	[i].buf_count = DT_INST_PROP(i, buf_count),					\
	[i].buf_len = DT_INST_PROP(i, buf_data_length),					\
	[i].agg_buffers = (struct aggregator_buffer *) &agg_ ## i ## _bufs,		\
	[i].active_buf = (struct aggregator_buffer *) &agg_ ## i ## _bufs,

#define __DEFINE_AGG							\
	static struct aggregator aggregators[] = {			\
		DT_INST_FOREACH_STATUS_OKAY(__DEFINE_AGGREGATOR)	\
	}

struct aggregator_buffer {
	uint8_t *data;		/* Dynamic data. */
	bool busy;		/* Buffer status. */
	uint8_t sample_cnt;	/* Number of samples already saved in the buffer. */
};

struct aggregator {
	const char *sensor_descr;		/* sensor_description of the sensor. */
	struct aggregator_buffer *agg_buffers;	/* Buffers. */
	struct aggregator_buffer *active_buf;	/* Active buffer to which data will be placed. */
	enum sensor_state sensor_state;		/* Sensors state. */
	const uint8_t sensor_data_size;		/* Size of sensor data in bytes. */
	const uint8_t buf_count;		/* Number of buffers. */
	const uint8_t buf_len;			/* Size of buffor data in bytes. */
};


DT_INST_FOREACH_STATUS_OKAY(__DEFINE_BUF_DATA) /* no semicolon on purpose. */
static struct aggregator aggregators[] = {
	DT_INST_FOREACH_STATUS_OKAY(__DEFINE_AGGREGATOR)
};


static struct aggregator_buffer *get_free_buffer(struct aggregator *agg)
{
	for (size_t i = 0; i < agg->buf_count; i++) {
		if (!agg->agg_buffers[i].busy) {
			return &agg->agg_buffers[i];
		}
	}
	return NULL;
}

static struct aggregator *get_aggregator(const char *sensor_descr)
{
	for (size_t i = 0; i < ARRAY_SIZE(aggregators); i++) {
		if (sensor_descr == aggregators[i].sensor_descr) {
			return &aggregators[i];
		}
	}
	return NULL;
}

static void release_buffer(struct aggregator *agg, struct aggregator_buffer *ab)
{
	__ASSERT_NO_MSG(ab);

	ab->sample_cnt = 0;
	ab->busy = false;
	if (agg->active_buf == NULL) {
		agg->active_buf = ab;
	}
}

static void send_buffer(struct aggregator *agg, struct aggregator_buffer *ab)
{
	ab->busy = true;
	struct sensor_data_aggregator_event *event = new_sensor_data_aggregator_event();

	event->buf = ab->data;
	event->sample_cnt = ab->sample_cnt;
	event->sensor_state = agg->sensor_state;
	event->sensor_descr = agg->sensor_descr;
	APP_EVENT_SUBMIT(event);
}

static int enqueue_sample(struct aggregator *agg, struct sensor_event *event)
{
	if (event->dyndata.size != agg->sensor_data_size) {
		return -EBADMSG;
	}
	if (!agg->active_buf) {
		return -ENOMEM;
	}
	struct aggregator_buffer *ab = agg->active_buf;

	size_t pos = ab->sample_cnt * agg->sensor_data_size;
	size_t avail = agg->buf_len - pos;

	if (avail < agg->sensor_data_size) {
		__ASSERT_NO_MSG(false);
		return -ENOMEM;
	}
	memcpy(&ab->data[pos], event->dyndata.data, event->dyndata.size);
	ab->sample_cnt++;
	avail -= event->dyndata.size;

	if (avail < agg->sensor_data_size) {
		send_buffer(agg, ab);
		agg->active_buf = get_free_buffer(agg);
	}

	return 0;
}

static bool event_handler(const struct app_event_header *aeh)
{
	if (is_sensor_event(aeh)) {
		struct sensor_event *event = cast_sensor_event(aeh);
		struct aggregator *agg = get_aggregator(event->descr);

		if (agg) {
			int err = enqueue_sample(agg, event);

			if (err) {
				LOG_ERR("Error code: %d", err);
			}
		} else {
			LOG_WRN("Dropped sample: %s. Found no adequate aggregator.",
				event->descr);
		}
		return false;
	}

	if (is_sensor_data_aggregator_release_buffer_event(aeh)) {
		const struct sensor_data_aggregator_release_buffer_event *event =
				cast_sensor_data_aggregator_release_buffer_event(aeh);
		struct aggregator *agg = get_aggregator(event->sensor_descr);

		__ASSERT_NO_MSG(agg);

		for (size_t i = 0; i < agg->buf_count; i++) {
			if (agg->agg_buffers[i].data == event->buf) {
				release_buffer(agg, &agg->agg_buffers[i]);
				break;
			}
		}

		return false;
	}

	if (is_sensor_state_event(aeh)) {
		struct sensor_state_event *event = cast_sensor_state_event(aeh);
		struct aggregator *agg = get_aggregator(event->descr);

		if (agg) {
			struct aggregator_buffer *ab = agg->active_buf;

			agg->sensor_state = event->state;
			send_buffer(agg, ab);
			agg->active_buf = get_free_buffer(agg);
		}

		return false;
	}

	/* If event is unhandled, unsubscribe. */
	__ASSERT_NO_MSG(false);

	return false;
}

APP_EVENT_LISTENER(MODULE, event_handler);
APP_EVENT_SUBSCRIBE(MODULE, sensor_data_aggregator_release_buffer_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_state_event);
APP_EVENT_SUBSCRIBE(MODULE, sensor_event);
