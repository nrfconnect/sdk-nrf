/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zephyr/logging/log.h>
#include <modem/at_monitor.h>
#include <modem/lte_lc.h>
#include <modem/lte_lc_trace.h>
#include <modem/at_parser.h>
#include <nrf_modem_at.h>
#include <nrf_errno.h>

#include "modules/periodicsearchconf.h"

LOG_MODULE_DECLARE(lte_lc, CONFIG_LTE_LINK_CONTROL_LOG_LEVEL);

#if defined(CONFIG_UNITY)
int periodicsearchconf_parse(const char *const pattern_str,
				    struct lte_lc_periodic_search_pattern *pattern)
#else
static int periodicsearchconf_parse(const char *const pattern_str,
				    struct lte_lc_periodic_search_pattern *pattern)
#endif /* CONFIG_UNITY */
{
	int err;
	int values[5];
	size_t param_count;

	__ASSERT_NO_MSG(pattern_str != NULL);
	__ASSERT_NO_MSG(pattern != NULL);

	err = sscanf(pattern_str, "%d,%u,%u,%u,%u,%u", (int *)&pattern->type, &values[0],
		     &values[1], &values[2], &values[3], &values[4]);
	if (err < 1) {
		LOG_ERR("Unrecognized pattern type");
		return -EBADMSG;
	}

	param_count = err;

	if ((pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE) && (param_count >= 3)) {
		/* The 'time_to_final_sleep' parameter is optional and may not always be present.
		 * If that's the case, there will be only 3 matches, and we need a
		 * workaround to get the 'pattern_end_point' value.
		 */
		if (param_count == 3) {
			param_count = sscanf(pattern_str, "%*u,%*u,%*u,,%u", &values[3]);
			if (param_count != 1) {
				LOG_ERR("Could not find 'pattern_end_point' value");
				return -EBADMSG;
			}

			values[2] = -1;
		}

		pattern->range.initial_sleep = values[0];
		pattern->range.final_sleep = values[1];
		pattern->range.time_to_final_sleep = values[2];
		pattern->range.pattern_end_point = values[3];
	} else if ((pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE) && (param_count >= 2)) {
		/* Populate optional parameters only if matched, otherwise set
		 * to disabled, -1.
		 */
		pattern->table.val_1 = values[0];
		pattern->table.val_2 = param_count > 2 ? values[1] : -1;
		pattern->table.val_3 = param_count > 3 ? values[2] : -1;
		pattern->table.val_4 = param_count > 4 ? values[3] : -1;
		pattern->table.val_5 = param_count > 5 ? values[4] : -1;
	} else {
		LOG_DBG("No valid pattern found");
		return -EBADMSG;
	}

	return 0;
}

#if defined(CONFIG_UNITY)
char *
periodicsearchconf_pattern_get(char *const buf, size_t buf_size,
			       const struct lte_lc_periodic_search_pattern *const pattern)
#else
static char *
periodicsearchconf_pattern_get(char *const buf, size_t buf_size,
			       const struct lte_lc_periodic_search_pattern *const pattern)
#endif /* CONFIG_UNITY */
{
	int len = 0;

	__ASSERT_NO_MSG(buf != NULL);
	__ASSERT_NO_MSG(pattern != NULL);

	if (pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_RANGE) {
		/* Range format:
		 * "<type>,<initial_sleep>,<final_sleep>,[<time_to_final_sleep>],
		 *  <pattern_end_point>"
		 */
		if (pattern->range.time_to_final_sleep != -1) {
			len = snprintk(buf, buf_size, "\"0,%u,%u,%u,%u\"",
				       pattern->range.initial_sleep, pattern->range.final_sleep,
				       pattern->range.time_to_final_sleep,
				       pattern->range.pattern_end_point);
		} else {
			len = snprintk(buf, buf_size, "\"0,%u,%u,,%u\"",
				       pattern->range.initial_sleep, pattern->range.final_sleep,
				       pattern->range.pattern_end_point);
		}
	} else if (pattern->type == LTE_LC_PERIODIC_SEARCH_PATTERN_TABLE) {
		/* Table format: "<type>,<val1>[,<val2>][,<val3>][,<val4>][,<val5>]". */
		if (pattern->table.val_2 == -1) {
			len = snprintk(buf, buf_size, "\"1,%u\"", pattern->table.val_1);
		} else if (pattern->table.val_3 == -1) {
			len = snprintk(buf, buf_size, "\"1,%u,%u\"", pattern->table.val_1,
				       pattern->table.val_2);
		} else if (pattern->table.val_4 == -1) {
			len = snprintk(buf, buf_size, "\"1,%u,%u,%u\"", pattern->table.val_1,
				       pattern->table.val_2, pattern->table.val_3);
		} else if (pattern->table.val_5 == -1) {
			len = snprintk(buf, buf_size, "\"1,%u,%u,%u,%u\"", pattern->table.val_1,
				       pattern->table.val_2, pattern->table.val_3,
				       pattern->table.val_4);
		} else {
			len = snprintk(buf, buf_size, "\"1,%u,%u,%u,%u,%u\"", pattern->table.val_1,
				       pattern->table.val_2, pattern->table.val_3,
				       pattern->table.val_4, pattern->table.val_5);
		}
	} else {
		LOG_ERR("Unrecognized periodic search pattern type");
		buf[0] = '\0';
	}

	if (len >= buf_size) {
		LOG_ERR("Encoding periodic search pattern failed. Too small buffer (%d/%d)", len,
			buf_size);
		buf[0] = '\0';
	}

	return buf;
}

