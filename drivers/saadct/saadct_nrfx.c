/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_nrf_saadct

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_saadc.h>
#include <drivers/saadct.h>
#ifdef CONFIG_HAS_NORDIC_DMM
#include <dmm.h>
#endif

LOG_MODULE_REGISTER(saadct, CONFIG_SAADCT_LOG_LEVEL);

/* Nordic SoCs currently expose a single SAADC instance. */
K_FIFO_DEFINE(saadct_fifo);

#define TIMER_PERIOD_US(sample_rate_hz) ((uint32_t)(USEC_PER_SEC / (sample_rate_hz)))

static const struct device *saadc_device;

struct saadct_meas_block {
	void *fifo_reserved;
	nrf_saadc_value_t data[];
};

typedef enum {
	SAADCT_STATE_IDLE,
	SAADCT_STATE_RUNNING,
	SAADCT_STATE_STOPPING,
	SAADCT_STATE_ERROR,
} saadct_state_t;

struct saadct_drv_data {
	nrfx_timer_t timer;
#if !NRF_SAADC_HAS_SHORTS
	nrfx_gppi_handle_t ppi_end_start;
#endif
	nrfx_gppi_handle_t ppi_sample;
	uint32_t ppi_sample_evt;
	uint32_t ppi_sample_tsk;
	bool gppi_ready;
	uint8_t num_of_channels;
	nrf_saadc_resolution_t resolution;
	uint32_t sample_rate_hz;
	struct saadct_meas_block *curr_block;
	struct saadct_meas_block *next_block;
	atomic_t pending_series;
	void (*user_handler)(void *context);
	void *user_context;
	struct k_mem_slab *mem_slab;
	volatile saadct_state_t state;
	saadct_mode_t mode;
	uint32_t num_of_meas;
	nrfx_saadc_channel_t const *channels_config;
#ifdef CONFIG_HAS_NORDIC_DMM
	void *mem_reg;
#endif
};

static void saadct_handler(nrfx_saadc_evt_t const *p_event);

#if defined(CONFIG_CLOCK_CONTROL_NRF) || defined(CONFIG_CLOCK_CONTROL_NRF_HFCLK) ||                \
	defined(CONFIG_CLOCK_CONTROL_NRF_XO)
static struct onoff_client clk_cli;
static bool clock_requested;
#if defined(CONFIG_CLOCK_CONTROL_NRF)
static struct onoff_manager *clk_mgr;
#else
static const struct device *clk_dev = DEVICE_DT_GET_ONE(COND_CODE_1(NRF_CLOCK_HAS_HFCLK,
								    (nordic_nrf_clock_hfclk),
								    (nordic_nrf_clock_xo)));
#endif

static void clock_started_callback(struct onoff_manager *mgr,
				   struct onoff_client *cli,
				   uint32_t state,
				   int res)
{
	struct saadct_drv_data *p_data = saadc_device->data;
	int ret;

	ARG_UNUSED(mgr);
	ARG_UNUSED(cli);
	ARG_UNUSED(state);

	if (res < 0) {
		LOG_ERR("HF clock start failed: %d", res);
		p_data->state = SAADCT_STATE_ERROR;
		return;
	}

	ret = nrfx_saadc_offset_calibrate(saadct_handler);
	if (ret < 0) {
		LOG_ERR("SAADC offset calibration failed: %d", ret);
		p_data->state = SAADCT_STATE_ERROR;
		return;
	}

	clock_requested = true;
}

static int hf_clock_request(void)
{
	sys_notify_init_callback(&clk_cli.notify, clock_started_callback);

#if defined(CONFIG_CLOCK_CONTROL_NRF)
	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);

	return onoff_request(clk_mgr, &clk_cli);
#else
	return nrf_clock_control_request(clk_dev, NULL, &clk_cli);
#endif
}

static int hf_clock_release(void)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF)
	return onoff_release(clk_mgr);
#else
	return nrf_clock_control_release(clk_dev, NULL);
#endif
}

static int start_sampling(void)
{
	return hf_clock_request();
}
#elif CONFIG_CLOCK_CONTROL_NRF54H_HFXO
static bool hfxo_requested;

