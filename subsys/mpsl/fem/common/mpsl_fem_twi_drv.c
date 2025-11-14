/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_fem_twi_drv.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/i2c_nrfx_twim.h>
#include <../drivers/i2c/i2c_nrfx_twim_common.h>

static int32_t mpsl_fem_twi_drv_impl_xfer_read(void *p_instance, uint8_t slave_address,
					       uint8_t internal_address, uint8_t *p_data,
					       uint8_t data_length)
{
	mpsl_fem_twi_drv_t *drv = (mpsl_fem_twi_drv_t *)p_instance;

	return i2c_burst_read(drv->dev, slave_address, internal_address, p_data, data_length);
}

static int32_t mpsl_fem_twi_drv_impl_xfer_write(void *p_instance, uint8_t slave_address,
						uint8_t internal_address, const uint8_t *p_data,
						uint8_t data_length)
{
	mpsl_fem_twi_drv_t *drv = (mpsl_fem_twi_drv_t *)p_instance;

	return i2c_burst_write(drv->dev, slave_address, internal_address, p_data, data_length);
}

static inline void mpsl_fem_twi_drv_nrfx_twim_callback_replace(mpsl_fem_twi_drv_t *drv,
							       nrfx_twim_event_handler_t callback)
{
	const struct i2c_nrfx_twim_common_config *config = drv->dev->config;
	int err;

	nrfx_twim_callback_get(config->twim, &drv->nrfx_twim_callback_saved,
		&drv->nrfx_twim_callback_ctx_saved);

	err = nrfx_twim_callback_set(config->twim, callback, drv);

	__ASSERT_NO_MSG(err >= 0);
	(void)err;
}

static inline void mpsl_fem_twi_drv_nrfx_twim_callback_restore(mpsl_fem_twi_drv_t *drv)
{
	const struct i2c_nrfx_twim_common_config *config = drv->dev->config;
	int err;

	err = nrfx_twim_callback_set(config->twim, drv->nrfx_twim_callback_saved,
		drv->nrfx_twim_callback_ctx_saved);

	__ASSERT_NO_MSG(err >= 0);
	(void)err;
}

static void mpsl_fem_twi_drv_nrfx_twim_evt_handler(nrfx_twim_event_t const *p_event, void *p_context)
{
	mpsl_fem_twi_drv_t *drv = (mpsl_fem_twi_drv_t *)p_context;
	int32_t res = 0;

	mpsl_fem_twi_drv_nrfx_twim_callback_restore(drv);

	if (p_event->type != NRFX_TWIM_EVT_DONE) {
		res = -EIO;
	}

	/* Call the callback which was passed to the mpsl_fem_twi_drv_impl_xfer_write_async call,
	 * that started the transfer.
	 */
	drv->fem_twi_async_xfwr_write_cb(drv, res, drv->fem_twi_async_xfwr_write_cb_ctx);
}

static int32_t mpsl_fem_twi_drv_impl_xfer_write_async(void *p_instance, uint8_t slave_address,
						      const uint8_t *p_data, uint8_t data_length,
						      mpsl_fem_twi_async_xfer_write_cb_t p_callback,
						      void *p_context)
{
	mpsl_fem_twi_drv_t *drv = (mpsl_fem_twi_drv_t *)p_instance;
	const struct i2c_nrfx_twim_common_config *config = drv->dev->config;

	/* At this moment the exclusive access to the drv->dev should have been already acquired.
	 * No ongoing twi transfers are expected. Because of that it is safe to replace
	 * original event handler of the TWIM with custom one, perform twim transfer and then
	 * restore the original event handler.
	 */

	nrfx_twim_xfer_desc_t cur_xfer = {
		.address = slave_address,
		.type = NRFX_TWIM_XFER_TX,
		.p_primary_buf = (uint8_t *)p_data,
		.primary_length = data_length,
	};
	int err;

	drv->fem_twi_async_xfwr_write_cb = p_callback;
	drv->fem_twi_async_xfwr_write_cb_ctx = p_context;

	mpsl_fem_twi_drv_nrfx_twim_callback_replace(drv, mpsl_fem_twi_drv_nrfx_twim_evt_handler);

	err = nrfx_twim_xfer(config->twim, &cur_xfer, 0);

	if (err < 0) {
		mpsl_fem_twi_drv_nrfx_twim_callback_restore(drv);
	}

	return err;
}

static uint32_t mpsl_fem_twi_drv_frequency_hz_get(mpsl_fem_twi_drv_t *drv)
{
	const struct i2c_nrfx_twim_common_config *config = drv->dev->config;

	switch (config->twim_config.frequency) {
	case NRF_TWIM_FREQ_100K:
		return 100000;
	case NRF_TWIM_FREQ_250K:
		return 250000;
	case NRF_TWIM_FREQ_400K:
		return 400000;
#if NRF_TWIM_HAS_1000_KHZ_FREQ
	case NRF_TWIM_FREQ_1000K:
		return 1000000;
#endif
	default:
		__ASSERT_NO_MSG(false);
		return 100000;
	}
}

static uint32_t mpsl_fem_twi_drv_impl_xfer_write_async_time_get(void *p_instance,
								uint8_t data_length)
{
	mpsl_fem_twi_drv_t *drv = (mpsl_fem_twi_drv_t *)p_instance;

	static const uint32_t sw_overhead_safety_margin_time_us = 10U;
	/* Note: on nRF5 devices the first bit of each data octet is delayed by one period,
	 * thus +1 below.
	 */
	static const uint32_t i2c_data_byte_sck_periods_with_ack = 1U + 8U + 1U;
	static const uint32_t i2c_start_bit_sck_periods = 2U;
	static const uint32_t i2c_stop_bit_sck_periods = 2U;
	uint32_t i2c_sck_frequency_hz = mpsl_fem_twi_drv_frequency_hz_get(drv);

	/* Total number of sck periods needed to perform a write transfer. */
	uint32_t total_periods = i2c_start_bit_sck_periods
				+ (data_length + 1U) * i2c_data_byte_sck_periods_with_ack
				+ i2c_stop_bit_sck_periods;

	return (total_periods * 1000000 / i2c_sck_frequency_hz) + sw_overhead_safety_margin_time_us;
}

void mpsl_fem_twi_drv_fem_twi_if_prepare(mpsl_fem_twi_drv_t *drv, mpsl_fem_twi_if_t *twi_if,
					uint8_t address)
{
	twi_if->enabled = true;
	twi_if->slave_address = address;
	twi_if->p_instance = (void *)drv;
	twi_if->p_xfer_read = mpsl_fem_twi_drv_impl_xfer_read;
	twi_if->p_xfer_write = mpsl_fem_twi_drv_impl_xfer_write;
}

void mpsl_fem_twi_drv_fem_twi_if_prepare_add_async(mpsl_fem_twi_if_t *twi_if)
{
	twi_if->p_xfer_write_async = mpsl_fem_twi_drv_impl_xfer_write_async;
	twi_if->p_xfer_write_async_time_get = mpsl_fem_twi_drv_impl_xfer_write_async_time_get;
}
