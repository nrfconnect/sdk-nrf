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
#include <nrf_security_mutexes.h>

#if !defined(CONFIG_BUILD_WITH_TFM)
#define LOG_ERR_MSG(msg) LOG_ERR(msg)
#define LOG_INF_MSG(msg) LOG_INF(msg)
#define LOG_DBG_MSG(msg) LOG_DBG(msg)
#endif

static int users;

NRF_SECURITY_MUTEX_DEFINE(cracen_mutex);

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

void cracen_acquire(void)
{
	nrf_security_mutex_lock(&cracen_mutex);

	if (users++ == 0) {
		nrf_cracen_module_enable(NRF_CRACEN, CRACEN_ENABLE_CRYPTOMASTER_Msk |
							     CRACEN_ENABLE_RNG_Msk |
							     CRACEN_ENABLE_PKEIKG_Msk);
		irq_enable(CRACEN_IRQn);
		LOG_DBG_MSG("Power on CRACEN.");
	}

	nrf_security_mutex_unlock(&cracen_mutex);
}

void cracen_release(void)
{
	nrf_security_mutex_lock(&cracen_mutex);

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
		LOG_DBG_MSG("Powered off CRACEN.");
	}

	nrf_security_mutex_unlock(&cracen_mutex);
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

exit:
	cracen_release();

	if (status == PSA_SUCCESS) {
		cracen_initialized = CRACEN_INITIALIZED;
	}

	return status;
}
