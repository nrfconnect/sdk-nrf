/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/logging/log.h>
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>
#include <suit_orchestrator.h>

#include <stdbool.h>
#include <suit.h>
#include <suit_platform.h>
#include <suit_storage.h>
#include <suit_plat_mem_util.h>
#include <suit_plat_decode_util.h>
#include <suit_mci.h>
#include <suit_plat_digest_cache.h>
#include "suit_plat_err.h"
#include <suit_execution_mode.h>
#include <suit_dfu_cache.h>

LOG_MODULE_REGISTER(suit_orchestrator, CONFIG_SUIT_LOG_LEVEL);

#define SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err) ((err) == SUIT_SUCCESS ? 0 : -EACCES)

#define SUIT_PLAT_ERR_TO_ZEPHYR_ERR(err) ((err) == SUIT_PLAT_SUCCESS ? 0 : -EACCES)

enum suit_orchestrator_state {
	STATE_STARTUP,
	STATE_INVOKE,
	STATE_INVOKE_RECOVERY,
	STATE_INSTALL,
	STATE_INSTALL_RECOVERY,
	STATE_POST_INVOKE,
	STATE_POST_INVOKE_RECOVERY,
	STATE_POST_INSTALL,
	STATE_ENTER_RECOVERY,
	STATE_INSTALL_NORDIC_TOP,
	STATE_POST_INSTALL_NORDIC_TOP,
};

static suit_plat_err_t storage_emergency_flag_get(bool *flag)
{
	const uint8_t *fail_report;
	size_t fail_report_len;
	suit_plat_err_t ret = suit_storage_report_read(0, &fail_report, &fail_report_len);

	if (flag == NULL) {
		return SUIT_PLAT_ERR_INVAL;
	}

	if (ret == SUIT_PLAT_ERR_NOT_FOUND) {
		*flag = false;
	} else if (ret == SUIT_PLAT_SUCCESS) {
		*flag = true;
	} else {
		LOG_ERR("Unable to read recovery flag: %d", ret);
		*flag = true;
		return ret;
	}

	return SUIT_PLAT_SUCCESS;
}

static int enter_emergency_recovery(void)
{
	/* TODO: Report boot status */
	suit_plat_err_t ret = suit_storage_report_save(0, NULL, 0);

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to set recovery flag: %d", ret);
		return -EIO;
	}

	return 0;
}

static void leave_emergency_recovery(void)
{
	suit_plat_err_t ret = suit_storage_report_clear(0);

	if (ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to clear recovery flag: %d", ret);
	}
}

static int validate_update_candidate_address_and_size(const uint8_t *addr, size_t size)
{
	if (addr == NULL || addr == (void *)EMPTY_STORAGE_VALUE) {
		LOG_DBG("Invalid update candidate address: %p", (void *)addr);
		return -EFAULT;
	}

	if (size == 0 || size == EMPTY_STORAGE_VALUE) {
		LOG_DBG("Invalid update candidate size: %d", size);
		return -EFAULT;
	}

	return 0;
}

static int initialize_dfu_cache(const suit_plat_mreg_t *update_regions, size_t update_regions_len)
{
	struct dfu_cache cache = {0};

	if (update_regions == NULL || update_regions_len < 1) {
		return -EINVAL;
	}

	if ((update_regions_len - 1) > ARRAY_SIZE(cache.pools)) {
		return -EINVAL;
	}

	cache.pools_count = update_regions_len - 1;

	for (size_t i = 1; i < update_regions_len; i++) {
		cache.pools[i - 1].address = (uint8_t *)update_regions[i].mem;
		cache.pools[i - 1].size = update_regions[i].size;
	}

	return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(suit_dfu_cache_initialize(&cache));
}

