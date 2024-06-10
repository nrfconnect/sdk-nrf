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

#define FIXED_PARTITION_ERASE_BLOCK_SIZE(label)                                                    \
	DT_PROP(DT_GPARENT(DT_NODELABEL(label)), erase_block_size)

#define DFU_PARTITION_LABEL	 dfu_partition
#define DFU_PARTITION_ADDRESS	 suit_plat_mem_nvm_ptr_get(DFU_PARTITION_OFFSET)
#define DFU_PARTITION_OFFSET	 FIXED_PARTITION_OFFSET(DFU_PARTITION_LABEL)
#define DFU_PARTITION_SIZE	 FIXED_PARTITION_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_EB_SIZE	 FIXED_PARTITION_ERASE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_WRITE_SIZE FIXED_PARTITION_WRITE_BLOCK_SIZE(DFU_PARTITION_LABEL)
#define DFU_PARTITION_DEVICE	 FIXED_PARTITION_DEVICE(DFU_PARTITION_LABEL)

LOG_MODULE_REGISTER(suit_dfu, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err) ((err) == SUIT_SUCCESS ? 0 : -EACCES)

#if CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING

static int dfu_partition_erase(void)
{
	const struct device *fdev = DFU_PARTITION_DEVICE;

	if (!device_is_ready(fdev)) {
		return -ENODEV;
	}

	/* Division is used to round down so that erase_size is aligned to DFU_PARTITION_EB_SIZE  */
	size_t erase_size = (DFU_PARTITION_SIZE / DFU_PARTITION_EB_SIZE) * DFU_PARTITION_EB_SIZE;

	LOG_INF("Erasing dfu partition");

	int rc = flash_erase(fdev, DFU_PARTITION_OFFSET, erase_size);

	if (rc < 0) {
		return -EIO;
	}

	return 0;
}

#endif /* CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING */

int suit_dfu_initialize(void)
{
#if CONFIG_SUIT_STREAM_IPC_PROVIDER
	suit_ipc_streamer_provider_init();
#endif

#if CONFIG_SUIT_PROCESSOR
	int err = suit_processor_init();

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to initialize suit processor: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}
#endif /* CONFIG_SUIT_PROCESSOR */

	LOG_DBG("SUIT DFU module init ok");

	return 0;
}

#if CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING

int suit_dfu_cleanup(void)
{
	int err = 0;

	suit_envelope_info_reset();

	err = dfu_partition_erase();
	if (err != 0) {
		return err;
	}

#if CONFIG_SUIT_CACHE_RW
	if (suit_dfu_cache_rw_deinitialize() != SUIT_PLAT_SUCCESS) {
		return -EIO;
	}
#endif

	return 0;
}

int suit_dfu_candidate_envelope_stored(void)
{
	suit_plat_err_t err;

	err = suit_envelope_info_candidate_stored((const uint8_t *)DFU_PARTITION_ADDRESS,
						  DFU_PARTITION_SIZE);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Invalid update candidate: %d", err);

		return -ENOTSUP;
	}

#if CONFIG_SUIT_CACHE_RW
	/* All data to initialize cache partitions is present - initialize DFU cache. */
	const uint8_t *envelope_address = NULL;
	size_t envelope_size = 0;

	err = suit_envelope_info_get(&envelope_address, &envelope_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Error when getting envelope address and size: %d", err);
		return -EPIPE;
	}

	err = suit_dfu_cache_rw_initialize((void *)envelope_address, envelope_size);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Error when initializing DFU cache: %d", err);
		return -EIO;
	}
#endif /* CONFIG_SUIT_CACHE_RW */

	return 0;
}

int suit_dfu_candidate_preprocess(void)
{
#if CONFIG_SUIT_PROCESSOR
	uint8_t *candidate_envelope_address;
	size_t candidate_envelope_size;

	int err = suit_envelope_info_get((const uint8_t **)&candidate_envelope_address,
					 &candidate_envelope_size);

	if (err != SUIT_PLAT_SUCCESS) {
		LOG_INF("Invalid update candidate: %d", err);

		return -ENOTSUP;
	}

	LOG_DBG("Update candidate address: %p", candidate_envelope_address);
	LOG_DBG("Update candidate size: %d", candidate_envelope_size);

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

	return 0;
}

int suit_dfu_update_start(void)
{
	const uint8_t *region_address;
	size_t region_size;
	size_t update_regions_count = 1;

	int err = suit_envelope_info_get((const uint8_t **)&region_address, &region_size);

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
	for (size_t i = 0; i < CONFIG_SUIT_CACHE_MAX_CACHES; i++) {
		if (suit_dfu_cache_rw_partition_info_get(i, &region_address, &region_size) ==
		    SUIT_PLAT_SUCCESS) {
			update_candidate[update_regions_count].mem = region_address;
			update_candidate[update_regions_count].size = region_size;
			update_regions_count++;
		}
	}
#endif

	return suit_trigger_update(update_candidate, update_regions_count);
}

#endif /* CONFIG_SUIT_ORCHESTRATOR_APP_CANDIDATE_PROCESSING */
