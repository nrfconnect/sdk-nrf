/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>

#include <stdint.h>

#include "../hw/ba414/ba414_status.h"
#include "../hw/ba414/pkhardware_ba414e.h"
#include "../hw/ba414/regs_addr.h"
#include "../hw/ik/ikhardware.h"
#include "../hw/ik/regs_addr.h"
#include <silexpk/core.h>
#include <silexpk/iomem.h>
#include <cracen/interrupts.h>
#include <cracen/statuscodes.h>
#include "internal.h"

#include <hal/nrf_cracen.h>
#include <security/cracen.h>
#include <nrf_security_mutexes.h>

#if CONFIG_CRACEN_HW_VERSION_LITE && !CONFIG_SOC_NRF54LM20A && !CONFIG_SOC_NRF54LV10A
#error Check to see if the current board needs the IKG-PKE interrupt workaround or not, \
then update this error
#endif

#ifndef ADDR_BA414EP_REGS_BASE
#define ADDR_BA414EP_REGS_BASE CRACEN_ADDR_BA414EP_REGS_BASE
#endif
#ifndef ADDR_BA414EP_CRYPTORAM_BASE
#define ADDR_BA414EP_CRYPTORAM_BASE CRACEN_ADDR_BA414EP_CRYPTORAM_BASE
#endif

#ifndef NULL
#define NULL (void *)0
#endif

#define ADDR_BA414EP_REGS(instance) ((char *)(ADDR_BA414EP_REGS_BASE) + 0x10000 * (instance))
#define ADDR_BA414EP_CRYPTORAM(instance)                                                           \
	((char *)ADDR_BA414EP_CRYPTORAM_BASE + 0x10000 * (instance))

#define PK_BUSY_MASK_BA414EP 0x00010000
#define PK_BUSY_MASK_IK	     0x00050000

#define IK_ENTROPY_ERROR     0xc6

struct sx_pk_cnx {
	struct sx_pk_req instance;
	struct sx_pk_capabilities caps;
	struct sx_pk_blinder *b;
};

static struct sx_pk_cnx silex_pk_engine;

NRF_SECURITY_MUTEX_DEFINE(cracen_mutex_asymmetric);

bool ba414ep_is_busy(sx_pk_req *req)
{
	return (bool)(sx_pk_rdreg(&req->regs, PK_REG_STATUS) & PK_BUSY_MASK_BA414EP);
}

bool ik_is_busy(sx_pk_req *req)
{
	return (bool)(sx_pk_rdreg(&req->regs, IK_REG_PK_STATUS) & PK_BUSY_MASK_IK);
}

bool is_busy(sx_pk_req *req)
{
#if defined(CONFIG_CRACEN_IKG)
	if (sx_pk_is_ik_cmd(req)) {
		return ik_is_busy(req);
	}
#endif

	return ba414ep_is_busy(req);
}

void sx_clear_interrupt(sx_pk_req *req)
{
	sx_pk_wrreg(&req->regs, PK_REG_CONTROL, PK_RB_CONTROL_CLEAR_IRQ);

#if defined(CONFIG_CRACEN_IKG)
	if (sx_pk_is_ik_cmd(req)) {
		sx_pk_wrreg(&req->regs, IK_REG_PK_CONTROL, IK_PK_CONTROL_CLEAR_IRQ);
	}
#endif
}

int read_status(sx_pk_req *req)
{
#if defined(CONFIG_CRACEN_IKG)
	if (sx_pk_is_ik_cmd(req)) {
		return sx_ik_read_status(req);
	}
#endif

	uint32_t status = sx_pk_rdreg(&req->regs, PK_REG_STATUS);

	return convert_ba414_status(status);
}

int sx_pk_wait(sx_pk_req *req)
{
	do {
		if (!IS_ENABLED(CONFIG_CRACEN_HW_VERSION_LITE) &&
		    IS_ENABLED(CONFIG_CRACEN_USE_INTERRUPTS)) {
			/* In CRACEN Lite the PKE-IKG interrupt is only active when in PK mode.
			 * This is to work around a hardware issue where the interrupt is never
			 * cleared. Therefore sx_pk_wait needs to use polling and not interrupts for
			 * CRACEN Lite.
			 */
			if (IS_ENABLED(CONFIG_CRACEN_IKG)) {
				if (!sx_pk_is_ik_cmd(req)) {
					cracen_wait_for_pke_interrupt();
				}
			}
		} else if (IS_ENABLED(CONFIG_CRACEN_HW_VERSION_LITE)) {
			/* In CRACEN Lite the IKG sometimes fails due to an entropy error.
			 * Error code is returned here so the entire operation can be rerun
			 */
			if (sx_pk_rdreg(&req->regs, IK_REG_STATUS) == IK_ENTROPY_ERROR) {
				return SX_ERR_RETRY;
			}
		}

	} while (is_busy(req));

	return read_status(req);
}

