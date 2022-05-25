/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include "config_channel_transport.h"
#include "hid_report_desc.h"

#define MODULE config_channel_transport
#define TRANSPORT_HEADER_SIZE		4
#define CONFIG_STATUS_POS		2

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(MODULE, CONFIG_DESKTOP_CONFIG_CHANNEL_LOG_LEVEL);

static int frame_length_check(size_t length)
{
	BUILD_ASSERT(TRANSPORT_HEADER_SIZE < REPORT_SIZE_USER_CONFIG);

	const size_t min_size = TRANSPORT_HEADER_SIZE;
	const size_t max_size = REPORT_SIZE_USER_CONFIG;

	if ((length < min_size) || (length > max_size)) {
		LOG_WRN("Unsupported report length %zu", length);
		return -EMSGSIZE;
	}

	return 0;
}

static int data_len_check(size_t event_data_len)
{
	BUILD_ASSERT(TRANSPORT_HEADER_SIZE < REPORT_SIZE_USER_CONFIG);

	const size_t min_size = TRANSPORT_HEADER_SIZE;
	const size_t max_size = REPORT_SIZE_USER_CONFIG;

	if (event_data_len > max_size - min_size) {
		LOG_WRN("Unsupported event data length %" PRIu8,
			event_data_len);
		return -EMSGSIZE;
	}

	return 0;
}

static void fill_response_pending(uint8_t *buffer)
{
	memset(buffer, 0, TRANSPORT_HEADER_SIZE);
	buffer[CONFIG_STATUS_POS] = CONFIG_STATUS_PENDING;
}

int config_channel_report_parse(const uint8_t *buffer, size_t length,
				struct config_event *event)
{
	int err = frame_length_check(length);

	if (err) {
		return err;
	}

	size_t pos = 0;

	event->recipient = buffer[pos];
	pos += sizeof(event->recipient);

	event->event_id = buffer[pos];
	pos += sizeof(event->event_id);

	__ASSERT_NO_MSG(pos == CONFIG_STATUS_POS);

	event->status = buffer[pos];
	pos += sizeof(event->status);

	uint8_t config_data_len = buffer[pos];
	pos += sizeof(config_data_len);

	__ASSERT_NO_MSG(pos == TRANSPORT_HEADER_SIZE);

	err = data_len_check(config_data_len);
	if (err) {
		return err;
	}

	if ((length - pos) < config_data_len) {
		LOG_ERR("Not enough data in buffer");
		return -EMSGSIZE;
	}

	if (event->dyndata.size < config_data_len) {
		LOG_ERR("Invalid packet size %zu < %zu", event->dyndata.size,
			config_data_len);
		return -ENOMEM;
	}
	event->dyndata.size = config_data_len;

	memcpy(event->dyndata.data, &buffer[pos], config_data_len);
	pos += config_data_len;

	return pos;
}

int config_channel_report_fill(uint8_t *buffer, const size_t length,
			       const struct config_event *event)
{
	int err = frame_length_check(length);
	if (!err) {
		err = data_len_check(event->dyndata.size);
	}

	if (err) {
		return err;
	}

	size_t pos = 0;

	buffer[pos] = event->recipient;
	pos += sizeof(event->recipient);

	buffer[pos] = event->event_id;
	pos += sizeof(event->event_id);

	__ASSERT_NO_MSG(pos == CONFIG_STATUS_POS);

	buffer[pos] = event->status;
	pos += sizeof(event->status);

	buffer[pos] = (uint8_t)event->dyndata.size;
	pos += sizeof(buffer[pos]);

	__ASSERT_NO_MSG(pos == TRANSPORT_HEADER_SIZE);
	__ASSERT_NO_MSG(pos + event->dyndata.size <= length);

	if (event->dyndata.size > 0) {
		memcpy(&buffer[pos], event->dyndata.data, event->dyndata.size);
		pos += event->dyndata.size;
	}

	return pos;
}

static void drop_transactions(struct config_channel_transport *transport)
{
	/* Transport ID is changed, responses for submitted requests will
	 * be dropped.
	 */
	if ((transport->transport_id & 0xFF) == 0xFF) {
		transport->transport_id &= ~0xFF;
	}
	transport->transport_id++;
}

static void timeout_fn(struct k_work *work)
{
	struct config_channel_transport *transport = CONTAINER_OF(work,
					struct config_channel_transport,
					timeout);

	__ASSERT_NO_MSG(transport->state == CONFIG_CHANNEL_TRANSPORT_WAIT_RSP);

	/* Send response with timeout status, without aditional data. */
	transport->data[CONFIG_STATUS_POS] = CONFIG_STATUS_TIMEOUT;
	drop_transactions(transport);
	transport->state = CONFIG_CHANNEL_TRANSPORT_RSP_READY;
}

