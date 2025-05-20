/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_platform_internal.h>
#include <stdbool.h>
#include <suit_platform.h>
#include <suit_plat_decode_util.h>
#include <suit_cpu_run.h>
#ifdef CONFIG_SUIT_EVENTS
#include <suit_events.h>
#endif /* CONFIG_SUIT_EVENTS */

LOG_MODULE_REGISTER(suit_plat_invoke, CONFIG_SUIT_LOG_LEVEL);

int suit_plat_check_invoke(suit_component_t image_handle, struct zcbor_string *invoke_args)
{
	struct zcbor_string *component_id;
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	intptr_t run_address;
	uint8_t cpu_id;
	size_t size;

	if (suit_plat_component_id_get(image_handle, &component_id) != SUIT_SUCCESS) {
		LOG_ERR("suit_plat_component_id_get failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_component_id(component_id, &cpu_id, &run_address, &size) !=
	    SUIT_PLAT_SUCCESS) {
		LOG_ERR("suit_plat_decode_component_id failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("suit_plat_decode_component_type failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_invoke_args(invoke_args, NULL, NULL)) {
		LOG_ERR("suit_plat_decode_invoke_args failed");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	/* Check if component type supports invocation */
	switch (component_type) {
	case SUIT_COMPONENT_TYPE_MEM:
		/* memory-mapped */
		return SUIT_SUCCESS;
	default:
		LOG_ERR("Unsupported component type");
		break;
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}

int suit_plat_invoke(suit_component_t image_handle, struct zcbor_string *invoke_args)
{
	struct zcbor_string *component_id;
	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;
	uint32_t timeout_ms = 0;
	bool synchronous = false;
	int ret = 0;
	intptr_t run_address;
	uint8_t cpu_id;
	size_t size;

	if (suit_plat_component_id_get(image_handle, &component_id) != SUIT_SUCCESS) {
		LOG_ERR("suit_plat_component_id_get failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_component_id(component_id, &cpu_id, &run_address, &size) !=
	    SUIT_PLAT_SUCCESS) {
		LOG_ERR("suit_plat_decode_component_id failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("suit_plat_decode_component_type failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_invoke_args(invoke_args, &synchronous, &timeout_ms)) {
		LOG_ERR("suit_plat_decode_invoke_args failed");
		return SUIT_ERR_UNSUPPORTED_PARAMETER;
	}

	if (component_type != SUIT_COMPONENT_TYPE_MEM) {
		LOG_ERR("Unsupported component type");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	ret = suit_plat_cpu_halt(cpu_id);
	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to halt CPU %d", cpu_id);
		return SUIT_ERR_CRASH;
	}
#ifdef CONFIG_SUIT_EVENTS
	/* Clear all pending invoke events */
	(void)suit_event_clear(cpu_id, SUIT_EVENT_INVOKE_MASK);
#endif /* CONFIG_SUIT_EVENTS */

	ret = suit_plat_cpu_run(cpu_id, run_address);

	if ((ret == SUIT_SUCCESS) && synchronous) {
#ifdef CONFIG_SUIT_EVENTS
		/* Wait for one of the invoke events */
		uint32_t invoke_events =
			suit_event_wait(cpu_id, SUIT_EVENT_INVOKE_MASK, false, timeout_ms);

		if (invoke_events & SUIT_EVENT_INVOKE_SUCCESS) {
			return SUIT_SUCCESS;
		}

		/* Event timeout or invoke failed */
		/* Allow to handle invoke failure inside the manifest. */
		return SUIT_FAIL_CONDITION;
#else
		/* Synchronous invoke not supported */
		return SUIT_ERR_CRASH;
#endif /* CONFIG_SUIT_EVENTS */
	}

	return ret;
}
