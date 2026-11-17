/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_pulse_meas

#include <zephyr/kernel.h>
#include <zephyr/irq.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>
#include <soc.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <gpiote_nrfx.h>
#include <drivers/pulse_meas.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>

#if defined(CONFIG_NRF_SYS_EVENT_IRQ_LATENCY)
#include <nrf_sys_event.h>
#elif defined(CONFIG_MRAM_LATENCY)
#include <mram_latency.h>
#endif

LOG_MODULE_REGISTER(pulse_meas, CONFIG_PULSE_MEAS_LOG_LEVEL);

K_FIFO_DEFINE(pulse_meas_fifo);

#define PULSE_MEAS_RESOLUTION MHZ(1)

#define PULSE_MEAS_PIN_UNUSED UINT32_MAX

struct pulse_meas_block {
	void *fifo_reserved;
	uint32_t data[];
};

typedef enum {
	/* Device is initialized but not configured */
	PULSE_MEAS_STATE_IDLE,

	/* Device is configured and ready to start a measurement */
	PULSE_MEAS_STATE_READY,

	/* Device is running a measurement */
	PULSE_MEAS_STATE_RUNNING,

	/* Device is stopping a measurement. Transitioning to ready state. */
	PULSE_MEAS_STATE_STOPPING,

	/* Device encountered an error */
	PULSE_MEAS_STATE_ERROR,
} pulse_meas_state_t;

struct pulse_meas_drv_data {
	nrfx_timer_t timer;
	nrfx_gppi_handle_t ppi_start;
	nrfx_gppi_handle_t ppi_end;
#if defined(CONFIG_NRF_SYS_EVENT_IRQ_LATENCY)
	int sys_event_handle;
#endif
	uint32_t timer_frequency;
	struct pulse_meas_block *curr_block;
	uint32_t meas_count;
	uint32_t meas_num;
	void (*user_handler)(void *context);
	void *user_context;
	struct k_mem_slab *mem_slab;
	atomic_t pending_series;

	uint8_t gpiote_ch[2];
	pulse_meas_mode_t mode;
	volatile pulse_meas_state_t state;
	pulse_meas_pulse_t pulse_type;
	nrf_gpio_pin_pull_t pull_cfg;
	pulse_meas_latency_t latency;
};

struct pulse_meas_drv_cfg {
	uint32_t assert_pin;
	uint32_t deassert_pin;
	nrfx_gpiote_t *gpiote;
	NRF_TIMER_Type *timer_reg;
};

#if IS_ENABLED(CONFIG_PULSE_MEAS_USE_HFCLK)
#if IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF)
static struct onoff_manager *clk_mgr;
static struct onoff_client clk_cli;
static K_SEM_DEFINE(clock_ready_sem, 0, 1);

static void clock_started_callback(struct onoff_manager *mgr, struct onoff_client *cli,
				   uint32_t state, int res)
{
	ARG_UNUSED(mgr);
	ARG_UNUSED(cli);
	ARG_UNUSED(state);
	ARG_UNUSED(res);

	k_sem_give(&clock_ready_sem);
}

static int hf_clock_request(void)
{
	int ret;

	sys_notify_init_callback(&clk_cli.notify, clock_started_callback);
	clk_mgr = z_nrf_clock_control_get_onoff(CLOCK_CONTROL_NRF_SUBSYS_HF);
	ret = onoff_request(clk_mgr, &clk_cli);
	if (ret < 0) {
		return ret;
	}

	ret = k_sem_take(&clock_ready_sem, K_MSEC(50));
	if (ret < 0) {
		(void)onoff_cancel_or_release(clk_mgr, &clk_cli);
		return ret;
	}

	return 0;
}

static int hf_clock_release(void)
{
	return onoff_release(clk_mgr);
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF54H_HFXO) ||                                                 \
	(IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_COMMON) && !IS_ENABLED(CONFIG_SOC_SERIES_NRF92) &&    \
	 !IS_ENABLED(CONFIG_SOC_SERIES_NRF54L))
static const struct device *clock_dev =
		COND_CODE_1(NRF_CLOCK_HAS_HFCLK, (DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk)),
			    (COND_CODE_1(NRF_CLOCK_HAS_XO, (DEVICE_DT_GET_ONE(nordic_nrf_clock_xo)),
					 (DEVICE_DT_GET_ONE(nordic_nrf54h_hfxo)))));

