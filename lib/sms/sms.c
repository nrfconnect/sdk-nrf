/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <modem/sms.h>
#include <errno.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>

#include "sms_submit.h"
#include "sms_deliver.h"
#include "sms_internal.h"

LOG_MODULE_REGISTER(sms, CONFIG_SMS_LOG_LEVEL);

/** @brief AT command to check if a client already exist. */
#define AT_SMS_SUBSCRIBER_READ "AT+CNMI?"
/** @brief AT command to register an SMS client. */
#define AT_SMS_SUBSCRIBER_REGISTER "AT+CNMI=3,2,0,1"
/** @brief AT command to unregister an SMS client. */
#define AT_SMS_SUBSCRIBER_UNREGISTER "AT+CNMI=0,0,0,0"
/** @brief AT command to an ACK in PDU mode. */
#define AT_SMS_PDU_ACK "AT+CNMA=1"

/** @brief SMS structure where received SMS is parsed. */
static struct sms_data sms_data_info;

/**
 * @brief Worker handling SMS acknowledgements because we cannot call
 * nrf_modem_at_printf from AT monitor callback.
 */
static struct k_work sms_ack_work;
/**
 * @brief Worker handling notifying SMS subscribers about received messages because we cannot
 * notify subscribers in ISR context where AT notifications are received.
 */
static struct k_work sms_notify_work;

/* Reserving internal temporary buffers that are used for various functions requiring memory. */
uint8_t sms_buf_tmp[SMS_BUF_TMP_LEN];
uint8_t sms_payload_tmp[SMS_MAX_PAYLOAD_LEN_CHARS];

/**
 * @brief Indicates that the module has been successfully initialized
 * and registered as an SMS client.
 */
static bool sms_client_registered;

/** @brief SMS subscriber information. */
struct sms_subscriber {
	/** Listener user context. */
	void *ctx;
	/** Listener callback. */
	sms_callback_t listener;
};

/** @brief List of subscribers. */
static struct sms_subscriber subscribers[CONFIG_SMS_SUBSCRIBERS_MAX_CNT];

/* AT monitors for received SMS messages (CMT) and status reports (CDS). */
AT_MONITOR_ISR(sms_at_handler_cmt, "+CMT", sms_at_cmd_handler_cmt, PAUSED);
AT_MONITOR_ISR(sms_at_handler_cds, "+CDS", sms_at_cmd_handler_cds, PAUSED);

/**
 * @brief Acknowledge SMS messages towards network.
 *
 * @param[in] work Unused k_work instance required for k_work_submit signature.
 */
static void sms_ack(struct k_work *work)
{
	int ret = nrf_modem_at_printf(AT_SMS_PDU_ACK);

	if (ret != 0) {
		LOG_ERR("Unable to ACK the SMS PDU");
	}
}

/**
 * @brief Notify SMS subscribers about received SMS or status report.
 *
 * @param[in] work Unused k_work instance required for k_work_submit signature.
 */
static void sms_notify(struct k_work *work)
{
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].listener != NULL) {
			subscribers[i].listener(&sms_data_info, subscribers[i].ctx);
		}
	}
}

/**
 * @brief Callback handler for CMT notification.
 *
 * @param[in] at_notif AT notification string.
 */
static void sms_at_cmd_handler_cmt(const char *at_notif)
{
	int err;

	if (at_notif == NULL) {
		return;
	}

	memset(&sms_data_info, 0, sizeof(struct sms_data));

	/* Parse AT command and SMS PDU */
	err = sscanf(
		at_notif,
		"+CMT: %*[^,],%*u\r\n"
		"%"STRINGIFY(SMS_BUF_TMP_LEN)"s\r\n",
		sms_buf_tmp);
	if (err < 1) {
		LOG_ERR("Unable to parse CMT notification, err=%d: %s", err, log_strdup(at_notif));
		goto sms_ack_send;
	}

	sms_data_info.type = SMS_TYPE_DELIVER;
	err = sms_deliver_pdu_parse(sms_buf_tmp, &sms_data_info);
	if (err) {
		goto sms_ack_send;
	}
	LOG_DBG("Valid SMS notification decoded");

	k_work_submit(&sms_notify_work);

sms_ack_send:
	k_work_submit(&sms_ack_work);
}

/**
 * @brief Callback handler for CDS notification.
 *
 * @param[in] at_notif AT notification string.
 */
static void sms_at_cmd_handler_cds(const char *at_notif)
{
	/* This indicates SMS-STATUS-REPORT has been received. However, its content is not
	 * parsed so we don't know if the message is delivered or if an error occurred.
	 */
	LOG_DBG("SMS status report received");
	memset(&sms_data_info, 0, sizeof(struct sms_data));
	sms_data_info.type = SMS_TYPE_STATUS_REPORT;

	k_work_submit(&sms_notify_work);
	k_work_submit(&sms_ack_work);
}

