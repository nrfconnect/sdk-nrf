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

LOG_MODULE_REGISTER(pulse_meas, CONFIG_PULSE_MEAS_LOG_LEVEL);

K_FIFO_DEFINE(pulse_meas_fifo);

#define PULSE_MEAS_RESOLUTION MHZ(1)

struct pulse_meas_block {
	void *fifo_reserved;
	uint32_t data[];
};

typedef enum {
	PULSE_MEAS_STATE_IDLE,
	PULSE_MEAS_STATE_RUNNING,
	PULSE_MEAS_STATE_STOPPING,
	PULSE_MEAS_STATE_ERROR,
} pulse_meas_state_t;

struct pulse_meas_drv_data {
	nrfx_timer_t timer;
	nrfx_gpiote_t *gpiote;
	nrfx_gppi_handle_t ppi_start;
	nrfx_gppi_handle_t ppi_end;
	bool gppi_ready;
	bool gpiote_ready;
	bool timer_ready;
	uint32_t timer_frequency;
	struct pulse_meas_block *curr_block;
	uint32_t meas_count;
	pulse_meas_pulse_t pulse_type;
	uint32_t meas_num;
	pulse_meas_mode_t mode;
	nrf_gpio_pin_pull_t pull_cfg;
	void (*user_handler)(void *context);
	void *user_context;
	struct k_mem_slab *mem_slab;
	volatile pulse_meas_state_t state;
	atomic_t pending_series;
};

struct pulse_meas_drv_cfg {
	uint32_t assert_pin;
	uint32_t deassert_pin;
};

#if IS_ENABLED(CONFIG_PULSE_MEAS_USE_HFCLK)
#if IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF)
static struct onoff_manager *clk_mgr;
static struct onoff_client clk_cli;
static K_SEM_DEFINE(clock_ready_sem, 0, 1);
static bool clock_requested;

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

	clock_requested = true;

	return 0;
}

static int hf_clock_release(void)
{
	if (!clock_requested) {
		return 0;
	}

	clock_requested = false;

	return onoff_release(clk_mgr);
}

#elif defined(CONFIG_CLOCK_CONTROL_NRF54H_HFXO) ||                                                 \
	(IS_ENABLED(CONFIG_CLOCK_CONTROL_NRF_COMMON) && !IS_ENABLED(CONFIG_SOC_SERIES_NRF92) &&    \
	 !IS_ENABLED(CONFIG_SOC_SERIES_NRF54L))
static bool clock_requested;

static int hf_clock_request(void)
{
	const struct device *dev =
		COND_CODE_1(NRF_CLOCK_HAS_HFCLK, (DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk)),
			    (COND_CODE_1(NRF_CLOCK_HAS_XO, (DEVICE_DT_GET_ONE(nordic_nrf_clock_xo)),
					 (DEVICE_DT_GET_ONE(nordic_nrf54h_hfxo)))));
	int ret;

	ret = nrf_clock_control_request_sync(dev, NULL, K_MSEC(2000));
	if (ret < 0) {
		return ret;
	}

	clock_requested = true;

	return 0;
}

static int hf_clock_release(void)
{
	if (!clock_requested) {
		return 0;
	}

	const struct device *dev =
		COND_CODE_1(NRF_CLOCK_HAS_HFCLK, (DEVICE_DT_GET_ONE(nordic_nrf_clock_hfclk)),
			    (COND_CODE_1(NRF_CLOCK_HAS_XO, (DEVICE_DT_GET_ONE(nordic_nrf_clock_xo)),
					 (DEVICE_DT_GET_ONE(nordic_nrf54h_hfxo)))));
	clock_requested = false;

	return nrf_clock_control_release(dev, NULL);
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
	struct pulse_meas_drv_data *p_data = dev->data;

	nrfx_gpiote_trigger_enable(p_data->gpiote, p_cfg->assert_pin, false);
	nrfx_gpiote_trigger_enable(p_data->gpiote, p_cfg->deassert_pin, true);
}

static void gpiote_disable(const struct device *dev)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;
	struct pulse_meas_drv_data *p_data = dev->data;

	nrfx_gpiote_trigger_disable(p_data->gpiote, p_cfg->assert_pin);
	nrfx_gpiote_trigger_disable(p_data->gpiote, p_cfg->deassert_pin);
}

static void gppi_disable(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	if (!p_data->gppi_ready) {
		return;
	}

	nrfx_gppi_conn_disable(p_data->ppi_start);
	nrfx_gppi_conn_disable(p_data->ppi_end);
}

static void gppi_uninit(const struct device *dev, struct pulse_meas_drv_data *p_data)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;
	uint32_t evt_to_start;
	uint32_t evt_to_cpt;
	uint32_t start_task;
	uint32_t end_task;

	if (!p_data->gppi_ready) {
		return;
	}

	gppi_disable(dev);

	evt_to_start = nrfx_gpiote_in_event_address_get(p_data->gpiote, p_cfg->assert_pin);
	evt_to_cpt = nrfx_gpiote_in_event_address_get(p_data->gpiote, p_cfg->deassert_pin);
	start_task = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CLEAR);
	end_task = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CAPTURE0);

	nrfx_gppi_conn_free(evt_to_start, start_task, p_data->ppi_start);
	nrfx_gppi_conn_free(evt_to_cpt, end_task, p_data->ppi_end);
	p_data->gppi_ready = false;
}

