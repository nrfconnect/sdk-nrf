/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/types.h>

#include <modem/at_cmd_parser.h>
#include "at_utils.h"

#define AT_CMD_MAX_ARRAY_SIZE 32

#define AT_CMD_CGEV_LEN         5
#define AT_CMD_CPIN_LEN         5
#define AT_CMD_SHORTSWVER_LEN   11
#define AT_CMD_HWVERSION_LEN    10
#define AT_CMD_XMODEMUUID_LEN   11
#define AT_CMD_XICCID_LEN       7

enum at_parser_state {
	IDLE,
	ARRAY,
	STRING,
	QUOTED_STRING,
	NUMBER,
	SMS_PDU,
	NOTIFICATION,
	COMMAND,
	OPTIONAL,
	CLAC,
};

static enum at_parser_state state;

static bool set_type_string;

static inline void set_new_state(enum at_parser_state new_state)
{
	state = new_state;
}

static inline void reset_state(void)
{
	state = IDLE;

	set_type_string = false;
}

static inline void skip_command_prefix(const char **cmd)
{
	*cmd += sizeof("AT") - 1;

	if (is_lfcr(**cmd) || is_terminated(**cmd)) {
		return;
	}

	(*cmd)++;
}

static inline bool check_response_for_forced_string(const char *tmpstr)
{
	bool retval = false;

	if (!strncmp(tmpstr, "+CGEV", AT_CMD_CGEV_LEN) ||
	    !strncmp(tmpstr, "+CPIN", AT_CMD_CPIN_LEN) ||
	    !strncmp(tmpstr, "%SHORTSWVER", AT_CMD_SHORTSWVER_LEN) ||
	    !strncmp(tmpstr, "%HWVERSION", AT_CMD_HWVERSION_LEN) ||
	    !strncmp(tmpstr, "%XMODEMUUID", AT_CMD_XMODEMUUID_LEN) ||
	    !strncmp(tmpstr, "%XICCID", AT_CMD_XICCID_LEN)) {
			retval = true;
	}

	return retval;
}

static bool is_result(const char *str)
{
	int diff;
	static const char * const toclip[] = {
		"OK\r\n",
		"ERROR\r\n",
		"+CME ERROR",
		"+CMS ERROR"
	};

	for (size_t i = 0; i < ARRAY_SIZE(toclip); i++) {
		diff = strncmp(str, toclip[i], strlen(toclip[i]));
		if (!diff) {
			return true;
		}
	}

	return false;
}

static int at_parse_detect_type(const char **str, int index)
{
	const char *tmpstr = *str;

	if ((index == 0) && is_notification(*tmpstr)) {
		/* Only first parameter in the string can be
		 * notification ID, (eg +CEREG:)
		 */
		set_new_state(NOTIFICATION);

		/* Check for responses we know need to be strings */
		set_type_string = check_response_for_forced_string(tmpstr);

	} else if (set_type_string) {
		set_new_state(STRING);
	} else if ((index > 0) && is_clac(tmpstr)) {
		/* Next, check if we deal with CLAC response (eg AT+, AT%)
		 * NOTE - need to go back to index 0 and parse as CLAC state
		 * NOTE - AT+CLAC always returns more than one line
		 */
		set_new_state(CLAC);
		return -2;
	} else if ((index == 0) && is_command(tmpstr)) {
		/* Next, check if we deal with command (eg AT+CCLK) */
		set_new_state(COMMAND);
	} else if (index == 0) {
		/* If the string start without an notification
		 * ID, we treat the whole string as one string
		 * parameter
		 */
		set_new_state(STRING);
	} else if ((index > 0) && is_notification(*tmpstr)) {
		/* If notifications is detected later in the
		 * string we should stop parsing and return
		 * EAGAIN
		 */
		*str = tmpstr;
		return -1;
	} else if (is_number(*tmpstr)) {
		set_new_state(NUMBER);

	} else if (is_dblquote(*tmpstr)) {
		set_new_state(QUOTED_STRING);
		tmpstr++;
	} else if (is_array_start(*tmpstr)) {
		set_new_state(ARRAY);
		tmpstr++;
	} else if (is_lfcr(*tmpstr) && (state == NUMBER)) {
		/* If \n or \r is detected in the string and the
		 * previous param was a number we assume the
		 * next parameter is PDU data
		 */

		while (is_lfcr(*tmpstr)) {
			tmpstr++;
		}

		set_new_state(SMS_PDU);
	} else if (is_lfcr(*tmpstr) && (state == OPTIONAL)) {
		set_new_state(OPTIONAL);
	} else if (is_separator(*tmpstr)) {
		/* If a separator is detected we have detected
		 * and empty optional parameter
		 */
		set_new_state(OPTIONAL);
	} else {
		/* The rule set is exhausted, and cannot
		 * continue. Break the loop and return an error
		 */
		*str = tmpstr;
		return -1;
	}

	*str = tmpstr;
	return 0;
}