static int start_sampling(void)
{
	const struct device *hfxo = DEVICE_DT_GET(DT_NODELABEL(hfxo));
	struct saadct_drv_data *p_data = saadc_device->data;
	int ret;

	ret = nrf_clock_control_request_sync(hfxo, NULL, K_MSEC(2000));
	if (ret < 0) {
		return ret;
	}

	hfxo_requested = true;

	ret = nrfx_saadc_offset_calibrate(saadct_handler);
	if (ret < 0) {
		LOG_ERR("SAADC offset calibration failed: %d", ret);
		p_data->state = SAADCT_STATE_ERROR;
	}

	return ret;
}
#else
static int start_sampling(void)
{
	struct saadct_drv_data *p_data = saadc_device->data;
	int ret;

	ret = nrfx_saadc_offset_calibrate(saadct_handler);
	if (ret < 0) {
		LOG_ERR("SAADC offset calibration failed: %d", ret);
		p_data->state = SAADCT_STATE_ERROR;
	}

	return ret;
}
#endif /* CLOCK_CONTROL_NRF || CLOCK_CONTROL_NRF_HFCLK || CLOCK_CONTROL_NRF_XO */

static void clock_release(const struct device *dev)
{
#if defined(CONFIG_CLOCK_CONTROL_NRF) || defined(CONFIG_CLOCK_CONTROL_NRF_HFCLK) ||                \
	defined(CONFIG_CLOCK_CONTROL_NRF_XO)
	if (clock_requested) {
		(void)hf_clock_release();
		clock_requested = false;
	}
#elif CONFIG_CLOCK_CONTROL_NRF54H_HFXO
	if (hfxo_requested) {
		const struct device *hfxo = DEVICE_DT_GET(DT_NODELABEL(hfxo));

		(void)nrf_clock_control_release(hfxo, NULL);
		hfxo_requested = false;
	}
#else
	ARG_UNUSED(dev);
#endif
}

static void gppi_enable(const struct device *dev)
{
	struct saadct_drv_data *p_data = dev->data;

	nrfx_gppi_conn_enable(p_data->ppi_sample);
#if NRF_SAADC_HAS_SHORTS
	nrf_saadc_shorts_enable(NRF_SAADC, NRF_SAADC_SHORT_END_START_MASK);
#else
	nrfx_gppi_conn_enable(p_data->ppi_end_start);
#endif
}

static void gppi_disable(const struct device *dev)
{
	struct saadct_drv_data *p_data = dev->data;

	nrfx_gppi_conn_disable(p_data->ppi_sample);
#if NRF_SAADC_HAS_SHORTS
	nrf_saadc_shorts_disable(NRF_SAADC, NRF_SAADC_SHORT_END_START_MASK);
#else
	nrfx_gppi_conn_disable(p_data->ppi_end_start);
#endif
}

static void gppi_uninit(const struct device *dev, struct saadct_drv_data *p_data)
{
	if (!p_data->gppi_ready) {
		return;
	}

	gppi_disable(dev);
	nrfx_gppi_conn_free(p_data->ppi_sample_evt, p_data->ppi_sample_tsk, p_data->ppi_sample);
#if !NRF_SAADC_HAS_SHORTS
	uint32_t ppi_end_evt = nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END);
	uint32_t ppi_end_tsk = nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START);

	nrfx_gppi_conn_free(ppi_end_evt, ppi_end_tsk, p_data->ppi_end_start);
#endif
	p_data->gppi_ready = false;
}

static void meas_stop(const struct device *dev)
{
	struct saadct_drv_data *p_data = dev->data;

	nrfx_timer_disable(&p_data->timer);
	gppi_disable(dev);
	clock_release(dev);
	pm_device_runtime_put_async(dev, K_NO_WAIT);
}

static struct saadct_meas_block *buffer_prepare(const struct device *dev)
{
	struct saadct_drv_data *p_data = dev->data;
	struct saadct_meas_block *block;
	uint32_t num_of_samples = p_data->num_of_meas * p_data->num_of_channels;
	int ret;

