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
#include <hal/nrf_rramc.h>

#include <cracen/lib_kmu.h>

/*
 * Enable writes and set the write buffer size to 0
 * bytes. IPS states that this is invalid but the IPS is
 * wrong. Setting it to 0 will prevent buffering and therefore
 * prevent writes from being discarded on soft reset.
 */
static void rramc_enable_writes(void)
{
	nrf_rramc_config_t const config = {
		.mode_write = true, .write_buff_size = 0 /* Unbuffered */
	};
	nrf_rramc_config_set(NRF_RRAMC_S, &config);

	while (!nrf_rramc_ready_check(NRF_RRAMC_S)) {
		;
	}
}

/*
 * Reset the CONFIG register back to having writes disabled. Also set
 * buffer size to 1.
 */
static void rramc_disable_writes(void)
{
	nrf_rramc_config_t const config = {.mode_write = false, .write_buff_size = 1};

	nrf_rramc_config_set(NRF_RRAMC_S, &config);
	while (!nrf_rramc_ready_check(NRF_RRAMC_S)) {
		;
	}
}

void lib_kmu_clear_all_events(void)
{
	NRF_KMU_S->EVENTS_METADATAREAD = 0;
	NRF_KMU_S->EVENTS_ERROR = 0;
	NRF_KMU_S->EVENTS_PROVISIONED = 0;
	NRF_KMU_S->EVENTS_PUSHED = 0;
	NRF_KMU_S->EVENTS_REVOKED = 0;
}

static int trigger_task_and_wait_for_event_or_error(volatile uint32_t *task,
						    volatile uint32_t *event)
{
	int result = -LIB_KMU_ERROR;

	lib_kmu_clear_all_events();

	*task = 1;

	while (!(*event || NRF_KMU_S->EVENTS_ERROR)) {
		/*
		 * Poll until KMU completes or fails the operation. This is
		 * not expected to take long.
		 *
		 * The body is intentionally left empty.
		 */
	}

	if (NRF_KMU_S->EVENTS_ERROR) {
		result = -LIB_KMU_ERROR;
	} else if (*event) {
		result = LIB_KMU_SUCCESS;
	} else {
		CODE_UNREACHABLE;
	}

	lib_kmu_clear_all_events();

	return result;
}

int lib_kmu_provision_slot(int slot_id, struct kmu_src_t *kmu_src)
{
	if (kmu_src == NULL) {
		return -LIB_KMU_NULL_PNT;
	}

	/* DEST must be on a 64-bit boundary */
	__ASSERT(IS_PTR_ALIGNED(kmu_src->dest, uint64_t), "unaligned kmu_src->dest");

	int result = 1;

	rramc_enable_writes();

	NRF_KMU_S->KEYSLOT = slot_id;
	NRF_KMU_S->SRC = (uint32_t)kmu_src;

	result = trigger_task_and_wait_for_event_or_error(&(NRF_KMU_S->TASKS_PROVISION),
							  &(NRF_KMU_S->EVENTS_PROVISIONED));

	rramc_disable_writes();

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
	rramc_enable_writes();

	NRF_KMU_S->KEYSLOT = slot_id;

	int result = trigger_task_and_wait_for_event_or_error(&(NRF_KMU_S->TASKS_REVOKE),
							      &(NRF_KMU_S->EVENTS_REVOKED));

	rramc_disable_writes();

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