static int at_parse_process_element(const char **str, int index,
				    struct at_param_list *const list)
{
	const char *tmpstr = *str;

	if (is_terminated(*tmpstr)) {
		return -1;
	}

	if (state == NOTIFICATION) {
		const char *start_ptr = tmpstr++;

		while (is_valid_notification_char(*tmpstr)) {
			tmpstr++;
		}

		at_params_string_put(list, index, start_ptr,
				     tmpstr - start_ptr);
	} else if (state == COMMAND) {
		const char *start_ptr = tmpstr;

		skip_command_prefix(&tmpstr);

		while (is_valid_command_char(*tmpstr)) {
			tmpstr++;
		}

		at_params_string_put(list, index, start_ptr,
				     tmpstr - start_ptr);

		/* Skip read/test special characters. */
		if ((*tmpstr == AT_CMD_SEPARATOR) &&
		    (*(tmpstr + 1) == AT_CMD_READ_TEST_IDENTIFIER)) {
			tmpstr += 2;
		} else if (*tmpstr == AT_CMD_READ_TEST_IDENTIFIER) {
			tmpstr++;
		}

	} else if (state == OPTIONAL) {
		at_params_empty_put(list, index);

	} else if (state == STRING) {
		const char *start_ptr = tmpstr;

		while (!is_lfcr(*tmpstr) && !is_terminated(*tmpstr)) {
			tmpstr++;
		}

		at_params_string_put(list, index, start_ptr,
				     tmpstr - start_ptr);

		tmpstr++;
	} else if (state == QUOTED_STRING) {
		const char *start_ptr = tmpstr;

		while (!is_dblquote(*tmpstr) && !is_terminated(*tmpstr)) {
			tmpstr++;
		}

		at_params_string_put(list, index, start_ptr,
				     tmpstr - start_ptr);

		tmpstr++;
	} else if (state == ARRAY) {
		char *next;
		size_t i = 0;
		uint32_t tmparray[AT_CMD_MAX_ARRAY_SIZE];

		tmparray[i++] = (uint32_t)strtoul(tmpstr, &next, 10);
		tmpstr = next;

		while (!is_array_stop(*tmpstr) && !is_terminated(*tmpstr)) {
			if (is_separator(*tmpstr)) {
				tmparray[i++] =
					(uint32_t)strtoul(++tmpstr, &next, 10);

				if (strlen(tmpstr) == strlen(next)) {
					break;
				} else {
					tmpstr = next;
				}

			} else {
				tmpstr++;
			}

			if (i == AT_CMD_MAX_ARRAY_SIZE) {
				break;
			}
		}

		at_params_array_put(list, index, tmparray, i * sizeof(uint32_t));

		tmpstr++;
	} else if (state == NUMBER) {
		char *next;
		int64_t value = (int64_t)strtoll(tmpstr, &next, 10);

		tmpstr = next;

		at_params_int_put(list, index, value);
	} else if (state == SMS_PDU) {
		const char *start_ptr = tmpstr;

		while (isxdigit((int)*tmpstr)) {
			tmpstr++;
		}

		at_params_string_put(list, index, start_ptr,
				     tmpstr - start_ptr);
	} else if (state == CLAC) {
		const char *start_ptr = tmpstr;

		while (!is_terminated(*tmpstr)) {
			tmpstr++;
		}

		at_params_string_put(list, index, start_ptr,
				     tmpstr - start_ptr);
	}

