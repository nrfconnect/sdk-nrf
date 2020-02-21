/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <sms.h>
#include <errno.h>
#include <modem/at_cmd.h>
#include <at_cmd_parser/at_cmd_parser.h>
#include <at_cmd_parser/at_params.h>
#include <at_notif.h>

LOG_MODULE_REGISTER(sms, CONFIG_SMS_LOG_LEVEL);

#define AT_SMS_PARAMS_COUNT_MAX 6
#define AT_SMS_RESPONSE_MAX_LEN 256

#define AT_CNMI_PARAMS_COUNT 6
#define AT_CMT_PARAMS_COUNT 4

/** @brief AT command to check if a client already exist. */
#define AT_SMS_SUBSCRIBER_READ "AT+CNMI?"

/** @brief AT command to register an SMS client. */
#define AT_SMS_SUBSCRIBER_REGISTER "AT+CNMI=3,2,0,1"

/** @brief AT command to unregister an SMS client. */
#define AT_SMS_SUBSCRIBER_UNREGISTER "AT+CNMI=0,0,0,0"

/** @brief AT command to an ACK in PDU mode. */
#define AT_SMS_PDU_ACK "AT+CNMA=1"

/** @brief Start of AT notification for incoming SMS. */
#define AT_SMS_NOTIFICATION "+CMT:"
#define AT_SMS_NOTIFICATION_LEN (sizeof(AT_SMS_NOTIFICATION) - 1)

static struct at_param_list resp_list;
static char resp[AT_SMS_RESPONSE_MAX_LEN];

/**
 * @brief Indicates that the module has been successfully initialized
 * and registered as an SMS client.
 */
static bool sms_client_registered;

/** @brief SMS event. */
static struct sms_data cmt_rsp;

struct sms_subscriber {
	/* Listener user context. */
	void *ctx;
	/* Listener callback. */
	sms_callback_t listener;
};

/** @brief List of subscribers. */
static struct sms_subscriber subscribers[CONFIG_SMS_MAX_SUBSCRIBERS_CNT];

/** @brief Check if the received AT notification is a +CMT event. */
static bool sms_is_cmd_notification(const char *const at_notif)
{
	if (at_notif == NULL) {
		return false;
	}

	if (strlen(at_notif) <= AT_SMS_NOTIFICATION_LEN) {
		return false;
	}

	return (strncmp(at_notif, AT_SMS_NOTIFICATION,
			AT_SMS_NOTIFICATION_LEN) == 0);
}

/** @brief Parse the +CMT unsolicited received message in PDU mode. */
static int sms_cmt_notif_parse(const char *const buf)
{
	/* Parse the received message. */
	int err = at_parser_max_params_from_str(buf, NULL, &resp_list,
						AT_CMT_PARAMS_COUNT);

	if (err != 0) {
		LOG_ERR("Unable to parse CMT notification, err=%d", err);
		return err;
	}

	/* Check the AT response format. */
	if (at_params_valid_count_get(&resp_list) != AT_CMT_PARAMS_COUNT) {
		LOG_ERR("Invalid CMT notification format");
		return -EAGAIN;
	}

	return 0;
}

/** @brief Save the SMS notification parameters. */
static int sms_cmt_notif_save(void)
{
	if (cmt_rsp.alpha != NULL) {
		k_free(cmt_rsp.alpha);
	}

	if (cmt_rsp.pdu != NULL) {
		k_free(cmt_rsp.pdu);
	}

	/* Save alpha as a null-terminated String. */
	size_t alpha_len;
	(void)at_params_size_get(&resp_list, 1, &alpha_len);

	cmt_rsp.alpha = k_malloc(alpha_len + 1);
	if (cmt_rsp.alpha == NULL) {
		return -ENOMEM;
	}
	(void)at_params_string_get(&resp_list, 1, cmt_rsp.alpha, &alpha_len);
	cmt_rsp.alpha[alpha_len] = '\0';

	/* Length field saved as number. */
	(void)at_params_short_get(&resp_list, 2, &cmt_rsp.length);

	/* Save PDU as a null-terminated String. */
	size_t pdu_len;
	(void)at_params_size_get(&resp_list, 3, &pdu_len);
	cmt_rsp.pdu = k_malloc(pdu_len + 1);
	if (cmt_rsp.pdu == NULL) {
		return -ENOMEM;
	}

	(void)at_params_string_get(&resp_list, 3, cmt_rsp.pdu, &pdu_len);
	cmt_rsp.pdu[pdu_len] = '\0';

	return 0;
}

