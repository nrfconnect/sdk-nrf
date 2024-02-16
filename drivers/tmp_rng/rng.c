/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */
#include <psa/crypto.h>
#include <stdint.h>
#include <stdbool.h>
#include <nrfx.h>
#include <hal/nrf_cracen.h>

#include "tmp_rng.h"

#define RNG_CLKDIV		  (0)
#define RNG_OFF_TIMER_VAL	  (0)
#define RNG_INIT_WAIT_VAL	  (512)
#define RNG_NB_128BIT_BLOCKS	  (4)
#define RNG_FIFOLEVEL_GRANULARITY (4)
#define RNG_NO_OF_COND_KEYS	  (4)

#ifndef SX_CM_REGS_ADDR
#define SX_CM_REGS_ADDR ((uint32_t)NRF_CRACENCORE)
#endif
#ifndef SX_TRNG_REGS_OFFSET
#define SX_TRNG_REGS_OFFSET 0x1000
#endif
#ifndef SX_TRNG_REGS_ADDR
#define SX_TRNG_REGS_ADDR ((SX_CM_REGS_ADDR) + (SX_TRNG_REGS_OFFSET))
#endif

/** Control: Control register */
#define BA431_REG_Control_OFST			(0x00)
/** FIFOLevel: FIFO level register. */
#define BA431_REG_FIFOLevel_OFST		(0x4u)
/** FIFOThreshold: FIFO threshold register. */
#define BA431_REG_FIFOThreshold_OFST		(0x8u)
/** FIFODepth: FIFO depth register. */
#define BA431_REG_FIFODepth_OFST		(0xCu)
/** Key0: Key register (MSBu). */
#define BA431_REG_Key0_OFST			(0x10u)
/** Key1: Key register. */
#define BA431_REG_Key1_OFST			(0x14u)
/** Key2: Key register. */
#define BA431_REG_Key2_OFST			(0x18u)
/** Key3: Key register (LSBu). */
#define BA431_REG_Key3_OFST			(0x1Cu)
/** Status: Status register. */
#define BA431_REG_Status_OFST			(0x30u)
/** InitWaitVal: Initial wait counter value. */
#define BA431_REG_InitWaitVal_OFST		(0x34u)
/** SwOffTmrVal: Switch off timer value. */
#define BA431_REG_SwOffTmrVal_OFST		(0x40u)
/** ClkDiv: Sample clock divider. */
#define BA431_REG_ClkDiv_OFST			(0x44u)
/** FIFO Data */
#define BA431_REG_FIFODATA_OFST			(0x80u)
/** Enable: Enable the NDRNG. */
#define BA431_FLD_Control_Enable_MASK		(0x1u)
/** SoftRst: Software reset. */
#define BA431_FLD_Control_SoftRst_MASK		(0x100u)
/** Nb128BitBlocks: Number of 128 bit blocks used in AES-CBCMAC
 * post-processing. This value cannot be zero.
 */
#define BA431_FLD_Control_Nb128BitBlocks_LSB	(16u)
#define BA431_FLD_Control_Nb128BitBlocks_MASK	(0xF0000u)
/** State: State of the control FSM. */
#define BA431_FLD_Status_State_LSB		(1u)
#define BA431_FLD_Status_State_MASK		(0xEu)
/** ForceActiveROs: Force oscillators to run when FIFO is full. */
#define BA431_FLD_Control_ForceActiveROs_MASK	(0x800u)
/** HealthTestBypass: Bypass NIST tests such that the results of the start-up
 * and online test do not affect the FSM state.
 */
#define BA431_FLD_Control_HealthTestBypass_MASK (0x1000u)
/** AIS31Bypass: Bypass AIS31 tests such that the results of the start-up
 * and online tests do not affect the FSM state.
 */
#define BA431_FLD_Control_AIS31Bypass_MASK	(0x2000u)

/** BA431 operating states */
#define BA431_STATE_RESET	   (0x00)
#define BA431_STATE_STARTUP	   (0x01u)
#define BA431_STATE_IDLE_RINGS_ON  (0x02u)
#define BA431_STATE_IDLE_RINGS_OFF (0x03u)
#define BA431_STATE_FILL_FIFO	   (0x04u)
#define BA431_STATE_ERROR	   (0x05u)