static int hf_clock_request(void)
{
	return nrf_clock_control_request_sync(clock_dev, NULL, K_MSEC(2000));
}

static int hf_clock_release(void)
{
	return nrf_clock_control_release(clock_dev, NULL);
}
#else
static int hf_clock_request(void)
{
	return 0;
}

static int hf_clock_release(void)
{
	return 0;
}
#endif /* CONFIG_CLOCK_CONTROL_NRF */
#endif /* IS_ENABLED(CONFIG_PULSE_MEAS_USE_HFCLK) */

static void gpiote_enable(const struct device *dev)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;

	nrfx_gpiote_trigger_enable(p_cfg->gpiote, p_cfg->assert_pin, false);

	if (p_cfg->deassert_pin != PULSE_MEAS_PIN_UNUSED) {
		nrfx_gpiote_trigger_enable(p_cfg->gpiote, p_cfg->deassert_pin, true);
	} else {
		nrfx_gpiote_ext_trigger_enable(p_cfg->gpiote, p_cfg->assert_pin, true);
	}
}

static void gpiote_disable(const struct device *dev)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;

	nrfx_gpiote_trigger_disable(p_cfg->gpiote, p_cfg->assert_pin);
	if (p_cfg->deassert_pin != PULSE_MEAS_PIN_UNUSED) {
		nrfx_gpiote_trigger_disable(p_cfg->gpiote, p_cfg->deassert_pin);
	} else {
		nrfx_gpiote_ext_trigger_disable(p_cfg->gpiote, p_cfg->assert_pin);
	}
}

static void gppi_disable(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	nrfx_gppi_conn_disable(p_data->ppi_start);
	nrfx_gppi_conn_disable(p_data->ppi_end);
}

static void gppi_enable(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	nrfx_gppi_conn_enable(p_data->ppi_start);
	nrfx_gppi_conn_enable(p_data->ppi_end);
}

static void low_latency_request(const struct device *dev)
{
#if defined(CONFIG_NRF_SYS_EVENT_IRQ_LATENCY)
	struct pulse_meas_drv_data *p_data = dev->data;

	p_data->sys_event_handle = nrf_sys_event_register(0, true);
#elif defined(CONFIG_MRAM_LATENCY)
	mram_no_latency_sync_request();
#endif
}

static void low_latency_release(const struct device *dev)
{
#if defined(CONFIG_NRF_SYS_EVENT_IRQ_LATENCY)
	struct pulse_meas_drv_data *p_data = dev->data;

	if (p_data->sys_event_handle >= 0) {
		nrf_sys_event_unregister(p_data->sys_event_handle, false);
	}
#elif defined(CONFIG_MRAM_LATENCY)
	mram_no_latency_sync_release();
#endif
}

static void meas_stop(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	gppi_disable(dev);
	gpiote_disable(dev);
	nrfx_timer_disable(&p_data->timer);
#if IS_ENABLED(CONFIG_PULSE_MEAS_USE_HFCLK)
	hf_clock_release();
#endif
	pulse_meas_state_t old_state = p_data->state;

	if (p_data->latency == PULSE_MEAS_LATENCY_LOW) {
		low_latency_release(dev);
	}

	p_data->state = PULSE_MEAS_STATE_READY;
	if (old_state != PULSE_MEAS_STATE_READY) {
		pm_device_runtime_put(dev);
	}
}

static struct pulse_meas_block *buffer_prepare(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	struct pulse_meas_block *block;
	int ret;

	ret = k_mem_slab_alloc(p_data->mem_slab, (void **)&block, K_NO_WAIT);
	if (ret < 0) {
		LOG_ERR("Failed to allocate measurement block: %d", ret);
		return NULL;
	}

	return block;
}

