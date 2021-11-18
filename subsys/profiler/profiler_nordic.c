/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <kernel_structs.h>
#include <sys/util.h>
#include <sys/byteorder.h>
#include <zephyr.h>
#include <SEGGER_RTT.h>
#include <profiler.h>
#include <string.h>
#include <nrfx.h>


enum state {
	STATE_DISABLED,
	STATE_INACTIVE,
	STATE_ACTIVE,
	STATE_TERMINATED,
};

/* By default, when there is no shell, all events are profiled. */
struct profiler_event_enabled_bm _profiler_event_enabled_bm;

static K_SEM_DEFINE(profiler_sem, 0, 1);
static atomic_t profiler_state;
static uint16_t fatal_error_event_id;
static struct k_spinlock lock;

enum nordic_command {
	NORDIC_COMMAND_START	= 1,
	NORDIC_COMMAND_STOP	= 2,
	NORDIC_COMMAND_INFO	= 3
};

char descr[PROFILER_MAX_NUMBER_OF_APPLICATION_AND_INTERNAL_EVENTS]
	  [CONFIG_PROFILER_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS];
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

	while (num_bytes_send != data_len) {
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
		(void)send_info_data(&end_line, 1);
	}
}

static void profiler_nordic_thread_fn(void)
{
	while (atomic_get(&profiler_state) != STATE_TERMINATED) {
		uint8_t read_data;
		enum nordic_command command;

		if (SEGGER_RTT_Read(
		     CONFIG_PROFILER_NORDIC_RTT_CHANNEL_COMMANDS,
		     &read_data, sizeof(read_data))) {
			command = (enum nordic_command)read_data;
			switch (command) {
			case NORDIC_COMMAND_START:
				atomic_cas(&profiler_state, STATE_INACTIVE, STATE_ACTIVE);
				break;
			case NORDIC_COMMAND_STOP:
				atomic_cas(&profiler_state, STATE_ACTIVE, STATE_INACTIVE);
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
	k_sched_lock();

	if (!atomic_cas(&profiler_state, STATE_DISABLED, STATE_INACTIVE)) {
		k_sched_unlock();
		return 0;
	}

	if (!IS_ENABLED(CONFIG_SHELL)) {
		for (size_t i = 0; i < PROFILER_MAX_NUMBER_OF_APPLICATION_AND_INTERNAL_EVENTS;
		     i++) {
			atomic_set_bit(_profiler_event_enabled_bm.flags, i);
		}
	}

	if (IS_ENABLED(CONFIG_PROFILER_NORDIC_START_LOGGING_ON_SYSTEM_START)) {
		atomic_cas(&profiler_state, STATE_INACTIVE, STATE_ACTIVE);
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

	/* Registering fatal error event */
	fatal_error_event_id = profiler_register_event_type("_profiler_fatal_error_event_",
							    NULL, NULL, 0);

	k_sched_unlock();
	return 0;
}

void profiler_term(void)
{
	if (atomic_set(&profiler_state, STATE_TERMINATED) == STATE_TERMINATED) {
		/* Already terminated. */
		return;
	}

	k_wakeup(protocol_thread_id);
	k_sem_take(&profiler_sem, K_FOREVER);
}

const char *profiler_get_event_descr(size_t profiler_event_id)
{
	return descr[profiler_event_id];
}

uint16_t profiler_register_event_type(const char *name, const char * const *args,
				   const enum profiler_arg *arg_types,
				   uint8_t arg_cnt)
{
	/* Lock to make sure that this function can be called
	 * from multiple threads
	 */
	k_sched_lock();
	uint8_t ne = profiler_num_events;

	__ASSERT_NO_MSG(ne + 1 <= PROFILER_MAX_NUMBER_OF_APPLICATION_AND_INTERNAL_EVENTS);
	size_t temp = snprintf(descr[ne],
			CONFIG_PROFILER_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS,
			"%s,%d", name, ne);
	size_t pos = temp;

	__ASSERT_NO_MSG((pos < CONFIG_PROFILER_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS)
			 && (temp > 0));

	for (size_t t = 0; t < arg_cnt; t++) {
		temp = snprintf(descr[ne] + pos,
			 CONFIG_PROFILER_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos,
			 ",%s", arg_types_encodings[arg_types[t]]);
		pos += temp;
		__ASSERT_NO_MSG(
		  (pos < CONFIG_PROFILER_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS)
		   && (temp > 0));
	}

	for (size_t t = 0; t < arg_cnt; t++) {
		temp = snprintf(descr[ne] + pos,
			CONFIG_PROFILER_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS - pos,
			",%s", args[t]);
		pos += temp;
		__ASSERT_NO_MSG(
		  (pos < CONFIG_PROFILER_MAX_LENGTH_OF_CUSTOM_EVENTS_DESCRIPTIONS)
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
	buf->payload = buf->payload_start + sizeof(uint8_t);
	profiler_log_encode_uint32(buf, k_cycle_get_32());
}

void profiler_log_encode_uint32(struct log_event_buf *buf, uint32_t data)
{
	__ASSERT_NO_MSG(buf->payload - buf->payload_start + sizeof(data)
			 <= CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN);
	sys_put_le32(data, buf->payload);
	buf->payload += sizeof(data);
}

void profiler_log_encode_int32(struct log_event_buf *buf, int32_t data)
{
	profiler_log_encode_uint32(buf, (uint32_t)data);
}

void profiler_log_encode_uint16(struct log_event_buf *buf, uint16_t data)
{
	__ASSERT_NO_MSG(buf->payload - buf->payload_start + sizeof(data)
			 <= CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN);
	sys_put_le16(data, buf->payload);
	buf->payload += sizeof(data);
}

void profiler_log_encode_int16(struct log_event_buf *buf, int16_t data)
{
	profiler_log_encode_uint16(buf, (uint16_t)data);
}

void profiler_log_encode_uint8(struct log_event_buf *buf, uint8_t data)
{
	__ASSERT_NO_MSG(buf->payload - buf->payload_start + sizeof(data)
			 <= CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN);
	*(buf->payload) = data;
	buf->payload += sizeof(data);
}

void profiler_log_encode_int8(struct log_event_buf *buf, int8_t data)
{
	profiler_log_encode_uint8(buf, (uint8_t)data);
}

void profiler_log_encode_string(struct log_event_buf *buf, const char *string)
{
	size_t string_len = strlen(string);

	if (string_len > UINT8_MAX) {
		string_len = UINT8_MAX;
	}
	/* First byte that is send denotes string length.
	 * Null character is not being sent.
	 */
	__ASSERT_NO_MSG(buf->payload - buf->payload_start + sizeof(uint8_t) + string_len
			 <= CONFIG_PROFILER_CUSTOM_EVENT_BUF_LEN);
	*(buf->payload) = (uint8_t) string_len;
	buf->payload++;

	memcpy(buf->payload, string, string_len);
	buf->payload += string_len;
}

void profiler_log_add_mem_address(struct log_event_buf *buf,
				  const void *mem_address)
{
	profiler_log_encode_uint32(buf, (uint32_t)mem_address);
}

static bool profiler_RTT_send(struct log_event_buf *buf, uint8_t type_id)
{
	buf->payload_start[0] = type_id;
	size_t data_len = buf->payload - buf->payload_start;

	size_t num_bytes_send = SEGGER_RTT_WriteNoLock(
			CONFIG_PROFILER_NORDIC_RTT_CHANNEL_DATA,
			buf->payload_start, data_len);
	return (num_bytes_send == data_len);
}

static void profiler_fatal_error(void)
{
	struct log_event_buf buf;

	profiler_log_start(&buf);
	while (true) {
		/* Sending Fatal Error event */
		if (profiler_RTT_send(&buf, (uint8_t)fatal_error_event_id)) {
			break;
		}
	}
	k_oops();
}

void profiler_log_send(struct log_event_buf *buf, uint16_t event_type_id)
{
	__ASSERT_NO_MSG(event_type_id <= UINT8_MAX);

	if (atomic_get(&profiler_state) == STATE_ACTIVE) {
		uint8_t type_id = event_type_id & UINT8_MAX;

		k_spinlock_key_t key = k_spin_lock(&lock);

		if (!profiler_RTT_send(buf, type_id)) {
			profiler_fatal_error();
		}
		k_spin_unlock(&lock, key);
	}
}
