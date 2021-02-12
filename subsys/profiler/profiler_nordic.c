/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <kernel_structs.h>
#include <sys/printk.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <SEGGER_RTT.h>
#include <profiler.h>
#include <string.h>


/* By default, when there is no shell, all events are profiled. */
#ifndef CONFIG_SHELL
uint32_t profiler_enabled_events = 0xffffffff;
#endif


static K_SEM_DEFINE(profiler_sem, 0, 1);
static bool protocol_running;
static bool sending_events;

enum nordic_command {
	NORDIC_COMMAND_START	= 1,
	NORDIC_COMMAND_STOP	= 2,
	NORDIC_COMMAND_INFO	= 3
};

char descr[CONFIG_MAX_NUMBER_OF_CUSTOM_EVENTS]
	  [CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS];
static char *arg_types_encodings[] = {
					"u8",  /* uint8_t */
					"s8",  /* int8_t */
					"u16", /* uint16_t */
					"s16", /* int16_t */
					"u32", /* uint32_t */
					"s32", /* int32_t */
					"s",   /* string */
					"t"    /* time */
				     };

uint8_t profiler_num_events;

static uint8_t buffer_data[CONFIG_PROFILER_NORDIC_DATA_BUFFER_SIZE];
static uint8_t buffer_info[CONFIG_PROFILER_NORDIC_INFO_BUFFER_SIZE];
static uint8_t buffer_commands[CONFIG_PROFILER_NORDIC_COMMAND_BUFFER_SIZE];

static k_tid_t protocol_thread_id;

static K_THREAD_STACK_DEFINE(profiler_nordic_stack,
			     CONFIG_PROFILER_NORDIC_STACK_SIZE);
static struct k_thread profiler_nordic_thread;

static int send_info_data(const char *data, size_t data_len)
{
	uint8_t retry_cnt = 0;
	static const uint8_t retry_cnt_max = 100;

	size_t num_bytes_send;

	num_bytes_send = SEGGER_RTT_WriteNoLock(
				  CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO,
				  data, data_len);

	while (num_bytes_send == 0) {
		/* Give host time to read the data and free some space
		 * in the buffer. */
		k_sleep(K_MSEC(100));
		num_bytes_send = SEGGER_RTT_WriteNoLock(
				  CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO,
				  data, data_len);

		/* Avoid being blocked in while loop if host does not read
		 * the RTT data.
		 */
		retry_cnt++;
		if (retry_cnt > retry_cnt_max) {
			return -ENOBUFS;
		}
	}

	return 0;
}

static void send_system_description(void)
{
	/* Memory barrier to make sure that data is visible
	 * before being accessed
	 */
	uint8_t ne = profiler_num_events;

	__DMB();
	char end_line = '\n';
	int err = 0;

	for (size_t t = 0; ((t < ne) && !err); t++) {
		err = send_info_data(descr[t], strlen(descr[t]));
		if (!err) {
			err = send_info_data(&end_line, 1);
		}
	}

	if (!err) {
		err = send_info_data(&end_line, 1);
	}
}

static void profiler_nordic_thread_fn(void)
{
	while (protocol_running) {
		uint8_t read_data;
		enum nordic_command command;

		if (SEGGER_RTT_Read(
		     CONFIG_PROFILER_NORDIC_RTT_CHANNEL_COMMANDS,
		     &read_data, sizeof(read_data))) {
			command = (enum nordic_command)read_data;
			switch (command) {
			case NORDIC_COMMAND_START:
				sending_events = true;
				break;
			case NORDIC_COMMAND_STOP:
				sending_events = false;
				break;
			case NORDIC_COMMAND_INFO:
				send_system_description();
				break;
			default:
				__ASSERT_NO_MSG(false);
				break;
			}
		}
		k_sleep(K_MSEC(500));
	}
	k_sem_give(&profiler_sem);
}

