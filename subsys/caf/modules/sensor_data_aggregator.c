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

/* The name of the structure array that describes the aggregator buffers */
#define __AGG_BUFFS_NAME(agg_node) DT_CAT3(agg_, agg_node, _buffs)

/* This macros are used only if no memory region is used and the aggregator buffers are created
 * in BSS.
 */
#define __DATA_BUFF_NAME(agg_node, n) DT_CAT5(agg_, agg_node,  _buff_,  n,  _data)
#define __DEFINE_DATA(n, agg_node, size)                         \
	static struct sensor_value __DATA_BUFF_NAME(agg_node, n) \
		[DIV_ROUND_UP(size, sizeof(struct sensor_value))]
/* End of BSS version only macros. */

#define __INITIALIZE_BUFF(n, agg_node)                                                        \
	COND_CODE_1(DT_NODE_HAS_PROP(agg_node, memory_region),                                \
		({(struct sensor_value *) (DT_REG_ADDR(DT_PHANDLE(agg_node, memory_region)) + \
			n * (DT_PROP(agg_node, buf_data_length)))}),                          \
		({__DATA_BUFF_NAME(agg_node, n)})                                             \
	)

#define __XDEFINE_BUF_DATA(agg_node)                                                    \
	COND_CODE_0(DT_NODE_HAS_PROP(agg_node, memory_region),                          \
		(LISTIFY(DT_PROP(agg_node, buf_count), __DEFINE_DATA, (;),              \
			agg_node, DT_PROP(agg_node, buf_data_length));),                \
		()                                                                      \
	)                                                                               \
	static struct aggregator_buffer __AGG_BUFFS_NAME(agg_node)[] = {                \
		LISTIFY(DT_PROP(agg_node, buf_count), __INITIALIZE_BUFF, (,), agg_node) \
	};                                                                              \
	BUILD_ASSERT((DT_PROP(agg_node, buf_data_length) %                              \
		(DT_PROP(agg_node, sample_size) * sizeof(struct sensor_value))) == 0,   \
		"Wrong sensor data or buffer size in " DT_NODE_FULL_NAME(agg_node));

#define __DEFINE_BUF_DATA(i) __XDEFINE_BUF_DATA(DT_DRV_INST(i))

#define __DEFINE_AGGREGATOR(i)                               \
	[i].sensor_descr = DT_INST_PROP(i, sensor_descr),    \
	[i].values_in_sample = DT_INST_PROP(i, sample_size), \
	[i].buf_count = DT_INST_PROP(i, buf_count),          \
	[i].buf_len = DT_INST_PROP(i, buf_data_length),      \
	[i].agg_buffers = __AGG_BUFFS_NAME(DT_DRV_INST(i)),  \
	[i].active_buf  = __AGG_BUFFS_NAME(DT_DRV_INST(i)),


struct aggregator_buffer {
	struct sensor_value *samples;	/* Dynamic data. */
	bool busy;			/* Buffer status. */
	uint8_t sample_cnt;		/* Number of samples already saved in the buffer. */
};

struct aggregator {
	const char *sensor_descr;		/* sensor_description of the sensor. */
	struct aggregator_buffer *agg_buffers;	/* Buffers. */
	struct aggregator_buffer *active_buf;	/* Active buffer to which data will be placed. */
	enum sensor_state sensor_state;		/* Sensors state. */
	const uint8_t values_in_sample;		/* Number of sensor values in a sample. */
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
	event->values_in_sample = agg->values_in_sample;
	event->samples = ab->samples;
	event->sample_cnt = ab->sample_cnt;
	event->sensor_state = agg->sensor_state;
	event->sensor_descr = agg->sensor_descr;
	APP_EVENT_SUBMIT(event);
}

static int enqueue_sample(struct aggregator *agg, struct sensor_event *event)
{
	size_t chunk_bytes = agg->values_in_sample * sizeof(struct sensor_value);

	if ((event->dyndata.size) != chunk_bytes) {
		return -EBADMSG;
	}
	if (!agg->active_buf) {
		return -ENOMEM;
	}

	struct aggregator_buffer *ab = agg->active_buf;
	size_t pos_values = ab->sample_cnt * agg->values_in_sample;
	size_t avail_bytes = agg->buf_len - pos_values * sizeof(struct sensor_value);

	if (avail_bytes < chunk_bytes) {
		__ASSERT_NO_MSG(false);
		return -ENOMEM;
	}
	memcpy(&ab->samples[pos_values], (uint8_t *)event->dyndata.data, chunk_bytes);
	ab->sample_cnt++;
	avail_bytes -= chunk_bytes;

	if (avail_bytes < chunk_bytes) {
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
			if (agg->agg_buffers[i].samples == event->samples) {
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
