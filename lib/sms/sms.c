/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <stdio.h>
#include <modem/sms.h>
#include <errno.h>
#include <modem/at_monitor.h>
#include <nrf_modem_at.h>
#include <nrf_errno.h>
#if defined(CONFIG_LTE_LINK_CONTROL)
#include <modem/lte_lc.h>
#endif
#include <modem/sms.h>

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
/** @brief AT notification informing that SMS client has been unregistered. */
#define AT_SMS_UNREGISTERED_NTF "+CMS ERROR: 524"

/** @brief SMS structure where received SMS is parsed. */
static struct sms_data sms_data_info;

/**
 * @brief Worker handling SMS acknowledgements because we cannot call
 * nrf_modem_at_printf from AT monitor callback.
 */
static struct k_work_delayable sms_ack_work;
/**
 * @brief Worker handling notifying SMS subscribers about received messages because we cannot
 * notify subscribers in ISR context where AT notifications are received.
 */
static struct k_work sms_notify_work;
/**
 * @brief Worker handling re-registration in case modem performs SMS client unregistration.
 */
static struct k_work sms_reregister_work;

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

/* AT monitors for received SMS messages (CMT), status reports (CDS) and message service
 * errors (CMS).
 */
AT_MONITOR_ISR(sms_at_handler_cmt, "+CMT", sms_at_cmd_handler_cmt, PAUSED);
AT_MONITOR_ISR(sms_at_handler_cds, "+CDS", sms_at_cmd_handler_cds, PAUSED);
AT_MONITOR_ISR(sms_at_handler_cms, "+CMS", sms_at_cmd_handler_cms, PAUSED);

/* Keep this function public so that it can be called by tests. */
void sms_ack_resp_handler(const char *resp)
{
	/* No need to do anything really */
	if (strlen(resp) < 2 || (resp[0] != 'O' && resp[1] != 'K')) {
		LOG_WRN("AT+CNMA=1 response not OK: %s", resp);
	}
}

/**
 * @brief Acknowledge SMS messages towards network.
 *
 * @param[in] work Unused k_work instance required for work handler signature.
 */
static void sms_ack(struct k_work *work)
{
	int ret = nrf_modem_at_cmd_async(sms_ack_resp_handler, AT_SMS_PDU_ACK);

	if (ret == -NRF_EINPROGRESS) {
		LOG_DBG("Another AT command ongoing. Retrying to ACK the SMS PDU in a second");
		k_work_reschedule(&sms_ack_work, K_SECONDS(1));
	} else if (ret != 0) {
		LOG_ERR("Unable to ACK the SMS PDU");
	}
}

/**
 * @brief Notify SMS subscribers about received SMS or status report.
 *
 * @param[in] work Unused k_work instance required for work handler signature.
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
 * @brief Re-register SMS client.
 *
 * @param[in] work Unused k_work instance required for work handler signature.
 */
