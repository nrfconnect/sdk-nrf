/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <stdio.h>
#include <strings.h>
#include <zephyr.h>
#include <zephyr/types.h>
#include <toolchain/common.h>
#include <net/icalendar_parser.h>
#include <logging/log.h>

LOG_MODULE_REGISTER(icalendar_parser, CONFIG_ICAL_PARSER_LOG_LEVEL);

static size_t unfold_contentline(const char *buf, char *prop_buf)
{
	size_t unfold_size = 0;
	const char *sol = buf;
	char *eol;
	u16_t total_line_len = 0, single_line_len = 0;

	while (1) {
		eol = strstr(sol, "\r\n");
		if (!eol) {
			return 0;
		}
		single_line_len = eol - sol;
		if ((total_line_len + single_line_len)
		    <= CONFIG_MAX_PROPERTY_SIZE) {
			memcpy(prop_buf +
			       total_line_len, sol, single_line_len);
			total_line_len += single_line_len;
		} else {
			LOG_DBG("Property value overflow."
				"Increase CONFIG_MAX_PROPERTY_SIZE.");
			return 0;
		}
		if (!strncmp(eol, "\r\n ", strlen("\r\n "))) {
			/* Long content line is split into multiple lines.
			 * Copy the folded part to unfolded buffer.
			 */
			sol = eol + strlen("\r\n ");
		} else {
			/* Content line is delimited. Count parsed bytes */
			unfold_size = eol - buf;
			break;
		}
	}

	return unfold_size;
}

static bool parse_desc_props(const char *buf,
			     const char *name,
			     size_t name_size,
			     char *value,
			     size_t max_value_len)
{
	bool ret;
	size_t unfold_size;
	char ical_prop_buf[CONFIG_MAX_PROPERTY_SIZE];

	unfold_size = unfold_contentline(buf, ical_prop_buf);
	if (unfold_size <= 0) {
		/* Property wrong format - no parameter or value. */
		LOG_ERR("%s no value/param.", name);
		return false;
	}

	if (ical_prop_buf[name_size] == ':') {
		size_t value_len = unfold_size - name_size - 1;

		if (value_len <= max_value_len) {
			memcpy(value,
				ical_prop_buf + name_size + 1,
				value_len);
			ret = true;
		} else {
			/* Property value overflow. */
			LOG_ERR("%s value overflow.", name);
			ret = false;
		}
	} else if (ical_prop_buf[name_size] == ';') {
		/* Does not support property parameter. */
		LOG_ERR("%s param not supported.", name);
		ret = false;
	} else {
		/* Property wrong format - no parameter or value. */
		LOG_ERR("%s wrong format.", name);
		ret = false;
	}

	return ret;
}

static bool parse_datetime_props(const char *buf,
				 const char *name,
				 size_t name_size,
				 char *value,
				 size_t max_value_len)
{
	bool ret;
	size_t unfold_size;
	char ical_prop_buf[CONFIG_MAX_PROPERTY_SIZE];

	unfold_size = unfold_contentline(buf, ical_prop_buf);
	if (unfold_size <= 0) {
		/* Property wrong format - fail to unfold property. */
		LOG_ERR("%s wrong format. Fail to unfold property", name);
		return false;
	}

	if (ical_prop_buf[name_size] == ':') {
		size_t value_len = unfold_size - name_size - 1;

		if (value_len <= max_value_len) {
			memcpy(value,
				ical_prop_buf + name_size + 1,
				value_len);
			ret = true;
		} else {
			/* Property value overflow. */
			LOG_ERR("%s value overflow.", name);
			ret = false;
		}
	} else if (ical_prop_buf[name_size] == ';') {
		char *dtvalue;

		dtvalue = strchr(ical_prop_buf, ':');
		if (dtvalue) {
			dtvalue = dtvalue + 1;
			size_t value_len;

			value_len = unfold_size -
					(dtvalue - ical_prop_buf);
			if (value_len <= max_value_len) {
				memcpy(value, dtvalue, value_len);
				ret = true;
			} else {
				/* Property value overflow. */
				LOG_ERR("%s value overflow.", name);
				ret = false;
			}
		} else {
			/* Property wrong format - no value. */
			LOG_ERR("%s wrong format - no value.", name);
			ret = false;
		}
	} else {
		/* Property wrong format - no parameter or value. */
		LOG_ERR("%s wrong format.", name);
		ret = false;
	}

	return ret;
}