int profiler_init(void)
{
	protocol_running = true;
	if (IS_ENABLED(CONFIG_PROFILER_NORDIC_START_LOGGING_ON_SYSTEM_START)) {
		sending_events = true;
	}
	int ret;

	ret = SEGGER_RTT_ConfigUpBuffer(
		CONFIG_PROFILER_NORDIC_RTT_CHANNEL_DATA,
		"Nordic profiler data",
		buffer_data,
		CONFIG_PROFILER_NORDIC_DATA_BUFFER_SIZE,
		SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	__ASSERT_NO_MSG(ret >= 0);

	ret = SEGGER_RTT_ConfigUpBuffer(
		CONFIG_PROFILER_NORDIC_RTT_CHANNEL_INFO,
		"Nordic profiler info",
		buffer_info,
		CONFIG_PROFILER_NORDIC_INFO_BUFFER_SIZE,
		SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	__ASSERT_NO_MSG(ret >= 0);

	ret = SEGGER_RTT_ConfigDownBuffer(
		CONFIG_PROFILER_NORDIC_RTT_CHANNEL_COMMANDS,
		"Nordic profiler command",
		buffer_commands,
		CONFIG_PROFILER_NORDIC_COMMAND_BUFFER_SIZE,
		SEGGER_RTT_MODE_NO_BLOCK_SKIP);
	__ASSERT_NO_MSG(ret >= 0);

	protocol_thread_id =  k_thread_create(&profiler_nordic_thread,
			profiler_nordic_stack,
			K_THREAD_STACK_SIZEOF(profiler_nordic_stack),
			(k_thread_entry_t) profiler_nordic_thread_fn,
			NULL, NULL, NULL,
			CONFIG_PROFILER_NORDIC_THREAD_PRIORITY, 0, K_NO_WAIT);
	return 0;
}

void profiler_term(void)
{
	sending_events = false;
	protocol_running = false;
	k_wakeup(protocol_thread_id);
	k_sem_take(&profiler_sem, K_FOREVER);
}

const char *profiler_get_event_descr(size_t profiler_event_id)
{
	return descr[profiler_event_id];
}

uint16_t profiler_register_event_type(const char *name, const char **args,
				   const enum profiler_arg *arg_types,
				   uint8_t arg_cnt)
{
	/* Lock to make sure that this function can be called
	 * from multiple threads
	 */
	k_sched_lock();
	uint8_t ne = profiler_num_events;
	size_t temp = snprintf(descr[ne],
			CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS,
			"%s,%d", name, ne);
	size_t pos = temp;

	__ASSERT_NO_MSG((pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS)
			 && (temp > 0));

	for (size_t t = 0; t < arg_cnt; t++) {
		temp = snprintf(descr[ne] + pos,
			 CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos,
			 ",%s", arg_types_encodings[arg_types[t]]);
		pos += temp;
		__ASSERT_NO_MSG(
		  (pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS)
		   && (temp > 0));
	}

	for (size_t t = 0; t < arg_cnt; t++) {
		temp = snprintf(descr[ne] + pos,
			CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos,
			",%s", args[t]);
		pos += temp;
		__ASSERT_NO_MSG(
		  (pos < CONFIG_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS)
		   && (temp > 0));
	}
	/* Memory barrier to make sure that data is visible
	 * before being accessed
	 */
	__DMB();
	profiler_num_events++;
	k_sched_unlock();

	return ne;
}

void profiler_log_start(struct log_event_buf *buf)
{
	/* Adding one to pointer to make space for event type ID */
	__ASSERT_NO_MSG(sizeof(uint8_t) <= CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN);
	buf->payload = buf->payload_start + sizeof(uint8_t);
	profiler_log_encode_u32(buf, k_cycle_get_32());
}

void profiler_log_encode_u32(struct log_event_buf *buf, uint32_t data)
{
	__ASSERT_NO_MSG(buf->payload - buf->payload_start + sizeof(data)
			 <= CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN);
	sys_put_le32(data, buf->payload);
	buf->payload += sizeof(data);
}

void profiler_log_add_mem_address(struct log_event_buf *buf,
				  const void *mem_address)
{
	profiler_log_encode_u32(buf, (uint32_t)mem_address);
}

void profiler_log_send(struct log_event_buf *buf, uint16_t event_type_id)
{
	__ASSERT_NO_MSG(event_type_id <= UCHAR_MAX);
	if (sending_events) {
		uint8_t type_id = event_type_id & UCHAR_MAX;

		buf->payload_start[0] = type_id;
		int key = irq_lock();

		uint8_t num_bytes_send = SEGGER_RTT_WriteNoLock(
				CONFIG_PROFILER_NORDIC_RTT_CHANNEL_DATA,
				buf->payload_start,
				buf->payload - buf->payload_start);
		ARG_UNUSED(num_bytes_send);
		irq_unlock(key);
		__ASSERT_NO_MSG(num_bytes_send > 0);
	}
}