static void sms_reregister(struct k_work *work)
{
	int err;

	/* Re-registration is triggered when modem informs that the SMS client has been
	 * unregistered. When there is no existing registration, the registration is expected to
	 * succeed. Because of this there is no error handling or retry logic for the
	 * re-registration.
	 */
	err = nrf_modem_at_printf(AT_SMS_SUBSCRIBER_REGISTER);
	if (err) {
		LOG_ERR("Unable to re-register SMS client, err: %d", err);
		/* Pause AT commands notifications. */
		at_monitor_pause(&sms_at_handler_cmt);
		at_monitor_pause(&sms_at_handler_cds);
		at_monitor_pause(&sms_at_handler_cms);

		/* Clear all observers. */
		for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
			subscribers[i].ctx = NULL;
			subscribers[i].listener = NULL;
		}

		sms_client_registered = false;
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

	__ASSERT_NO_MSG(at_notif != NULL);

	memset(&sms_data_info, 0, sizeof(struct sms_data));

	/* Parse AT command and SMS PDU */
	err = sscanf(
		at_notif,
		"+CMT: %*[^,],%*u\r\n"
		"%"STRINGIFY(SMS_BUF_TMP_LEN)"s\r\n",
		sms_buf_tmp);
	if (err < 1) {
		LOG_ERR("Unable to parse CMT notification, err=%d: %s", err, at_notif);
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
	k_work_reschedule(&sms_ack_work, K_NO_WAIT);
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
	k_work_reschedule(&sms_ack_work, K_NO_WAIT);
}

/**
 * @brief Callback handler for CMS notification.
 *
 * @param[in] at_notif AT notification string.
 */
static void sms_at_cmd_handler_cms(const char *at_notif)
{
	if (strstr(at_notif, AT_SMS_UNREGISTERED_NTF) != NULL) {
		LOG_WRN("Modem unregistered the SMS client, performing re-registration");

		k_work_submit(&sms_reregister_work);
	}
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

	k_work_init_delayable(&sms_ack_work, &sms_ack);
	k_work_init(&sms_notify_work, &sms_notify);
	k_work_init(&sms_reregister_work, &sms_reregister);

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
		if (sms_client_registered) {
			LOG_DBG("SMS client is already registered");
			return 0;
		}
		LOG_ERR("Only one SMS client can be registered");
		return -EBUSY;
	}

	/* Register for AT commands notifications before creating the client. */
	at_monitor_resume(&sms_at_handler_cmt);
	at_monitor_resume(&sms_at_handler_cds);
	at_monitor_resume(&sms_at_handler_cms);

	/* Register this module as an SMS client. */
	ret = nrf_modem_at_printf(AT_SMS_SUBSCRIBER_REGISTER);
	if (ret) {
		at_monitor_pause(&sms_at_handler_cmt);
		at_monitor_pause(&sms_at_handler_cds);
		at_monitor_pause(&sms_at_handler_cms);
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

	/* Check if SMS client is registered. */
	if (!sms_client_registered) {
		return;
	}

	/* Don't do anything if there are subscribers */
	count = sms_subscriber_count();
	if (count > 0) {
		LOG_DBG("Unregistering skipped as there are %d subscriber(s)",
			count);
		return;
	}

	ret = nrf_modem_at_printf(AT_SMS_SUBSCRIBER_UNREGISTER);
	if (ret < 0) {
		LOG_ERR("Sending AT command failed, err: %d", ret);
		return;
	} else if (ret > 0) {
		/* Modem returned an error, assume that there is no registration anymore. */
		LOG_WRN("Failed to unregister the SMS client from modem, err: %d", ret);
	}

	LOG_DBG("SMS client unregistered");

	/* Pause AT commands notifications. */
	at_monitor_pause(&sms_at_handler_cmt);
	at_monitor_pause(&sms_at_handler_cds);
	at_monitor_pause(&sms_at_handler_cms);

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
	if (text == NULL) {
		return -EINVAL;
	}
	return sms_send(number, text, strlen(text), SMS_DATA_TYPE_ASCII);
}

int sms_send(const char *number, const uint8_t *data, uint16_t data_len, enum sms_data_type type)
{
	if (data == NULL) {
		return -EINVAL;
	}
	return sms_submit_send(number, data, data_len, type);
}

#if defined(CONFIG_LTE_LINK_CONTROL)
LTE_LC_ON_CFUN(sms_cfun_hook, sms_on_cfun, NULL);

static void sms_on_cfun(enum lte_lc_func_mode mode, void *ctx)
{
	int err;

	/* When LTE is activated, reinitialize SMS registration with the LTE network
	 * if it had been registered earlier.
	 */
	if (sms_client_registered) {
		if (mode == LTE_LC_FUNC_MODE_NORMAL ||
		    mode == LTE_LC_FUNC_MODE_ACTIVATE_LTE) {

			LOG_DBG("Reinitialize SMS subscription when LTE is set ON");

			err = sms_init();
			if (err != 0) {
				sms_client_registered = false;
			}
		}
	}
}
#endif /* CONFIG_LTE_LINK_CONTROL */