void config_channel_transport_init(struct config_channel_transport *transport)
{
	static uint8_t transport_id_base;

	__ASSERT_NO_MSG(transport_id_base < UCHAR_MAX);
	transport_id_base++;
	transport->transport_id = (transport_id_base << 8);

	__ASSERT_NO_MSG(transport->state == CONFIG_CHANNEL_TRANSPORT_DISABLED);
	transport->state = CONFIG_CHANNEL_TRANSPORT_IDLE;
	k_work_init_delayable(&transport->timeout, timeout_fn);
}

int config_channel_transport_get(struct config_channel_transport *transport,
				 uint8_t *buffer, size_t length)
{
	__ASSERT_NO_MSG(transport->state != CONFIG_CHANNEL_TRANSPORT_DISABLED);

	if (length < transport->data_len) {
		LOG_ERR("Host fetched incomplete data");
	}

	memcpy(buffer, transport->data, length);

	if (transport->state == CONFIG_CHANNEL_TRANSPORT_RSP_READY) {
		transport->state = CONFIG_CHANNEL_TRANSPORT_IDLE;
	}

	return 0;
}

int config_channel_transport_set(struct config_channel_transport *transport,
				 const uint8_t *buffer, size_t length)
{
	__ASSERT_NO_MSG(transport->state != CONFIG_CHANNEL_TRANSPORT_DISABLED);

	if (transport->state == CONFIG_CHANNEL_TRANSPORT_WAIT_RSP) {
		LOG_WRN("Transport %p busy", (void *)transport);
		return -EBUSY;
	}

	if (transport->state == CONFIG_CHANNEL_TRANSPORT_RSP_READY) {
		LOG_WRN("Host ignored previous response (transport: %p)",
			(void *)transport);
	}

	struct config_event *event =
		new_config_event(length - TRANSPORT_HEADER_SIZE);

	int err = config_channel_report_parse(buffer, length, event);

	if (err < 0) {
		LOG_WRN("Received improper frame");
		app_event_manager_free(event);
		return -EINVAL;
	}

	event->transport_id = transport->transport_id;
	event->is_request = true;
	APP_EVENT_SUBMIT(event);

	BUILD_ASSERT(CONFIG_DESKTOP_CONFIG_CHANNEL_TIMEOUT > 0, "");
	k_work_reschedule(&transport->timeout, K_SECONDS(CONFIG_DESKTOP_CONFIG_CHANNEL_TIMEOUT));

	/* Store the data to send it as pending response. */
	fill_response_pending(transport->data);
	transport->data_len = TRANSPORT_HEADER_SIZE;
	transport->state = CONFIG_CHANNEL_TRANSPORT_WAIT_RSP;

	return 0;
}

int config_channel_transport_get_disabled(uint8_t *buffer, size_t length)
{
	int err = frame_length_check(length);

	if (err) {
		return err;
	}

	/* Recipient ID and Event ID are both set to zero. */
	memset(buffer, 0, length);
	buffer[CONFIG_STATUS_POS] = CONFIG_STATUS_DISCONNECTED;

	return 0;
}

bool config_channel_transport_rsp_receive(struct config_channel_transport *transport,
					  struct config_event *event)
{
	/* It's not us. */
	if (event->transport_id != transport->transport_id) {
		return false;
	}

	if (event->is_request) {
		/* Mark request as unhandled. */
		event->status = CONFIG_STATUS_DISCONNECTED;
		event->dyndata.size = 0;
	}

	int pos = config_channel_report_fill(transport->data,
				event->dyndata.size + TRANSPORT_HEADER_SIZE,
				event);

	__ASSERT_NO_MSG(pos > 0);
	ARG_UNUSED(pos);

	transport->state = CONFIG_CHANNEL_TRANSPORT_RSP_READY;

	int err = k_work_cancel_delayable(&transport->timeout);

	__ASSERT_NO_MSG(!err);
	ARG_UNUSED(err);

	return true;
}

void config_channel_transport_disconnect(struct config_channel_transport *transport)
{
	__ASSERT_NO_MSG(transport->state != CONFIG_CHANNEL_TRANSPORT_DISABLED);

	/* Make sure that responses for ongoing transactions will be dropped. */
	if (transport->state == CONFIG_CHANNEL_TRANSPORT_WAIT_RSP) {
		drop_transactions(transport);
	}
	transport->state = CONFIG_CHANNEL_TRANSPORT_IDLE;

	int err = k_work_cancel_delayable(&transport->timeout);

	__ASSERT_NO_MSG(!err);
	ARG_UNUSED(err);
}
