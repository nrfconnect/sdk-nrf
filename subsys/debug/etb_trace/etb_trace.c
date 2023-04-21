/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <debug/etb_trace.h>

#include "etb_trace_private.h"

/* Declaring as static variables to make them appear in ELF file
 * for simplified trace processing.
 */

/* Bit 3: Branch broadcast mode enabled. */
static volatile uint32_t etm_trcconfigr = BIT(3);
/* Set trace ID to 0x10  */
static volatile uint32_t etm_trctraceidr = 0x10;

static void etm_init(void)
{
	/* Disable ETM to allow configuration */
	SET_REG(ETM_TRCPRGCTLR, 0);

	/* Wait until ETM is idle and programmer's model is stable */
	while ((GET_REG(ETM_TRCSTATR) & (ETM_TRCSTATR_PMSTABLE | ETM_TRCSTATR_IDLE))
	       != (ETM_TRCSTATR_PMSTABLE | ETM_TRCSTATR_IDLE)) {
	}

	/* Configure the ETM */
	SET_REG(ETM_TRCCONFIGR, etm_trcconfigr);

	/* Disable stalling of CPU as the trace unit can not stall the processor
	 * for instruction traces, at the risk of losing traces.
	 */
	SET_REG(ETM_TRCSTALLCTLR, 0);

	/* Set Global Timestamp Control Register to zero so that no stamps included in the trace. */
	SET_REG(ETM_TRCTSCTLR, 0);

	/* Set the trace stream ID */
	SET_REG(ETM_TRCTRACEIDR, etm_trctraceidr);

	/* Bit 0: Enable event 0.
	 * Bit 9: Indicates the current status of the start/stop logic.
	 * Bit 10: Always trace reset exceptions.
	 * Bit 11: Always trace system error exceptions.
	 */
	SET_REG(ETM_TRCVICTLR,
		ETM_TRCVICTLR_TRCERR |
		ETM_TRCVICTLR_TRCRESET |
		ETM_TRCVICTLR_SSSTATUS |
		ETM_TRCVICTLR_EVENT0);

	/* Disable ETM events */
	SET_REG(ETM_TRCEVENTCTL0R, 0);
	SET_REG(ETM_TRCEVENTCTL1R, 0);

	/* Enable ETM */
	SET_REG(ETM_TRCPRGCTLR, ETM_TRCPRGCTLR_ENABLE);
}

static void etm_stop(void)
{
	/* Disable ETM */
	SET_REG(ETM_TRCPRGCTLR, 0);
}

static void atb_init(void)
{
	/* ATB replicator */
	CS_UNLOCK(ATB_REPLICATOR_BASE_ADDR);

	/* ID filter for master port 0 */
	SET_REG(ATB_REPLICATOR_IDFILTER0, 0xFFFFFFFFUL);
	/* ID filter for master port 1, allowing ETM traces from CM33 to ETB */
	SET_REG(ATB_REPLICATOR_IDFILTER1, 0xFFFFFFFDUL);

	CS_LOCK(ATB_REPLICATOR_BASE_ADDR);

	/* ATB funnel 1 */
	CS_UNLOCK(ATB_1_BASE_ADDR);

	/* Set priority 1 for ports 0 and 1 */
	SET_REG(ATB_1_PRIO, 0x00000009UL);

	/* Enable port 0 and 1, and set hold time to 4 transactions */
	SET_REG(ATB_1_CTL, 0x00000303UL);

	CS_LOCK(ATB_1_BASE_ADDR);

	/* ATB funnel 2 */
	CS_UNLOCK(ATB_2_BASE_ADDR);

	/* Set priority 3 for port 3 */
	SET_REG(ATB_2_PRIO, 0x00003000UL);

	/* Enable ETM traces on port 3, and set hold time to 4 transactions */
	SET_REG(ATB_2_CTL,  0x00000308UL);

	CS_LOCK(ATB_2_BASE_ADDR);
}

static void etb_init(void)
{
	CS_UNLOCK(ETB_BASE_ADDR);

	/* Disable ETB */
	SET_REG(ETB_CTL, 0);

	/* Wait for ETB formatter to stop */
	while (!ETB_FORMATTER_STOPPED) {
	}

	/* Set ETB formatter to continuous mode */
	SET_REG(ETB_FFCR, ETB_FFCR_ENFCONT | ETB_FFCR_ENFTC);

	/* Enable ETB */
	SET_REG(ETB_CTL, ETB_CTL_TRACECAPTEN);

	/* Wait until the ETB Formatter has started */
	while (ETB_FORMATTER_STOPPED) {
	}

	CS_LOCK(ETB_BASE_ADDR);
}

static void etb_stop(void)
{
	CS_UNLOCK(ETB_BASE_ADDR);

	/* Disable ETB */
	SET_REG(ETB_CTL, 0);

	/* Wait for ETB formatter to flush */
	while (ETB_FLUSH_IN_PROGRESS) {
	}

	CS_LOCK(ETB_BASE_ADDR);
}

static void trace_debug_clock_start(void)
{
	NRF_TAD_S->TASKS_CLOCKSTART = TAD_TASKS_CLOCKSTART_TASKS_CLOCKSTART_Msk;
}

void etb_trace_start(void)
{
	trace_debug_clock_start();
	atb_init();
	etb_init();
	etm_init();
}

void etb_trace_stop(void)
{
	etm_stop();
	etb_stop();
	/* atb doesn't need to be stopped */
}

size_t etb_data_get(uint32_t *buf, size_t buf_size)
{
	size_t i = 0;

	if (buf_size == 0 || buf == NULL) {
		return -EINVAL;
	}

	CS_UNLOCK(ETB_BASE_ADDR);

	/* Set read pointer to the last write pointer */
	SET_REG(ETB_RRP, GET_REG(ETB_RWP));

	for (; i < buf_size; i++) {
		buf[i] = GET_REG(ETB_RRD);
	}

	CS_LOCK(ETB_BASE_ADDR);

	return i + 1;
}

#if defined(CONFIG_ETB_TRACE_SYS_INIT)
static int init(void)
{
	etb_trace_start();

	return 0;
}

SYS_INIT(init, EARLY, 0);
#endif /* defined (CONFIG_ETB_TRACE_SYS_INIT) */