static int validate_update_candidate_manifest(uint8_t *manifest_address, size_t manifest_size)
{
	suit_independent_updateability_policy_t policy = SUIT_INDEPENDENT_UPDATE_DENIED;
	suit_manifest_class_id_t *manifest_class_id = NULL;
	struct zcbor_string manifest_component_id = {
		.value = NULL,
		.len = 0,
	};

	int err = suit_processor_get_manifest_metadata(manifest_address, manifest_size, true,
						       &manifest_component_id, NULL, NULL, NULL);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Unable to read update candidate manifest metadata: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}

	err = suit_plat_decode_manifest_class_id(&manifest_component_id, &manifest_class_id);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to parse update candidate manifest class ID: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(SUIT_ERR_MANIFEST_VALIDATION);
	}

	err = suit_mci_independent_update_policy_get(manifest_class_id, &policy);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to read independent updateability policy: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(SUIT_ERR_UNSUPPORTED_COMPONENT_ID);
	}

	if (policy != SUIT_INDEPENDENT_UPDATE_ALLOWED) {
		LOG_ERR("Independent updates of the provided update candidate denied.");
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(SUIT_ERR_AUTHENTICATION);
	}

	err = suit_process_sequence(manifest_address, manifest_size, SUIT_SEQ_PARSE);

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to validate update candidate manifest: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}

	return 0;
}

static int clear_update_candidate(void)
{
	int err = 0;

	err = suit_storage_update_cand_set(NULL, 0);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to clear update candidate: %d", err);
		return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(err);
	}

	LOG_DBG("Update candidate cleared");

	return 0;
}

static int update_path(void)
{
	const suit_plat_mreg_t *update_regions = NULL;
	size_t update_regions_len = 0;

	int err = suit_storage_update_cand_get(&update_regions, &update_regions_len);

	if ((err != SUIT_PLAT_SUCCESS) || (update_regions_len < 1)) {
		LOG_ERR("Failed to get update candidate data: %d", err);
		return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(err);
	}

	LOG_DBG("Update candidate address: %p", (void *)update_regions[0].mem);
	LOG_DBG("Update candidate size: %d", update_regions[0].size);

	err = validate_update_candidate_address_and_size(update_regions[0].mem,
							 update_regions[0].size);
	if (err != 0) {
		LOG_INF("Invalid update candidate: %d", err);
		return err;
	}

	err = initialize_dfu_cache(update_regions, update_regions_len);

	if (err != 0) {
		LOG_ERR("Failed to initialize DFU cache pools: %d", err);
		return err;
	}

	err = validate_update_candidate_manifest((uint8_t *)update_regions[0].mem,
						 update_regions[0].size);
	if (err != 0) {
		LOG_ERR("Failed to validate update candidate manifest: %d", err);
		return err;
	}
	LOG_INF("Manifest validated");

	err = suit_process_sequence((uint8_t *)update_regions[0].mem, update_regions[0].size,
				    SUIT_SEQ_CAND_VERIFICATION);
	if (err != SUIT_SUCCESS) {
		if (err == SUIT_ERR_UNAVAILABLE_COMMAND_SEQ) {
			LOG_DBG("Command sequence suit-candidate-verification not available - skip "
				"it");
			err = 0;
		} else {
			LOG_ERR("Failed to execute suit-candidate-verification: %d", err);
			return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
		}
	}
	LOG_DBG("suit-candidate-verification successful");

	err = suit_process_sequence((uint8_t *)update_regions[0].mem, update_regions[0].size,
				    SUIT_SEQ_INSTALL);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to execute suit-install: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}

	LOG_INF("suit-install successful");

	return err;
}

static int boot_envelope(const suit_manifest_class_id_t *class_id)
{
	const uint8_t *installed_envelope_address = NULL;
	size_t installed_envelope_size = 0;

	suit_plat_err_t err = suit_storage_installed_envelope_get(
		class_id, &installed_envelope_address, &installed_envelope_size);
	if (err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to get installed envelope data: %d", err);
		return -ENOENT;
	}
	if (installed_envelope_address == NULL) {
		LOG_ERR("Invalid envelope address");
		return -ENOENT;
	}
	if (installed_envelope_size == 0) {
		LOG_ERR("Invalid envelope size");
		return -ENOENT;
	}
	LOG_DBG("Found installed envelope");

	err = suit_process_sequence(installed_envelope_address, installed_envelope_size,
				    SUIT_SEQ_PARSE);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to validate installed root manifest: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}
	LOG_DBG("Validated installed root manifest");

	unsigned int seq_num;

	err = suit_processor_get_manifest_metadata(installed_envelope_address,
						   installed_envelope_size, true, NULL, NULL, NULL,
						   &seq_num);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to read manifest version and digest: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}
	LOG_INF("Booting from manifest version: 0x%x", seq_num);

	err = suit_process_sequence(installed_envelope_address, installed_envelope_size,
				    SUIT_SEQ_VALIDATE);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to execute suit-validate: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}

	LOG_INF("Processed suit-validate");

	err = suit_process_sequence(installed_envelope_address, installed_envelope_size,
				    SUIT_SEQ_LOAD);
	if (err != SUIT_SUCCESS) {
		if (err == SUIT_ERR_UNAVAILABLE_COMMAND_SEQ) {
			LOG_DBG("Command sequence suit-load not available - skip it");
			err = 0;
		} else {
			LOG_ERR("Failed to execute suit-load: %d", err);
			return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
		}
	}
	LOG_INF("Processed suit-load");

	err = suit_process_sequence(installed_envelope_address, installed_envelope_size,
				    SUIT_SEQ_INVOKE);
	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to execute suit-invoke: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}
	LOG_INF("Processed suit-invoke");

	return 0;
}