#if IS_ENABLED(CONFIG_PULSE_MEAS_USE_HFCLK)
static void clock_release(void)
{
	(void)hf_clock_release();
}
#endif

static void meas_stop(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	gppi_disable(dev);
	gpiote_disable(dev);
	nrfx_timer_disable(&p_data->timer);
#if IS_ENABLED(CONFIG_PULSE_MEAS_USE_HFCLK)
	clock_release();
#endif
	p_data->state = PULSE_MEAS_STATE_IDLE;
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

static void gppi_enable(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	nrfx_gppi_conn_enable(p_data->ppi_start);
	nrfx_gppi_conn_enable(p_data->ppi_end);
}

static int gppi_init(const struct device *dev)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;
	struct pulse_meas_drv_data *p_data = dev->data;
	uint32_t evt_to_start;
	uint32_t evt_to_cpt;
	uint32_t start_task;
	uint32_t end_task;
	int ret;

	gppi_uninit(dev, p_data);

	evt_to_start = nrfx_gpiote_in_event_address_get(p_data->gpiote, p_cfg->assert_pin);
	evt_to_cpt = nrfx_gpiote_in_event_address_get(p_data->gpiote, p_cfg->deassert_pin);

	start_task = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CLEAR);
	ret = nrfx_gppi_conn_alloc(evt_to_start, start_task, &p_data->ppi_start);
	if (ret < 0) {
		return ret;
	}

	end_task = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CAPTURE0);
	ret = nrfx_gppi_conn_alloc(evt_to_cpt, end_task, &p_data->ppi_end);
	if (ret < 0) {
		nrfx_gppi_conn_free(evt_to_start, start_task, p_data->ppi_start);
		return ret;
	}

	p_data->gppi_ready = true;

	return 0;
}

static void gpiote_uninit(const struct device *dev, struct pulse_meas_drv_data *p_data)
{
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;

	if (!p_data->gpiote_ready) {
		return;
	}

	gpiote_disable(dev);
	(void)nrfx_gpiote_pin_uninit(p_data->gpiote, p_cfg->assert_pin);
	(void)nrfx_gpiote_pin_uninit(p_data->gpiote, p_cfg->deassert_pin);
	p_data->gpiote_ready = false;
}

static int gpiote_configure(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	const struct pulse_meas_drv_cfg *p_cfg = dev->config;
	uint8_t rising_edge_in_ch;
	uint8_t falling_edge_in_ch;
	int err;

	gpiote_uninit(dev, p_data);

	err = nrfx_gpiote_init(p_data->gpiote, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
	if (err < 0 && err != -EALREADY) {
		LOG_ERR("Failed to initialize GPIOTE: %d", err);
		return err;
	}

	err = nrfx_gpiote_channel_alloc(p_data->gpiote, &rising_edge_in_ch);
	if (err < 0) {
		LOG_ERR("Failed to allocate GPIOTE channel: %d", err);
		return err;
	}

	err = nrfx_gpiote_channel_alloc(p_data->gpiote, &falling_edge_in_ch);
	if (err < 0) {
		LOG_ERR("Failed to allocate GPIOTE channel: %d", err);
		(void)nrfx_gpiote_channel_free(p_data->gpiote, rising_edge_in_ch);
		return err;
	}

	const nrfx_gpiote_trigger_config_t rising_trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_LOTOHI,
		.p_in_channel = &rising_edge_in_ch,
	};

	const nrfx_gpiote_trigger_config_t falling_trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &falling_edge_in_ch,
	};

	nrfx_gpiote_handler_config_t handler_config = {
		.handler = gpiote_handler,
		.p_context = (void *)(uintptr_t)dev,
	};

	nrfx_gpiote_input_pin_config_t gpiote_cfg_assert = {
		.p_pull_config = &p_data->pull_cfg,
		.p_trigger_config = &rising_trigger_config,
		.p_handler_config = NULL,
	};

	nrfx_gpiote_input_pin_config_t gpiote_cfg_deassert = {
		.p_pull_config = &p_data->pull_cfg,
		.p_trigger_config = &falling_trigger_config,
		.p_handler_config = &handler_config,
	};

	if (p_data->pulse_type == PULSE_MEAS_PULSE_NEGATIVE) {
		gpiote_cfg_assert.p_trigger_config = &falling_trigger_config;
		gpiote_cfg_deassert.p_trigger_config = &rising_trigger_config;
	} else if (p_data->pulse_type != PULSE_MEAS_PULSE_POSITIVE) {
		return -EINVAL;
	}

	err = nrfx_gpiote_input_configure(p_data->gpiote, p_cfg->assert_pin, &gpiote_cfg_assert);
	if (err < 0) {
		return err;
	}

	err = nrfx_gpiote_input_configure(p_data->gpiote, p_cfg->deassert_pin,
					  &gpiote_cfg_deassert);
	if (err < 0) {
		return err;
	}

	p_data->gpiote_ready = true;

	return 0;
}