static void gpiote_handler(nrfx_gpiote_pin_t pin, nrfx_gpiote_trigger_t action, void *p_context)
{
	const struct device *dev = p_context;
	struct pulse_meas_drv_data *p_data = dev->data;
	uint32_t idx = p_data->meas_count;
	struct pulse_meas_block *block = p_data->curr_block;
	uint32_t timer_cc_val;

	ARG_UNUSED(pin);
	ARG_UNUSED(action);

	if (block == NULL) {
		return;
	}

	timer_cc_val = nrf_timer_cc_get(p_data->timer.p_reg, NRF_TIMER_CC_CHANNEL0);
	block->data[idx] = timer_cc_val;
	p_data->meas_count = idx + 1;

	if (p_data->meas_count < p_data->meas_num) {
		return;
	}

	k_fifo_put(&pulse_meas_fifo, block);
	atomic_inc(&p_data->pending_series);
	p_data->meas_count = 0;

	if (p_data->state == PULSE_MEAS_STATE_STOPPING) {
		p_data->curr_block = NULL;
		meas_stop(dev);
	} else if (p_data->state == PULSE_MEAS_STATE_RUNNING &&
		   p_data->mode == PULSE_MEAS_MODE_CONTINUOUS) {
		p_data->curr_block = buffer_prepare(dev);
		if (p_data->curr_block == NULL) {
			p_data->state = PULSE_MEAS_STATE_ERROR;
			meas_stop(dev);
		}
	} else {
		p_data->curr_block = NULL;
	}

	if (p_data->user_handler != NULL) {
		p_data->user_handler(p_data->user_context);
	}
}

static uint32_t get_deassert_event(const struct device *dev)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;

	if (p_cfg->deassert_pin != PULSE_MEAS_PIN_UNUSED) {
		return nrfx_gpiote_in_event_address_get(p_cfg->gpiote, p_cfg->deassert_pin);
	} else {
		return nrfx_gpiote_ext_in_event_address_get(p_cfg->gpiote, p_cfg->assert_pin);
	}
}

static void gppi_unconfigure(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;
	uint32_t assert_event, deassert_event;
	uint32_t capture0_task, clear_task;

	assert_event = nrfx_gpiote_in_event_address_get(p_cfg->gpiote, p_cfg->assert_pin);
	deassert_event = get_deassert_event(dev);
	clear_task = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CLEAR);
	capture0_task = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CAPTURE0);

	nrfx_gppi_conn_free(assert_event, clear_task, p_data->ppi_start);
	nrfx_gppi_conn_free(deassert_event, capture0_task, p_data->ppi_end);
}

static int gppi_configure(const struct device *dev)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;
	struct pulse_meas_drv_data *p_data = dev->data;
	uint32_t assert_event, deassert_event;
	uint32_t capture0_task, clear_task;
	int ret;

	assert_event = nrfx_gpiote_in_event_address_get(p_cfg->gpiote, p_cfg->assert_pin);
	deassert_event = get_deassert_event(dev);
	clear_task = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CLEAR);
	capture0_task = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CAPTURE0);

	ret = nrfx_gppi_conn_alloc(assert_event, clear_task, &p_data->ppi_start);
	if (ret < 0) {
		return ret;
	}

	ret = nrfx_gppi_conn_alloc(deassert_event, capture0_task, &p_data->ppi_end);
	if (ret < 0) {
		nrfx_gppi_conn_free(assert_event, clear_task, p_data->ppi_start);
		return ret;
	}

	return 0;
}

static void gpiote_unconfigure(const struct device *dev)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;

	gpiote_disable(dev);
	(void)nrfx_gpiote_pin_uninit(p_cfg->gpiote, p_cfg->assert_pin);
	if (p_cfg->deassert_pin != PULSE_MEAS_PIN_UNUSED) {
		(void)nrfx_gpiote_pin_uninit(p_cfg->gpiote, p_cfg->deassert_pin);
	}
}

