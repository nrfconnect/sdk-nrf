/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "azure_iot_hub_topic.h"

#include <logging/log.h>
LOG_MODULE_REGISTER(azure_iot_hub_topic, CONFIG_AZURE_IOT_HUB_LOG_LEVEL);

#define PROP_BAG_STR		"%s=%s"
#define PROP_BAG_STR_EMPTY_VAL	"%s="
#define PROP_BAG_STR_NO_VAL	"%s"

static char *topic_prefixes[] = {
	[TOPIC_TYPE_DEVICEBOUND] = TOPIC_PREFIX_DEVICEBOUND,
	[TOPIC_TYPE_TWIN_UPDATE_DESIRED] = TOPIC_PREFIX_TWIN_DESIRED,
	[TOPIC_TYPE_TWIN_UPDATE_RESULT] = TOPIC_PREFIX_TWIN_RES,
	[TOPIC_TYPE_DPS_REG_RESULT] = TOPIC_PREFIX_DPS_REG_RESULT,
	[TOPIC_TYPE_DIRECT_METHOD] = TOPIC_PREFIX_DIRECT_METHOD,
};

/* If the topic type is TOPIC_TYPE_DEVICEBOUND, the dynamic value in the
 * topic (the device ID), is placed in the middle of the topic, and
 * the following string needs to be skipped before reaching the property
 * bag part of the topic. This can't be achieved by searching for '?' as
 * the separator indicating start of property bag component, as its not
 * consistently enforced on the cloud side, and the topic may or may not
 * contain '?' as separator.
 */
#define TOPIC_DEVICE_BOUND_SUFFIX	"messages/devicebound/"

/* Move a pointer forward by the number of bytes corresponding to the
 * prefix length of the relevant topic.
 */
static inline char *skip_prefix(char *const buf, enum topic_type type)
{
	return buf + strlen(topic_prefixes[type]);
}

/* Get the next occurence of a character within a string, where the length
 * of the string to inspect is set by the max argument.
 */
static inline char *get_next_occurrence(char *str, char c, size_t max)
{
	for (size_t i = 0; i < max; i++) {
		if (str[i] == c) {
			return &str[i];
		}
	}

	return NULL;
}

/* Gets the next property bag, on the format "<key>[=[<value>]]", where '=' and
 * value are optional.
 *
 * @retval the length of parsed property bag on success.
 * @retval 0 if there was no property bag the provided buffer.
 * @retval -ENOMEM if the key or value is too long for the buffer set by
 *	   CONFIG_AZURE_IOT_HUB_TOPIC_ELEMENT_MAX_LEN.
 */
static int get_next_prop_bag(char *buf, size_t buf_len,
			     struct topic_parser_prop_bag *const bag)
{
	char *start_ptr = buf;
	const char *max_ptr = buf + buf_len;
	char *end_ptr;
	size_t len;
	size_t parsed_len;

	if (buf_len == 0) {
		return 0;
	}

	/* Skip forward to next property bag */
	if ((*start_ptr == '?') || (*start_ptr == '&')) {
		start_ptr += 1;
		if (start_ptr >= max_ptr) {
			return 0;
		}
	} else if ((*(start_ptr + 1) == '?') || (*(start_ptr + 1) == '&')) {
		start_ptr += 2;
		if (start_ptr >= max_ptr) {
			return 0;
		}
	}

	parsed_len = buf - start_ptr;

	/* Pointer is at '/', '?' or '&', move to first character of the key */

	/* The key has one of three formats:
	 *	<key>
	 *	<key>=
	 *	<key>=<value>
	 *
	 * The end of the key can be detected in following ways:
	 *	<key><end of buffer>
	 *	<key>&
	 *	<key>=
	 */
	end_ptr = start_ptr;

	while ((*end_ptr != '=') &&
	       (*end_ptr != '&') &&
	       (end_ptr < max_ptr)) {
		end_ptr++;
	}

	len = end_ptr - start_ptr;

	if (len > TOPIC_PROP_BAG_FIELD_MAX_LEN) {
		LOG_ERR("Property bag element is too long for buffer");
		return -ENOMEM;
	}

	memcpy(bag->key, start_ptr, len);
	bag->key[len] = '\0';

	LOG_DBG("Key: %s", log_strdup(bag->key));

	if (end_ptr == max_ptr) {
		return 0;
	} else if (*end_ptr == '&') {
		LOG_DBG("No value in bag");
		bag->value[0] = '\0';

		return len;
	} else if (end_ptr + 1 == max_ptr) {
		LOG_DBG("Value is empty string");

		return len + 1;
	}

	/* Set start_ptr to first character of value */
	start_ptr = end_ptr + 1;
	end_ptr = start_ptr;

	while ((*end_ptr != '&') && (end_ptr < max_ptr)) {
		end_ptr++;
	}

	len = end_ptr - start_ptr;

	if (len > TOPIC_PROP_BAG_FIELD_MAX_LEN) {
		LOG_ERR("Property bag element is too long for buffer");
		return -ENOMEM;
	}

	parsed_len = start_ptr - buf + len;

	memcpy(bag->value, start_ptr, len);
	bag->value[len] = '\0';

	LOG_DBG("Value: %s", log_strdup(bag->value));

	return parsed_len;
}

enum topic_type topic_type_get(const char *buf, const size_t len)
{
	if (buf == NULL || len == 0) {
		return TOPIC_TYPE_EMPTY;
	}

