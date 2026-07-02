#define DT_DRV_COMPAT nordic_nrf_pulse_meas

#include <stdlib.h>
#include <soc.h>
#include <zephyr/kernel.h>
#include <helpers/nrfx_gppi.h>
#include <nrfx_timer.h>
#include <nrfx_gpiote.h>
#include <assert.h>
#include <zephyr/irq.h>
#include <zephyr/dt-bindings/gpio/nordic-nrf-gpio.h>
#include <nrf_pulse_meas.h>
#include <zephyr/drivers/gpio.h>

K_FIFO_DEFINE(nrf_pulse_meas_fifo);

struct nrf_pulse_meas_meas_block {
	void *fifo_reserved;
	uint32_t data[];
};

typedef enum {
	nrf_pulse_meas_idle,
	nrf_pulse_meas_running,
	nrf_pulse_meas_stopping,
	nrf_pulse_meas_error,
} nrf_pulse_meas_state_t;

struct nrf_pulse_meas_drv_data {
	nrfx_timer_t timer;
	nrfx_gpiote_t gpiote;
	nrfx_gppi_handle_t ppi_start;
	nrfx_gppi_handle_t ppi_end;
	struct nrf_pulse_meas_meas_block *cur_block;
	uint32_t meas_count;
	nrf_pulse_meas_pulse_t pulse_type;
	uint32_t meas_num;
	nrf_pulse_meas_mode_t mode;
	nrf_gpio_pin_pull_t pull_cfg;
	void (*user_handler)(void * context);
	void *user_context;
	struct k_mem_slab *mem_slab;
	volatile nrf_pulse_meas_state_t state;
	atomic_t pending_series;
};

struct nrf_pulse_meas_drv_cfg {
    uint32_t rising_edge_pin;
    uint32_t falling_edge_pin;
};

static void gpiote_enable(const struct device *dev)
{
	const struct nrf_pulse_meas_drv_cfg *p_cfg = dev->config;
	struct nrf_pulse_meas_drv_data *p_data = dev->data;

	nrfx_gpiote_trigger_enable(&p_data->gpiote,
				   p_cfg->falling_edge_pin,
				   p_data->pulse_type == nrf_pulse_meas_pulse_positive);
	nrfx_gpiote_trigger_enable(&p_data->gpiote,
				   p_cfg->rising_edge_pin,
				   p_data->pulse_type == nrf_pulse_meas_pulse_negative);
}

static void gpiote_disable(const struct device *dev)
{
	const struct nrf_pulse_meas_drv_cfg *p_cfg = dev->config;
	struct nrf_pulse_meas_drv_data *p_data = dev->data;

	nrfx_gpiote_trigger_disable(&p_data->gpiote,
				    p_cfg->falling_edge_pin);
	nrfx_gpiote_trigger_disable(&p_data->gpiote,
				    p_cfg->rising_edge_pin);
}

static void gppi_disable(const struct device *dev)
{
	struct nrf_pulse_meas_drv_data *p_data = dev->data;

	nrfx_gppi_conn_disable(p_data->ppi_start);
	nrfx_gppi_conn_disable(p_data->ppi_end);
}

static void meas_stop(const struct device *dev)
{
	struct nrf_pulse_meas_drv_data *p_data = dev->data;

	gppi_disable(dev);
	gpiote_disable(dev);
	nrfx_timer_disable(&p_data->timer);
	p_data->state = nrf_pulse_meas_idle;
}

static void gpiote_handler(nrfx_gpiote_pin_t pin,
			   nrfx_gpiote_trigger_t action, void * p_context)
{
	const struct device *dev = (const struct device *)p_context;
	struct nrf_pulse_meas_drv_data *p_data = dev->data;
	uint32_t idx = p_data->meas_count;
	struct nrf_pulse_meas_meas_block *block = p_data->cur_block;

	uint32_t timer_cc_val = nrf_timer_cc_get(p_data->timer.p_reg, NRF_TIMER_CC_CHANNEL0);
	block->data[idx] = (timer_cc_val >> 4);
	p_data->meas_count = idx + 1;
	if (p_data->meas_count >= p_data->meas_num) {
		k_fifo_put(&nrf_pulse_meas_fifo, block);
		atomic_inc(&p_data->pending_series);
		p_data->meas_count = 0;
		if (p_data->state == nrf_pulse_meas_stopping) {
			meas_stop(dev);
		} else {
			if (k_mem_slab_alloc(p_data->mem_slab, (void **)&p_data->cur_block,
					     K_NO_WAIT) < 0) {
				meas_stop(dev);
				//p_data->state = nrf_pulse_meas_error;
			}
		}
		if (p_data->user_handler) {
			p_data->user_handler(p_data->user_context);
		}
	}
}

static void gppi_enable(const struct device *dev)
{
	struct nrf_pulse_meas_drv_data *p_data = dev->data;

	nrfx_gppi_conn_enable(p_data->ppi_start);
	nrfx_gppi_conn_enable(p_data->ppi_end);
}

