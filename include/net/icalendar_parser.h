/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef ICALENDAR_PARSER_H__
#define ICALENDAR_PARSER_H__

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
	/** Start to parse a iCalendar. */
	ICAL_PARSER_EVT_CALENDAR,
	/** Calendar component parsed. */
	ICAL_PARSER_EVT_COMPONENT,
	/** Calendar downloaded. */
	ICAL_PARSER_EVT_COMPLETE,
	/** An error has occurred during download. */
	ICAL_PARSER_EVT_ERROR,
};

/**
 * @brief iCalendar parser state.
 */
enum ical_parser_state {
	/** Stopped state */
	ICAL_PARSER_STATE_STOPPED,
	/** Downloading state. */
	ICAL_PARSER_STATE_DOWNLOADING,
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
struct icalendar_parser_evt {
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
	char summary[CONFIG_MAX_SUMMARY_PROPERTY_SIZE];
	/** LOCATION property buffer. */
	char location[CONFIG_MAX_LOCATION_PROPERTY_SIZE];
	/** DESCRIPTION property buffer. */
	char description[CONFIG_MAX_DESCRIPTION_PROPERTY_SIZE];
	/** DTSTART property. */
	char dtstart[CONFIG_MAX_DTSTART_PROPERTY_SIZE];
	/** DTEND property buffer. */
	char dtend[CONFIG_MAX_DTEND_PROPERTY_SIZE];
};

/**
 * @brief iCalendar parser asynchronous event handler.
 *
 * Through this callback, the application receives events, such as
 * parsed component, download completion, or errors.
 *
 * If the callback returns a non-zero value, the download stops.
 * To resume the download, use @ref icalendar_parser_start().
 *
 * @param[out]  event	The event.
 * @param[out] parsed iCalendar component.
 *
 * @return Zero to continue the download, non-zero otherwise.
 */
typedef int (*icalendar_parser_callback_t)(
	const struct icalendar_parser_evt *event,
	struct ical_component *p_ical_component);

/**
 * @brief iCalendar parser instance.
 */
struct icalendar_parser {
	/** Calendar state */
	enum ical_parser_state state;
	/** HTTP socket. */
	int fd;
	/** Buffer for HTTP get request */
	char http_tx_buf[CONFIG_ICALENDAR_PARSER_MAX_TX_SIZE];
	/** Buffer for HTTP response */
	char http_rx_buf[CONFIG_ICALENDAR_PARSER_MAX_RX_SIZE];
	/** Offset of HTTP TX/RX buffer. */
	size_t offset;
	/** Download progress, number of bytes downloaded. */
	size_t progress;

	/** Whether the HTTP header for
	 * the current fragment has been processed.
	 */
	bool has_header;
	/** The server has closed the connection. */
	bool connection_close;

	/** Server hosting the file, null-terminated. */
	const char *host;
	/** File name, null-terminated. */
	const char *file;
	/** Credential to be referred. */
	int sec_tag;
	/** begin of iCalendar object delimiter pair */
	bool icalobject_begin;
	/** end of iCalendar object delimiter pair */
	bool icalobject_end;
	/** Time zone */
	int timezone;
	/** Current date */
	unsigned long current_date;
	/** Current time */
	unsigned long current_time;

	/** Internal thread ID. */
	k_tid_t tid;
	/** Internal download thread. */
	struct k_thread thread;
	/** Internal thread stack. */
	K_THREAD_STACK_MEMBER(thread_stack,
			      CONFIG_ICALENDAR_PARSER_STACK_SIZE);

	/** Event handler. */
	icalendar_parser_callback_t callback;
};

/**
 * @brief Initialize the iCalendar parser.
 *
 * @param[in] ical	iCalendar instance.
 * @param[in] callback	Callback function.
 *
 * @retval int Zero on success, otherwise a negative error code.
 */
int ical_parser_init(struct icalendar_parser *ical,
		     icalendar_parser_callback_t callback);

/**
 * @brief Establish a connection to the calendar server.
 *
 * @param[in] ical		Calendar instance.
 * @param[in] host		Calendar server to connect to, null-terminated.
 * @param[in] sec_tag	Credential to be referred.
 *
 * @retval int Zero on success, a negative error code otherwise.
 */
int ical_parser_connect(struct icalendar_parser *ical, const char *host,
			int sec_tag);

/**
 * @brief Start to get calendar fragment and parse.
 *
 * @param[in] ical		Calendar instance.
 * @param[in] file		File to download, null-terminated.
 *
 * @retval int Zero on success, a negative error code otherwise.
 */
int ical_parser_start(struct icalendar_parser *ical, const char *file);

/**
 * @brief Disconnect from the server.
 *
 * @param[in] client	Calendar instance.
 *
 * @return Zero on success, a negative error code otherwise.
 */
int ical_parser_disconnect(struct icalendar_parser *client);

#ifdef __cplusplus
}
#endif

#endif /* ICALENDAR_PARSER_H__ */

/**@} */