	for (size_t i = 0; i < ARRAY_SIZE(topic_prefixes); i++) {
		if (strncmp(topic_prefixes[i], buf,
			    MIN(strlen(topic_prefixes[i]), len)) == 0) {
			return i;
		}
	}

	return TOPIC_TYPE_UNEXPECTED;
}

int azure_iot_hub_topic_parse(struct topic_parser_data *const data)
{
	char *start_ptr, *stop_ptr;
	const char *max_ptr = data->topic + data->topic_len;
	size_t len;

	if (!data->topic || (data->topic_len == 0)) {
		return -EINVAL;
	}

	if (data->type >= TOPIC_TYPE_UNKNOWN) {
		data->type = topic_type_get(data->topic, data->topic_len);

		if ((data->type == TOPIC_TYPE_EMPTY) ||
		    (data->type == TOPIC_TYPE_UNEXPECTED)) {
			return 0;
		}
	}

	start_ptr = skip_prefix((char *)data->topic, data->type);

	/* This is the common format for topics:
	 *	<prefix>/<dynamic value>/<suffix>/<property bags>
	 *
	 * Where <dynamic value> and <suffix> fields are not present for all.
	 *
	 * Property bags have the following format:
	 *	<key 1>=<value 1>&<key 2>=<value 2>&...
	 *
	 * Where the value field may be left empty. It's also allowed to leave
	 * out the '=' sign.
	 */

	/* Detect if the topic carries more information than just the prefix. */
	if (start_ptr == max_ptr) {
		return 0;
	}

	/* Get the dynamic value for topics that have one */
	if (data->type != TOPIC_TYPE_TWIN_UPDATE_DESIRED) {
		stop_ptr = get_next_occurrence(start_ptr, '/',
					data->topic_len -
					(start_ptr - data->topic));
		len = stop_ptr ? stop_ptr - start_ptr : 0;

		if (len == 0) {
			return -EFAULT;
		} else if (len > TOPIC_DYNAMIC_VALUE_MAX_LEN) {
			return -ENOMEM;
		}

		memcpy(data->name, start_ptr, len);
		data->name[len] = '\0';

		LOG_DBG("Dynamic value: %s", log_strdup(data->name));

		if ((data->type == TOPIC_TYPE_TWIN_UPDATE_RESULT) ||
		    (data->type == TOPIC_TYPE_DPS_REG_RESULT)) {
			char *end_ptr;

			errno = 0;
			data->status = strtoul(data->name, &end_ptr, 10);

			if ((errno == ERANGE) || (*end_ptr != '\0')) {
				LOG_ERR("Failed to parse string as number");
				return -EFAULT;
			}
		} else if (data->type == TOPIC_TYPE_DEVICEBOUND) {
			stop_ptr += sizeof(TOPIC_DEVICE_BOUND_SUFFIX) - 1;
		}

		/* Move the start pointer to the end of the method name plus the
		 * subsequent '/'. start_ptr will now be at the start of the
		 * property bags, if there are any.
		 */
		start_ptr = stop_ptr + 1;
	}

	data->prop_bag_count = 0;

	while ((start_ptr < max_ptr) &&
	       (data->prop_bag_count < TOPIC_PROP_BAG_COUNT)) {
		int ret = get_next_prop_bag(start_ptr, max_ptr - start_ptr,
				&data->prop_bag[data->prop_bag_count]);

		if (ret <= 0) {
			return ret;
		}

		data->prop_bag_count += 1;
		start_ptr  += ret;
	}


	return 0;
}

char *azure_iot_hub_prop_bag_str_get(struct azure_iot_hub_prop_bag *bags,
				     size_t count)
{
	/* Reserve space for null-terminator. */
	size_t total_len = 1;
	size_t len, written = 0;
	char *buf;

	for (size_t i = 0; i < count; i++) {
		if (bags[i].key) {
			/* Allocate space also for '?' or '&'. */
			total_len += strlen(bags[i].key) + 1;
		}

		if (bags[i].value) {
			/* Allocate space also for '='. */
			total_len += strlen(bags[i].value) + 1;
		}
	}

	LOG_DBG("Requested memory block size: %d", total_len);

	buf = k_malloc(total_len);
	if (buf == NULL) {
		LOG_ERR("Memory could not be allocated");
		return NULL;
	}

	for (size_t i = 0; i < count; i++) {
		if (i == 0) {
#if defined(CONFIG_AZURE_IOT_HUB_TOPIC_PROPERTY_BAG_PREFIX)
			buf[0] = '?';
			written++;
#endif
		} else {
			buf[written] = '&';
			written++;
		}

		if (bags[i].value == NULL) {
			len = snprintk(&buf[written], total_len - written,
				       PROP_BAG_STR_NO_VAL, bags[i].key);
		} else if (strlen(bags[i].value) == 0) {
			len = snprintk(&buf[written], total_len - written,
				       PROP_BAG_STR_EMPTY_VAL, bags[i].key);
		} else {
			len = snprintk(&buf[written], total_len - written,
				       PROP_BAG_STR, bags[i].key,
				       bags[i].value);
		}

		if ((len < 0) || (len > total_len)) {
			LOG_ERR("Failed to add property bag");
			goto clean_exit;
		}

		written += len;
	}

	return buf;

clean_exit:
	k_free(buf);
	return NULL;
}