static int gpiote_configure(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;
	bool single_pin_mode = p_cfg->deassert_pin == PULSE_MEAS_PIN_UNUSED;
	bool positive_pulse;
	int err;

	switch (p_data->pulse_type) {
	case PULSE_MEAS_PULSE_POSITIVE:
		positive_pulse = true;
		break;
	case PULSE_MEAS_PULSE_NEGATIVE:
		positive_pulse = false;
		break;
	default:
		return -EINVAL;
	}

	nrfx_gpiote_trigger_config_t trigger_config = {
		.trigger = positive_pulse ? NRFX_GPIOTE_TRIGGER_LOTOHI : NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &p_data->gpiote_ch[0],
	};

	nrfx_gpiote_handler_config_t handler_config = {
		.handler = gpiote_handler,
		.p_context = (void *)(uintptr_t)dev,
	};

	nrfx_gpiote_input_pin_config_t gpiote_cfg = {
		.p_pull_config = &p_data->pull_cfg,
		.p_trigger_config = &trigger_config,
		.p_handler_config = single_pin_mode ? &handler_config : NULL,
		.p_ext_in_channel = single_pin_mode ? &p_data->gpiote_ch[1] : NULL,
	};

	err = nrfx_gpiote_input_configure(p_cfg->gpiote, p_cfg->assert_pin, &gpiote_cfg);
	if (err < 0) {
		return err;
	}

	if (single_pin_mode) {
		return 0;
	}

	/* If deassert pin is used, configure it as an opposite edge trigger, using another
	 * GPIOTE channel. Assign handler as well.
	 */
	trigger_config.trigger =
		positive_pulse ? NRFX_GPIOTE_TRIGGER_HITOLO : NRFX_GPIOTE_TRIGGER_LOTOHI;
	trigger_config.p_in_channel = &p_data->gpiote_ch[1];
	gpiote_cfg.p_handler_config = &handler_config;

	err = nrfx_gpiote_input_configure(p_cfg->gpiote, p_cfg->deassert_pin, &gpiote_cfg);
	if (err < 0) {
		return err;
	}

	return 0;
}

static void timer_prepare(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	nrfx_timer_clear(&p_data->timer);
}

static int validate_config(const struct pulse_meas_config *cfg)
{
	if (cfg == NULL) {
		return -EINVAL;
	}

	if (cfg->num_of_meas == 0U) {
		return -EINVAL;
	}

	if (cfg->pulse_type != PULSE_MEAS_PULSE_POSITIVE &&
	    cfg->pulse_type != PULSE_MEAS_PULSE_NEGATIVE) {
		return -EINVAL;
	}

	return 0;
}

int pulse_meas_configure(const struct device *dev, const struct pulse_meas_config *cfg)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	int ret;

	if (p_data->state != PULSE_MEAS_STATE_IDLE) {
		return -EBUSY;
	}

	ret = validate_config(cfg);
	if (ret < 0) {
		return ret;
	}

	p_data->meas_num = cfg->num_of_meas;
	p_data->pulse_type = cfg->pulse_type;
	p_data->mode = cfg->mode;
	p_data->pull_cfg = cfg->pull_config;
	p_data->user_handler = cfg->user_handler;
	p_data->user_context = cfg->user_context;
	p_data->latency = cfg->latency;

	ret = gpiote_configure(dev);
	if (ret < 0) {
		return ret;
	}

	timer_prepare(dev);

	ret = gppi_configure(dev);
	if (ret < 0) {
		return ret;
	}

	p_data->state = PULSE_MEAS_STATE_READY;
	return 0;
}

int pulse_meas_reset(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	if (p_data->state != PULSE_MEAS_STATE_READY) {
		return -EBUSY;
	}

	gppi_unconfigure(dev);
	gpiote_unconfigure(dev);

	p_data->state = PULSE_MEAS_STATE_IDLE;

	return 0;
}

static void discard_active_buffers(struct pulse_meas_drv_data *p_data)
{
	if (p_data->curr_block != NULL) {
		k_mem_slab_free(p_data->mem_slab, p_data->curr_block);
		p_data->curr_block = NULL;
	}
}

int pulse_meas_start(const struct device *dev, struct k_mem_slab *slab)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	int ret;

	if (p_data->state != PULSE_MEAS_STATE_READY) {
		return -EBUSY;
	}

	if (slab == NULL) {
		return -EINVAL;
	}

	p_data->mem_slab = slab;
	p_data->meas_count = 0;
	discard_active_buffers(p_data);

	p_data->curr_block = buffer_prepare(dev);
	if (p_data->curr_block == NULL) {
		return -ENOMEM;
	}

	p_data->state = (p_data->mode == PULSE_MEAS_MODE_ONE_SHOT) ? PULSE_MEAS_STATE_STOPPING
								   : PULSE_MEAS_STATE_RUNNING;

#if IS_ENABLED(CONFIG_PULSE_MEAS_USE_HFCLK)
	ret = hf_clock_request();
	if (ret < 0) {
		discard_active_buffers(p_data);
		p_data->state = PULSE_MEAS_STATE_READY;
		return ret;
	}