	*str = tmpstr;
	return 0;
}

/*
 * Internal function.
 * Parameters cannot be null. String must be null terminated.
 */
static int at_parse_param(const char **at_params_str,
			  struct at_param_list *const list,
			  const size_t max_params)
{
	int index = 0;
	const char *str = *at_params_str;
	bool oversized = false;
	int ret;

	reset_state();

	/* trim leading CRLF */
	while (is_lfcr(*str)) {
		str++;
	}

	while ((!is_terminated(*str)) && (index < max_params)) {
		if (isspace((int)*str)) {
			str++;
		}

		ret = at_parse_detect_type(&str, index);
		if (ret == -1) {
			break;
		}
		if (ret == -2) {
			/* CLAC response */
			str = *at_params_str;
			index = 0;
		}

		if (at_parse_process_element(&str, index, list) == -1) {
			break;
		}

		if (is_separator(*str)) {
			if (is_lfcr(*(str + 1))) {
				/* Make sure we catch the last empty parameter
				 **/
				index++;

				if (index == max_params) {
					oversized = true;
					break;
				}

				if (at_parse_detect_type(&str, index) == -1) {
					break;
				}

				if (at_parse_process_element(&str, index,
							     list) == -1) {
					break;
				}
			}

			str++;
		}

		/* Peek forward to see if we will be terminated */
		if (is_lfcr(*str)) {
			int i = 0;

			while (is_lfcr(str[++i])) {
			}

			if (is_terminated(str[i]) || is_notification(str[i]) ||
			    is_result(str + i)) {
				str += i;
				break;
			}
		}

		index++;

		if (index == max_params) {
			oversized = true;
		}
	}

	*at_params_str = str;

	if (oversized) {
		return -E2BIG;
	}

	if (!is_terminated(*str) && !is_result(str)) {
		return -EAGAIN;
	}

	return 0;
}

int at_parser_params_from_str(const char *at_params_str, char **next_params_str,
			      struct at_param_list *const list)
{
	return at_parser_max_params_from_str(at_params_str, next_params_str,
					     list, list->param_count);
}

int at_parser_max_params_from_str(const char *at_params_str,
				  char **next_param_str,
				  struct at_param_list *const list,
				  size_t max_params_count)
{
	int err = 0;

	if (at_params_str == NULL || list == NULL || list->params == NULL) {
		return -EINVAL;
	}

	at_params_list_clear(list);

	max_params_count = MIN(max_params_count, list->param_count);

	err = at_parse_param(&at_params_str, list, max_params_count);

	if (next_param_str) {
		*next_param_str = (char *)at_params_str;
	}

	return err;
}

enum at_cmd_type at_parser_cmd_type_get(const char *at_cmd)
{
	enum at_cmd_type type;

	if (!is_command(at_cmd)) {
		return AT_CMD_TYPE_UNKNOWN;
	}

	skip_command_prefix(&at_cmd);

	while (is_valid_notification_char(*at_cmd)) {
		at_cmd++;
	}

	if ((*at_cmd == AT_CMD_SEPARATOR) &&
	    (*(at_cmd + 1) == AT_CMD_READ_TEST_IDENTIFIER)) {
		type = AT_CMD_TYPE_TEST_COMMAND;
	} else if (*at_cmd == AT_CMD_READ_TEST_IDENTIFIER) {
		type = AT_CMD_TYPE_READ_COMMAND;
	} else if ((*at_cmd == AT_CMD_SEPARATOR) || is_lfcr(*at_cmd) ||
		   is_terminated(*at_cmd)) {
		type = AT_CMD_TYPE_SET_COMMAND;
	} else {
		type = AT_CMD_TYPE_UNKNOWN;
	}

	return type;
}