static int boot_path(bool emergency)
{
	const suit_manifest_class_id_t *class_ids_to_boot[CONFIG_SUIT_STORAGE_N_ENVELOPES] = {NULL};
	size_t class_ids_to_boot_len = ARRAY_SIZE(class_ids_to_boot);
	int ret = 0;

	mci_err_t mci_ret = suit_mci_invoke_order_get(
		(const suit_manifest_class_id_t **)&class_ids_to_boot, &class_ids_to_boot_len);
	if (mci_ret != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Unable to get invoke order (MCI err: %d)", mci_ret);
		return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(mci_ret);
	}

	for (size_t i = 0; i < class_ids_to_boot_len; i++) {
		ret = boot_envelope((const suit_manifest_class_id_t *)class_ids_to_boot[i]);
		if (ret != 0) {
			LOG_ERR("Booting %d manifest failed (%d)", i, ret);
			if (emergency) {
				/* Conditionally continue booting other envelopes.
				 * Recovery FW shall discover which parts of the system
				 * are available.
				 */
				continue;
			}
			return ret;
		}

		LOG_DBG("Manifest %d booted", i);
	}

	return ret;
}

int suit_orchestrator_init(void)
{
	bool emergency_flag = true;
	const suit_plat_mreg_t *update_regions = NULL;
	size_t update_regions_len = 0;

	int err = suit_processor_init();

	if (err != SUIT_SUCCESS) {
		LOG_ERR("Failed to initialize suit processor: %d", err);
		return SUIT_PROCESSOR_ERR_TO_ZEPHYR_ERR(err);
	}

	suit_plat_err_t plat_err = suit_storage_init();

	if (plat_err != SUIT_PLAT_SUCCESS) {
		switch (plat_err) {
		case SUIT_PLAT_ERR_AUTHENTICATION:
			LOG_ERR("Failed to load MPI: invalid digest");
			plat_err = suit_execution_mode_set(EXECUTION_MODE_FAIL_NO_MPI);
			break;
		case SUIT_PLAT_ERR_NOT_FOUND:
			LOG_ERR("Failed to load MPI: essential (Nordic, root) roles unconfigured");
			plat_err = suit_execution_mode_set(EXECUTION_MODE_FAIL_MPI_INVALID_MISSING);
			break;
		case SUIT_PLAT_ERR_OUT_OF_BOUNDS:
			LOG_ERR("Failed to load MPI: invalid MPI format (i.e. version, values)");
			plat_err = suit_execution_mode_set(EXECUTION_MODE_FAIL_MPI_INVALID);
			break;
		case SUIT_PLAT_ERR_UNSUPPORTED:
			LOG_ERR("Failed to load MPI: unsupported configuration");
			plat_err = suit_execution_mode_set(EXECUTION_MODE_FAIL_MPI_UNSUPPORTED);
			break;
		default:
			LOG_ERR("Failed to init suit storage: %d", plat_err);
			return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(plat_err);
		}

		if (plat_err != SUIT_PLAT_SUCCESS) {
			LOG_ERR("Setting execution mode failed state failed: %d", plat_err);
			return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(plat_err);
		}

		plat_err = suit_storage_update_cand_get(&update_regions, &update_regions_len);
		if ((plat_err == SUIT_PLAT_SUCCESS) && (update_regions_len > 0)) {
			LOG_WRN("SDFW update provided in a FAILED state");

			mci_err_t mci_err = suit_mci_init();

			if (mci_err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Failed to init MCI for SDFW update: %d", mci_err);
				return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(mci_err);
			}

			plat_err = suit_execution_mode_set(EXECUTION_MODE_FAIL_INSTALL_NORDIC_TOP);
			if (plat_err != SUIT_PLAT_SUCCESS) {
				LOG_ERR("Setting SDFW update execution mode failed: %d", plat_err);
				return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(plat_err);
			}
		}

		LOG_WRN("Execution mode in a FAILED state");
		return SUIT_PLAT_SUCCESS;
	}

	mci_err_t mci_err = suit_mci_init();

	if (mci_err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Failed to init MCI: %d", mci_err);
		return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(mci_err);
	}

	plat_err = suit_storage_update_cand_get(&update_regions, &update_regions_len);
	if (plat_err == SUIT_PLAT_ERR_NOT_FOUND) {
		LOG_DBG("No update candidate provided");
		update_regions_len = 0;
	} else if (plat_err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Reading update candidate info failed: %d", plat_err);
		update_regions_len = 0;
	}

	plat_err = storage_emergency_flag_get(&emergency_flag);
	if (plat_err != SUIT_PLAT_SUCCESS) {
		LOG_WRN("Unable to read emergency flag: %d", plat_err);
		emergency_flag = true;
	}

	if (update_regions_len > 0) {
		if (emergency_flag == false) {
			plat_err = suit_execution_mode_set(EXECUTION_MODE_INSTALL);
		} else {
			plat_err = suit_execution_mode_set(EXECUTION_MODE_INSTALL_RECOVERY);
		}
	} else {
		if (emergency_flag == false) {
			plat_err = suit_execution_mode_set(EXECUTION_MODE_INVOKE);
		} else {
			plat_err = suit_execution_mode_set(EXECUTION_MODE_INVOKE_RECOVERY);
		}
	}

	if (plat_err != SUIT_PLAT_SUCCESS) {
		LOG_ERR("Setting execution mode failed: %d", plat_err);
		return SUIT_PLAT_ERR_TO_ZEPHYR_ERR(plat_err);
	}

	LOG_DBG("SUIT orchestrator init ok");
	return 0;
}