	ret = k_mem_slab_alloc(p_data->mem_slab, (void **)&block, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to allocate measurement block: %d", ret);
		return NULL;
	}

#ifdef CONFIG_HAS_NORDIC_DMM
	void *samples_buffer;

	ret = dmm_buffer_in_prepare(p_data->mem_reg, block->data,
				    NRFX_SAADC_SAMPLES_TO_BYTES(num_of_samples),
				    &samples_buffer);
	if (ret != 0) {
		LOG_ERR("DMM buffer allocation failed: %d", ret);
		k_mem_slab_free(p_data->mem_slab, block);
		return NULL;
	}
	ret = nrfx_saadc_buffer_set(samples_buffer, num_of_samples);
#else
	ret = nrfx_saadc_buffer_set(block->data, num_of_samples);
#endif

	if (ret < 0) {
		LOG_ERR("Failed to set SAADC buffer: %d", ret);
		p_data->state = SAADCT_STATE_ERROR;
		k_mem_slab_free(p_data->mem_slab, block);
		return NULL;
	}

	return block;
}

static void saadct_handler(nrfx_saadc_evt_t const *p_event)
{
	struct saadct_drv_data *p_data = saadc_device->data;
	int status;

	switch (p_event->type) {
	case NRFX_SAADC_EVT_CALIBRATEDONE:
		status = nrfx_saadc_mode_trigger();
		if (status < 0) {
			LOG_ERR("Failed to trigger SAADC: %d", status);
			p_data->state = SAADCT_STATE_ERROR;
			meas_stop(saadc_device);
		}
		break;

	case NRFX_SAADC_EVT_READY:
		nrfx_timer_clear(&p_data->timer);
		gppi_enable(saadc_device);
		nrfx_timer_enable(&p_data->timer);
		break;

	case NRFX_SAADC_EVT_BUF_REQ:
		if (p_data->mode == saadct_mode_continuous &&
		    p_data->state == SAADCT_STATE_RUNNING) {
			p_data->next_block = buffer_prepare(saadc_device);
			if (p_data->next_block == NULL) {
				p_data->state = SAADCT_STATE_ERROR;
				meas_stop(saadc_device);
			}
		}
		break;

	case NRFX_SAADC_EVT_DONE:
#ifdef CONFIG_HAS_NORDIC_DMM
		dmm_buffer_in_release(p_data->mem_reg, p_data->curr_block->data,
				      NRFX_SAADC_SAMPLES_TO_BYTES(p_data->num_of_meas *
								  p_data->num_of_channels),
				      p_event->data.done.p_buffer);
#endif
		k_fifo_put(&saadct_fifo, p_data->curr_block);
		atomic_inc(&p_data->pending_series);
		p_data->curr_block = p_data->next_block;
		p_data->next_block = NULL;

		if (p_data->user_handler != NULL) {
			p_data->user_handler(p_data->user_context);
		}
		break;

	case NRFX_SAADC_EVT_FINISHED:
		p_data->state = SAADCT_STATE_IDLE;
		meas_stop(saadc_device);
		k_fifo_cancel_wait(&saadct_fifo);
		break;

	default:
		break;
	}
}

static int timer_init(const struct device *dev)
{
	struct saadct_drv_data *p_data = dev->data;
	uint32_t sample_rate_hz = p_data->sample_rate_hz;
	nrfx_timer_t *timer = &p_data->timer;
	nrfx_timer_config_t timer_cfg = {
		.frequency = NRFX_TIMER_BASE_FREQUENCY_GET(timer),
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};
	uint32_t desired_ticks;
	int rc;

	desired_ticks = nrfx_timer_us_to_ticks(timer, TIMER_PERIOD_US(sample_rate_hz));
	rc = nrfx_timer_init(timer, &timer_cfg, NULL);
	if (rc < 0) {
		return rc;
	}
	nrfx_timer_extended_compare(timer, NRF_TIMER_CC_CHANNEL0, desired_ticks,
				    NRF_TIMER_SHORT_COMPARE0_CLEAR_MASK, false);

	nrfx_timer_clear(timer);

	return 0;
}