static size_t parse_calprops(const char *buf)
{
	const char *parsed = buf;
	char *end = NULL;
	char prop_value[CONFIG_MAX_PROPERTY_SIZE];

	parsed = strstr(parsed, "BEGIN:VCALENDAR\r\n");
	if (parsed == NULL) {
		return 0;
	}
	parsed += strlen("BEGIN:VCALENDAR\r\n");

	end = strstr(parsed, "\r\nBEGIN:");
	if (end == NULL) {
		return 0;
	}
	end += strlen("\r\n");

	while (parsed < end) {
		memset(prop_value, 0, sizeof(prop_value));
		if (!strncmp(parsed, "PRODID", 6)) {
			if (!parse_desc_props(parsed, "PRODID", 6,
					prop_value, sizeof(prop_value))) {
				LOG_ERR("Wrong PRODID");
			}
		} else if (!strncmp(parsed, "VERSION", 7)) {
			if (!parse_desc_props(parsed, "VERSION", 7,
					prop_value, sizeof(prop_value))) {
				LOG_ERR("Wrong VERSION");
			}
		}
		parsed = strstr(parsed, "\r\n");
		parsed += strlen("\r\n");
	}

	return (parsed - buf);
}

static size_t parse_eventprop(const char *buf,
			      struct ical_component *ical_com)
{
	const char *parsed = buf;
	char *com_end;

	if (strncmp(buf, "BEGIN:VEVENT\r\n", strlen("BEGIN:VEVENT\r\n"))) {
		return 0;
	}

	com_end = strstr(buf, "END:VEVENT\r\n");
	if (com_end == NULL) {
		return 0;
	}

	com_end += strlen("END:VEVENT\r\n");
	while (parsed < com_end) {
		if (!strncasecmp(parsed, "SUMMARY", 7)) {
			if (!parse_desc_props(
				parsed, "SUMMARY", 7,
				ical_com->summary,
				CONFIG_SUMMARY_SIZE)) {
				LOG_ERR("Wrong SUMMARY");
			}
		} else if (!strncasecmp(parsed, "LOCATION", 8)) {
			if (!parse_desc_props(
				parsed, "LOCATION", 8,
				ical_com->location,
				CONFIG_LOCATION_SIZE)) {
				LOG_ERR("Wrong LOCATION");
			}
		} else if (!strncasecmp(parsed, "DESCRIPTION", 11)) {
			if (!parse_desc_props(
				parsed, "DESCRIPTION", 11,
				ical_com->description,
				CONFIG_DESCRIPTION_SIZE)) {
				LOG_ERR("Wrong DESCRIPTION");
			}
		} else if (!strncasecmp(parsed, "DTSTART", 7)) {
			if (!parse_datetime_props(
				parsed, "DTSTART", 7,
				ical_com->dtstart,
				CONFIG_DTSTART_SIZE)) {
				LOG_ERR("Wrong DTSTART");
			}
		} else if (!strncasecmp(parsed,
					"DTEND", 5)) {
			if (!parse_datetime_props(
				parsed, "DTEND", 5,
				ical_com->dtend,
				CONFIG_DTEND_SIZE)) {
				LOG_ERR("Wrong DTEND");
			}
		}

		/* Move parsed pointer to end of line break */
		parsed = strstr(parsed, "\r\n") + 2;
	}

	return parsed - buf;
}

static size_t parse_todoprop(const char *buf,
				struct ical_component *ical_com)
{
	const char *parsed = buf;
	char *com_end;

	com_end = strstr(buf, "END:VTODO\r\n");
	if (com_end) {
		parsed = com_end + strlen("END:VTODO\r\n");
	}

	return parsed - buf;
}

static size_t parse_jourprop(const char *buf,
				struct ical_component *ical_com)
{
	const char *parsed = buf;
	char *com_end;