static void timer_uninit(struct pulse_meas_drv_data *p_data)
{
	if (!p_data->timer_ready) {
		return;
	}

	nrfx_timer_disable(&p_data->timer);
	nrfx_timer_uninit(&p_data->timer);
	p_data->timer_ready = false;
}

static int timer_init(const struct device *dev)
{
	struct pulse_meas_drv_data *p_data = dev->data;
	static const nrfx_timer_config_t timer_cfg = {
		.frequency = PULSE_MEAS_RESOLUTION,
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL,
	};
	int rc;

	timer_uninit(p_data);

	rc = nrfx_timer_init(&p_data->timer, &timer_cfg, NULL);
	if (rc < 0) {
		return rc;
	}

	p_data->timer_frequency = timer_cfg.frequency;
	p_data->timer_ready = true;
	nrfx_timer_clear(&p_data->timer);

	return 0;
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

	ret = gpiote_configure(dev);
	if (ret < 0) {
		return ret;
	}

	ret = timer_init(dev);
	if (ret < 0) {
		return ret;
	}

	return gppi_init(dev);
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
#if IS_ENABLED(CONFIG_PULSE_MEAS_USE_HFCLK)
	int ret;
#endif

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
		p_data->state = PULSE_MEAS_STATE_IDLE;
		return ret;
	}
#endif

	nrfx_timer_clear(&p_data->timer);
	nrfx_timer_enable(&p_data->timer);
	gppi_enable(dev);
	gpiote_enable(dev);

	return 0;
}

int pulse_meas_stop(const struct device *dev, bool immediate)
{
	struct pulse_meas_drv_data *p_data = dev->data;

	if (immediate) {
		struct pulse_meas_block *block = p_data->curr_block;

		p_data->state = PULSE_MEAS_STATE_IDLE;
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

static bool measurement_active(struct pulse_meas_drv_data *p_data)
{
	return p_data->state == PULSE_MEAS_STATE_RUNNING ||
	       p_data->state == PULSE_MEAS_STATE_STOPPING;
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
#define GPIOTE_PHANDLE(instance, gpio)                                                             \
	DT_PROP(DT_GPIO_CTLR(DT_DRV_INST(instance), gpio), gpiote_instance)
#define TIMER_PHANDLE(instance) DT_INST_PHANDLE(instance, timer_instance)

#define GPIO_PORT_NUMBER(instance, gpio) DT_PROP(DT_GPIO_CTLR(DT_DRV_INST(instance), gpio), port)
#define GPIO_PIN_NUMBER(instance, gpio)	 DT_GPIO_PIN(DT_DRV_INST(instance), gpio)
#define ABSOLUTE_PIN_NUMBER(instance, gpio)                                                        \
	NRF_GPIO_PIN_MAP(GPIO_PORT_NUMBER(instance, gpio), GPIO_PIN_NUMBER(instance, gpio))

#define PULSE_MEAS_DEVICE(inst)                                                                    \
	BUILD_ASSERT(DT_SAME_NODE(GPIOTE_PHANDLE(inst, assert_gpios),                              \
				  GPIOTE_PHANDLE(inst, deassert_gpios)),                           \
		     "assert-gpio and deassert-gpio must use the same GPIOTE instance");           \
	static struct pulse_meas_drv_data drv_data_##inst = {                                      \
		.timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(TIMER_PHANDLE(inst))),                    \
		.gpiote = &GPIOTE_NRFX_INST_BY_NODE(GPIOTE_PHANDLE(inst, assert_gpios)),           \
	};                                                                                         \
	static const struct pulse_meas_drv_cfg drv_cfg_##inst = {                                  \
		.assert_pin = ABSOLUTE_PIN_NUMBER(inst, assert_gpios),                             \
		.deassert_pin = ABSOLUTE_PIN_NUMBER(inst, deassert_gpios),                         \
	};                                                                                         \
	static int pulse_meas_##inst##_init(const struct device *dev)                              \
	{                                                                                          \
		ARG_UNUSED(dev);                                                                   \
		COND_CODE_0(                                                                       \
			IS_ENABLED(CONFIG_GPIO),                                                   \
			(NRF_DT_IRQ_CONNECT(                                                       \
				 GPIOTE_PHANDLE(inst, assert_gpios), nrfx_gpiote_irq_handler,      \
				 &GPIOTE_NRFX_INST_BY_NODE(GPIOTE_PHANDLE(inst, assert_gpios)));), \
			())                                                                        \
		return 0;                                                                          \
	}                                                                                          \
	DEVICE_DT_INST_DEFINE(inst, pulse_meas_##inst##_init, NULL, &drv_data_##inst,              \
			      &drv_cfg_##inst, POST_KERNEL, CONFIG_PULSE_MEAS_INIT_PRIORITY,       \
			      NULL);

DT_INST_FOREACH_STATUS_OKAY(PULSE_MEAS_DEVICE)