static int saadc_configure(const struct device *dev)
{
	struct saadct_drv_data *p_data = dev->data;
	nrfx_saadc_adv_config_t adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
	uint32_t channel_mask;
	int rc;

	rc = nrfx_saadc_channels_config(p_data->channels_config, p_data->num_of_channels);
	if (rc < 0) {
		return rc;
	}

	adv_config.internal_timer_cc = 0;
	adv_config.start_on_end = false;

	channel_mask = nrfx_saadc_channels_configured_get();
	rc = nrfx_saadc_advanced_mode_set(channel_mask, p_data->resolution, &adv_config,
					  saadct_handler);

	return rc;
}

static int gppi_init(const struct device *dev)
{
	struct saadct_drv_data *p_data = dev->data;
	int ret;

	gppi_uninit(dev, p_data);

	p_data->ppi_sample_evt = nrfx_timer_event_address_get(&p_data->timer,
							      NRF_TIMER_EVENT_COMPARE0);
	p_data->ppi_sample_tsk = nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_SAMPLE);

	ret = nrfx_gppi_conn_alloc(p_data->ppi_sample_evt, p_data->ppi_sample_tsk,
				   &p_data->ppi_sample);
	if (ret < 0) {
		return ret;
	}

#if !NRF_SAADC_HAS_SHORTS
	uint32_t ppi_end_evt = nrf_saadc_event_address_get(NRF_SAADC, NRF_SAADC_EVENT_END);
	uint32_t ppi_end_tsk = nrf_saadc_task_address_get(NRF_SAADC, NRF_SAADC_TASK_START);

	ret = nrfx_gppi_conn_alloc(ppi_end_evt, ppi_end_tsk, &p_data->ppi_end_start);
	if (ret < 0) {
		nrfx_gppi_conn_free(p_data->ppi_sample_evt, p_data->ppi_sample_tsk,
				    p_data->ppi_sample);
		return ret;
	}
#endif

	p_data->gppi_ready = true;

	return 0;
}

static int validate_config(const struct saadct_config *cfg)
{
	if (cfg == NULL || cfg->channels_config == NULL) {
		return -EINVAL;
	}

	if (cfg->num_of_channels == 0U || cfg->num_of_meas == 0U || cfg->sample_rate_hz == 0U) {
		return -EINVAL;
	}

	return 0;
}

int saadct_configure(const struct device *dev, const struct saadct_config *cfg)
{
	struct saadct_drv_data *p_data = dev->data;
	int ret;

	ret = validate_config(cfg);
	if (ret < 0) {
		return ret;
	}

	p_data->num_of_channels = cfg->num_of_channels;
	p_data->resolution = cfg->resolution;
	p_data->sample_rate_hz = cfg->sample_rate_hz;
	p_data->user_handler = cfg->user_handler;
	p_data->user_context = cfg->user_context;
	p_data->num_of_meas = cfg->num_of_meas;
	p_data->mode = cfg->mode;
	p_data->channels_config = cfg->channels_config;
	saadc_device = dev;

	ret = timer_init(dev);
	if (ret < 0) {
		return ret;
	}

	ret = gppi_init(dev);
	if (ret < 0) {
		return ret;
	}

	return saadc_configure(dev);
}

static void discard_active_buffers(struct saadct_drv_data *p_data)
{
	if (p_data->curr_block != NULL) {
		k_mem_slab_free(p_data->mem_slab, p_data->curr_block);
		p_data->curr_block = NULL;
	}

	if (p_data->next_block != NULL) {
		k_mem_slab_free(p_data->mem_slab, p_data->next_block);
		p_data->next_block = NULL;
	}
}

int saadct_start(const struct device *dev, struct k_mem_slab *slab)
{
	struct saadct_drv_data *p_data = dev->data;
	int ret;

	if (slab == NULL) {
		return -EINVAL;
	}

	p_data->mem_slab = slab;
	discard_active_buffers(p_data);
	p_data->curr_block = buffer_prepare(dev);
	if (p_data->curr_block == NULL) {
		return -ENOMEM;
	}

	p_data->next_block = NULL;
	p_data->state = (p_data->mode == saadct_mode_one_shot) ? SAADCT_STATE_STOPPING
							       : SAADCT_STATE_RUNNING;

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
		discard_active_buffers(p_data);
		p_data->state = SAADCT_STATE_IDLE;
		return ret;
	}

	ret = start_sampling();
	if (ret < 0) {
		pm_device_runtime_put_async(dev, K_NO_WAIT);
		discard_active_buffers(p_data);
		p_data->state = SAADCT_STATE_IDLE;
		return ret;
	}

	return 0;
}

