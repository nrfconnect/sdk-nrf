/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <security/cracen.h>
#include <cracen_psa.h>
#include <cracen/interrupts.h>
#include <cracen/lib_kmu.h>

#include "common.h"
#include "microcode_binary.h"

/* The KMU slot that is used for the CRACEN->SEED. */
#define SLOT_OFFSET 0

static int users;

K_MUTEX_DEFINE(cracen_mutex);

LOG_MODULE_REGISTER(cracen, CONFIG_CRACEN_LOG_LEVEL);

/* Lumos defines NRF_CRACEN_S and Haltium NRF_CRACEN */
#ifdef NRF_CRACEN_S
#define NRF_CRACEN NRF_CRACEN_S
#endif

#define UCODE_PHYSICAL_RAM_BYTES 5120

_Static_assert(BA414EP_UCODE_SIZE * 4 <= UCODE_PHYSICAL_RAM_BYTES, "Check that ba414ep_ucode fits");

static void cracen_load_microcode(void)
{
	uint32_t volatile *microcode_addr = (uint32_t volatile *)CRACEN_SX_PK_MICROCODE_ADDRESS;

	LOG_DBG("Copying %d bytes of microcode from 0x%x to 0x%x\n", sizeof(ba414ep_ucode),
		(uint32_t)ba414ep_ucode, CRACEN_SX_PK_MICROCODE_ADDRESS);

	for (size_t i = 0; i < BA414EP_UCODE_SIZE; i++) {
		microcode_addr[i] = ba414ep_ucode[i];
	}
}

/**
 * @brief Function to check if the KMU seed slot is empty
 *
 * This will check three slots from a given slot_ofset to see
 * that they are filled.
 *
 * @param[in]	slot_offset	Offset for the KMU seed slot
 *
 * @retval true if empty, otherwise false.
 */
static bool cracen_is_kmu_seed_empty(uint32_t slot_offset)
{
	for (size_t i = 0; i < 3; i++) {
		if (!lib_kmu_is_slot_empty(slot_offset + i)) {
			return false;
		}
	}

	return true;
}

/**
 * @brief Function to load the CRACEN IKG seed
 *
 * Use the KMU peripheral to load a seed from KMU slot "slot_offset",
 * "slot_offset+1", and "slot_offset+2" into the CRACEN IKG seed registers.
 *
 * These KMU slots must first have been provisioned with an appropriate seed.
 */
static int cracen_load_kmu_seed(uint32_t slot_offset)
{
	for (size_t i = 0; i < 3; i++) {
		LOG_DBG("Pushing KMU slot %d\n", slot_offset + i);

		int err = lib_kmu_push_slot(slot_offset + i);

		if (err) {
			LOG_ERR("Failed to push KMU slot %d\n", slot_offset + i);
			return err;
		}
	}

	return 0;
}

static psa_status_t cracen_prng_init_kmu_seed(uint32_t slot_offset)
{
	psa_status_t status;
	int err;

	LOG_DBG("Generating KMU seed\n");

	/* If the IKG seed is already loaded (and locked). New seed can't be
	 * generated!
	 */
	bool is_locked = nrf_cracen_seedram_lock_check(NRF_CRACEN);

	if (is_locked) {
		LOG_DBG("CRACEN IKG seed is already loaded!\n");
		return PSA_SUCCESS;
	}

	/* Initialize CRACEN PRNG */
	status = cracen_init_random(NULL);
	if (status != PSA_SUCCESS) {
		LOG_DBG("Unable to initialize PRNG: %d\n", status);
		return status;
	}

	for (size_t i = 0; i < IKG_SEED_KMU_SLOT_NUM; i++) {
		struct kmu_src_t src;

		/* Check that the KMU slot is empty before generation. */
		if (!lib_kmu_is_slot_empty(slot_offset + i)) {
			LOG_DBG("KMU isn't empty (slot %d). Can't generate CRACEN IKG seed!\n",
				slot_offset + i);
			return PSA_ERROR_BAD_STATE;
		}

		/* Populate src.value using the random data */
		status = cracen_get_random(NULL, (char *)src.value, sizeof(src.value));
		if (status != PSA_SUCCESS) {
			return status;
		}

		src.rpolicy = LIB_KMU_REV_POLICY_LOCKED;
		src.dest = (uint64_t *)(&(NRF_CRACEN->SEED[i * 4]));

		/* There is no significance to the metadata value yet */
		src.metadata = 0x5EB0;

		err = lib_kmu_provision_slot(slot_offset + i, &src);
		if (err) {
			return PSA_ERROR_GENERIC_ERROR;
		}

		LOG_DBG("Provisioned KMU slot %d\n", i);
	}

	return PSA_SUCCESS;
}

