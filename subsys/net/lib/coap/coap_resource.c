/*
 * Copyright (c) 2014 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
 */

#include <logging/log.h>
#define LOG_LEVEL CONFIG_NRF_COAP_LOG_LEVEL
LOG_MODULE_REGISTER(coap_resource);

#include <string.h>
#include <errno.h>

#include <net/coap_api.h>

#include "coap.h"
#include "coap_resource.h"

#define COAP_RESOURCE_MAX_AGE_INIFINITE  0xFFFFFFFF

static coap_resource_t *root_resource;
static char scratch_buffer[(COAP_RESOURCE_MAX_NAME_LEN + 1) *
			   COAP_RESOURCE_MAX_DEPTH + 6];

u32_t coap_resource_init(void)
{
	root_resource = NULL;
	return 0;
}

u32_t coap_resource_create(coap_resource_t *resource, const char *name)
{
	NULL_PARAM_CHECK(resource);
	NULL_PARAM_CHECK(name);

	if (strlen(name) > COAP_RESOURCE_MAX_NAME_LEN) {
		return EINVAL;
	}

	memcpy(resource->name, name, strlen(name));

	if (root_resource == NULL) {
		root_resource = resource;
	}

	resource->max_age = COAP_RESOURCE_MAX_AGE_INIFINITE;

	return 0;
}

u32_t coap_resource_child_add(coap_resource_t *parent, coap_resource_t *child)
{
	NULL_PARAM_CHECK(parent);
	NULL_PARAM_CHECK(child);

	if (parent->child_count == 0) {
		parent->front = child;
		parent->tail = child;
	} else {
		coap_resource_t *last_sibling = parent->tail;

		last_sibling->sibling = child;
		parent->tail = child;
	}

	parent->child_count++;

	return 0;
}

static u32_t generate_path(u16_t buffer_pos, coap_resource_t *current_resource,
			   char *parent_path, u8_t *string, u16_t *length)
{
	u32_t err_code = 0;

	if (parent_path == NULL) {
		scratch_buffer[buffer_pos++] = '<';

		if (current_resource->front != NULL) {
			coap_resource_t *next_child = current_resource->front;

			do {
				err_code = generate_path(buffer_pos, next_child,
							 scratch_buffer, string,
							 length);
				if (err_code != 0) {
					return err_code;
				}
				next_child = next_child->sibling;
			} while (next_child != NULL);
		}
	} else {
		u16_t size = strlen(current_resource->name);

		scratch_buffer[buffer_pos++] = '/';
		memcpy(&scratch_buffer[buffer_pos], current_resource->name,
		       size);
		buffer_pos += size;

		if (current_resource->front != NULL) {
			coap_resource_t *next_child = current_resource->front;

			do {
				err_code = generate_path(buffer_pos, next_child,
							 scratch_buffer, string,
							 length);
				if (err_code != 0) {
					return err_code;
				}
				next_child = next_child->sibling;
			} while (next_child != NULL);
		}

		scratch_buffer[buffer_pos++] = '>';

		/* If the resource is observable, append 'obs;' token. */
		if ((current_resource->permission & COAP_PERM_OBSERVE) > 0) {
			memcpy(&scratch_buffer[buffer_pos], ";obs", 4);
			buffer_pos += 4;
		}

		scratch_buffer[buffer_pos++] = ',';

		if (buffer_pos <= (*length)) {
			*length -= buffer_pos;
			memcpy(&string[strlen((char *)string)], scratch_buffer,
			       buffer_pos);
		} else {
			return ENOMEM;
		}
	}

	return err_code;
}

u32_t coap_resource_well_known_generate(u8_t *string, u16_t *length)
{
	NULL_PARAM_CHECK(string);
	NULL_PARAM_CHECK(length);

	if (root_resource == NULL) {
		return ENOENT;
	}

	memset(string, 0, *length);

	u32_t err_code = generate_path(0, root_resource, NULL, string, length);

	string[strlen((char *)string) - 1] = '\0'; /* remove the last comma */

	return err_code;
}

static coap_resource_t *coap_resource_child_resolve(coap_resource_t *parent,
						    char *path)
{
	coap_resource_t *result = NULL;

	if (parent->front != NULL) {
		coap_resource_t *sibling_in_question = parent->front;

		do {
			/* Check if the sibling name match. */
			size_t size = strlen(sibling_in_question->name);

			if (strncmp(sibling_in_question->name,
				    path, size) == 0) {
				return sibling_in_question;
			}

			sibling_in_question = sibling_in_question->sibling;
		} while (sibling_in_question != NULL);
	}
	return result;
}

u32_t coap_resource_get(coap_resource_t **resource, u8_t **uri_pointers,
			u8_t num_of_uris)
{
	if (root_resource == NULL) {
		/* Make sure pointer is set to NULL before returning. */
		*resource = NULL;
		return ENOENT;
	}

	coap_resource_t *current_resource = root_resource;

	/* Every node should start at root. */
	for (u8_t i = 0; i < num_of_uris; i++) {
		current_resource = coap_resource_child_resolve(
				current_resource, (char *)uri_pointers[i]);

		if (current_resource == NULL) {
			/* Stop looping as this direction will not give anything
			 * more.
			 */
			break;
		}
	}

	if (current_resource != NULL) {
		*resource = current_resource;
		return 0;
	}

	/* If nothing has been found. */
	*resource = NULL;
	return ENOENT;
}

u32_t coap_resource_root_get(coap_resource_t **resource)
{
	NULL_PARAM_CHECK(resource);

	if (root_resource == NULL) {
		return ENOENT;
	}

	*resource = root_resource;

	return 0;
}
