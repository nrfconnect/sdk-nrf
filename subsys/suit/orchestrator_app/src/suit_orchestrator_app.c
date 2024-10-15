/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <dfu/suit_dfu.h>

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>

#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>

#if CONFIG_SUIT_PROCESSOR
#include <suit.h>
#endif
#include <sdfw/sdfw_services/suit_service.h>
#include <suit_envelope_info.h>
#include <suit_plat_mem_util.h>
#if CONFIG_SUIT_CACHE_RW
#include <suit_dfu_cache_rw.h>
#endif

#if CONFIG_SUIT_STREAM_IPC_PROVIDER
#include <suit_ipc_streamer.h>
#endif

LOG_MODULE_REGISTER(suit_dfu, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err) ((err) == SUIT_SUCCESS ? 0 : -EACCES)

#if CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING

static int dfu_partition_erase(void)
{

	if (suit_dfu_partition_is_empty()) {
		LOG_DBG("DFU partition is arleady erased");
		return 0;
	}

	struct suit_nvm_device_info device_info;
	int err = suit_dfu_partition_device_info_get(&device_info);

	if (err != SUIT_PLAT_SUCCESS) {
		return -EIO;
	}

	if (!device_is_ready(device_info.fdev)) {
		return -ENODEV;
	}

	LOG_DBG("Erasing DFU partition");
	int rc = flash_erase(device_info.fdev, device_info.partition_offset,
			     device_info.partition_size);
	if (rc < 0) {
		return -EIO;
	}

	return 0;
}

#endif /* CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING */

int suit_dfu_initialize(void)
{
	LOG_DBG("Enter");

	struct suit_nvm_device_info device_info;
	int err = suit_dfu_partition_device_info_get(&device_info);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Error when getting DFU partition address and size: %d", err);
		return -EIO;
	}

	LOG_INF("DFU partition detected, addr: %p, size %d bytes",
		(void *)device_info.mapped_address, device_info.partition_size);

#if CONFIG_SUIT_CLEANUP_ON_INIT
	suit_dfu_cleanup();

#else /* CONFIG_SUIT_CLEANUP_ON_INIT */
	const uint8_t *uc_env_addr = NULL;
	size_t uc_env_size = 0;

	err = suit_dfu_partition_envelope_info_get(&uc_env_addr, &uc_env_size);
	if (err == SUIT_PLAT_SUCCESS) {
		LOG_INF("Update candidate envelope detected, addr: %p, size %d bytes",
			(void *)uc_env_addr, uc_env_size);
	}

#if CONFIG_SUIT_CACHE_RW
	suit_dfu_cache_validate_content();
#endif /* CONFIG_SUIT_CACHE_RW */

#endif /* CONFIG_SUIT_CLEANUP_ON_INIT */

#if CONFIG_SUIT_STREAM_IPC_PROVIDER
	suit_ipc_streamer_provider_init();
#endif

#if CONFIG_SUIT_PROCESSOR
	err = suit_processor_init();

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to initialize suit processor: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}
#endif /* CONFIG_SUIT_PROCESSOR */

	LOG_DBG("Exit with success");

	return 0;
}

#if CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING

int suit_dfu_cleanup(void)
{
	LOG_DBG("Enter");

	int err = 0;

	err = dfu_partition_erase();
	if (err != 0) {
		return err;
	}

#if CONFIG_SUIT_CACHE_RW
	suit_dfu_cache_0_resize();
	suit_dfu_cache_drop_content();
#endif
	LOG_DBG("Exit with success");

	return 0;
}

int suit_dfu_candidate_envelope_stored(void)
{
	LOG_DBG("Enter");

	const uint8_t *uc_env_addr = NULL;
	size_t uc_env_size = 0;
	int err = suit_dfu_partition_envelope_info_get(&uc_env_addr, &uc_env_size);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Invalid update candidate: %d", err);
		return -ENOTSUP;
	}

#if CONFIG_SUIT_CACHE_RW
	suit_dfu_cache_0_resize();
#endif /* CONFIG_SUIT_CACHE_RW */

	LOG_DBG("Exit with success");
	return 0;
}

int suit_dfu_candidate_preprocess(void)
{
	LOG_DBG("Enter");

#if CONFIG_SUIT_PROCESSOR
	uint8_t *candidate_envelope_address;
	size_t candidate_envelope_size;

	int err = suit_dfu_partition_envelope_info_get(
		(const uint8_t **)&candidate_envelope_address, &candidate_envelope_size);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Invalid update candidate: %d", err);
		return -ENOTSUP;
	}

	LOG_INF("Update candidate envelope detected, addr: %p, size %d bytes",
		(void *)candidate_envelope_address, candidate_envelope_size);

	err = suit_process_sequence(candidate_envelope_address, candidate_envelope_size,
				    SUIT_SEQ_DEP_RESOLUTION);
	if (err == SUIT_SUCCESS) {
		LOG_DBG("suit-dependency-resolution successful");
	} else if (err == SUIT_ERR_UNAVAILABLE_COMMAND_SEQ) {
		LOG_DBG("suit-dependency-resolution sequence unavailable");
	} else {
		LOG_ERR("Failed to execute suit-dependency-resolution: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}

	err = suit_process_sequence(candidate_envelope_address, candidate_envelope_size,
				    SUIT_SEQ_PAYLOAD_FETCH);
	if (err == SUIT_SUCCESS) {
		LOG_DBG("suit-payload-fetch successful");
	} else if (err == SUIT_ERR_UNAVAILABLE_COMMAND_SEQ) {
		LOG_DBG("suit-payload-fetch sequence unavailable");
	} else {
		LOG_ERR("Failed to execute suit-payload-fetch: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}

#endif /* CONFIG_SUIT_PROCESSOR */

	LOG_DBG("Exit with success");
	return 0;
}

int suit_dfu_update_start(void)
{
	LOG_DBG("Enter");

	const uint8_t *region_address;
	size_t region_size;
	size_t update_regions_count = 1;

	int err = suit_dfu_partition_envelope_info_get(&region_address, &region_size);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Invalid update candidate: %d", err);

		return -ENOTSUP;
	}

	LOG_INF("Reboot the system and trigger the update");

#if CONFIG_SUIT_CACHE_RW
	suit_plat_mreg_t update_candidate[CONFIG_SUIT_CACHE_MAX_CACHES + 1];
#else
	suit_plat_mreg_t update_candidate[1];
#endif

	update_candidate[0].mem = region_address;
	update_candidate[0].size = region_size;

#if CONFIG_SUIT_CACHE_RW
	struct suit_nvm_device_info device_info;

	for (size_t i = 0; i < CONFIG_SUIT_CACHE_MAX_CACHES; i++) {
		if (suit_dfu_cache_rw_device_info_get(i, &device_info) == SUIT_PLAT_SUCCESS) {

			update_candidate[update_regions_count].mem = device_info.mapped_address;
			update_candidate[update_regions_count].size = device_info.partition_size;
			update_regions_count++;
		}
	}
#endif

	return suit_trigger_update(update_candidate, update_regions_count);
}

#endif /* CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING */
