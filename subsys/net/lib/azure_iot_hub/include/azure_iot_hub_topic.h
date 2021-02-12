/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef AZURE_IOT_HUB_TOPIC_H__
#define AZURE_IOT_HUB_TOPIC_H__

#include <stdio.h>
#include <net/azure_iot_hub.h>

#ifdef __cplusplus
extern "C" {
#endif

#define TOPIC_DYNAMIC_VALUE_MAX_LEN  CONFIG_AZURE_IOT_HUB_TOPIC_ELEMENT_MAX_LEN
#define TOPIC_PROP_BAG_FIELD_MAX_LEN CONFIG_AZURE_IOT_HUB_TOPIC_ELEMENT_MAX_LEN
#define TOPIC_PROP_BAG_COUNT	     CONFIG_AZURE_IOT_HUB_PROPERTY_BAG_MAX_COUNT

#define TOPIC_PREFIX_DEVICEBOUND	"devices/"
#define TOPIC_PREFIX_TWIN_DESIRED	"$iothub/twin/PATCH/properties/desired/"
#define TOPIC_PREFIX_TWIN_RES		"$iothub/twin/res/"
#define TOPIC_PREFIX_DPS_REG_RESULT	"$dps/registrations/res/"
#define TOPIC_PREFIX_DIRECT_METHOD	"$iothub/methods/POST/"

enum topic_type {
	TOPIC_TYPE_DEVICEBOUND,
	TOPIC_TYPE_TWIN_UPDATE_DESIRED,
	TOPIC_TYPE_TWIN_UPDATE_RESULT,
	TOPIC_TYPE_DPS_REG_RESULT,
	TOPIC_TYPE_DIRECT_METHOD,

	TOPIC_TYPE_UNKNOWN,
	TOPIC_TYPE_UNEXPECTED,
	TOPIC_TYPE_EMPTY,
};

struct topic_parser_prop_bag {
	/* Null-terminated key string. */
	char key[TOPIC_PROP_BAG_FIELD_MAX_LEN + 1];
	/* Null-terminated value string. */
	char value[TOPIC_PROP_BAG_FIELD_MAX_LEN + 1];
};

/* Topic parser data. Used for both input and output. It's the caller's
 * responsibility to allocate memory to the various array members before
 * calling any function that may operate on the structure members.
 */
struct topic_parser_data {
	/* Topic type. */
	enum topic_type type;
	/* Topic buffer. */
	const char *topic;
	/* Length of topic buffer. */
	size_t topic_len;
	/* Each topic may contain one dynamic topic level that carries
	 * information.
	 */
	union {
		/* Direct method name for TOPIC_TYPE_DIRECT_METHOD. */
		char name[TOPIC_DYNAMIC_VALUE_MAX_LEN + 1];
		/* Status code for TOPIC_TYPE_TWIN_UPDATE_DESIRED and
		 * TOPIC_TYPE_DPS_REG_RESULT.
		 */
		uint32_t status;
	};
	/* Array of property bag elements. */
	struct topic_parser_prop_bag prop_bag[TOPIC_PROP_BAG_COUNT];
	/* Number of property bag elements in the above array. */
	size_t prop_bag_count;
};

/* @brief Free an allocated property bag buffer.
 *
 * @param buf Pointer to property bag.
 */
static inline void azure_iot_hub_prop_bag_free(char *buf)
{
	k_free(buf);
}

/* @brief Get topic type.
 *
 * @param buf Topic buffer.
 * @param len Length of topic buffer.
 *
 * @return Topic type @ref topic_type.
 */
enum topic_type topic_type_get(const char *buf, const size_t len);

/* @brief Parse topic.
 *
 * @param data Pointer to topic data structure. The structure must
 *	       be initialized with a topic buffer pointer and topic
 *	       length. The struct member type must be set to
 *	       TOPIC_TYPE_UNKNOWN if the topic type has not already
 *	       been correctly determined.
 *
 * @return 0 on success, otherwise negative error code.
 */
int azure_iot_hub_topic_parse(struct topic_parser_data *const data);

/* @brief Create a string with all property bags as a querystring.
 *	  The caller is responsible for calling k_free() on the returned
 *	  (non-NULL) pointer after use.
 *
 * @param bags Array of property bags.
 * @param count Number of property bag elements n the bags array.
 *
 * @return Pointer to the created querystring on success, NULL on failure.
 */
char *azure_iot_hub_prop_bag_str_get(struct azure_iot_hub_prop_bag *bags,
				     size_t count);

#ifdef __cplusplus
}
#endif

#endif /* AZURE_IOT_HUB_TOPIC_H__ */
