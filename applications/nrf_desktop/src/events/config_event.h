/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#ifndef _CONFIG_EVENT_H_
#define _CONFIG_EVENT_H_

/**
 * @brief Config Event
 * @defgroup config_event Config Event
 * @{
 */

#include "event_manager.h"
#include "hid_report_desc.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Maximum length of fetched data. */
#define CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE 16

/* Config event, setup group macros */

#define MOD_FIELD_POS		4
#define MOD_FIELD_SIZE		4
#define MOD_FIELD_MASK		BIT_MASK(MOD_FIELD_SIZE)
#define MOD_FIELD_SET(module)	((module & MOD_FIELD_MASK) << MOD_FIELD_POS)
#define MOD_FIELD_GET(event_id) ((event_id >> MOD_FIELD_POS) & MOD_FIELD_MASK)

#define OPT_FIELD_POS		0
#define OPT_FIELD_SIZE		4
#define OPT_FIELD_MASK		BIT_MASK(OPT_FIELD_SIZE)
#define OPT_FIELD_SET(option)	((option & OPT_FIELD_MASK) << OPT_FIELD_POS)
#define OPT_FIELD_GET(event_id) ((event_id >> OPT_FIELD_POS) & OPT_FIELD_MASK)

#define OPT_ID_GET(opt_field)	(opt_field - 1)

/* Common module option macros */
#define MODULE_BROADCAST		BIT_MASK(MOD_FIELD_SIZE)
#define BROADCAST_OPT_MAX_MOD_ID	0x0
#define MODULE_OPT_MODULE_DESCR		0x0

/* Character used to inform about end of module description. */
#define MODULE_DESCR_END_CHAR '\n'

/* Description of the option representing module type. */
#define OPT_DESCR_MODULE_TYPE "module_type"

/** @brief Configuration channel event.
 * Used to change firmware parameters at runtime.
 */
struct config_event {
	struct event_header header;

	u8_t id;
	struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(config_event);

/** @brief Configuration channel fetch event.
 * Used to fetch firmware parameters to host.
 */
struct config_fetch_event {
	struct event_header header;

	u16_t recipient;
	u8_t id;
	void *channel_id;
	struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(config_fetch_event);

/** @brief Configuration channel fetch request event.
 * Used to request fetching firmware parameters to host.
 */
struct config_fetch_request_event {
	struct event_header header;

	u16_t recipient;
	u8_t id;
	void *channel_id;
};

EVENT_TYPE_DECLARE(config_fetch_request_event);

enum config_status {
	CONFIG_STATUS_SUCCESS,
	CONFIG_STATUS_PENDING,
	CONFIG_STATUS_FETCH,
	CONFIG_STATUS_TIMEOUT,
	CONFIG_STATUS_REJECT,
	CONFIG_STATUS_WRITE_ERROR,
	CONFIG_STATUS_DISCONNECTED_ERROR,
};

/** @brief Configuration channel forward event.
 * Used to pass configuration from dongle to connected devices.
 */
struct config_forward_event {
	struct event_header header;

	u16_t recipient;
	u8_t id;
	enum config_status status;

	struct event_dyndata dyndata;
};

EVENT_TYPE_DYNDATA_DECLARE(config_forward_event);

/** @brief Configuration channel forward get event.
 * Used to forward configuration channel get request to connected devices.
 */
struct config_forward_get_event {
	struct event_header header;

	u16_t recipient;
	u8_t id;
	void *channel_id;
	enum config_status status;
};

EVENT_TYPE_DECLARE(config_forward_get_event);

/** @brief Configuration channel forwarded event.
 * Used to confirm that event has been successfully forwarded.
 */
struct config_forwarded_event {
	struct event_header header;

	enum config_status status;
};

EVENT_TYPE_DECLARE(config_forwarded_event);

#define GEN_CONFIG_EVENT_HANDLERS(mod_name, opt_descr, config_set_fn, config_fetch_fn, last)	\
	BUILD_ASSERT(ARRAY_SIZE(opt_descr) > 0);						\
	BUILD_ASSERT(ARRAY_SIZE(opt_descr) <= OPT_FIELD_MASK);					\
	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE)) {					\
		static u8_t config_module_id = MODULE_BROADCAST;				\
		static u8_t cur_opt_descr;							\
												\
		if (is_config_event(eh)) {							\
			struct config_event *event = cast_config_event(eh);			\
												\
			if ((MOD_FIELD_GET(event->id) == MODULE_BROADCAST) &&			\
			    (OPT_FIELD_GET(event->id) == BROADCAST_OPT_MAX_MOD_ID)) {		\
				config_module_id = event->dyndata.data[0];			\
				__ASSERT(config_module_id < MODULE_BROADCAST,			\
					 "You can use up to 15 configuration modules");		\
				event->dyndata.data[0]++;					\
			} else if (MOD_FIELD_GET(event->id) == config_module_id) {		\
				__ASSERT_NO_MSG(config_set_fn != NULL); 			\
				(*config_set_fn)(OPT_ID_GET(OPT_FIELD_GET(event->id)),		\
						 event->dyndata.data,				\
						 event->dyndata.size);				\
			}									\
												\
			return false;								\
		}										\
												\
		if (is_config_fetch_request_event(eh)) {					\
			const struct config_fetch_request_event *event =			\
				cast_config_fetch_request_event(eh);				\
			u8_t data_buf[CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE];			\
			size_t data_size = 0;							\
												\
			memset(data_buf, 0, sizeof(data_buf));					\
												\
			if ((MOD_FIELD_GET(event->id) == MODULE_BROADCAST) &&			\
			    (OPT_FIELD_GET(event->id) == BROADCAST_OPT_MAX_MOD_ID)) {		\
				cur_opt_descr = 0;						\
				if (last) {							\
					data_buf[0] = config_module_id;				\
					data_size = (sizeof(config_module_id));			\
				}								\
			} else if (MOD_FIELD_GET(event->id) == config_module_id) {		\
				if (OPT_FIELD_GET(event->id) == MODULE_OPT_MODULE_DESCR) {	\
					if (cur_opt_descr < ARRAY_SIZE(opt_descr) + 1) {	\
						const char *data_ptr;				\
												\
						if (cur_opt_descr == 0) {			\
							data_ptr = mod_name;			\
						} else {					\
							data_ptr = opt_descr[cur_opt_descr - 1];\
						}						\
						data_size = strlen(data_ptr);			\
						__ASSERT_NO_MSG(data_size <			\
							CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE);	\
						strcpy(data_buf, data_ptr);			\
						cur_opt_descr++;				\
					} else {						\
						data_size = sizeof(u8_t);			\
						data_buf[0] = MODULE_DESCR_END_CHAR;		\
						cur_opt_descr = 0;				\
					}							\
				} else {							\
					__ASSERT_NO_MSG(config_fetch_fn != NULL);		\
					(*config_fetch_fn)(OPT_ID_GET(OPT_FIELD_GET(event->id)),\
							   data_buf,				\
							   &data_size);				\
				}								\
			}									\
												\
			if (!data_size) {							\
				return false;							\
			}									\
												\
			struct config_fetch_event *fetch_event =				\
			  new_config_fetch_event(data_size);					\
												\
			memcpy(fetch_event->dyndata.data, data_buf, data_size);			\
			fetch_event->id = event->id;						\
			fetch_event->recipient = event->recipient;				\
			fetch_event->channel_id = event->channel_id;				\
												\
			EVENT_SUBMIT(fetch_event);						\
												\
			return false;								\
		}										\
	}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CONFIG_EVENT_H_ */
