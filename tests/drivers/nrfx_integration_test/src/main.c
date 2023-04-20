/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/ztest.h>
#include <nrfx.h>

/*
 * The code below is not intended to do anything meaningful. Its only purpose
 * is to use the definitions provided in nrfx_glue.h, to ensure that they are
 * correct and thus nrfx is properly integrated into the environment of Zephyr.
 */

NRFX_STATIC_ASSERT(true);

ZTEST(test_nrfx_integration, test_build)
{
	IRQn_Type dummy_irq_number = (IRQn_Type)0;
	nrfx_atomic_t dummy_atomic_object;
	volatile uint32_t used_resources;

	NRFX_ASSERT(true);

	NRFX_IRQ_PRIORITY_SET(dummy_irq_number, 0);
	NRFX_IRQ_ENABLE(dummy_irq_number);
	if (NRFX_IRQ_IS_ENABLED(dummy_irq_number)) {
		NRFX_IRQ_DISABLE(dummy_irq_number);
	}
	NRFX_IRQ_PENDING_SET(dummy_irq_number);
	if (NRFX_IRQ_IS_PENDING(dummy_irq_number)) {
		NRFX_IRQ_PENDING_CLEAR(dummy_irq_number);
	}

	NRFX_CRITICAL_SECTION_ENTER();
	NRFX_CRITICAL_SECTION_EXIT();

	NRFX_DELAY_US(0);

	NRFX_ATOMIC_FETCH_STORE(&dummy_atomic_object, 0);
	NRFX_ATOMIC_FETCH_OR(&dummy_atomic_object, 0);
	NRFX_ATOMIC_FETCH_AND(&dummy_atomic_object, 0);
	NRFX_ATOMIC_FETCH_XOR(&dummy_atomic_object, 0);
	NRFX_ATOMIC_FETCH_ADD(&dummy_atomic_object, 0);
	NRFX_ATOMIC_FETCH_SUB(&dummy_atomic_object, 0);

	used_resources = NRFX_DPPI_CHANNELS_USED | NRFX_DPPI_GROUPS_USED |
			 NRFX_PPI_CHANNELS_USED | NRFX_PPI_GROUPS_USED |
			 NRFX_EGUS_USED | NRFX_TIMERS_USED;
}

ZTEST_SUITE(test_nrfx_integration, NULL, NULL, NULL, NULL, NULL);