static int suit_orchestrator_run(void)
{
	enum suit_orchestrator_state state = STATE_STARTUP;
	int ret = -EFAULT;

	/* Set the default state, based on the execution mode. */
	switch (suit_execution_mode_get()) {
	case EXECUTION_MODE_INVOKE:
		state = STATE_INVOKE;
		break;
	case EXECUTION_MODE_INVOKE_RECOVERY:
		state = STATE_INVOKE_RECOVERY;
		break;
	case EXECUTION_MODE_INSTALL:
		state = STATE_INSTALL;
		break;
	case EXECUTION_MODE_INSTALL_RECOVERY:
		state = STATE_INSTALL_RECOVERY;
		break;
	case EXECUTION_MODE_FAIL_INSTALL_NORDIC_TOP:
		state = STATE_INSTALL_NORDIC_TOP;
		break;

	case EXECUTION_MODE_FAIL_NO_MPI:
	case EXECUTION_MODE_FAIL_MPI_INVALID:
	case EXECUTION_MODE_FAIL_MPI_INVALID_MISSING:
	case EXECUTION_MODE_FAIL_MPI_UNSUPPORTED:
	case EXECUTION_MODE_FAIL_INVOKE_RECOVERY:
		return -EFAULT;

	case EXECUTION_MODE_STARTUP:
	case EXECUTION_MODE_POST_INVOKE:
	case EXECUTION_MODE_POST_INVOKE_RECOVERY:
	default:
		break;
	}

	while (true) {
		switch (state) {
		case STATE_INSTALL:
			LOG_INF("Update path");
			ret = update_path();
			state = STATE_POST_INSTALL;
			break;

		case STATE_INSTALL_RECOVERY:
			LOG_INF("Recovery update path");
			ret = update_path();
			if (ret == 0) {
				leave_emergency_recovery();
			}

			state = STATE_POST_INSTALL;
			break;

		case STATE_POST_INSTALL: {
			LOG_INF("Update finished: %d", ret);

			/* TODO: Report update status */

			int clear_ret = clear_update_candidate();

			if (clear_ret != 0) {
				LOG_WRN("Unable to clear update candidate info: %d", clear_ret);
			}

			if (IS_ENABLED(CONFIG_SUIT_UPDATE_REBOOT_ENABLED)) {
				LOG_INF("Reboot the system after update: %d", ret);
				LOG_PANIC();

				sys_reboot(SYS_REBOOT_COLD);
			}
			return ret;
		}

		case STATE_INSTALL_NORDIC_TOP:
			LOG_INF("SDFW update path");
			ret = update_path();
			state = STATE_POST_INSTALL_NORDIC_TOP;
			break;

		case STATE_POST_INSTALL_NORDIC_TOP: {
			LOG_INF("SDFW update finished: %d", ret);

			/* TODO: Report update status */

			int clear_ret = clear_update_candidate();

			if (clear_ret != 0) {
				LOG_WRN("Unable to clear update candidate info: %d", clear_ret);
			}

			if (IS_ENABLED(CONFIG_SUIT_UPDATE_REBOOT_ENABLED)) {
				LOG_INF("Reboot the system after update: %d", ret);
				LOG_PANIC();

				sys_reboot(SYS_REBOOT_COLD);
			}
			return ret;
		}

		case STATE_INVOKE:
			LOG_INF("Boot path");
			ret = boot_path(false);
			if (ret == 0) {
				state = STATE_POST_INVOKE;
			} else {
				state = STATE_ENTER_RECOVERY;
			}
			break;

		case STATE_INVOKE_RECOVERY:
			LOG_INF("Recovery boot path");
			ret = boot_path(true);
			state = STATE_POST_INVOKE_RECOVERY;
			break;

		case STATE_ENTER_RECOVERY:
			LOG_INF("Enter recovery mode");

			ret = enter_emergency_recovery();
			if (ret != 0) {
				LOG_WRN("Unable to enter emergency recovery: %d", ret);
			}

			if (IS_ENABLED(CONFIG_SUIT_BOOT_RECOVERY_REBOOT_ENABLED)) {
				LOG_INF("Reboot the system after unsuccessful boot: %d", ret);
				LOG_PANIC();
				sys_reboot(SYS_REBOOT_COLD);
			}
			return -ENOTSUP;

		case STATE_POST_INVOKE:
			if (suit_execution_mode_set(EXECUTION_MODE_POST_INVOKE) !=
			    SUIT_PLAT_SUCCESS) {
				LOG_WRN("Unable to change execution mode to INVOKE");
			}
			return ret;

		case STATE_POST_INVOKE_RECOVERY:
			if (ret == 0) {
				if (suit_execution_mode_set(EXECUTION_MODE_POST_INVOKE_RECOVERY) !=
				    SUIT_PLAT_SUCCESS) {
					LOG_WRN("Unable to change execution mode to INVOKE "
						"RECOVERY");
				}
			} else {
				if (suit_execution_mode_set(EXECUTION_MODE_FAIL_INVOKE_RECOVERY) !=
				    SUIT_PLAT_SUCCESS) {
					LOG_WRN("Unable to change execution mode to FAIL INVOKE "
						"RECOVERY");
				}
			}
			return ret;

		default:
			LOG_ERR("Invalid state: %d", state);
			return -EINVAL;
		}
	}

	return -EFAULT;
}

int suit_orchestrator_entry(void)
{
#if CONFIG_SUIT_DIGEST_CACHE
	if (suit_plat_digest_cache_remove_all() != SUIT_SUCCESS) {
		LOG_WRN("Unable to clear digest cache. Digest cache state corrupted.");
	}
#endif

	int ret = suit_orchestrator_run();

#ifdef CONFIG_SUIT_LOG_SECDOM_VERSION
	LOG_INF("Secdom version: %s", CONFIG_SUIT_SECDOM_VERSION);
#endif /* CONFIG_SUIT_LOG_SECDOM_VERSION */

	return ret;
}