static int gppi_init(const struct device *dev)
{
	const struct nrf_pulse_meas_drv_cfg *p_cfg = dev->config;
	uint32_t evt_to_start;
	uint32_t evt_to_cpt;
	uint32_t tsk;
	int ret;
	struct nrf_pulse_meas_drv_data *p_data = dev->data;

	if (p_data->pulse_type == nrf_pulse_meas_pulse_positive) {
		evt_to_start =
			nrfx_gpiote_in_event_address_get(&p_data->gpiote, p_cfg->rising_edge_pin);
		evt_to_cpt =
			nrfx_gpiote_in_event_address_get(&p_data->gpiote, p_cfg->falling_edge_pin);
	}
	else if (p_data->pulse_type == nrf_pulse_meas_pulse_negative) {
		evt_to_cpt =
			nrfx_gpiote_in_event_address_get(&p_data->gpiote, p_cfg->rising_edge_pin);
		evt_to_start =
			nrfx_gpiote_in_event_address_get(&p_data->gpiote, p_cfg->falling_edge_pin);
	} else {
		return -EINVAL;
	}

	tsk = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CLEAR);
	ret = nrfx_gppi_conn_alloc(evt_to_start, tsk, &p_data->ppi_start);
	if (ret < 0) {
		return ret;
	}

	tsk = nrfx_timer_task_address_get(&p_data->timer, NRF_TIMER_TASK_CAPTURE0);
	ret = nrfx_gppi_conn_alloc(evt_to_cpt, tsk, &p_data->ppi_end);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

static int timer_init(const struct device *dev)
{
	struct nrf_pulse_meas_drv_data *p_data = dev->data;
	nrfx_timer_config_t timer_cfg = {
		.frequency = NRFX_MHZ_TO_HZ(16),
		.mode = NRF_TIMER_MODE_TIMER,
		.bit_width = NRF_TIMER_BIT_WIDTH_32,
		.interrupt_priority = NRFX_TIMER_DEFAULT_CONFIG_IRQ_PRIORITY,
		.p_context = NULL
	};

    return nrfx_timer_init(&p_data->timer, &timer_cfg, NULL);
}

int gpiote_configure(const struct device *dev)
{
    struct nrf_pulse_meas_drv_data *p_data = dev->data;
	const struct nrf_pulse_meas_drv_cfg *p_cfg = dev->config;
	uint8_t rising_edge_in_ch, falling_edge_in_ch;
	int err;

	(void)nrfx_gpiote_init(&p_data->gpiote, 7);
	err = nrfx_gpiote_channel_alloc(&p_data->gpiote, &rising_edge_in_ch);
	if (err < 0) {
		printk("Failed to allocate in_channel, error: 0x%08X", err);
	}

	err = nrfx_gpiote_channel_alloc(&p_data->gpiote, &falling_edge_in_ch);
	if (err < 0) {
		printk("Failed to allocate in_channel, error: 0x%08X", err);
	}

	const nrfx_gpiote_trigger_config_t rising_trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_LOTOHI,
		.p_in_channel = &rising_edge_in_ch,
	};

	const nrfx_gpiote_trigger_config_t falling_trigger_config = {
		.trigger = NRFX_GPIOTE_TRIGGER_HITOLO,
		.p_in_channel = &falling_edge_in_ch,
	};
	static nrfx_gpiote_handler_config_t handler_config= {
		.handler = gpiote_handler,
	};
	handler_config.p_context = (void*) dev;
	nrfx_gpiote_input_pin_config_t gpiote_cfg_rising = {
		.p_pull_config = &p_data->pull_cfg,
		.p_trigger_config = &rising_trigger_config,
		.p_handler_config = NULL
	};
	nrfx_gpiote_input_pin_config_t gpiote_cfg_falling = {
		.p_pull_config = &p_data->pull_cfg,
		.p_trigger_config = &falling_trigger_config,
		.p_handler_config = NULL
	};

	if(p_data->pulse_type == nrf_pulse_meas_pulse_positive) {
		gpiote_cfg_falling.p_handler_config = &handler_config;
	} else {
		gpiote_cfg_rising.p_handler_config = &handler_config;
	}

	err = nrfx_gpiote_input_configure(&p_data->gpiote, p_cfg->rising_edge_pin,
					  &gpiote_cfg_rising);
	if (err < 0) {
		return err;
	}

	err = nrfx_gpiote_input_configure(&p_data->gpiote, p_cfg->falling_edge_pin,
					  &gpiote_cfg_falling);
	return err;
}

static int nrf_pulse_meas_configure(const struct device *dev,
					 struct api_nrf_pulse_meas_config * cfg)
{
	struct nrf_pulse_meas_drv_data *p_data = dev->data;
	int ret;

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