	com_end = strstr(buf, "END:VJOURNAL\r\n");
	if (com_end) {
		parsed = com_end + strlen("END:VJOURNAL\r\n");
	}

	return parsed - buf;
}

static size_t parse_fbprop(const char *buf, struct ical_component *ical_com)
{
	const char *parsed = buf;
	char *com_end;

	com_end = strstr(buf, "END:VFREEBUSY\r\n");
	if (com_end) {
		parsed = com_end + strlen("END:VFREEBUSY\r\n");
	}

	return parsed - buf;
}

static size_t parse_tzprop(const char *buf, struct ical_component *ical_com)
{
	const char *parsed = buf;
	char *com_end;

	com_end = strstr(buf, "END:VTIMEZONE\r\n");
	if (com_end) {
		parsed = com_end + strlen("END:VTIMEZONE\r\n");
	}

	return parsed - buf;
}

static size_t parse_component(char *buf,
			      struct ical_component *ical_com)
{
	char *com_begin = buf;
	size_t ret = 0;

	/* Search begin of component */
	if (!strncmp(com_begin, "BEGIN:VEVENT\r\n", 14)) {
		ret += parse_eventprop(com_begin, ical_com);
	} else if (!strncmp(com_begin, "BEGIN:VTODO\r\n", 13)) {
		ret += parse_todoprop(com_begin, ical_com);
	} else if (!strncmp(com_begin, "BEGIN:VJOURNAL\r\n", 16)) {
		ret += parse_jourprop(com_begin, ical_com);
	} else if (!strncmp(com_begin, "BEGIN:VFREEBUSY\r\n", 17)) {
		ret += parse_fbprop(com_begin, ical_com);
	} else if (!strncmp(com_begin, "BEGIN:VTIMEZONE\r\n", 17)) {
		ret += parse_tzprop(com_begin, ical_com);
	}

	return ret;
}

static size_t parse_icalbody(char *buf,
				icalendar_parser_callback_t callback)
{
	size_t parsed_bytes = 0, parsed_offset = 0;

	do {
		struct ical_component ical_com;

		memset(&ical_com, 0, sizeof(ical_com));
		parsed_bytes = parse_component(buf + parsed_offset,
					       &ical_com);
		if (parsed_bytes > 0) {
			const struct ical_parser_evt evt = {
				.id = ICAL_PARSER_EVT_COMPONENT,
			};
			callback(&evt, &ical_com);
		}
		parsed_offset += parsed_bytes;
	} while (parsed_bytes > 0);

	return parsed_offset;
}

size_t ical_parser_parse(struct icalendar_parser *ical,
			const char *data, size_t len)
{
	size_t parsed_offset = 0;

	if (CONFIG_ICAL_PARSER_BUFFER_SIZE < (ical->offset + len)) {
		return -ENOBUFS;
	}

	memcpy(ical->buf + ical->offset, data, len);
	ical->buf[ical->offset + len] = '\0';

	/* Check begin of iCalendar object delimiter
	 * Reference: RFC 5545 3.4 iCalendar Object
	 */
	if (!ical->icalobject_begin) {
		/* The body of the iCalendar object consists of
		 * a sequence of calendar properties and
		 * one or more calendar components.
		 *
		 * Reference: RFC 5545 3.6 Calendar Components
		 */
		parsed_offset += parse_calprops(ical->buf);
		if (parsed_offset > 0) {
			LOG_DBG("Found a calendar stream");
			ical->icalobject_begin = true;
		}
	}

	/* If we got a calendar property, start parsing calendar body. */
	if (ical->icalobject_begin) {
		parsed_offset += parse_icalbody(ical->buf + parsed_offset,
						ical->callback);
	}

	if (parsed_offset) {
		ical->offset = ical->offset + len - parsed_offset;
		memcpy(ical->buf, ical->buf + parsed_offset, ical->offset);
	}

	return parsed_offset;
}

int ical_parser_init(struct icalendar_parser *ical,
		     icalendar_parser_callback_t callback)
{
	if (ical == NULL || callback == NULL) {
		return -EINVAL;
	}

	ical->callback = callback;
	ical->icalobject_begin = false;
	ical->offset = 0;

	return 0;
}
