/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <nrf.h>

#include <nrfx_rramc.h>

#include <cracen/lib_kmu.h>

void lib_kmu_clear_all_events(void)
{
	NRF_KMU_S->EVENTS_METADATAREAD = 0;
	NRF_KMU_S->EVENTS_ERROR = 0;
	NRF_KMU_S->EVENTS_PROVISIONED = 0;
	NRF_KMU_S->EVENTS_PUSHED = 0;
	NRF_KMU_S->EVENTS_REVOKED = 0;
	NRF_KMU_S->EVENTS_PUSHBLOCKED = 0;
}

static int trigger_task_and_wait_for_event_or_error(volatile uint32_t *task,
						    volatile uint32_t *event)
{
	int result = -LIB_KMU_ERROR;

	lib_kmu_clear_all_events();

	*task = 1;

	while (!(*event || NRF_KMU_S->EVENTS_ERROR || NRF_KMU_S->EVENTS_REVOKED)) {
		/*
		 * Poll until KMU completes or fails the operation. This is
		 * not expected to take long.
		 *
		 * The body is intentionally left empty.
		 */
	}

	if (*event) {
		result = LIB_KMU_SUCCESS;
	} else {
		/* Check that REVOKED event is not expected. This can be when a revoke task is
		 * triggered, where the REVOKED event actually means success.
		 */
		if (event != &NRF_KMU_S->EVENTS_REVOKED && NRF_KMU_S->EVENTS_REVOKED) {
			result = -LIB_KMU_REVOKED;
		} else {
			result = -LIB_KMU_ERROR;
		}
	}

	lib_kmu_clear_all_events();

	return result;
}

int lib_kmu_provision_slot(int slot_id, struct kmu_src *kmu_src)
{
	if (kmu_src == NULL) {
		return -LIB_KMU_NULL_PNT;
	}

	/* The DEST address must be 64-bit aligned on nrf54l15pdk.
	 * On nrf54l15dk, the alignment requirement changed to 128 bits.
	 */
	__ASSERT(IS_PTR_ALIGNED_BYTES(kmu_src->dest,
				      (IS_ENABLED(CONFIG_SOC_NRF54L15_ENGA_CPUAPP) ? 8 : 16)),
		 "DEST misaligned");

	int result = 1;

#if defined(__NRF_TFM__)
	nrf_rramc_config_t rramc_config;

	nrf_rramc_config_get(NRF_RRAMC_S, &rramc_config);
	const uint8_t orig_write_buf_size = rramc_config.write_buff_size;

	rramc_config.write_buff_size = 0;
	nrf_rramc_config_set(NRF_RRAMC_S, &rramc_config);
#else
	nrfx_rramc_write_enable_set(true, 0);
#endif

	NRF_KMU_S->KEYSLOT = slot_id;
	NRF_KMU_S->SRC = (uint32_t)kmu_src;

	result = trigger_task_and_wait_for_event_or_error(&(NRF_KMU_S->TASKS_PROVISION),
							  &(NRF_KMU_S->EVENTS_PROVISIONED));

#if defined(__NRF_TFM__)
	rramc_config.write_buff_size = orig_write_buf_size;
	nrf_rramc_config_set(NRF_RRAMC_S, &rramc_config);
#else
	nrfx_rramc_write_enable_set(false, 0);
#endif

	return result;
}

int lib_kmu_push_slot(int slot_id)
{
	NRF_KMU_S->KEYSLOT = slot_id;

	return trigger_task_and_wait_for_event_or_error(&(NRF_KMU_S->TASKS_PUSH),
							&(NRF_KMU_S->EVENTS_PUSHED));
}

int lib_kmu_revoke_slot(int slot_id)
{
#if !defined(__NRF_TFM__)
	nrfx_rramc_write_enable_set(true, 0);
#endif

	NRF_KMU_S->KEYSLOT = slot_id;

	int result = trigger_task_and_wait_for_event_or_error(&(NRF_KMU_S->TASKS_REVOKE),
							      &(NRF_KMU_S->EVENTS_REVOKED));

#if !defined(__NRF_TFM__)
	nrfx_rramc_write_enable_set(false, 0);
#endif

	return result;
}

int lib_kmu_read_metadata(int slot_id, uint32_t *metadata)
{
	if (metadata == NULL) {
		return -LIB_KMU_NULL_PNT;
	}

	NRF_KMU_S->KEYSLOT = slot_id;

	int result = trigger_task_and_wait_for_event_or_error(&(NRF_KMU_S->TASKS_READMETADATA),
							      &(NRF_KMU_S->EVENTS_METADATAREAD));

	if (result == LIB_KMU_SUCCESS) {
		*metadata = NRF_KMU_S->METADATA;
	}

	return result;
}

bool lib_kmu_is_slot_empty(int slot_id)
{
	uint32_t dummy;

	int err = lib_kmu_read_metadata(slot_id, &dummy);

	/*
	 * We assume the slot must be empty if and only if we fail to read
	 * the metadata.
	 */

	return err != 0;
}