/**
 * @brief Initialize the SMS subscriber module.
 *
 * @return Zero on success, or a negative error code. The EBUSY error
 *         indicates that one SMS client has already been registered.
 */
static int sms_init(void)
{
	uint32_t cnmi_value1 = 0xFFFFFFFF;
	uint32_t cnmi_value2 = 0xFFFFFFFF;
	uint32_t cnmi_value3 = 0xFFFFFFFF;
	uint32_t cnmi_value4 = 0xFFFFFFFF;
	int ret;

	k_work_init(&sms_ack_work, &sms_ack);
	k_work_init(&sms_notify_work, &sms_notify);

	/* Check if one SMS client has already been registered. */
	ret = nrf_modem_at_cmd(sms_buf_tmp, SMS_BUF_TMP_LEN, AT_SMS_SUBSCRIBER_READ);
	if (ret) {
		LOG_ERR("Unable to check if an SMS client exists, err: %d", ret);
		return ret;
	}

	ret = sscanf(
		sms_buf_tmp,
		"+CNMI: %u,%u,%u,%u",
		&cnmi_value1, &cnmi_value2, &cnmi_value3, &cnmi_value4);
	if (ret < 4) {
		LOG_ERR("CNMI parsing failure, err: %d", ret);
		return -EBADMSG;
	}
	/* Parameters 1 to 4 should be 0 if no client is registered. */
	if (cnmi_value1 != 0 || cnmi_value2 != 0 || cnmi_value3 != 0 || cnmi_value4 != 0) {
		LOG_ERR("Only one SMS client can be registered");
		return -EBUSY;
	}

	/* Register for AT commands notifications before creating the client. */
	at_monitor_resume(sms_at_handler_cmt);
	at_monitor_resume(sms_at_handler_cds);

	/* Register this module as an SMS client. */
	ret = nrf_modem_at_printf(AT_SMS_SUBSCRIBER_REGISTER);
	if (ret) {
		at_monitor_pause(sms_at_handler_cmt);
		at_monitor_pause(sms_at_handler_cds);
		LOG_ERR("Unable to register a new SMS client, err: %d", ret);
		return ret;
	}

	sms_client_registered = true;
	LOG_DBG("SMS client successfully registered");
	return 0;
}

/**
 * @brief Return number of subscribers.
 *
 * @return Number of registered subscribers to this module.
 */
static int sms_subscriber_count(void)
{
	int count = 0;

	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].ctx != NULL ||
		    subscribers[i].listener != NULL) {
			count++;
		}
	}
	return count;
}

int sms_register_listener(sms_callback_t listener, void *context)
{
	int err;

	if (listener == NULL) {
		return -EINVAL; /* Invalid parameter. */
	}

	if (!sms_client_registered) {
		err = sms_init();
		if (err != 0) {
			return err;
		}
	}

	/* Search for a free slot to register a new listener. */
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].ctx == NULL &&
		    subscribers[i].listener == NULL) {
			subscribers[i].ctx = context;
			subscribers[i].listener = listener;
			return i;
		}
	}

	/* Too many subscribers. */
	return -ENOSPC;
}

/**
 * @brief Uninitialize the SMS subscriber module.
 *
 * @details Doesn't do anything if there are still subscribers that haven't unregistered.
 */
static void sms_uninit(void)
{
	int ret;
	int count;

	/* Don't do anything if there are subscribers */
	count = sms_subscriber_count();
	if (count > 0) {
		LOG_DBG("Unregistering skipped as there are %d subscriber(s)",
			count);
		return;
	}

	if (sms_client_registered) {
		ret = nrf_modem_at_printf(AT_SMS_SUBSCRIBER_UNREGISTER);
		if (ret) {
			LOG_ERR("Unable to unregister the SMS client, err: %d",
				ret);
			return;
		}
		LOG_DBG("SMS client unregistered");

		/* Pause AT commands notifications. */
		at_monitor_pause(sms_at_handler_cmt);
		at_monitor_pause(sms_at_handler_cds);

		/* Clear all observers. */
		for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
			subscribers[i].ctx = NULL;
			subscribers[i].listener = NULL;
		}
	}

	sms_client_registered = false;
}

void sms_unregister_listener(int handle)
{
	/* Unregister the listener. */
	if (handle < 0 || handle >= ARRAY_SIZE(subscribers)) {
		/* Invalid handle. Unknown listener. */
		return;
	}

	subscribers[handle].ctx = NULL;
	subscribers[handle].listener = NULL;

	sms_uninit();
}

int sms_send_text(const char *number, const char *text)
{
	return sms_submit_send(number, text);
}