#define SX_OK			 0
#define SX_ERR_RESET_NEEDED	 (-82)
#define SX_ERR_HW_PROCESSING	 (-1)
#define SX_ERR_TOO_BIG		 (-64)
#define SX_ERR_UNINITIALIZED_OBJ (-33)

struct sx_trng {
	bool initialized;
	int conditioning_key_set;
};

struct sx_trng_config {
	unsigned int wakeup_level;
	unsigned int init_wait;
	unsigned int off_time_delay;
	unsigned int sample_clock_div;
	uint32_t control_bitmask;
};

/** Write value 'val' at address 'addr' of the trng */
static inline void sx_wr_trng(uint32_t addr, uint32_t val)
{
	volatile uint32_t *p = (uint32_t *)(SX_TRNG_REGS_ADDR + addr);

	__COMPILER_BARRIER();
	*p = val;
	__COMPILER_BARRIER();
}

/** Read register at address 'addr' from the trng */
static inline uint32_t sx_rd_trng(uint32_t addr)
{
	volatile uint32_t *p = (uint32_t *)(SX_TRNG_REGS_ADDR + addr);
	uint32_t v;

	__COMPILER_BARRIER();
	v = *p;
	__COMPILER_BARRIER();

	return v;
}

static int ba431_check_state(void)
{
	uint32_t state = sx_rd_trng(BA431_REG_Status_OFST);

	state = (state & BA431_FLD_Status_State_MASK) >> BA431_FLD_Status_State_LSB;

	if (state == BA431_STATE_ERROR) {
		return SX_ERR_RESET_NEEDED;
	} else if ((state == BA431_STATE_RESET) || (state == BA431_STATE_STARTUP)) {
		return SX_ERR_HW_PROCESSING;
	}

	return SX_OK;
}

static void ba431_softreset(void)
{
	uint32_t ctrl_reg = sx_rd_trng(BA431_REG_Control_OFST);

	sx_wr_trng(BA431_REG_Control_OFST, ctrl_reg | BA431_FLD_Control_SoftRst_MASK);
	sx_wr_trng(BA431_REG_Control_OFST, ctrl_reg & ~BA431_FLD_Control_SoftRst_MASK);
}

static int sx_trng_open(struct sx_trng *ctx, const struct sx_trng_config *config)
{
	int err;
	*ctx = (struct sx_trng){0};

	nrf_cracen_module_enable(NRF_CRACEN, CRACEN_ENABLE_RNG_Msk);

	ctx->initialized = true;

	/* Clear the control reg first */
	sx_wr_trng(BA431_REG_Control_OFST, 0x00);

	ba431_softreset();

	uint32_t fifo_wakeup_level;
	uint32_t rng_off_timer_val = RNG_OFF_TIMER_VAL;
	uint32_t rng_clkdiv = RNG_CLKDIV;
	uint32_t rng_init_wait_val = RNG_INIT_WAIT_VAL;
	uint32_t ctrlbitmask = 0;

	fifo_wakeup_level = sx_rd_trng(BA431_REG_FIFODepth_OFST) / RNG_FIFOLEVEL_GRANULARITY - 1;
	if (config) {
		if (config->wakeup_level) {
			if (config->wakeup_level > fifo_wakeup_level) {
				err = SX_ERR_TOO_BIG;
				goto error;
			}
			fifo_wakeup_level = config->wakeup_level;
		}
		if (config->off_time_delay) {
			rng_off_timer_val = config->off_time_delay;
		}
		if (config->init_wait) {
			rng_init_wait_val = config->init_wait;
		}
		if (config->sample_clock_div) {
			rng_clkdiv = config->sample_clock_div;
		}
		ctrlbitmask = config->control_bitmask;
	}

	/* Program powerdown and clock settings */
	sx_wr_trng(BA431_REG_FIFOThreshold_OFST, fifo_wakeup_level);
	sx_wr_trng(BA431_REG_SwOffTmrVal_OFST, rng_off_timer_val);
	sx_wr_trng(BA431_REG_ClkDiv_OFST, rng_clkdiv);
	sx_wr_trng(BA431_REG_InitWaitVal_OFST, rng_init_wait_val);

	/* Configure the control register */
	uint32_t control = sx_rd_trng(BA431_REG_Control_OFST);

	control &= ~(BA431_FLD_Control_Nb128BitBlocks_MASK | BA431_FLD_Control_ForceActiveROs_MASK |
		     BA431_FLD_Control_HealthTestBypass_MASK | BA431_FLD_Control_AIS31Bypass_MASK);
	control |= (RNG_NB_128BIT_BLOCKS << BA431_FLD_Control_Nb128BitBlocks_LSB) | ctrlbitmask;
	sx_wr_trng(BA431_REG_Control_OFST, control);

	/* Enable NDRNG */
	sx_wr_trng(BA431_REG_Control_OFST, (control & ~BA431_FLD_Control_Enable_MASK) | 0x01);

	return SX_OK;

error:
	nrf_cracen_module_disable(NRF_CRACEN, CRACEN_ENABLE_RNG_Msk);
	ctx->initialized = false;
	return err;
}

