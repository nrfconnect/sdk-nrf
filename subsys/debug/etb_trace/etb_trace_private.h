/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/* Additional register definitions for configuring debug and trace components for ETB */
/* See Arm® CoreSight™ ETM-M33 Technical Reference Manual for details on the ETM unit */

#include <zephyr/types.h>

/* Lock Address Register (LAR) offset */
#define LAR_OFFSET			0xFB0
/* CoreSight software lock key */
#define CS_UNLOCK_KEY			0xC5ACCE55
/* CoreSight LAR reset value */
#define CS_LOCK_KEY			0x00000000

/* Embedded Trace Buffer registers */
#define ETB_BASE_ADDR			0xE0051000
#define ETB_RDP				(ETB_BASE_ADDR + 0x004)
#define ETB_STS				(ETB_BASE_ADDR + 0x00C)
#define ETB_RRD				(ETB_BASE_ADDR + 0x010)
#define ETB_RRP				(ETB_BASE_ADDR + 0x014)
#define ETB_RWP				(ETB_BASE_ADDR + 0x018)
#define ETB_TRG				(ETB_BASE_ADDR + 0x01C)
#define ETB_CTL				(ETB_BASE_ADDR + 0x020)
#define ETB_RWD				(ETB_BASE_ADDR + 0x024)
#define ETB_FFSR			(ETB_BASE_ADDR + 0x300)
#define ETB_FFCR			(ETB_BASE_ADDR + 0x304)
#define ETB_LAR				(ETB_BASE_ADDR + ETB_OFFSET)

#define ETB_CTL_TRACECAPTEN		BIT(0)

#define ETB_FFSR_FLINPROG		BIT(0)
#define ETB_FFSR_FTSTOPPED		BIT(1)

#define ETB_FFCR_ENFTC			BIT(0)
#define ETB_FFCR_ENFCONT		BIT(1)

/* Advanced Trace Bus 1 (ATB) registers */
#define ATB_1_BASE_ADDR			0xE005A000
#define ATB_1_CTL			(ATB_1_BASE_ADDR + 0x000)
#define ATB_1_PRIO			(ATB_1_BASE_ADDR + 0x004)

/* Advanced Trace Bus 2 (ATB) registers */
#define ATB_2_BASE_ADDR			0xE005B000
#define ATB_2_CTL			(ATB_2_BASE_ADDR + 0x000)
#define ATB_2_PRIO			(ATB_2_BASE_ADDR + 0x004)

#define ATB_REPLICATOR_BASE_ADDR	0xE0058000
#define ATB_REPLICATOR_IDFILTER0	(ATB_REPLICATOR_BASE_ADDR + 0x000)
#define ATB_REPLICATOR_IDFILTER1	(ATB_REPLICATOR_BASE_ADDR + 0x004)

/* Embedded Trace Macrocell registers */
#define ETM_BASE_ADDR			0xE0041000
#define ETM_TRCPRGCTLR			(ETM_BASE_ADDR + 0x004) /* Programming Control Register */
#define ETM_TRCSTATR			(ETM_BASE_ADDR + 0x00C) /* Status Register */
#define ETM_TRCCONFIGR			(ETM_BASE_ADDR + 0x010) /* Trace Configuration Register */
#define ETM_TRCCCCTLR			(ETM_BASE_ADDR + 0x038) /* Cycle Count Control Register */
#define ETM_TRCSTALLCTLR		(ETM_BASE_ADDR + 0x02C) /* Stall Control Register */
#define ETM_TRCTSCTLR			(ETM_BASE_ADDR + 0x030) /* Timestamp Control Register */
#define ETM_TRCTRACEIDR			(ETM_BASE_ADDR + 0x040) /* Trace ID Register */
#define ETM_TRCVICTLR			(ETM_BASE_ADDR + 0x080) /* ViewInst Main Control Register */
#define ETM_TRCEVENTCTL0R		(ETM_BASE_ADDR + 0x020) /* Event Control 0 Register */
#define ETM_TRCEVENTCTL1R		(ETM_BASE_ADDR + 0x024) /* Event Control 1 Register */
#define ETM_TRCPDSR			(ETM_BASE_ADDR + 0x314) /* Power down status register */

#define ETM_TRCSTATR_IDLE		BIT(0)
#define ETM_TRCSTATR_PMSTABLE		BIT(1)

#define ETM_TRCVICTLR_TRCERR		BIT(11)
#define ETM_TRCVICTLR_TRCRESET		BIT(10)
#define ETM_TRCVICTLR_SSSTATUS		BIT(9)
#define ETM_TRCVICTLR_EVENT0		BIT(0)

#define ETM_TRCPRGCTLR_ENABLE		BIT(0)

/* Instrumentation Trace Macrocell registers */
#define ITM_BASE_ADDR			0xE0000000
#define ITM_TER				(ITM_BASE_ADDR + 0xE00) /* Trace Enable Register */
#define ITM_TCR				(ITM_BASE_ADDR + 0xE80) /* Trace Control Register */

/* Data Watchpoint and Trace Unit registers */
#define DWT_BASE_ADDR			0xE0001000
#define DWT_CCYCCNT			(DWT_BASE_ADDR + 0x004) /* Cycle Count Register */

#define TIMESTAMP_GENERATOR_BASE_ADDR	0xE0053000
#define TIMESTAMP_GENERATOR_CNCTR	(TIMESTAMP_GENERATOR_BASE_ADDR + 0x000)

/* convenience macros */
#define SET_REG(reg, value)	(*((volatile uint32_t *)(reg)) = value)
#define GET_REG(reg)		(*((volatile uint32_t *)(reg)))
#define CS_UNLOCK(reg_base)	(*((volatile uint32_t *)(reg_base + LAR_OFFSET)) = CS_UNLOCK_KEY)
#define CS_LOCK(reg_base)	(*((volatile uint32_t *)(reg_base + LAR_OFFSET)) = CS_LOCK_KEY)
#define ETB_FORMATTER_STOPPED	((GET_REG(ETB_FFSR) & ETB_FFSR_FTSTOPPED))
#define ETB_FLUSH_IN_PROGRESS	((GET_REG(ETB_FFSR) & ETB_FFSR_FLINPROG))

void etb_trace_on_idle_exit(void);