#endif

	ret = pm_device_runtime_get(dev);
	if (ret < 0) {
#ifdef CONFIG_PULSE_MEAS_USE_HFCLK
		hf_clock_release();
#endif
		discard_active_buffers(p_data);
		p_data->state = PULSE_MEAS_STATE_READY;
		return ret;
	}

	if (p_data->latency == PULSE_MEAS_LATENCY_LOW) {
		low_latency_request(dev);
	}

	nrfx_timer_clear(&p_data->timer);
	nrfx_timer_enable(&p_data->timer);
	gppi_enable(dev);
	gpiote_enable(dev);

	return 0;
}

static bool measurement_active(struct pulse_meas_drv_data *p_data)
{
	return p_data->state == PULSE_MEAS_STATE_RUNNING ||
	       p_data->state == PULSE_MEAS_STATE_STOPPING;
}

int pulse_meas_stop(const struct device *dev, bool immediate)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	if (!measurement_active(p_data)) {
		return -EBUSY;
	}

	if (immediate) {
		struct pulse_meas_block *block = p_data->curr_block;

		meas_stop(dev);
		if (block != NULL) {
			k_mem_slab_free(p_data->mem_slab, block);
			p_data->curr_block = NULL;
		}
		discard_active_buffers(p_data);

		return 0;
	}

	p_data->state = PULSE_MEAS_STATE_STOPPING;

	return 0;
}

int pulse_meas_get(const struct device *dev, uint32_t **data)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	struct pulse_meas_block *block;

	if (data == NULL) {
		return -EINVAL;
	}

	block = k_fifo_get(&pulse_meas_fifo, K_NO_WAIT);
	if (block == NULL) {
		if (atomic_get(&p_data->pending_series) > 0 ||
		    k_mem_slab_num_used_get(p_data->mem_slab) > 0 || measurement_active(p_data)) {
			return -EAGAIN;
		}

		return -EIO;
	}

	atomic_dec(&p_data->pending_series);
	*data = block->data;

	return 0;
}

void pulse_meas_put(const struct device *dev, uint32_t *data)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	struct pulse_meas_block *block = CONTAINER_OF((void *)data, struct pulse_meas_block, data);

	k_mem_slab_free(p_data->mem_slab, block);
}

uint32_t pulse_meas_pending(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	return atomic_get(&p_data->pending_series);
}

static void pin_retain_set(uint32_t pin, bool enable)
{
#if NRF_GPIO_HAS_RETENTION_SETCLEAR
	if (enable) {
		nrf_gpio_pin_retain_enable(pin);
	} else {
		nrf_gpio_pin_retain_disable(pin);
	}
#else
	ARG_UNUSED(pin);
	ARG_UNUSED(enable);
#endif
}

static void pin_clock_set(uint32_t pin, bool enable)
{
#if NRF_GPIO_HAS_CLOCKPIN
	nrf_gpio_pin_clock_set(pin, enable);
#else
	ARG_UNUSED(pin);
	ARG_UNUSED(enable);
#endif
}

