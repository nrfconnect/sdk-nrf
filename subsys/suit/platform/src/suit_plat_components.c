/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_error_convert.h>
#include <suit_memptr_storage.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(plat_components, CONFIG_SUIT_LOG_LEVEL);

struct suit_plat_component {
	/** Slot to store the implementation-specific private data. */
	void *impl_data;
	/** Slot to store the component ID. The implementation
	 * may use it instead of building full internal context.
	 */
	struct zcbor_string component_id;
	bool in_use;
};

/** Platform component list, populated as a result of successful SUIT manifest validation. */
static struct suit_plat_component components[SUIT_MAX_NUM_COMPONENT_PARAMS];

static inline bool is_mem_mapped(suit_component_type_t component_type)
{
	return (SUIT_COMPONENT_TYPE_CAND_IMG == component_type) ||
	       (SUIT_COMPONENT_TYPE_CAND_MFST == component_type) ||
	       (SUIT_COMPONENT_TYPE_MEM == component_type);
}

/** Resolve pointer to the component structure into component handle.
 */
static inline suit_component_t handle_from_component(const struct suit_plat_component *component)
{
	return (suit_component_t)component;
}

/** Resolve component handle into component structure.
 *
 * @details The component handle is considered as valid if its value
 *	    points to the component array element and the element has
 *	    an implementation assigned.
 */
static struct suit_plat_component *component_from_handle(suit_component_t handle)
{
	const suit_component_t first_component = (suit_component_t)(&components[0]);
	struct suit_plat_component *component = (struct suit_plat_component *)handle;

	if ((handle < (intptr_t)&components[0]) ||
	    (handle > (intptr_t)&components[SUIT_MAX_NUM_COMPONENT_PARAMS - 1]) ||
	    ((handle - first_component) % (sizeof(struct suit_plat_component)) != 0) ||
	    (!component->in_use)) {
		return NULL;
	}

	return component;
}

int suit_plat_release_component_handle(suit_component_t handle)
{
	struct suit_plat_component *component = component_from_handle(handle);

	if (component == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	if (suit_plat_decode_component_type(&component->component_id, &component_type) !=
	    SUIT_PLAT_SUCCESS) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (is_mem_mapped(component_type)) {
		suit_memptr_storage_err_t ret = suit_memptr_storage_release(component->impl_data);

		if (ret != SUIT_PLAT_SUCCESS) {
			return SUIT_ERR_CRASH;
		}

		component->impl_data = NULL;
	}

	component->in_use = false;

	return SUIT_SUCCESS;
}

int suit_plat_create_component_handle(struct zcbor_string *component_id,
				      suit_component_t *component_handle)
{
	suit_memptr_storage_err_t err;
	struct suit_plat_component *component = NULL;

	if (component_id == NULL || component_handle == NULL) {
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	for (size_t i = 0; i < SUIT_MAX_NUM_COMPONENT_PARAMS; i++) {
		if (components[i].in_use == false) {
			component = &components[i];
			break;
		}
	}

	if (component == NULL) {
		LOG_ERR("component is NULL");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	*component_handle = (intptr_t)component;
	memset(component, 0, sizeof(struct suit_plat_component));

	/* Store component ID and keys, so the selected implementation can use it in the future. */
	component->component_id.value = component_id->value;
	component->component_id.len = component_id->len;

	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Error decoding component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if ((component_type == SUIT_COMPONENT_TYPE_CAND_IMG) ||
	    (component_type == SUIT_COMPONENT_TYPE_CAND_MFST)) {
		err = suit_memptr_storage_get(&component->impl_data);
		if (err != SUIT_PLAT_SUCCESS) {
			return suit_plat_err_to_processor_err_convert(err);
		}
	}

	if (component_type == SUIT_COMPONENT_TYPE_MEM) {
		/* Get address and size of the component from its id */
		intptr_t address = (intptr_t)NULL;
		size_t size = 0;

		if (suit_plat_decode_address_size(&component->component_id, &address, &size) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to decode address and size");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		/* Associate memptr_storage with component's implementation data */
		err = suit_memptr_storage_get(&component->impl_data);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to get memptr_storage: %d", err);

			return suit_plat_err_to_processor_err_convert(err);
		}

		/* Set the address as in component id but set size with 0 */
		size = 0;
		suit_memptr_storage_err_t err = suit_memptr_storage_ptr_store(
			component->impl_data, (uint8_t *)address, size);
		if (err != SUIT_PLAT_SUCCESS) {
			suit_memptr_storage_release(component->impl_data);
			component->impl_data = NULL;
			LOG_ERR("Failed to store memptr ptr: %d", err);
			return SUIT_ERR_CRASH;
		}
	}

	component->in_use = true;
	return SUIT_SUCCESS;
}

int suit_plat_component_impl_data_set(suit_component_t handle, void *impl_data)
{
	struct suit_plat_component *component = component_from_handle(handle);

	if (component == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	component->impl_data = impl_data;
	return SUIT_SUCCESS;
}

int suit_plat_component_impl_data_get(suit_component_t handle, void **impl_data)
{
	const struct suit_plat_component *component = component_from_handle(handle);

	if (component == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	*impl_data = component->impl_data;
	return SUIT_SUCCESS;
}

int suit_plat_component_id_get(suit_component_t handle, struct zcbor_string **component_id)
{
	struct suit_plat_component *component = component_from_handle(handle);

	if (component == NULL) {
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	*component_id = &(component->component_id);
	return SUIT_SUCCESS;
}

int suit_plat_override_image_size(suit_component_t handle, size_t size)
{
	struct zcbor_string *component_id = NULL;

	int err = suit_plat_component_id_get(handle, &component_id);

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to get component id: %i", err);
		return err;
	}

	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to decode component type: %i", err);
		return SUIT_ERR_DECODING;
	}

	/* Size override is done only for MEM type component */
	if (component_type == SUIT_COMPONENT_TYPE_MEM) {
		intptr_t run_address;
		size_t component_size;

		if (suit_plat_decode_address_size(component_id, &run_address, &component_size) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("suit_plat_decode_address_size failed");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		LOG_DBG("%s, component size: %u, input size %u", __func__, component_size, size);

		if (size > component_size) {
			LOG_ERR("Input size exceeds component size");
			return SUIT_ERR_UNSUPPORTED_PARAMETER;
		}

		void *impl_data = NULL;

		err = suit_plat_component_impl_data_get(handle, &impl_data);
		if (err != SUIT_SUCCESS) {
			LOG_ERR("Failed to get implementation data: %i", err);
			return err;
		}

		const uint8_t *payload_ptr = NULL;
		size_t payload_size = 0;

		err = suit_memptr_storage_ptr_get(impl_data, &payload_ptr, &payload_size);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to get memptr: %i", err);
			return SUIT_ERR_CRASH;
		}

		payload_size = size;
		err = suit_memptr_storage_ptr_store(impl_data, payload_ptr, payload_size);
		if (err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Failed to update memptr: %i", err);
			return SUIT_ERR_CRASH;
		}

		return SUIT_SUCCESS;
	}

	LOG_ERR("%s does not support component type %d", __func__, component_type);
	return SUIT_ERR_UNSUPPORTED_COMMAND;
}

int suit_plat_component_type_get(suit_component_t handle, suit_component_type_t *component_type)
{
	struct zcbor_string *component_id;

	int ret = suit_plat_component_id_get(handle, &component_id);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("suit_plat_component_id_get failed: %i", ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_component_type(component_id, component_type) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("suit_plat_decode_component_type failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	return SUIT_SUCCESS;
}