int periodicsearchconf_set(const struct lte_lc_periodic_search_cfg *const cfg)
{
	int err;
	char pattern_buf[4][40];

	if (!cfg || (cfg->pattern_count == 0) || (cfg->pattern_count > 4)) {
		return -EINVAL;
	}

	/* Command syntax:
	 *	AT%PERIODICSEARCHCONF=<mode>[,<loop>,<return_to_pattern>,<band_optimization>,
	 *	<pattern_1>[,<pattern_2>][,<pattern_3>][,<pattern_4>]]
	 */

	err = nrf_modem_at_printf(
		"AT%%PERIODICSEARCHCONF=0," /* Write mode */
		"%hu,"			    /* <loop> */
		"%hu,"			    /* <return_to_pattern> */
		"%hu,"			    /* <band_optimization> */
		"%s%s"			    /* <pattern_1> */
		"%s%s"			    /* <pattern_2> */
		"%s%s"			    /* <pattern_3> */
		"%s",			    /* <pattern_4> */
		cfg->loop, cfg->return_to_pattern, cfg->band_optimization,
		/* Pattern 1 */
		periodicsearchconf_pattern_get(pattern_buf[0], sizeof(pattern_buf[0]),
					       &cfg->patterns[0]),
		/* Pattern 2, if configured */
		cfg->pattern_count > 1 ? "," : "",
		cfg->pattern_count > 1
			? periodicsearchconf_pattern_get(pattern_buf[1], sizeof(pattern_buf[1]),
							 &cfg->patterns[1])
			: "",
		/* Pattern 3, if configured */
		cfg->pattern_count > 2 ? "," : "",
		cfg->pattern_count > 2
			? periodicsearchconf_pattern_get(pattern_buf[2], sizeof(pattern_buf[2]),
							 &cfg->patterns[2])
			: "",
		/* Pattern 4, if configured */
		cfg->pattern_count > 3 ? "," : "",
		cfg->pattern_count > 3
			? periodicsearchconf_pattern_get(pattern_buf[3], sizeof(pattern_buf[3]),
							 &cfg->patterns[3])
			: "");
	if (err < 0) {
		/* Failure to send the AT command. */
		LOG_ERR("AT command failed, returned error code: %d", err);
		return -EFAULT;
	} else if (err > 0) {
		/* The modem responded with "ERROR". */
		LOG_ERR("The modem rejected the configuration, err: %d", err);
		return -EBADMSG;
	}

	return 0;
}

int periodicsearchconf_get(struct lte_lc_periodic_search_cfg *const cfg)
{

	int err;
	char pattern_buf[4][40];
	uint16_t loop_tmp;

	if (!cfg) {
		return -EINVAL;
	}

	/* Response format:
	 *	%PERIODICSEARCHCONF=<loop>,<return_to_pattern>,<band_optimization>,<pattern_1>
	 *	[,<pattern_2>][,<pattern_3>][,<pattern_4>]
	 */

	err = nrf_modem_at_scanf("AT%PERIODICSEARCHCONF=1",
				 "%%PERIODICSEARCHCONF: "
				 "%hu,"		 /* <loop> */
				 "%hu,"		 /* <return_to_pattern> */
				 "%hu,"		 /* <band_optimization> */
				 "\"%40[^\"]\"," /* <pattern_1> */
				 "\"%40[^\"]\"," /* <pattern_2> */
				 "\"%40[^\"]\"," /* <pattern_3> */
				 "\"%40[^\"]\"", /* <pattern_4> */
				 &loop_tmp, &cfg->return_to_pattern, &cfg->band_optimization,
				 pattern_buf[0], pattern_buf[1], pattern_buf[2], pattern_buf[3]);
	if (err == -NRF_EBADMSG) {
		return -ENOENT;
	} else if (err < 0) {
		return -EFAULT;
	} else if (err < 4) {
		/* Not all parameters and at least one pattern found */
		return -EBADMSG;
	}

	cfg->loop = loop_tmp;

	/* Pattern count is matched parameters minus 3 for loop, return_to_pattern
	 * and band_optimization.
	 */
	cfg->pattern_count = err - 3;

	LOG_DBG("Pattern 1: %s", pattern_buf[0]);

	err = periodicsearchconf_parse(pattern_buf[0], &cfg->patterns[0]);
	if (err) {
		LOG_ERR("Failed to parse periodic search pattern");
		return err;
	}

	if (cfg->pattern_count >= 2) {
		LOG_DBG("Pattern 2: %s", pattern_buf[1]);

		err = periodicsearchconf_parse(pattern_buf[1], &cfg->patterns[1]);
		if (err) {
			LOG_ERR("Failed to parse periodic search pattern");
			return err;
		}
	}

	if (cfg->pattern_count >= 3) {
		LOG_DBG("Pattern 3: %s", pattern_buf[2]);

		err = periodicsearchconf_parse(pattern_buf[2], &cfg->patterns[2]);
		if (err) {
			LOG_ERR("Failed to parse periodic search pattern");
			return err;
		}
	}

	if (cfg->pattern_count == 4) {
		LOG_DBG("Pattern 4: %s", pattern_buf[3]);

		err = periodicsearchconf_parse(pattern_buf[3], &cfg->patterns[3]);
		if (err) {
			LOG_ERR("Failed to parse periodic search pattern");
			return err;
		}
	}

	return 0;
}

int periodicsearchconf_clear(void)
{
	int err;

	err = nrf_modem_at_printf("AT%%PERIODICSEARCHCONF=2");
	if (err < 0) {
		return -EFAULT;
	} else if (err > 0) {
		return -EBADMSG;
	}

	return 0;
}

int periodicsearchconf_request(void)
{
	return nrf_modem_at_printf("AT%%PERIODICSEARCHCONF=3") ? -EFAULT : 0;
}
