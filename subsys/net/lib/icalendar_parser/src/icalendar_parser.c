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

static size_t unfold_contentline(const char *p_buf, char *p_ical_prop_buf)
{
	size_t unfold_size = 0;
	const char *p_sol = p_buf;
	char *p_eol;
	u16_t total_line_len = 0, single_line_len = 0;

	while (1) {
		p_eol = strstr(p_sol, "\r\n");
		if (!p_eol) {
			return 0;
		}
		single_line_len = p_eol - p_sol;
		if ((total_line_len + single_line_len)
		    <= CONFIG_MAX_PROPERTY_SIZE) {
			memcpy(p_ical_prop_buf +
			       total_line_len, p_sol, single_line_len);
			total_line_len += single_line_len;
		} else {
			LOG_DBG("Property value overflow."
				"Increase CONFIG_MAX_PROPERTY_SIZE.");
			return 0;
		}
		if (!strncmp(p_eol, "\r\n ", strlen("\r\n "))) {
			/* Long content line is split into multiple lines.
			 * Copy the folded part to unfolded buffer.
			 */
			p_sol = p_eol + strlen("\r\n ");
		} else {
			/* Content line is delimited. Count parsed bytes */
			unfold_size = p_eol - p_buf;
			break;
		}
	}

	return unfold_size;
}

static bool parse_desc_props(const char *p_buf,
			     const char *p_name,
			     size_t name_size,
			     char *p_value,
			     size_t max_value_len)
{
	bool ret;
	size_t unfold_size;
	char ical_prop_buf[CONFIG_MAX_PROPERTY_SIZE];

	unfold_size = unfold_contentline(p_buf, ical_prop_buf);
	if (unfold_size > 0) {
		if (ical_prop_buf[name_size] == ':') {
			size_t value_len = unfold_size - name_size - 1;

			if (value_len <= max_value_len) {
				memcpy(p_value,
				       ical_prop_buf + name_size + 1,
				       value_len);
				ret = true;
			} else {
				/* Property value overflow. */
				LOG_ERR("%s value overflow.", p_name);
				ret = false;
			}
		} else if (ical_prop_buf[name_size] == ';') {
			/* Does not support property parameter. */
			LOG_ERR("%s param not supported.", p_name);
			ret = false;
		} else {
			/* Property wrong format - no parameter or value. */
			LOG_ERR("%s wrong format.", p_name);
			ret = false;
		}
	} else {
		/* Property wrong format - no parameter or value. */
		LOG_ERR("%s no value/param.", p_name);
		ret = false;
	}
	return ret;
}

static bool parse_datetime_props(const char *p_buf,
				 const char *p_name,
				 size_t name_size,
				 char *p_value,
				 size_t max_value_len)
{
	bool ret;
	size_t unfold_size;
	char ical_prop_buf[CONFIG_MAX_PROPERTY_SIZE];

	memset(ical_prop_buf, 0, sizeof(ical_prop_buf));
	unfold_size = unfold_contentline(p_buf, ical_prop_buf);
	if (unfold_size > 0) {
		if (ical_prop_buf[name_size] == ':') {
			size_t value_len = unfold_size - name_size - 1;

			if (value_len <= max_value_len) {
				memcpy(p_value,
				       ical_prop_buf + name_size + 1,
				       value_len);
				ret = true;
			} else {
				/* Property value overflow. */
				LOG_ERR("%s value overflow.", p_name);
				ret = false;
			}
		} else if (ical_prop_buf[name_size] == ';') {
			char *p_dtvalue;

			p_dtvalue = strchr(ical_prop_buf, ':');
			if (p_dtvalue) {
				p_dtvalue = p_dtvalue + 1;
				size_t value_len;

				value_len = unfold_size -
					    (p_dtvalue - ical_prop_buf);
				if (value_len <= max_value_len) {
					memcpy(p_value, p_dtvalue, value_len);
					ret = true;
				} else {
					/* Property value overflow. */
					LOG_ERR("%s value overflow.", p_name);
					ret = false;
				}
			} else {
				/* Property wrong format - no value. */
				LOG_ERR("%s wrong format - no value.", p_name);
				ret = false;
			}
		} else {
			/* Property wrong format - no parameter or value. */
			LOG_ERR("%s wrong format.", p_name);
			ret = false;
		}
	} else {
		/* Property wrong format - fail to unfold property. */
		LOG_ERR("%s wrong format. Fail to unfold property", p_name);
		ret = false;
	}

	return ret;
}

static size_t parse_calprops(const char *p_buf)
{
	const char *p_parsed = p_buf;
	char *p_end = NULL;
	char prop_value[CONFIG_MAX_PROPERTY_SIZE];

	p_parsed = strstr(p_parsed, "BEGIN:VCALENDAR\r\n");
	if (p_parsed) {

		p_parsed += strlen("BEGIN:VCALENDAR\r\n");
		p_end = strstr(p_parsed, "\r\nBEGIN:");
		if (p_end) {
			p_end += strlen("\r\n");
			while (p_parsed < p_end) {
				memset(prop_value, 0, sizeof(prop_value));
				if (!strncmp(p_parsed, "PRODID", 6)) {
					if (!parse_desc_props(
						    p_parsed, "PRODID", 6,
						    prop_value,
						    sizeof(prop_value))) {
						LOG_ERR("Wrong PRODID");
					}
				} else if (!strncmp(p_parsed, "VERSION", 7)) {
					if (!parse_desc_props(
						    p_parsed, "VERSION", 7,
						    prop_value,
						    sizeof(prop_value))) {
						LOG_ERR("Wrong VERSION");
					}
				}
				p_parsed = strstr(p_parsed, "\r\n");
				p_parsed += strlen("\r\n");
			}
		} else {
			return 0;
		}
	} else {
		return 0;
	}

	return (p_parsed - p_buf);
}