static void pin_low_power_enter(uint32_t pin)
{
	pin_retain_set(pin, false);
	nrfy_gpio_cfg(pin, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT,
		      NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
	pin_clock_set(pin, false);
	pin_retain_set(pin, true);
}

static void pin_low_power_exit(uint32_t pin, nrf_gpio_pin_pull_t pull)
{
	/* Retention must be released before the pin registers can be written. */
	pin_retain_set(pin, false);
	nrfy_gpio_cfg(pin, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, pull,
		      NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
	pin_retain_set(pin, true);
}

static int pulse_meas_pm_suspend(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	const struct pulse_meas_drv_cfg *p_config = dev->config;

	if (measurement_active(p_data)) {
		return -EBUSY;
	}

	pin_low_power_enter(p_config->assert_pin);
	if (p_config->deassert_pin != PULSE_MEAS_PIN_UNUSED) {
		pin_low_power_enter(p_config->deassert_pin);
	}

	return 0;
}

static int pulse_meas_pm_resume(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	const struct pulse_meas_drv_cfg *p_config = dev->config;

	pin_low_power_exit(p_config->assert_pin, p_data->pull_cfg);
	if (p_config->deassert_pin != PULSE_MEAS_PIN_UNUSED) {
		pin_low_power_exit(p_config->deassert_pin, p_data->pull_cfg);
	}

	return 0;
}

static int pulse_meas_pm_action(const struct device *dev, enum pm_device_action action)
{
	int ret;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		ret = pulse_meas_pm_suspend(dev);
		break;

	case PM_DEVICE_ACTION_RESUME:
		ret = pulse_meas_pm_resume(dev);
		break;

	case PM_DEVICE_ACTION_TURN_OFF:
	case PM_DEVICE_ACTION_TURN_ON:
		ret = -ENOTSUP;
		break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static int pulse_meas_init(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;
	static const nrfx_timer_config_t timer_cfg = {
		.frequency = PULSE_MEAS_RESOLUTION,
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};
	int rc;

	p_data->timer_frequency = timer_cfg.frequency;
	p_data->timer.p_reg = p_cfg->timer_reg;
	rc = nrfx_timer_init(&p_data->timer, &timer_cfg, NULL);
	if (rc < 0) {
		return rc;
	}

	rc = nrfx_gpiote_init(p_cfg->gpiote, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
	if (rc < 0 && rc != -EALREADY) {
		LOG_ERR("Failed to initialize GPIOTE: %d", rc);
		return rc;
	}

	for (size_t i = 0; i < ARRAY_SIZE(p_data->gpiote_ch); i++) {
		rc = nrfx_gpiote_channel_alloc(p_cfg->gpiote, &p_data->gpiote_ch[i]);
		if (rc < 0) {
			LOG_ERR("Failed to allocate GPIOTE channel: %d", rc);
			return rc;
		}
	}

	return pm_device_driver_init(dev, pulse_meas_pm_action);
}

#define GPIOTE_PHANDLE(instance, gpio)                                                             \
	DT_PROP(DT_GPIO_CTLR(DT_DRV_INST(instance), gpio), gpiote_instance)
#define TIMER_PHANDLE(instance) DT_INST_PHANDLE(instance, timer_instance)

#define GPIO_PORT_NUMBER(instance, gpio) DT_PROP(DT_GPIO_CTLR(DT_DRV_INST(instance), gpio), port)
#define GPIO_PIN_NUMBER(instance, gpio)	 DT_GPIO_PIN(DT_DRV_INST(instance), gpio)
#define ABSOLUTE_PIN_NUMBER(instance, gpio)                                                        \
	NRF_GPIO_PIN_MAP(GPIO_PORT_NUMBER(instance, gpio), GPIO_PIN_NUMBER(instance, gpio))

#define OPT_ABSOLUTE_PIN_NUMBER(instance, gpio)                                                    \
	COND_CODE_1(DT_HAS_PROP(DT_DRV_INST(instance), gpio),                                      \
		    (ABSOLUTE_PIN_NUMBER(instance, gpio)), (PULSE_MEAS_PIN_UNUSED))

#define PULSE_MEAS_DEVICE(inst)                                                                    \
	IF_ENABLED(DT_HAS_PROP(DT_DRV_INST(inst), deassert_gpios),                                 \
		(BUILD_ASSERT(DT_SAME_NODE(GPIOTE_PHANDLE(inst, assert_gpios),                     \
					   GPIOTE_PHANDLE(inst, deassert_gpios)),                  \
			      "assert-gpio and deassert-gpio must use the same GPIOTE instance");))\
	static struct pulse_meas_drv_data drv_data_##inst;                                         \
	static const struct pulse_meas_drv_cfg drv_cfg_##inst = {                                  \
		.assert_pin = ABSOLUTE_PIN_NUMBER(inst, assert_gpios),                             \
		.deassert_pin = OPT_ABSOLUTE_PIN_NUMBER(inst, deassert_gpios),                         \
		.gpiote = &GPIOTE_NRFX_INST_BY_NODE(GPIOTE_PHANDLE(inst, assert_gpios)),           \
		.timer_reg = (NRF_TIMER_Type *)DT_REG_ADDR(TIMER_PHANDLE(inst)),                   \
	};                                                                                         \
	PM_DEVICE_DT_INST_DEFINE(inst, pulse_meas_pm_action, PM_DEVICE_ISR_SAFE);                  \
	DEVICE_DT_INST_DEFINE(inst, pulse_meas_init, PM_DEVICE_DT_INST_GET(inst),                  \
			      &drv_data_##inst, &drv_cfg_##inst, POST_KERNEL,                      \
			      CONFIG_PULSE_MEAS_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(PULSE_MEAS_DEVICE)
