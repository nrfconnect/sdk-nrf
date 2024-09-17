/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <modem/lte_lc.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>

#include "modules/pall.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

/* Size of PLMN list entry (in string representation): `,"123456",1`.
 * @ref LTE_LC_PLMN_MCC_MNC_MAX for the PLMN value, 1 for ACT, 2 for commas, 2 for quotes.
 */
#define PLMN_ENTRY_STR_SIZE (LTE_LC_PLMN_MCC_MNC_MAX + 1 + 2 + 2)
/* AT response when reading an empty PLMN access list. */
#define PLMN_LIST_NO_ENTRY "%PALL: \r\nOK\r\n"

int plmn_access_list_write(struct lte_lc_plmn_entry *plmn_list, size_t size,
			   enum lte_lc_plmn_list_type list_type)
{
	int err;
	char *plmn_list_str_format;
	char entry_str_temp[PLMN_ENTRY_STR_SIZE + 1] = { 0 };

	if (!plmn_list) {
		LOG_ERR("Invalid arguments.");
		return -EINVAL;
	}

	if (size > LTE_LC_PLMN_LIST_MAX) {
		LOG_ERR("PLMN access list size is too big.");
		return -EINVAL;
	}

	/* Dynamically allocate the format string for the AT command. */
	int format_size = PLMN_ENTRY_STR_SIZE * size + 1;

	plmn_list_str_format = (char *)k_malloc(format_size);
	if (!plmn_list_str_format) {
		LOG_ERR("Failed to allocate PLMN access list AT command parameters.");
		return -ENOMEM;
	}
	/* Always zero the malloc'd buffer. */
	memset(plmn_list_str_format, 0, format_size);

	/* Populate the format string. */
	for (int i = 0; i < size; i++) {
		snprintf(entry_str_temp, sizeof(entry_str_temp), ",\"%s\",%d", plmn_list[i].mcc_mnc,
			 plmn_list[i].act_bitmask);
		strcat(plmn_list_str_format, entry_str_temp);
	}

	err = nrf_modem_at_printf("AT%%PALL=0,%d%s", list_type, plmn_list_str_format);
	if (err) {
		LOG_ERR("AT command failed, returned error code: %d", err);
		err = -EFAULT;
		goto clean_exit;
	}

clean_exit:
	k_free(plmn_list_str_format);

	return err;
}

int plmn_access_list_read(struct lte_lc_plmn_entry *plmn_list, size_t *size,
			  enum lte_lc_plmn_list_type *list_type)
{
	int err;
	int act;
	struct at_parser parser = { 0 };
	char *response_buffer;

	if (!plmn_list || !size || !list_type) {
		LOG_ERR("Invalid arguments.");
		return -EINVAL;
	}

	if (*size > LTE_LC_PLMN_LIST_MAX) {
		LOG_ERR("PLMN access list size is too big.");
		return -EINVAL;
	}

	/* Dynamically allocate the AT response buffer. */
	int response_buffer_size = strlen("%%PALL:_") +
			  PLMN_ENTRY_STR_SIZE * LTE_LC_PLMN_LIST_MAX +
			  strlen("\r\nOK\r\n") + 1;

	response_buffer = (char *)k_malloc(response_buffer_size);
	if (!response_buffer) {
		LOG_ERR("Failed to allocate PLMN access list response buffer.");
		return -ENOMEM;
	}
	/* Always zero the malloc'd buffer. */
	memset(response_buffer, 0, response_buffer_size);

	err = nrf_modem_at_cmd(response_buffer, response_buffer_size, "AT%%PALL=1");
	if (err) {
		LOG_ERR("AT command failed, returned error code: %d", err);
		LOG_ERR("%d %d", nrf_modem_at_err_type(err), nrf_modem_at_err(err));
		err = -EFAULT;
		goto clean_exit;
	}

	/* A response with at least one PLMN entry will look like: `%PALL:1,"123...` while a
	 * response with no PLMN entry will look like: `%PALL: \r\nOK\r\n`.
	 */
	if (memcmp(response_buffer, PLMN_LIST_NO_ENTRY, sizeof(PLMN_LIST_NO_ENTRY) == 0)) {
		*size = 0;
		goto clean_exit;
	}

	err = at_parser_init(&parser, response_buffer);
	if (err) {
		LOG_ERR("AT parser init failed, returned error code: %d", err);
		err = -EFAULT;
		goto clean_exit;
	}

	uint16_t type;

	err = at_parser_num_get(&parser, 1, &type);
	if (err) {
		LOG_ERR("AT parser list type get failed, returned error code: %d", err);
		err = -EFAULT;
		goto clean_exit;
	}

	*list_type = type;

	/* Skip "%PALL:<type>"" */
	int start_index = 2;
	int i = 0;

	for (i = 0; i < *size; i++) {
		int mcc_mnc_size = sizeof(plmn_list[i].mcc_mnc);

		err = at_parser_string_get(&parser, start_index + 2 * i, plmn_list[i].mcc_mnc,
					   &mcc_mnc_size);
		if (err == -EIO) {
			/* The list is finished. */
			err = 0;
			break;
		} else if (err) {
			LOG_ERR("AT parser MCC/MNC get failed, returned error code: %d", err);
			err = -EFAULT;
			goto clean_exit;
		}

		err = at_parser_num_get(&parser, start_index + 2 * i + 1, &act);
		if (err) {
			LOG_ERR("AT parser ACT get failed, returned error code: %d", err);
			err = -EFAULT;
			goto clean_exit;
		}

		plmn_list[i].act_bitmask = act;
	}

	*size = i;

clean_exit:
	k_free(response_buffer);

	return err;
}

int plmn_access_list_clear(void)
{
	int err = nrf_modem_at_printf("AT%%PALL=2");

	if (err) {
		LOG_ERR("AT command failed, returned error code: %d", err);
		return -EFAULT;
	}

	return 0;
}
