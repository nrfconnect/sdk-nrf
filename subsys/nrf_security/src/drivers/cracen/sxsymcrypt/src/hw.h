/*
 *  Copyright (c) 2023 Nordic Semiconductor ASA
 *
 *  SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef HW_HEADER_FILE
#define HW_HEADER_FILE

#include <cracen/membarriers.h>
#include <nrfx.h>
#include <stdint.h>
#include <stddef.h>
#include <sxsymcrypt/internal.h>
#include <cracen/interrupts.h>

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
#include <zephyr/sys/printk.h>
#endif

#ifdef CONFIG_DCACHE
#include <zephyr/cache.h>
#endif

#ifndef SX_EXTRA_IN_DESCS
#define SX_EXTRA_IN_DESCS 0
#endif

#ifndef SX_CM_REGS_ADDR
#define SX_CM_REGS_ADDR ((uint32_t)NRF_CRACENCORE)
#endif
#ifndef SX_TRNG_REGS_ADDR
#define SX_TRNG_REGS_ADDR ((uint32_t)&NRF_CRACENCORE->RNGCONTROL)
#endif

struct sxdesc;

/** Write value 'val' at address 'addr' of the registers 'regs' */
static inline void sx_wrreg(uint32_t addr, uint32_t val)
{
	volatile uint32_t *p = (uint32_t *)(SX_CM_REGS_ADDR + addr);

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_wrreg(addr=0x%x, p=%p, val=0x%x)\r\n", addr, p, val);
#endif

	wmb(); /* comment for compliance */
	*p = val;
	rmb(); /* comment for compliance */
}

/** Write value 'val' at address 'addr' of the trng */
static inline void sx_wr_trng(uint32_t addr, uint32_t val)
{
	volatile uint32_t *p = (uint32_t *)(SX_TRNG_REGS_ADDR + addr);

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_wr_trng(addr=0x%x, p=%p, val=0x%x)\r\n", addr, p, val);
#endif

	wmb(); /* comment for compliance */
	*p = val;
	rmb(); /* comment for compliance */
}

/** Write a pointer 'p' into register at 'regaddr' from 'regs' */
static inline void sx_wrreg_addr(uint32_t addr, struct sxdesc *p)
{
	volatile size_t *d = (volatile size_t *)(SX_CM_REGS_ADDR + addr);

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_wrreg_addr(addr=0x%x, d=%p, p=%p)\r\n", addr, d, p);
#endif
	wmb(); /* comment for compliance */
	*d = (size_t)p;
	rmb(); /* comment for compliance */
}

/** Read register at address 'addr' from registers 'regs' */
static inline uint32_t sx_rdreg(uint32_t addr)
{
	volatile uint32_t *p = (uint32_t *)(SX_CM_REGS_ADDR + addr);
	uint32_t v;

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_rdreg(addr=0x%x, p=%p)\r\n", addr, p);
#endif
	wmb(); /* comment for compliance */
	v = *p;
	rmb(); /* comment for compliance */

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("result = 0x%x\r\n", v);
#endif

	return v;
}

/** Read register at address 'addr' from the trng */
static inline uint32_t sx_rd_trng(uint32_t addr)
{
	volatile uint32_t *p = (uint32_t *)(SX_TRNG_REGS_ADDR + addr);
	uint32_t v;

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_rd_trng(addr=0x%x, p=%p)\r\n", addr, p);
#endif
	wmb(); /* comment for compliance */
	v = *p;
	rmb(); /* comment for compliance */

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("result = 0x%x\r\n", v);
#endif
	return v;
}

/** Reserve the cryptomaster instance
 *  and should be given back after use with sx_cmdma_release_hw().
 */
void sx_hw_reserve(struct sx_dmactl *dma);

/** Release the cryptomaster instance */
void sx_cmdma_release_hw(struct sx_dmactl *dma);

#endif
