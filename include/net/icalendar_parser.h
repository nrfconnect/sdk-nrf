/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef ICAL_PARSER_H__
#define ICAL_PARSER_H__

#include <zephyr/kernel.h>
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
	/** Event Component parsed */
	ICAL_EVT_VEVENT,
	/** Todo Component parsed */
	ICAL_EVT_VTODO,
	/** Journal Component parsed */
	ICAL_EVT_VJOURNAL,
	/** Timezone Component parsed */
	ICAL_EVT_VTIMEZONE,
	/** FreeBusy Component parsed */
	ICAL_EVT_VFREEBUSY,
};

/**
 * @brief iCalendar parser error IDs.
 */
enum ical_parser_error_id {
	/** No error */
	ICAL_ERROR_NONE,
	/** SUMMARY property error */
	ICAL_ERROR_SUMMARY,
	/** LOCATION property error */
	ICAL_ERROR_LOCATION,
	/** DESCRIPTION property error */
	ICAL_ERROR_DESCRIPTION,
	/** DTSTART property error */
	ICAL_ERROR_DTSTART,
	/** DTEND property error */
	ICAL_ERROR_DTEND,
	/** Component not supported error */
	ICAL_ERROR_COM_NOT_SUPPORTED,
};

/**
 * @brief iCalendar component.
 */
struct ical_component {
	/** SUMMARY property */
	char summary[CONFIG_ICAL_PARSER_SUMMARY_SIZE + 1];
	/** LOCATION property buffer. */
	char location[CONFIG_ICAL_PARSER_LOCATION_SIZE + 1];
	/** DESCRIPTION property buffer. */
	char description[CONFIG_ICAL_PARSER_DESCRIPTION_SIZE + 1];
	/** DTSTART property. */
	char dtstart[CONFIG_ICAL_PARSER_DTSTART_SIZE + 1];
	/** DTEND property buffer. */
	char dtend[CONFIG_ICAL_PARSER_DTEND_SIZE + 1];
};

/**
 * @brief iCalendar parser event.
 */
struct ical_parser_evt {
	/** Event ID. */
	enum ical_parser_evt_id id;
	/** Error cause. */
	enum ical_parser_error_id error;
	/** Calendar component data. */
	struct ical_component ical_com;
};

/**
 * @brief iCalendar parser asynchronous event handler.
 *
 * Through this callback, the application receives events, such as
 * parsed component with errors code.
 *
 *
 * @param[in] event  The iCalendar event.
 *
 * @return Zero to continue the parsing, non-zero otherwise.
 */
typedef int (*icalendar_parser_callback_t)(
	const struct ical_parser_evt *event);

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
 * @param[in,out] ical iCalendar parser instance.
 * @param[in] callback Callback for sending calendar parsing event.
 *
 * @return 0 If successful, or an error code on failure.
 */
int ical_parser_init(struct icalendar_parser *ical,
		     icalendar_parser_callback_t callback);

/**
 * @brief Parse the iCalendar data stream. Return the parsed bytes.
 *
 * @param[in,out] ical iCalendar parser instance.
 * @param[in] data Input data to be parsed.
 * @param[in] len  Length of input data stream.
 *
 * @retval size_t  Parsed bytes.
 */
size_t ical_parser_parse(struct icalendar_parser *ical,
			const char *data, size_t len);

/**@} */

#ifdef __cplusplus
}
#endif

#endif /* ICAL_PARSER_H__ */