int saadct_stop(const struct device *dev, bool immediate)
{
	struct saadct_drv_data *p_data = dev->data;

	if (immediate) {
		p_data->state = SAADCT_STATE_IDLE;
		meas_stop(dev);
		discard_active_buffers(p_data);
		k_fifo_cancel_wait(&saadct_fifo);
		return 0;
	}

	p_data->state = SAADCT_STATE_STOPPING;

	return 0;
}

static bool measurement_active(struct saadct_drv_data *p_data)
{
	return p_data->state == SAADCT_STATE_RUNNING ||
	       p_data->state == SAADCT_STATE_STOPPING;
}

int saadct_get(const struct device *dev, nrf_saadc_value_t **data, k_timeout_t timeout)
{
	struct saadct_drv_data *p_data = dev->data;
	struct saadct_meas_block *block;

	if (data == NULL) {
		return -EINVAL;
	}

	block = k_fifo_get(&saadct_fifo, timeout);
	if (block == NULL) {
		if (atomic_get(&p_data->pending_series) > 0 ||
		    (p_data->mem_slab != NULL && k_mem_slab_num_used_get(p_data->mem_slab) > 0) ||
		    measurement_active(p_data)) {
			return -EAGAIN;
		}

		return -EIO;
	}

	atomic_dec(&p_data->pending_series);
	*data = block->data;

	return 0;
}

void saadct_put(const struct device *dev, nrf_saadc_value_t *data)
{
	struct saadct_drv_data *p_data = dev->data;
	struct saadct_meas_block *block =
		CONTAINER_OF((void *)data, struct saadct_meas_block, data);

	k_mem_slab_free(p_data->mem_slab, block);
}

uint32_t saadct_pending(const struct device *dev)
{
	struct saadct_drv_data *p_data = dev->data;

	return atomic_get(&p_data->pending_series);
}

static int saadc_pm_handler(const struct device *dev, enum pm_device_action action)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(action);

	return 0;
}

#define TIMER_NODE(instance) DT_INST_PHANDLE(instance, timer_instance)

#define NRFX_SAADCT_DEVICE(inst)                                                                   \
	static struct saadct_drv_data drv_data_##inst = {                                          \
		.timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(TIMER_NODE(inst))),                       \
		COND_CODE_1(CONFIG_HAS_NORDIC_DMM,                                                 \
			(.mem_reg = DMM_DEV_TO_REG(DT_NODELABEL(adc))),                            \
			())                                                                        \
	};                                                                                         \
	static int saadct_##inst##_init(const struct device *dev)                                  \
	{                                                                                          \
		int rc;                                                                            \
                                                                                                   \
		IRQ_CONNECT(DT_INST_IRQN(inst), DT_INST_IRQ(inst, priority),                       \
			    nrfx_saadc_irq_handler, NULL, 0);                                      \
		rc = nrfx_saadc_init(NRFX_SAADC_DEFAULT_CONFIG_IRQ_PRIORITY);                      \
		if (rc < 0) {                                                                      \
			return rc;                                                                 \
		}                                                                                  \
                                                                                                   \
		return pm_device_driver_init(dev, saadc_pm_handler);                               \
	}                                                                                          \
	PM_DEVICE_DT_INST_DEFINE(inst, saadc_pm_handler);                                          \
	DEVICE_DT_INST_DEFINE(inst, saadct_##inst##_init, PM_DEVICE_DT_INST_GET(inst),             \
			      &drv_data_##inst, NULL, POST_KERNEL,                                 \
			      CONFIG_SAADCT_INIT_PRIORITY, NULL);

NRFX_SAADCT_DEVICE(0)