/** @brief Handler for AT responses and unsolicited events. */
void sms_at_handler(void *context, char *at_notif)
{
	ARG_UNUSED(context);

	/* Ignore all notifications except CMT events. */
	if (!sms_is_cmd_notification(at_notif)) {
		return;
	}

	/* Parse and validate the CMT notification, then extract parameters. */
	if (sms_cmt_notif_parse(at_notif) != 0) {
		LOG_ERR("Invalid CMT notification");
		return;
	}

	/* Extract and save the SMS notification parameters. */
	int valid_notif = sms_cmt_notif_save();

	/* Acknowledge the SMS PDU in any case. */
	int ret = at_cmd_write(AT_SMS_PDU_ACK, NULL, 0, NULL);

	if (ret != 0) {
		LOG_ERR("Unable to ACK the SMS PDU");
	}

	if (valid_notif != 0) {
		LOG_ERR("Invalid SMS notification format");
		return;
	}

	/* Notify all subscribers. */
	LOG_DBG("Valid SMS notification decoded");
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		if (subscribers[i].listener != NULL) {
			subscribers[i].listener(&cmt_rsp, subscribers[i].ctx);
		}
	}
}

int sms_init(void)
{
	int ret = at_params_list_init(&resp_list, AT_SMS_PARAMS_COUNT_MAX);

	if (ret) {
		LOG_ERR("AT params error, err: %d", ret);
		return ret;
	}

	/* Check if one SMS client has already been registered. */
	ret = at_cmd_write(AT_SMS_SUBSCRIBER_READ, resp, sizeof(resp), NULL);
	if (ret) {
		LOG_ERR("Unable to check if an SMS client exists, err: %d",
			ret);
		return ret;
	}

	ret = at_parser_max_params_from_str(resp, NULL, &resp_list,
					    AT_CNMI_PARAMS_COUNT);
	if (ret) {
		LOG_INF("%s", log_strdup(resp));
		LOG_ERR("Invalid AT response, err: %d", ret);
		return ret;
	}

	/* Check the response format and parameters. */
	for (int i = 1; i < (AT_CNMI_PARAMS_COUNT - 1); i++) {
		int value;

		ret = at_params_int_get(&resp_list, i, &value);
		if (ret) {
			LOG_ERR("Invalid AT response parameters, err: %d", ret);
			return ret;
		}

		/* Parameters 1 to 4 should be 0 if no client is registered. */
		if (value != 0) {
			LOG_ERR("Only one SMS client can be registered");
			return -EBUSY;
		}
	}

	/* Register for AT commands notifications before creating the client. */
	ret = at_notif_register_handler(NULL, sms_at_handler);
	if (ret) {
		LOG_ERR("Cannot register AT notification handler, err: %d",
			ret);
		return ret;
	}

	/* Register this module as an SMS client. */
	ret = at_cmd_write(AT_SMS_SUBSCRIBER_REGISTER, NULL, 0, NULL);
	if (ret) {
		(void)at_notif_deregister_handler(NULL, sms_at_handler);
		LOG_ERR("Unable to register a new SMS client, err: %d", ret);
		return ret;
	}

	/* Clear all observers. */
	for (size_t i = 0; i < ARRAY_SIZE(subscribers); i++) {
		subscribers[i].ctx = NULL;
		subscribers[i].listener = NULL;
	}

	sms_client_registered = true;
	LOG_INF("SMS client successfully registered");
	return 0;
}

int sms_register_listener(sms_callback_t listener, void *context)
{
	if (listener == NULL) {
		return -EINVAL; /* Invalid parameter. */
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
	return -ENOMEM;
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
}

void sms_uninit(void)
{
	/* Unregister the SMS client if this module was registered as client. */
	if (sms_client_registered) {
		int ret = at_cmd_write(AT_SMS_SUBSCRIBER_UNREGISTER, resp,
				       sizeof(resp), NULL);
		if (ret) {
			LOG_ERR("Unable to unregister the SMS client, err: %d",
				ret);
			return;
		}
		LOG_INF("SMS client unregistered");
	}

	/* Cleanup resources. */
	at_params_list_free(&resp_list);

	if (cmt_rsp.alpha != NULL) {
		k_free(cmt_rsp.alpha);
	}

	if (cmt_rsp.pdu != NULL) {
		k_free(cmt_rsp.pdu);
	}

	/* Unregister from AT commands notifications. */
	(void)at_notif_deregister_handler(NULL, sms_at_handler);

	sms_client_registered = false;
}