static int ba431_setup_conditioning_key(struct sx_trng *ctx)
{
	uint32_t key;
	uint32_t level = sx_rd_trng(BA431_REG_FIFOLevel_OFST);

	/* FIFO level must be 4 (4 * 32bit Word) */
	if (level < RNG_NO_OF_COND_KEYS) {
		return SX_ERR_HW_PROCESSING;
	}

	for (size_t i = 0; i < RNG_NO_OF_COND_KEYS; i++) {
		key = sx_rd_trng(BA431_REG_FIFODATA_OFST);
		sx_wr_trng(BA431_REG_Key0_OFST + i * sizeof(key), key);
	}

	ctx->conditioning_key_set = 1;

	return SX_OK;
}

static int sx_trng_get(struct sx_trng *ctx, char *dst, size_t size)
{
	int status = SX_OK;

	if (!ctx->initialized) {
		return SX_ERR_UNINITIALIZED_OBJ;
	}

	status = ba431_check_state();
	if (status) {
		return status;
	}

	/* Program random key for the conditioning function */
	if (!ctx->conditioning_key_set) {
		status = ba431_setup_conditioning_key(ctx);
		if (status != SX_OK) {
			return status;
		}
	}

	/* Block sizes above the FIFO wakeup level to guarantee that the
	 * hardware will be able at some time to provide the requested bytes.
	 */
	uint32_t wakeup_level = sx_rd_trng(BA431_REG_FIFOThreshold_OFST);

	if (size > (wakeup_level + 1) * 16) {
		return SX_ERR_TOO_BIG;
	}

	uint32_t level = sx_rd_trng(BA431_REG_FIFOLevel_OFST);
	/* FIFO level in 4-byte words */
	if (size > level * RNG_FIFOLEVEL_GRANULARITY) {
		return SX_ERR_HW_PROCESSING;
	}

	while (size) {
		uint32_t data;

		data = sx_rd_trng(BA431_REG_FIFODATA_OFST);
		for (size_t i = 0; (i < sizeof(data)) && (size); i++, size--) {
			*dst = (char)(data & 0xFF);
			dst++;
			data >>= 8;
		}
	}

	return status;
}
static int sx_trng_close(struct sx_trng *ctx)
{
	if (ctx->initialized) {
		nrf_cracen_module_disable(NRF_CRACEN, CRACEN_ENABLE_RNG_Msk);
		ctx->initialized = false;
	}
	return SX_OK;
}

psa_status_t tmp_rng(uint8_t *out, size_t outsize)
{
	int sx_err = SX_ERR_RESET_NEEDED;
	struct sx_trng trng;

	while (true) {
		switch (sx_err) {
		case SX_ERR_RESET_NEEDED:
			sx_err = sx_trng_open(&trng, NULL);
			if (sx_err != SX_OK) {
				return PSA_ERROR_HARDWARE_FAILURE;
			}
		/* fallthrough */
		case SX_ERR_HW_PROCESSING:
			/* Generate random numbers */
			while (outsize) {
				size_t req = MIN(outsize, 32);

				sx_err = sx_trng_get(&trng, out, req);
				if (sx_err) {
					break;
				}
				out += req;
				outsize -= req;
			}
			if (sx_err == SX_ERR_RESET_NEEDED) {
				(void)sx_trng_close(&trng);
			}
			break;
		default:
			(void)sx_trng_close(&trng);
			return sx_err == SX_OK ? PSA_SUCCESS : PSA_ERROR_HARDWARE_FAILURE;
		}
	}
}
