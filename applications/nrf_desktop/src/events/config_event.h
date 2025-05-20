/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef _CONFIG_EVENT_H_
#define _CONFIG_EVENT_H_

/**
 * @brief Config Event
 * @defgroup config_event Config Event
 * @{
 */

#include <app_event_manager.h>
#include <app_event_manager_profiler_tracer.h>
#include "hid_report_desc.h"

#ifdef __cplusplus
extern "C" {
#endif


/** @brief Config channel status list. */
#define CONFIG_STATUS_LIST		\
	X(PENDING)			\
	X(GET_MAX_MOD_ID)		\
	X(GET_HWID)			\
	X(GET_BOARD_NAME)		\
	X(INDEX_PEERS)			\
	X(GET_PEER)			\
	X(SET)				\
	X(FETCH)			\
	X(SUCCESS)			\
	X(TIMEOUT)			\
	X(REJECT)			\
	X(WRITE_FAIL)			\
	X(DISCONNECTED)			\
	X(GET_PEERS_CACHE)

enum config_status {
#define X(name) _CONCAT(CONFIG_STATUS_, name),
	CONFIG_STATUS_LIST
#undef X

	CONFIG_STATUS_COUNT
};

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
#define MODULE_OPT_MODULE_DESCR		0x0

/* Character used to inform about end of module description. */
#define MODULE_DESCR_END_CHAR '\n'

/* Description of the option representing module variant. */
#define OPT_DESCR_MODULE_VARIANT "module_variant"

/* Configuration channel local recipient. */
#define CFG_CHAN_RECIPIENT_LOCAL 0x00

/** @brief Configuration channel event.
 * Used to forward configuration channel request/response.
 */
struct config_event {
	struct app_event_header header;

	uint16_t transport_id;
	bool is_request;

	/* Data exchanged with host. */
	uint8_t event_id;
	uint8_t recipient;
	uint8_t status;
	struct event_dyndata dyndata;
};

APP_EVENT_TYPE_DYNDATA_DECLARE(config_event);

extern const uint8_t __start_config_channel_modules[];

#define GEN_CONFIG_EVENT_HANDLERS(mod_name, opt_descr, config_set_fn, config_fetch_fn)		\
	BUILD_ASSERT(ARRAY_SIZE(opt_descr) > 0);						\
	BUILD_ASSERT(ARRAY_SIZE(opt_descr) <= OPT_FIELD_MASK);					\
	if (IS_ENABLED(CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE) && is_config_event(aeh)) {		\
		static const uint8_t module_id_in_section					\
			__attribute__((__section__("config_channel_modules"))) = 0;		\
		uint8_t config_module_id =							\
			&module_id_in_section - (uint8_t *)__start_config_channel_modules;	\
		static uint8_t cur_opt_descr;							\
												\
		struct config_event *event = cast_config_event(aeh);				\
												\
		uint8_t rsp_data_buf[CONFIG_CHANNEL_FETCHED_DATA_MAX_SIZE];			\
		size_t rsp_data_size = 0;							\
		bool consume = false;								\
												\
		/* Not for us. */								\
		if (event->recipient != CFG_CHAN_RECIPIENT_LOCAL) {				\
			return false;								\
		}										\
												\
		if (event->status == CONFIG_STATUS_SET) {					\
			if (MOD_FIELD_GET(event->event_id) == config_module_id) {		\
				BUILD_ASSERT(config_set_fn != NULL); 				\
				(*config_set_fn)(OPT_ID_GET(OPT_FIELD_GET(event->event_id)),	\
						 event->dyndata.data,				\
						 event->dyndata.size);				\
				consume = true;							\
			}									\
		} else if (event->status == CONFIG_STATUS_FETCH) {				\
			if (MOD_FIELD_GET(event->event_id) == config_module_id) {		\
				if (OPT_FIELD_GET(event->event_id) == MODULE_OPT_MODULE_DESCR) {\
					if (cur_opt_descr < ARRAY_SIZE(opt_descr) + 1) {	\
						const char *data_ptr;				\
												\
						if (cur_opt_descr == 0) {			\
							data_ptr = mod_name;			\
						} else {					\
							data_ptr = opt_descr[cur_opt_descr - 1];\
						}						\
						rsp_data_size = strlen(data_ptr);		\
						__ASSERT_NO_MSG(rsp_data_size <=		\
								sizeof(rsp_data_buf));		\
						strncpy((char *) rsp_data_buf, data_ptr,	\
								rsp_data_size);			\
						cur_opt_descr++;				\
					} else {						\
						rsp_data_size = sizeof(uint8_t);		\
						rsp_data_buf[0] = MODULE_DESCR_END_CHAR;	\
						cur_opt_descr = 0;				\
					}							\
				} else {							\
					BUILD_ASSERT(config_fetch_fn != NULL);			\
					(*config_fetch_fn)(OPT_ID_GET(				\
							      OPT_FIELD_GET(event->event_id)),	\
							   rsp_data_buf,			\
							   &rsp_data_size);			\
				}								\
				consume = true;							\
			}									\
		}										\
												\
		if (consume) {									\
			struct config_event *rsp = new_config_event(rsp_data_size);		\
												\
			rsp->transport_id = event->transport_id;				\
			rsp->recipient = event->recipient;					\
			rsp->event_id = event->event_id;					\
			rsp->status = CONFIG_STATUS_SUCCESS;					\
			rsp->is_request = false;						\
												\
			if (rsp_data_size > 0) {						\
				memcpy(rsp->dyndata.data, rsp_data_buf, rsp_data_size);		\
			}									\
												\
			APP_EVENT_SUBMIT(rsp);						\
		}										\
												\
		return consume;									\
	}

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* _CONFIG_EVENT_H_ */
