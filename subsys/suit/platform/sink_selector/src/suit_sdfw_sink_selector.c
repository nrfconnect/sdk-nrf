/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <suit_sink_selector.h>
#include <suit_platform_internal.h>
#include <suit_plat_decode_util.h>
#include <suit_plat_err.h>
#include <suit_plat_error_convert.h>

#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
#include <suit_memptr_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */
#ifdef CONFIG_SUIT_STREAM_SINK_FLASH
#include <suit_flash_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_FLASH */
#ifdef CONFIG_SUIT_STREAM_SINK_RAM
#include <suit_ram_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_RAM */
#ifdef CONFIG_SUIT_STREAM_SINK_SDFW
#include <suit_sdfw_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_SDFW */
#ifdef CONFIG_SUIT_STREAM_SINK_SDFW_RECOVERY
#include <suit_sdfw_recovery_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_SDFW_RECOVERY */
#ifdef CONFIG_SUIT_STREAM_SINK_EXTMEM
#include <suit_extmem_sink.h>
#endif /* CONFIG_SUIT_STREAM_SINK_EXTMEM */

LOG_MODULE_REGISTER(suit_plat_sink_selector, CONFIG_SUIT_LOG_LEVEL);

int suit_sink_select(suit_component_t dst_handle, struct stream_sink *sink)
{
	struct zcbor_string *component_id;
	int ret = SUIT_SUCCESS;
#if defined(CONFIG_SUIT_STREAM_SINK_MEMPTR) || defined(CONFIG_SUIT_STREAM_SINK_FLASH) ||           \
	defined(CONFIG_SUIT_STREAM_SINK_SDFW)
	suit_plat_err_t sink_get_err = SUIT_PLAT_SUCCESS;
#endif

	suit_component_type_t component_type = SUIT_COMPONENT_TYPE_UNSUPPORTED;

	ret = suit_plat_component_id_get(dst_handle, &component_id);

	if (ret != SUIT_SUCCESS) {
		LOG_ERR("suit_plat_component_id_get failed: %i", ret);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	if (suit_plat_decode_component_type(component_id, &component_type) != SUIT_PLAT_SUCCESS) {
		LOG_ERR("suit_plat_decode_component_type failed");
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}

	/* Select sink based on component type */
	switch (component_type) {
#ifdef CONFIG_SUIT_STREAM_SINK_MEMPTR
	case SUIT_COMPONENT_TYPE_CAND_IMG:
	case SUIT_COMPONENT_TYPE_CAND_MFST: { /* memptr_sink */
		uint32_t number;
		memptr_storage_handle_t handle;

		if (suit_plat_decode_component_number(component_id, &number) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Missing component id number in candidate image component");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		ret = suit_plat_component_impl_data_get(dst_handle, &handle);
		if (ret != SUIT_SUCCESS) {
			LOG_ERR("Unable to get component data for candidate image (err: %d)", ret);
			return ret;
		}

		sink_get_err = suit_memptr_sink_get(sink, handle);
		return suit_plat_err_to_processor_err_convert(sink_get_err);
	} break;
#endif /* CONFIG_SUIT_STREAM_SINK_MEMPTR */

#ifdef CONFIG_SUIT_STREAM_SINK_COMPONENT_MEM_SUPPORTED
	case SUIT_COMPONENT_TYPE_MEM: { /* flash_sink or ram_sink */
		intptr_t run_address;
		size_t size;

		if (suit_plat_decode_address_size(component_id, &run_address, &size) !=
		    SUIT_PLAT_SUCCESS) {
			LOG_ERR("suit_plat_decode_address_size failed");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		/* Based on address span given by run_address-(run_address + size), check what type
		 *memory configuration we're working with. Possible cases include:
		 * - Internal MRAM on single controller
		 * - Internal MRAM spanning between two memory controllers
		 * - RAM
		 * - External Flash
		 * - ...
		 *
		 * Select sink based on detected memory configuration.
		 */

#ifdef CONFIG_SUIT_STREAM_SINK_FLASH
		/* Internal MRAM/Flash on single controller */
		if (suit_flash_sink_is_address_supported((uint8_t *)run_address)) {
			sink_get_err = suit_flash_sink_get(sink, (uint8_t *)run_address, size);
			if (sink_get_err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to get flash sink: %i", sink_get_err);
				return suit_plat_err_to_processor_err_convert(sink_get_err);
			}

			return SUIT_SUCCESS;
		}

		LOG_INF("Address not in Flash");
#endif /* CONFIG_SUIT_STREAM_SINK_FLASH */

#ifdef CONFIG_SUIT_STREAM_SINK_RAM
		/* Internal RAM */
		if (suit_ram_sink_is_address_supported((uint8_t *)run_address)) {
			sink_get_err = suit_ram_sink_get(sink, (uint8_t *)run_address, size);
			if (sink_get_err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to get RAM sink: %i", sink_get_err);
				return suit_plat_err_to_processor_err_convert(sink_get_err);
			}

			return SUIT_SUCCESS;
		}

		LOG_INF("Address not in RAM");
#endif /* CONFIG_SUIT_STREAM_SINK_RAM */

#ifdef CONFIG_SUIT_STREAM_SINK_EXTMEM
		/* External memory */
		if (suit_extmem_sink_is_address_supported((uint8_t *)run_address)) {
			sink_get_err = suit_extmem_sink_get(sink, (uint8_t *)run_address, size);
			if (sink_get_err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to get external memory sink: %i", sink_get_err);
				return suit_plat_err_to_processor_err_convert(sink_get_err);
			}

			return SUIT_SUCCESS;
		}

		LOG_INF("Address not in external memory");
#endif /* CONFIG_SUIT_STREAM_SINK_EXTMEM */

		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	} break;
#endif /* CONFIG_SUIT_STREAM_SINK_COMPONENT_MEM_SUPPORTED */
#ifdef CONFIG_SUIT_STREAM_SINK_SDFW
	case SUIT_COMPONENT_TYPE_SOC_SPEC: { /* sdfw_sink */
		uint32_t number;

		if (suit_plat_decode_component_number(component_id, &number) != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Missing component id number");
			return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
		}

		if (number == (uint32_t)SUIT_SECDOM_COMPONENT_NUMBER_SDFW) {
			if (IS_ENABLED(CONFIG_SUIT_STREAM_SINK_SDFW)) {
				sink_get_err = suit_sdfw_sink_get(sink);
				return suit_plat_err_to_processor_err_convert(sink_get_err);
			}

			LOG_ERR("SDFW sink not enabled");
		} else if (number == (uint32_t)SUIT_SECDOM_COMPONENT_NUMBER_SDFW_RECOVERY) {
			if (IS_ENABLED(CONFIG_SUIT_STREAM_SINK_SDFW_RECOVERY)) {
				sink_get_err = suit_sdfw_recovery_sink_get(sink);
				return suit_plat_err_to_processor_err_convert(sink_get_err);
			}

			LOG_ERR("SDFW Recovery sink not enabled");
		}

		LOG_ERR("Unsupported special component %d", number);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	} break;
#endif /* CONFIG_SUIT_STREAM_SINK_SDFW */
	default: {
		LOG_ERR("Unsupported component type: %d", component_type);
		return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
	}
	}

	return SUIT_ERR_UNSUPPORTED_COMPONENT_ID;
}
