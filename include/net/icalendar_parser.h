/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ICAL_PARSER_H__
#define ICAL_PARSER_H__

#include <zephyr.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file icalendar_parser.h
 *
 * @brief API for parsing iCalendar format calendaring information.
 * @defgroup icalendar_parser API for parsing iCalendar
 * @{
 */

/**
 * @brief iCalendar parser event IDs.
 */
enum ical_parser_evt_id {
	/** Calendar component parsed. */
	ICAL_PARSER_EVT_COMPONENT,
};

/**
 * @brief type of iCalendar component.
 */
enum ical_component_type {
	/** Event Component */
	ICAL_COMPOMEMT_EVENT,
	/** Todo Component */
	ICAL_COMPOMEMT_TODO,
	/** Journal Component */
	ICAL_COMPOMEMT_JOURNAL,
	/** Timezone Component */
	ICAL_COMPOMEMT_TIMEZONE,
	/** FreeBusy Component */
	ICAL_COMPOMEMT_FREEBUSY,
};

/**
 * @brief iCalendar parser event.
 */
struct ical_parser_evt {
	/** Event ID. */
	enum ical_parser_evt_id id;
	union {
		/** Error cause. */
		int error;
	};
};

/**
 * @brief iCalendar component.
 */
struct ical_component {
	/** SUMMARY property */
	char summary[CONFIG_SUMMARY_SIZE];
	/** LOCATION property buffer. */
	char location[CONFIG_LOCATION_SIZE];
	/** DESCRIPTION property buffer. */
	char description[CONFIG_DESCRIPTION_SIZE];
	/** DTSTART property. */
	char dtstart[CONFIG_DTSTART_SIZE];
	/** DTEND property buffer. */
	char dtend[CONFIG_DTEND_SIZE];
};

/**
 * @brief iCalendar parser asynchronous event handler.
 *
 * Through this callback, the application receives events, such as
 * parsed component, download completion, or errors.
 *
 *
 * @param[out]  event	The event.
 * @param[out] parsed iCalendar component.
 *
 * @return Zero to continue the download, non-zero otherwise.
 */
typedef int (*icalendar_parser_callback_t)(
	const struct ical_parser_evt *event,
	struct ical_component *p_ical_component);

/**
 * @brief iCalendar parser instance.
 */
struct icalendar_parser {
	/** Internal Buffer for parsing incoming data stream */
	char buf[CONFIG_ICAL_PARSER_BUFFER_SIZE + 1];
	/** Offset of unparsed data in buf. */
	size_t offset;
	/** begin of iCalendar object delimiter pair */
	bool icalobject_begin;
	/** Event handler. */
	icalendar_parser_callback_t callback;
};

/**
 * @brief Initialize iCalendar parser.
 *
 * @param[in] ical iCalendar parser instance.
 * @param[in] callback Callback for sending calendar parsing event.
 *
 * @return 0 If successful, or an error code on failure.
 */
int ical_parser_init(struct icalendar_parser *const ical,
		     icalendar_parser_callback_t callback);

/**
 * @brief Parse the iCalendar data stream. Return the parsed bytes.
 *
 * @param[in] ical iCalendar parser instance.
 * @param[in] data Input data to be parsed.
 * @param[in] len  Length of input data stream.
 *
 * @retval size_t  Parsed bytes.
 */
size_t ical_parser_parse(struct icalendar_parser *ical,
			const char *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* ICAL_PARSER_H__ */

/**@} */