	ret = gppi_init(dev);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int nrf_pulse_meas_start(const struct device *dev, struct k_mem_slab * slab)
{
	struct nrf_pulse_meas_drv_data *p_data = dev->data;
	struct nrf_pulse_meas_meas_block *block;
	int ret;

	p_data->meas_count = 0;
	p_data->mem_slab = slab;
	ret = k_mem_slab_alloc(p_data->mem_slab, (void **)&block, K_NO_WAIT);
	if (ret < 0) {
		printk("Failed to allocate TX block: %d\n", ret);
			return -ENOMEM;
	}
	p_data->cur_block = block;
	nrfx_timer_enable(&p_data->timer);
	nrfx_timer_clear(&p_data->timer);
	gppi_enable(dev);
	gpiote_enable(dev);
	if (p_data->mode == nrf_pulse_meas_mode_one_shot) {
		p_data->state = nrf_pulse_meas_stopping;
	} else {
		p_data->state = nrf_pulse_meas_running;
	}
	return 0;
}

static int nrf_pulse_meas_stop(const struct device *dev, bool wait_until_series_finished)
{
	struct nrf_pulse_meas_drv_data *p_data = dev->data;
	struct nrf_pulse_meas_meas_block *block = p_data->cur_block;

	if (!wait_until_series_finished) {
		meas_stop(dev);
		k_mem_slab_free(p_data->mem_slab, block);
		return 0;
	}

	p_data->state = nrf_pulse_meas_stopping;
	return 0;
}

static int nrf_pulse_meas_meas_get(const struct device *dev, uint32_t ** data)
{
	struct nrf_pulse_meas_meas_block *block;
	struct nrf_pulse_meas_drv_data *p_data = dev->data;

	block = k_fifo_get(&nrf_pulse_meas_fifo, K_NO_WAIT);
	if (!block) {
		if (k_mem_slab_num_used_get(p_data->mem_slab) > 0) {
			return -EAGAIN;
		}
		return -EIO;
	}
	atomic_dec(&p_data->pending_series);
	*data = block->data;
	return 0;
}

static void nrf_pulse_meas_meas_free(const struct device *dev, uint32_t *data)
{
	struct nrf_pulse_meas_drv_data *p_data = dev->data;

	struct nrf_pulse_meas_meas_block *block =
		CONTAINER_OF((void *)data, struct nrf_pulse_meas_meas_block, data);

	k_mem_slab_free(p_data->mem_slab, block);
}

static uint32_t nrf_pulse_meas_meas_pending(const struct device *dev)
{
    struct nrf_pulse_meas_drv_data *p_data = dev->data;
    return atomic_get(&p_data->pending_series);
}

static const struct nrf_pulse_meas_driver_api nrf_pulse_meas_nrf_drv_api = {
	.configure = nrf_pulse_meas_configure,
	.start = nrf_pulse_meas_start,
	.stop = nrf_pulse_meas_stop,
	.meas_get = nrf_pulse_meas_meas_get,
	.meas_free = nrf_pulse_meas_meas_free,
	.meas_pending = nrf_pulse_meas_meas_pending,
};

#define GPIOTE_PHANDLE(instance) DT_INST_PHANDLE(instance, gpiote_instance)
#define TIMER_PHANDLE(instance) DT_INST_PHANDLE(instance, timer_instance)
#define RISING_PIN_DT(instance) DT_INST_PROP(instance, rising_pin)
#define FALLING_PIN_DT(instance) DT_INST_PROP(instance, falling_pin)

#define GPIOTE_IRQ_HANDLER_CONNECT(node_id)							   \
	NRF_DT_IRQ_CONNECT(									   \
		node_id,									   \
		gpio_nrfx_gpiote_irq_handler,							   \
		&GPIOTE_NRFX_INST_BY_NODE(node_id)						   \

#define NRF_PULSE_MEAS_DEVICE(inst)								\
static struct nrf_pulse_meas_drv_data drv_data_##inst = {				\
	.timer = NRFX_TIMER_INSTANCE(DT_REG_ADDR(TIMER_PHANDLE(inst))),			\
	.gpiote = NRFX_GPIOTE_INSTANCE(DT_REG_ADDR(GPIOTE_PHANDLE(inst))),		\
};											\
static const struct nrf_pulse_meas_drv_cfg drv_cfg_##inst = {				\
	.rising_edge_pin = RISING_PIN_DT(inst),						\
	.falling_edge_pin = FALLING_PIN_DT(inst),					\
};											\
static int nrf_pulse_meas_##inst##_init(const struct device *dev)			\
{											\
	IRQ_CONNECT(DT_IRQN(TIMER_PHANDLE(inst)), IRQ_PRIO_LOWEST,			\
                nrfx_timer_irq_handler, &drv_data_##inst.timer, 0);			\
	IRQ_CONNECT(DT_IRQN(GPIOTE_PHANDLE(inst)), IRQ_PRIO_LOWEST,			\
                nrfx_gpiote_irq_handler, &drv_data_##inst.gpiote, 0);			\
    return 0;										\
}											\
DEVICE_DT_INST_DEFINE(inst, nrf_pulse_meas_##inst##_init, NULL, &drv_data_##inst,	\
					  &drv_cfg_##inst, POST_KERNEL, 1,		\
					  &nrf_pulse_meas_nrf_drv_api);

NRF_PULSE_MEAS_DEVICE(0)

//For multi-instance use this:
//DT_INST_FOREACH_STATUS_OKAY(NRF_PULSE_MEAS_DEVICE)