void cracen_acquire(void)
{
	k_mutex_lock(&cracen_mutex, K_FOREVER);

	if (users++ == 0) {
		nrf_cracen_module_enable(NRF_CRACEN, CRACEN_ENABLE_CRYPTOMASTER_Msk |
							     CRACEN_ENABLE_RNG_Msk |
							     CRACEN_ENABLE_PKEIKG_Msk);
		LOG_DBG("Power on CRACEN.");
		irq_enable(CRACEN_IRQn);
	}

	k_mutex_unlock(&cracen_mutex);
}

void cracen_release(void)
{
	k_mutex_lock(&cracen_mutex, K_FOREVER);

	if (--users == 0) {
		/* Disable IRQs in the ARM NVIC as the first operation to be
		 * sure no IRQs fire while we are turning CRACEN off.
		 */
		irq_disable(CRACEN_IRQn);

		uint32_t int_disable_mask = 0;

		int_disable_mask |= CRACEN_INTENCLR_CRYPTOMASTER_Msk;
		int_disable_mask |= CRACEN_INTENCLR_RNG_Msk;
		int_disable_mask |= CRACEN_INTENCLR_PKEIKG_Msk;

		/* Disable IRQs at the CRACEN peripheral */
		nrf_cracen_int_disable(NRF_CRACEN, int_disable_mask);

		uint32_t enable_mask = 0;

		enable_mask |= CRACEN_ENABLE_CRYPTOMASTER_Msk;
		enable_mask |= CRACEN_ENABLE_RNG_Msk;
		enable_mask |= CRACEN_ENABLE_PKEIKG_Msk;

		/* Power off CRACEN */
		nrf_cracen_module_disable(NRF_CRACEN, enable_mask);

		/* Clear CRACEN events */
		nrf_cracen_event_clear(NRF_CRACEN, NRF_CRACEN_EVENT_CRYPTOMASTER);
		nrf_cracen_event_clear(NRF_CRACEN, NRF_CRACEN_EVENT_RNG);
		nrf_cracen_event_clear(NRF_CRACEN, NRF_CRACEN_EVENT_PKE_IKG);

		/* Clear pending IRQs at the ARM NVIC */
		NVIC_ClearPendingIRQ(CRACEN_IRQn);

		LOG_DBG("Powered off CRACEN.");
	}

	k_mutex_unlock(&cracen_mutex);
}

#define CRACEN_NOT_INITIALIZED 0x207467
#define CRACEN_INITIALIZED     0x657311

int cracen_init(void)
{
	psa_status_t status = PSA_SUCCESS;
	int err = 0;

	static uint32_t cracen_initialized = CRACEN_NOT_INITIALIZED;

	if (cracen_initialized == CRACEN_INITIALIZED) {
		/* cracen_init has already succeeded */
		return 0;
	}

	cracen_acquire();
	cracen_interrupts_init();

	if (IS_ENABLED(CONFIG_CRACEN_LOAD_MICROCODE)) {
		/* NOTE: CRACEN needs power to load microcode */
		LOG_DBG("Loading microcode for CRACEN PKE+IKG support");
		cracen_load_microcode();
	}

	/* Initialize PK engine. */
	err = sx_pk_init();
	if (err != SX_OK) {
		status = silex_statuscodes_to_psa(err);
		goto exit;
	}

	if (IS_ENABLED(CONFIG_CRACEN_LOAD_KMU_SEED)) {
		/**
		 * Check that the KMU contains the CRACEN IKG seed value.
		 */
		if (cracen_is_kmu_seed_empty(SLOT_OFFSET)) {
			if (IS_ENABLED(CONFIG_CRACEN_GENERATE_KMU_SEED)) {
				err = cracen_prng_init_kmu_seed(SLOT_OFFSET);
				if (err != 0) {
					goto exit;
				}
			} else {
				/**
				 * Mismatch in configuration. We have enabled
				 * CONFIG_CRACEN_LOAD_KMU_SEED but we have no KMU stored and no way
				 * to generate it.
				 */
				LOG_ERR("CRACEN IKG seed is not in KMU, and we have no way to "
					"generate it!");
				k_panic();
			}
		}

		/* Try to push the CRACEN IKG seed from KMU */
		LOG_DBG("Loading CRACEN IKG seed from KMU...");
		if (cracen_load_kmu_seed(SLOT_OFFSET) != 0) {
			LOG_ERR("Unable to load CRACEN IKG seed!");
			err = -ENODATA;
			goto exit;
		}

		if (IS_ENABLED(CONFIG_SOC_SERIES_NRF54LX)) {
			/**
			 * Lock the CRACEN seed RAM.
			 *
			 * This must be done before the IKG engine is started.
			 *
			 * We only lock it on 54L because on 54H the SDROM locks the
			 * Seed RAM for us.
			 */
			nrf_cracen_seedram_lock_enable_set(NRF_CRACEN, true);
		}
	}

exit:
	cracen_release();

	if (status == PSA_SUCCESS) {
		cracen_initialized = CRACEN_INITIALIZED;
	}

	return status;
}
