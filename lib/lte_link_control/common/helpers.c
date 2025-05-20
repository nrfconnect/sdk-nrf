/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdlib.h>
#include <zephyr/logging/log.h>

#include <common/helpers.h>

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

int string_to_int(const char *str_buf, int base, int *output)
{
	int temp;
	char *end_ptr;

	__ASSERT_NO_MSG(str_buf != NULL);

	errno = 0;
	temp = strtol(str_buf, &end_ptr, base);

	if (end_ptr == str_buf || *end_ptr != '\0' ||
	    ((temp == LONG_MAX || temp == LONG_MIN) && errno == ERANGE)) {
		return -ENODATA;
	}

	*output = temp;

	return 0;
}

int string_param_to_int(struct at_parser *parser, size_t idx, int *output, int base)
{
	int err;
	char str_buf[16];
	size_t len = sizeof(str_buf);

	__ASSERT_NO_MSG(parser != NULL);
	__ASSERT_NO_MSG(output != NULL);

	err = at_parser_string_get(parser, idx, str_buf, &len);
	if (err) {
		return err;
	}

	if (string_to_int(str_buf, base, output)) {
		return -ENODATA;
	}

	return 0;
}

int plmn_param_string_to_mcc_mnc(struct at_parser *parser, size_t idx, int *mcc, int *mnc)
{
	int err;
	char str_buf[7];
	size_t len = sizeof(str_buf);

	err = at_parser_string_get(parser, idx, str_buf, &len);
	if (err) {
		LOG_ERR("Could not get PLMN, error: %d", err);
		return err;
	}

	str_buf[len] = '\0';

	/* Read MNC and store as integer. The MNC starts as the fourth character
	 * in the string, following three characters long MCC.
	 */
	err = string_to_int(&str_buf[3], 10, mnc);
	if (err) {
		LOG_ERR("Could not get MNC, error: %d", err);
		return err;
	}

	/* NUL-terminate MCC, read and store it. */
	str_buf[3] = '\0';

	err = string_to_int(str_buf, 10, mcc);
	if (err) {
		LOG_ERR("Could not get MCC, error: %d", err);
		return err;
	}

	return 0;
}