void sx_pk_wrreg(struct sx_regs *regs, uint32_t addr, uint32_t v)
{
	volatile uint32_t *p = (uint32_t *)(regs->base + addr);

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_pk_wrreg(addr=0x%x, p=%p, val=0x%x)\r\n", addr, p, v);
#endif
	if ((uintptr_t)p % 4) {
		SX_WARN_UNALIGNED_ADDR(p);
	}

	*p = v;
}

uint32_t sx_pk_rdreg(struct sx_regs *regs, uint32_t addr)
{
	volatile uint32_t *p = (uint32_t *)(regs->base + addr);
	uint32_t v;


#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("sx_pk_rdreg(addr=0x%x, p=%p)\r\n", addr, p);
#endif
	if ((uintptr_t)p % 4) {
		SX_WARN_UNALIGNED_ADDR(p);
	}

	v = *p;

#ifdef SX_INSTRUMENT_MMIO_WITH_PRINTFS
	printk("result = 0x%x\r\n", v);
#endif

	return v;
}

struct sx_pk_blinder **sx_pk_get_blinder(struct sx_pk_cnx *cnx)
{
	return &cnx->b;
}

const struct sx_pk_capabilities *sx_pk_fetch_capabilities(void)
{
	uint32_t ba414epfeatures = 0;

	if (silex_pk_engine.caps.maxpending) {
		/* capabilities already fetched from hardware. Return
		 * immediately.
		 */
		return &silex_pk_engine.caps;
	}

	ba414epfeatures = sx_pk_rdreg(&silex_pk_engine.instance.regs, PK_REG_HWCONFIG);
	convert_ba414_capabilities(ba414epfeatures, &silex_pk_engine.caps);
	silex_pk_engine.caps.maxpending = 1;

	return &silex_pk_engine.caps;
}

int sx_pk_init(void)
{
	struct sx_pk_cnx *cnx;

	cnx = &silex_pk_engine;
	cnx->b = NULL;

	cnx->instance.regs.base = ADDR_BA414EP_REGS(0);
	cnx->instance.cryptoram = ADDR_BA414EP_CRYPTORAM(0);

	if (!sx_pk_fetch_capabilities()) {
		return SX_ERR_PLATFORM_ERROR;
	}

	/** If cryptoRAM is sized for 4096b slots or less, use 4096b slots */
	int op_offset = 0x200;

	if (cnx->caps.max_gfp_opsz > 0x200) {
		/** If cryptoRAM is sized for 8192b slots use 8192b slots */
		op_offset = 0x400;
	}

	cnx->instance.slot_sz = op_offset;

	return SX_OK;
}

struct sx_pk_acq_req sx_pk_acquire_req(const struct sx_pk_cmd_def *cmd)
{
	struct sx_pk_acq_req req = {NULL, SX_OK};

	nrf_security_mutex_lock(cracen_mutex_asymmetric);
	req.req = &silex_pk_engine.instance;
	req.req->cmd = cmd;
	req.req->cnx = &silex_pk_engine;

	cracen_acquire();
	if (!IS_ENABLED(CONFIG_CRACEN_HW_VERSION_LITE) &&
	    IS_ENABLED(CONFIG_CRACEN_USE_INTERRUPTS)) {
		/* In CRACEN Lite the PKE-IKG interrupt is only active when in PK mode.
		 * This is to work around a hardware issue where the interrupt is never cleared.
		 * Therefore it is not enabled here for Cracen Lite.
		 */
		nrf_cracen_int_enable(NRF_CRACEN, NRF_CRACEN_INT_PKE_IKG_MASK);
	}

	/* Wait until initialized. */
	while (ba414ep_is_busy(req.req) || ik_is_busy(req.req)) {
		if (!IS_ENABLED(CONFIG_CRACEN_HW_VERSION_LITE) &&
		    IS_ENABLED(CONFIG_CRACEN_USE_INTERRUPTS)) {

			cracen_wait_for_pke_interrupt();
		}
	}

	return req;
}

sx_pk_req *sx_get_current_req(void)
{
	return &silex_pk_engine.instance;
}

void sx_pk_set_user_context(sx_pk_req *req, void *context)
{
	req->userctxt = context;
}

void *sx_pk_get_user_context(sx_pk_req *req)
{
	return req->userctxt;
}

void sx_pk_release_req(sx_pk_req *req)
{
	nrf_cracen_int_disable(NRF_CRACEN, NRF_CRACEN_INT_PKE_IKG_MASK);
	cracen_release();
	req->cmd = NULL;
	req->userctxt = NULL;
	nrf_security_mutex_unlock(cracen_mutex_asymmetric);
}

struct sx_regs *sx_pk_get_regs(void)
{
	return &silex_pk_engine.instance.regs;
}

struct sx_pk_capabilities *sx_pk_get_caps(void)
{
	return &silex_pk_engine.caps;
}
