/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <drivers/entropy.h>
#include <soc.h>
#include <device.h>
#include <irq.h>
#include <kernel.h>
#include <kernel_includes.h>
#include <sdc.h>
#include <sdc_soc.h>

#include <multithreading_lock.h>

#define DT_DRV_COMPAT	nordic_nrf_rng

/* The RNG driver data. */
struct rng_driver_data {
	/* Used to fill the pending client buffer with new random values after
	 * RNG IRQ is triggered.
	 */
	struct k_sem sem_sync;
};

static struct rng_driver_data rng_data;

static inline struct rng_driver_data *rng_driver_data_get(const struct device *dev)
{
	__ASSERT_NO_MSG(dev != NULL);
	__ASSERT_NO_MSG((intptr_t) dev->data == (intptr_t) &rng_data);

	return dev->data;
}

static int rng_driver_get_entropy(const struct device *dev, uint8_t *buf, uint16_t len)
{
	struct rng_driver_data *rng_dev = rng_driver_data_get(dev);
	uint8_t *p_dst = buf;
	uint32_t bytes_left = len;

	while (bytes_left > 0) {
		uint32_t bytes_read = 0;

		while (bytes_read == 0) {
			int errcode = MULTITHREADING_LOCK_ACQUIRE();

			if (errcode) {
				return errcode;
			}
			bytes_read = sdc_soc_rand_vector_poll(p_dst, bytes_left);
			MULTITHREADING_LOCK_RELEASE();

			if (!bytes_read) {
				/* Put the thread on wait until next interrupt
				 * to get more random values.
				 */
				k_sem_take(&rng_dev->sem_sync, K_FOREVER);
			}
		}

		p_dst += bytes_read;
		bytes_left -= bytes_read;
	}

	return 0;
}

static int rng_driver_get_entropy_isr(const struct device *dev, uint8_t *buf, uint16_t len,
				      uint32_t flags)
{

	int errcode = 0;

	if (likely((flags & ENTROPY_BUSYWAIT) == 0)) {
		errcode = MULTITHREADING_LOCK_ACQUIRE_NO_WAIT();
		if (!errcode) {
			errcode = sdc_soc_rand_vector_poll(buf, len);
			MULTITHREADING_LOCK_RELEASE();
		}
	} else {
		errcode = MULTITHREADING_LOCK_ACQUIRE_FOREVER_WAIT();
		if (!errcode) {
			sdc_soc_rand_vector_get(buf, len);
			MULTITHREADING_LOCK_RELEASE();
		}
	}

	if (!errcode) {
		return len;
	} else {
		return errcode;
	}
}

static void rng_driver_isr(void *param)
{
	ARG_UNUSED(param);

	sdc_RNG_IRQHandler();

	/* This sema wakes up the pending client buffer to fill it with new
	 * random values.
	 */
	k_sem_give(&rng_data.sem_sync);
}

static int rng_driver_init(const struct device *dev)
{
	struct rng_driver_data *rng_dev = rng_driver_data_get(dev);

	k_sem_init(&rng_dev->sem_sync, 0, 1);

	IRQ_CONNECT(RNG_IRQn,
		    CONFIG_ENTROPY_NRF_PRI,
		    rng_driver_isr,
		    NULL,
		    0);

	return 0;
}

static const struct entropy_driver_api rng_driver_api_funcs = {
	.get_entropy = rng_driver_get_entropy,
	.get_entropy_isr = rng_driver_get_entropy_isr
};

DEVICE_DT_INST_DEFINE(0, rng_driver_init, device_pm_control_nop,
		      &rng_data, NULL,
		      PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
		      &rng_driver_api_funcs);
