/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <logging/log.h>
#include <zephyr.h>
#include <stdio.h>
#include <modem/sms.h>
#include <errno.h>
#include <modem/at_cmd.h>
#include <modem/at_cmd_parser.h>
#include <modem/at_params.h>
#include <modem/at_notif.h>

#include "string_conversion.h"
#include "parser.h"
#include "sms_submit.h"
#include "sms_deliver.h"
#include "sms_internal.h"

LOG_MODULE_DECLARE(sms, CONFIG_SMS_LOG_LEVEL);

/** @brief Number of parameters in CMT AT notification. */
#define AT_CMT_PARAMS_COUNT 4
/* SMS PDU is a bit below 180 bytes which means 360 two character long
 * hexadecimal numbers. In addition CMT response has alpha and length so
 * reserving safely a bit over maximum.
 */
#define AT_CMT_PDU_MAX_LEN 512
/** @brief PDU index in CMT AT notification. */
#define AT_CMT_PDU_INDEX 3
/** @brief Number of parameters in CDS AT notification. */
#define AT_CDS_PARAMS_COUNT 3
/** @brief PDU index in CDS AT notification. */
#define AT_CDS_PDU_INDEX 2

/** @brief Start of AT notification for incoming SMS. */
#define AT_SMS_DELIVER "+CMT:"
/** @brief Length of AT notification for incoming SMS. */
#define AT_SMS_DELIVER_LEN (sizeof(AT_SMS_DELIVER) - 1)

/** @brief Start of AT notification for incoming SMS status report. */
#define AT_SMS_STATUS_REPORT "+CDS:"
/** @brief Length of AT notification for incoming SMS status report. */
#define AT_SMS_STATUS_REPORT_LEN (sizeof(AT_SMS_STATUS_REPORT) - 1)


/**
 * @brief Parses AT notification and finds PDU buffer.
 *
 * @param[in] buf AT notification buffer.
 * @param[out] pdu Output buffer where PDU is copied.
 * @param[in] pdu_len Length of the output buffer.
 * @param[in] at_params_count Maximum number of AT parameters in AT notification.
 * @param[in] pdu_index Index where PDU is found from AT notification parameter list.
 * @param[in] temp_resp_list Response list used by AT parser library. This is readily initialized
 *                 by caller and is passed here to avoid using another instance of the list.
 * @return Zero on success and negative value in error cases.
 */
static int sms_notif_at_parse(const char *const buf, char *pdu, size_t pdu_len,
	int at_params_count, int pdu_index, struct at_param_list *temp_resp_list)
{
	int err = at_parser_max_params_from_str(buf, NULL, temp_resp_list, at_params_count);

	if (err != 0) {
		LOG_ERR("Unable to parse AT notification, err=%d: %s", err, buf);
		return err;
	}

	err = at_params_string_get(temp_resp_list, pdu_index, pdu, &pdu_len);
	if (err != 0) {
		LOG_ERR("Unable to retrieve SMS PDU from AT notification, err=%d", err);
		return err;
	}

	pdu[pdu_len] = '\0';

	LOG_DBG("PDU: %s", log_strdup(pdu));

	return 0;
}

int sms_at_parse(const char *at_notif, struct sms_data *sms_data_info,
	struct at_param_list *temp_resp_list)
{
	int err;

	__ASSERT(at_notif != NULL, "at_notif is NULL");
	__ASSERT(sms_data_info != NULL, "sms_data_info is NULL");
	__ASSERT(temp_resp_list != NULL, "temp_resp_list is NULL");

	memset(sms_data_info, 0, sizeof(struct sms_data));

	if (strncmp(at_notif, AT_SMS_DELIVER, AT_SMS_DELIVER_LEN) == 0) {

		sms_data_info->type = SMS_TYPE_DELIVER;

		/* Extract and save the SMS notification parameters */
		err = sms_notif_at_parse(at_notif, sms_buf_tmp, SMS_BUF_TMP_LEN,
				AT_CMT_PARAMS_COUNT, AT_CMT_PDU_INDEX, temp_resp_list);
		if (err) {
			return err;
		}
		err = sms_deliver_pdu_parse(sms_buf_tmp, sms_data_info);
		if (err) {
			LOG_ERR("sms_deliver_pdu_parse error: %d\n", err);
			return err;
		}
	} else if (strncmp(at_notif, AT_SMS_STATUS_REPORT, AT_SMS_STATUS_REPORT_LEN) == 0) {

		/* This indicates SMS-STATUS-REPORT has been received. However, its content is not
		 * parsed so we don't know if the message is delivered or if an error occurred.
		 */
		LOG_DBG("SMS status report received");
		sms_data_info->type = SMS_TYPE_STATUS_REPORT;

		err = sms_notif_at_parse(at_notif, sms_buf_tmp, SMS_BUF_TMP_LEN,
				AT_CDS_PARAMS_COUNT, AT_CDS_PDU_INDEX, temp_resp_list);
		if (err != 0) {
			LOG_ERR("sms_cds_at_parse error: %d", err);
			return err;
		}
	} else {
		/* Ignore all other notifications */
		return -EINVAL;
	}

	return 0;
}
