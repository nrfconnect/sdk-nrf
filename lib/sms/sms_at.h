/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _SMS_AT_INCLUDE_H_
#define _SMS_AT_INCLUDE_H_

/** @brief Error code for AT command that is not SMS related. */
#define ENOTSMSAT 0x0FFFFFFF

/**
 * @brief Parse AT notifications finding relevant notifications for SMS and
 * dropping the rest.
 *
 * @param[in] at_notif AT notication string.
 * @param[out] sms_data_info Parsed output data.
 * @param[in] temp_resp_list Response list used by AT parser library. This is readily initialized
 *                 by caller and is passed here to avoid using another instance of the list.
 * @return Zero on success and negative value in error cases.
 */
int sms_at_parse(const char *at_notif, struct sms_data *sms_data_info,
	struct at_param_list *temp_resp_list);

#endif