static size_t parse_eventprop(const char *p_buf,
			      struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;

	if (!strncmp(p_buf, "BEGIN:VEVENT\r\n", strlen("BEGIN:VEVENT\r\n"))) {
		char *p_com_end;

		p_com_end = strstr(p_buf, "END:VEVENT\r\n");
		if (p_com_end) {
			p_com_end += strlen("END:VEVENT\r\n");
			while (p_parsed < p_com_end) {
				if (!strncasecmp(p_parsed,
						 "SUMMARY", 7)) {
					if (!parse_desc_props(
						p_parsed, "SUMMARY", 7,
						p_ical_com->summary,
						CONFIG_SUMMARY_SIZE)) {
						LOG_ERR("Wrong SUMMARY");
					}
				} else if (!strncasecmp(p_parsed,
							"LOCATION", 8)) {
					if (!parse_desc_props(
						p_parsed, "LOCATION", 8,
						p_ical_com->location,
						CONFIG_LOCATION_SIZE)) {
						LOG_ERR("Wrong LOCATION");
					}
				} else if (!strncasecmp(p_parsed,
							"DESCRIPTION", 11)) {
					if (!parse_desc_props(
						p_parsed, "DESCRIPTION", 11,
						p_ical_com->description,
						CONFIG_DESCRIPTION_SIZE)) {
						LOG_ERR("Wrong DESCRIPTION");
					}
				} else if (!strncasecmp(p_parsed,
							"DTSTART", 7)) {
					if (!parse_datetime_props(
						p_parsed, "DTSTART", 7,
						p_ical_com->dtstart,
						CONFIG_DTSTART_SIZE)) {
						LOG_ERR("Wrong DTSTART");
					}
				} else if (!strncasecmp(p_parsed,
							"DTEND", 5)) {
					if (!parse_datetime_props(
						p_parsed, "DTEND", 5,
						p_ical_com->dtend,
						CONFIG_DTEND_SIZE)) {
						LOG_ERR("Wrong DTEND");
					}
				}

				/* Move parsed pointer to end of line break */
				p_parsed = strstr(p_parsed, "\r\n") + 2;
			}
		}
	}

	return p_parsed - p_buf;
}

static size_t parse_todoprop(const char *p_buf,
				struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;
	char *p_com_end;

	p_com_end = strstr(p_buf, "END:VTODO\r\n");
	if (p_com_end) {
		p_parsed = p_com_end + strlen("END:VTODO\r\n");
	}

	return p_parsed - p_buf;
}

static size_t parse_jourprop(const char *p_buf,
				struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;
	char *p_com_end;

	p_com_end = strstr(p_buf, "END:VJOURNAL\r\n");
	if (p_com_end) {
		p_parsed = p_com_end + strlen("END:VJOURNAL\r\n");
	}

	return p_parsed - p_buf;
}

static size_t parse_fbprop(const char *p_buf, struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;
	char *p_com_end;

	p_com_end = strstr(p_buf, "END:VFREEBUSY\r\n");
	if (p_com_end) {
		p_parsed = p_com_end + strlen("END:VFREEBUSY\r\n");
	}

	return p_parsed - p_buf;
}

static size_t parse_tzprop(const char *p_buf, struct ical_component *p_ical_com)
{
	const char *p_parsed = p_buf;
	char *p_com_end;

	p_com_end = strstr(p_buf, "END:VTIMEZONE\r\n");
	if (p_com_end) {
		p_parsed = p_com_end + strlen("END:VTIMEZONE\r\n");
	}

	return p_parsed - p_buf;
}

static size_t parse_component(char *buf,
			      struct ical_component *p_ical_com)
{
	char *p_com_begin = buf;
	size_t ret = 0;

	/* Search begin of component */
	if (!strncmp(p_com_begin, "BEGIN:VEVENT\r\n", 14)) {
		ret += parse_eventprop(p_com_begin, p_ical_com);
	} else if (!strncmp(p_com_begin, "BEGIN:VTODO\r\n", 13)) {
		ret += parse_todoprop(p_com_begin, p_ical_com);
	} else if (!strncmp(p_com_begin, "BEGIN:VJOURNAL\r\n", 16)) {
		ret += parse_jourprop(p_com_begin, p_ical_com);
	} else if (!strncmp(p_com_begin, "BEGIN:VFREEBUSY\r\n", 17)) {
		ret += parse_fbprop(p_com_begin, p_ical_com);
	} else if (!strncmp(p_com_begin, "BEGIN:VTIMEZONE\r\n", 17)) {
		ret += parse_tzprop(p_com_begin, p_ical_com);
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

int ical_parser_init(struct icalendar_parser *const ical,
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
