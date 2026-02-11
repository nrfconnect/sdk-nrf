/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <ppi_seq/ppi_seq_i2c_spi.h>
#include <zephyr/drivers/pinctrl.h>
#include <hal/nrf_gpio.h>
#include <soc.h>

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_ppi_seq_i2c)
#define PPI_SEQ_I2C 1
#endif

#if DT_HAS_COMPAT_STATUS_OKAY(nordic_ppi_seq_spi)
#define PPI_SEQ_SPI 1
#endif

#define IS_SPI(dev)                                                                                \
	(IS_ENABLED(PPI_SEQ_SPI) &&                                                                \
	 (!IS_ENABLED(PPI_SEQ_I2C) || (((const struct ppi_seq_i2c_spi_data *)dev->config)->spi)))

union ppi_seq_i2c_spi_dev {
	nrfx_spim_t *spim;
	nrfx_twim_t *twim;
	void *generic;
};

struct ppi_seq_i2c_config {
	const nrfx_twim_config_t *twim;
	uint32_t frequency;
	struct k_sem *sem;
};

union ppi_seq_i2c_spi_dev_config {
	const nrfx_spim_config_t *spim;
	const struct ppi_seq_i2c_config *twim;
	const void *generic;
};

/** I2C/SPI PPI Sequencer structure. */
struct ppi_seq_i2c_spi_data {
	/** PPI sequencer. */
	struct ppi_seq seq;

	/** Placeholder for storing the current job. */
	struct ppi_seq_i2c_spi_job job;

	const struct device *dev;

	int result;

	/** Flag for internal use. */
	bool primary_buf;

	/** Flag indicating that SPI is used. */
	bool spi;
};

struct ppi_seq_i2c_spi_config {
	struct ppi_seq_config ppi_seq_config;

	const struct pinctrl_dev_config *pcfg;

	union ppi_seq_i2c_spi_dev_config dev_config;

	/** Pointer to the SPIM/TWIM driver instance. Need to be initialize outside of
	 * that module.
	 */
	union ppi_seq_i2c_spi_dev nrfx_dev;

	void *reg;
	NRF_TIMER_Type *timer_notifier_reg;
	uint32_t end_event;

	bool spi;
};

static void ppi_seq_i2c_spi_internal_cb(struct ppi_seq *seq, bool last)
{
	struct ppi_seq_i2c_spi_data *data = CONTAINER_OF(seq, struct ppi_seq_i2c_spi_data, seq);
	const struct device *dev = data->dev;
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	struct ppi_seq_i2c_spi_job *job = &data->job;
	struct ppi_seq_i2c_spi_batch batch;

	batch.batch_cnt = job->batch_cnt;
	if (IS_SPI(dev)) {
		batch.desc.spim = job->desc.spim;
		if (data->primary_buf == false) {
			if (job->tx_postinc) {
				batch.desc.spim.p_tx_buffer = job->tx_second_buf;
			}
			batch.desc.spim.p_rx_buffer = job->rx_second_buf;
		}
		if (last == true) {
			nrfx_spim_abort(config->nrfx_dev.spim);
		} else {
			config->nrfx_dev.spim->p_reg->DMA.RX.PTR =
				(uint32_t)(data->primary_buf ? job->rx_second_buf
							     : job->desc.spim.p_rx_buffer);
			if (job->tx_postinc) {
				config->nrfx_dev.spim->p_reg->DMA.TX.PTR =
					(uint32_t)(data->primary_buf ? job->tx_second_buf
								     : job->desc.spim.p_tx_buffer);
			}
		}
	} else {
		batch.desc.twim = job->desc.twim;
		if (data->primary_buf == false) {
			if (job->tx_postinc) {
				batch.desc.twim.p_primary_buf = job->tx_second_buf;
			}
			batch.desc.twim.p_secondary_buf = job->rx_second_buf;
		}
		if (last == true) {
			nrfx_twim_disable(config->nrfx_dev.twim);
		} else {
			config->nrfx_dev.twim->p_twim->DMA.RX.PTR =
				(uint32_t)(data->primary_buf ? job->rx_second_buf
							     : job->desc.twim.p_secondary_buf);
			if (job->tx_postinc) {
				config->nrfx_dev.twim->p_twim->DMA.TX.PTR =
					(uint32_t)(data->primary_buf
							   ? job->tx_second_buf
							   : job->desc.twim.p_primary_buf);
			}
		}
	}

	data->primary_buf = !data->primary_buf;
	data->job.cb(data->dev, &batch, last, data->job.user_data);
}

static int ppi_seq_i2c_spi_init(const struct device *dev)
{
	struct ppi_seq_i2c_spi_data *data = dev->data;
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	NRF_TIMER_Type *notifier_reg = config->timer_notifier_reg;

	if (notifier_reg) {
		config->ppi_seq_config.notifier->type = PPI_SEQ_NOTIFIER_NRFX_TIMER;
		config->ppi_seq_config.notifier->nrfx_timer.timer.p_reg = notifier_reg;
		config->ppi_seq_config.notifier->nrfx_timer.end_seq_event = config->end_event;
	} else {
		config->ppi_seq_config.notifier->type = PPI_SEQ_NOTIFIER_SYS_TIMER;
	}

	data->dev = dev;
	(void)pinctrl_apply_state(config->pcfg, PINCTRL_STATE_DEFAULT);

	int rv = ppi_seq_init(&data->seq, &config->ppi_seq_config);
	return rv;
}

uint32_t get_transfer_time(const struct device *dev, struct ppi_seq_i2c_spi_job *job)
{
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	uint32_t bytes;
	uint32_t frequency;

	if (IS_SPI(dev)) {
		bytes = MAX(job->desc.spim.tx_length, job->desc.spim.rx_length);
		frequency = config->dev_config.spim->frequency;
	} else {
		bytes = job->desc.twim.primary_length + job->desc.twim.secondary_length;
		frequency = config->dev_config.twim->frequency;
	}

	return (uint32_t)(((uint64_t)(8 * bytes) * 1000000) / frequency);
}

int ppi_seq_i2c_spi_start(const struct device *dev, size_t period, struct ppi_seq_i2c_spi_job *job)
{
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	struct ppi_seq_i2c_spi_data *data = dev->data;
	uint32_t batch_cnt;
	int rv;

	if (config->ppi_seq_config.extra_ops_count > 0) {
		batch_cnt = job->batch_cnt / (config->ppi_seq_config.extra_ops_count + 1);
	} else {
		batch_cnt = job->batch_cnt;
	}

	data->primary_buf = true;
	data->job = *job;

	if (config->ppi_seq_config.notifier->type == PPI_SEQ_NOTIFIER_SYS_TIMER) {
		uint32_t xfer_time = get_transfer_time(dev, job);

		config->ppi_seq_config.notifier->sys_timer.offset = xfer_time;
	}

	if (IS_SPI(dev)) {
		uint32_t flags = (job->tx_postinc ? NRFX_SPIM_FLAG_TX_POSTINC : 0) |
				 NRFX_SPIM_FLAG_RX_POSTINC | NRFX_SPIM_FLAG_NO_XFER_EVT_HANDLER |
				 NRFX_SPIM_FLAG_REPEATED_XFER | NRFX_SPIM_FLAG_HOLD_XFER;

		rv = nrfx_spim_xfer(config->nrfx_dev.spim, &data->job.desc.spim, flags);
		if (rv < 0) {
			return rv;
		}
	} else {
		uint32_t flags = (job->tx_postinc ? NRFX_TWIM_FLAG_TX_POSTINC : 0) |
				 NRFX_TWIM_FLAG_RX_POSTINC | NRFX_TWIM_FLAG_NO_XFER_EVT_HANDLER |
				 NRFX_TWIM_FLAG_REPEATED_XFER | NRFX_TWIM_FLAG_HOLD_XFER;

		nrfx_twim_enable(config->nrfx_dev.twim);
		rv = nrfx_twim_xfer(config->nrfx_dev.twim, &job->desc.twim, flags);
		if (rv < 0) {
			return rv;
		}
	}

	return ppi_seq_start(&data->seq, period, batch_cnt, job->repeat);
}

int ppi_seq_i2c_spi_stop(const struct device *dev, bool immediate)
{
	struct ppi_seq_i2c_spi_data *data = dev->data;

	return ppi_seq_stop(&data->seq, immediate);
}

int ppi_seq_i2c_spi_xfer(const struct device *dev, union ppi_seq_i2c_spi_xfer_desc *desc)
{
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	struct ppi_seq_i2c_spi_data *data = dev->data;
	int rv;

	if (IS_SPI(dev)) {
		return nrfx_spim_xfer(config->nrfx_dev.spim, &desc->spim, 0);
	}

	nrfx_twim_enable(config->nrfx_dev.twim);
	rv = nrfx_twim_xfer(config->nrfx_dev.twim, &desc->twim, 0);
	if (rv < 0) {
		return rv;
	}

	rv = k_sem_take(config->dev_config.twim->sem, K_MSEC(1000));
	if (rv < 0) {
		return rv;
	}
	nrfx_twim_disable(config->nrfx_dev.twim);

	return data->result;
}

#ifdef PPI_SEQ_SPI
static int ppi_seq_spi_init(const struct device *dev)
{
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	uint32_t ss_pin = config->dev_config.spim->ss_pin;
	uint32_t ss_dur = config->dev_config.spim->ss_duration;
	nrf_spim_csn_pol_t ss_pol = config->dev_config.spim->ss_active_high ? NRF_SPIM_CSN_POL_HIGH
									    : NRF_SPIM_CSN_POL_LOW;
	int rv;

	rv = ppi_seq_i2c_spi_init(dev);
	if (rv < 0) {
		return rv;
	}

	config->nrfx_dev.spim->p_reg = (NRF_SPIM_Type *)config->reg;

	/* Manually configure HW slave select. */
	if (config->dev_config.spim->ss_active_high) {
		nrf_gpio_pin_clear(ss_pin);
	} else {
		nrf_gpio_pin_set(ss_pin);
	}
	nrf_gpio_cfg_output(ss_pin);
	nrf_spim_csn_configure(config->nrfx_dev.spim->p_reg, ss_pin, ss_pol, ss_dur);

	return nrfx_spim_init(config->nrfx_dev.spim, config->dev_config.spim, NULL, NULL);
}
#endif

#ifdef PPI_SEQ_I2C

static void twim_event_handler(nrfx_twim_event_t const *event, void *context)
{
	const struct device *dev = context;
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	struct ppi_seq_i2c_spi_data *data = dev->data;

	data->result = (event->type == NRFX_TWIM_EVT_DONE) ? 0 : -EIO;
	if (data->seq.repeat == 0) {
		k_sem_give(config->dev_config.twim->sem);
	}
}

static int ppi_seq_i2c_init(const struct device *dev)
{
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	int rv;

	rv = ppi_seq_i2c_spi_init(dev);
	if (rv < 0) {
		return rv;
	}

	config->nrfx_dev.twim->p_twim = (NRF_TWIM_Type *)config->reg;
	k_sem_init(config->dev_config.twim->sem, 0, 1);

	return nrfx_twim_init(config->nrfx_dev.twim, config->dev_config.twim->twim,
			      twim_event_handler, (void *)dev);
}
#endif

#ifdef CONFIG_DEVICE_DEINIT_SUPPORT

#ifdef PPI_SEQ_SPI
static int ppi_seq_spi_deinit(const struct device *dev)
{
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	struct ppi_seq_i2c_spi_data *data = dev->data;

	nrfx_spim_uninit(config->nrfx_dev.spim);
	ppi_seq_uninit(&data->seq);
	return 0;
}
#endif

#ifdef PPI_SEQ_I2C
static int ppi_seq_i2c_deinit(const struct device *dev)
{
	const struct ppi_seq_i2c_spi_config *config = dev->config;
	struct ppi_seq_i2c_spi_data *data = dev->data;

	nrfx_twim_uninit(config->nrfx_dev.twim);
	ppi_seq_uninit(&data->seq);
	return 0;
}
#endif
#endif

#define IS_PPI_SEQ_NOTIFIER_NRFX_TIMER(idx, _event)

#define IS_PPI_SEQ_CONFIG(idx, _notifier, _task, _extra_ops, _extra_ops_len)                       \
	{                                                                                          \
		.notifier = _notifier,                                                             \
		IF_ENABLED(DT_INST_NODE_HAS_PROP(idx, rtc),                                        \
			   (.rtc_reg = (NRF_RTC_Type *)DT_REG_ADDR(DT_INST_PROP(idx, rtc)),))      \
			IF_ENABLED(DT_INST_NODE_HAS_PROP(idx, timer),                              \
				   (.timer_reg = (NRF_TIMER_Type *)DT_REG_ADDR(                    \
					    DT_INST_PROP(idx, timer)),))                           \
				.callback = ppi_seq_i2c_spi_internal_cb,                           \
		.task = _task,                                                                     \
		.skip_gppi = !IS_ENABLED(CONFIG_NRFX_GPPI),                                        \
		.extra_ops_count = _extra_ops_len,                                                 \
		.extra_ops = (_extra_ops_len > 0) ? _extra_ops : NULL,                             \
	}

#define PPI_SEQ_COMMON_IRQ_INIT(tag, idx)                                                          \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(idx, rtc),                                                \
		   (IRQ_CONNECT(DT_IRQN(DT_INST_PROP(idx, rtc)),                                   \
				DT_IRQ_BY_IDX(DT_INST_PROP(idx, rtc), 0, priority),                \
				ppi_seq_rtc_irq_handler, &ppi_seq_data_##tag##idx.seq, 0);         \
		    irq_enable(DT_IRQN(DT_INST_PROP(idx, rtc)));))                                 \
	IF_ENABLED(DT_INST_NODE_HAS_PROP(idx, timer_notifier),                                     \
		   (IRQ_CONNECT(DT_IRQN(DT_INST_PROP(idx, timer_notifier)),                        \
				DT_IRQ_BY_IDX(DT_INST_PROP(idx, timer_notifier), 0, priority),     \
				nrfx_timer_irq_handler,                                            \
				&ppi_seq_notifier_##tag##idx.nrfx_timer.timer, 0);))

#define PPI_SEQ_I2C_TASK(idx)                                                                      \
	(uint32_t)&((NRF_TWIM_Type *)DT_INST_REG_ADDR(idx))->TASKS_DMA.TX.START

#define PPI_SEQ_I2C_END_EVENT(idx)                                                                 \
	(uint32_t)&((NRF_TWIM_Type *)DT_INST_REG_ADDR(idx))->EVENTS_DMA.RX.END

#define PPI_SEQ_SPI_TASK(idx) (uint32_t)&((NRF_SPIM_Type *)DT_INST_REG_ADDR(idx))->TASKS_START

#define PPI_SEQ_SPI_END_EVENT(idx) (uint32_t)&((NRF_SPIM_Type *)DT_INST_REG_ADDR(idx))->EVENTS_END

#define PPI_SEQ_SPI_CONFIG_MODE(idx)                                                               \
	(DT_INST_PROP(idx, spi_cpha)                                                               \
		 ? (DT_INST_PROP(idx, spi_cpol) ? NRF_SPIM_MODE_0 : NRF_SPIM_MODE_2)               \
		 : (DT_INST_PROP(idx, spi_cpol) ? NRF_SPIM_MODE_1 : NRF_SPIM_MODE_3))

#define PPI_SEQ_SPI_CONFIG_SS_DURATION(idx)                                                        \
	(DT_INST_PROP(idx, spi_cs_setup_delay_ns) /                                                \
	 (1000000000ULL / NRF_PERIPH_GET_FREQUENCY(DT_DRV_INST(idx))))

#define PPI_SEQ_SPI_CONFIG(idx)                                                                    \
	{                                                                                          \
		.ss_pin =                                                                          \
			NRF_GPIO_PIN_MAP(DT_PROP(DT_GPIO_CTLR(DT_DRV_INST(idx), cs_gpios), port),  \
					 DT_GPIO_PIN(DT_DRV_INST(idx), cs_gpios)),                 \
		.bit_order = DT_INST_PROP(idx, spi_lsb_first) ? NRF_SPIM_BIT_ORDER_LSB_FIRST       \
							      : NRF_SPIM_BIT_ORDER_MSB_FIRST,      \
		.rx_delay = DT_INST_PROP(idx, rx_delay),                                           \
		.frequency = DT_INST_PROP(idx, frequency),                                         \
		.mode = PPI_SEQ_SPI_CONFIG_MODE(idx),                                              \
		.use_hw_ss = true,                                                                 \
		.ss_duration = PPI_SEQ_SPI_CONFIG_SS_DURATION(idx),                                \
		.ss_active_high = DT_INST_PROP(idx, spi_cs_high),                                  \
		.orc = DT_INST_PROP(idx, overrun_character),                                       \
		.skip_psel_cfg = true,                                                             \
		.skip_gpio_cfg = true,                                                             \
	}

#define PPI_SEQ_I2C_FREQ(idx)                                                                      \
	(DT_INST_PROP(idx, clock_frequency) < 250000)	? NRF_TWIM_FREQ_100K                       \
	: (DT_INST_PROP(idx, clock_frequency) < 400000) ? NRF_TWIM_FREQ_250K                       \
							: NRF_TWIM_FREQ_400K

#define PPI_SEQ_TIMER_NOTIFIER_REG(idx)                                                            \
	COND_CODE_1(DT_INST_NODE_HAS_PROP(idx, timer_notifier),                                    \
		    ((NRF_TIMER_Type *)DT_REG_ADDR(DT_INST_PROP(idx, timer_notifier))), (NULL))

#define PPI_SEQ_I2C_CONFIG(idx)                                                                    \
	{                                                                                          \
		.frequency = PPI_SEQ_I2C_FREQ(idx),                                                \
		.skip_psel_cfg = true,                                                             \
		.skip_gpio_cfg = true,                                                             \
	}

#define PPI_SEQ_EXTRA_OP(node, prop, idx, _task)                                                   \
	{.task = _task, .offset = DT_PROP_BY_IDX(node, prop, idx)}

#define PPI_SEQ_EXTRA_OPS_INIT(idx, _task)                                                         \
	{IF_ENABLED(DT_INST_NODE_HAS_PROP(idx, extra_transfers),                                   \
		    (DT_INST_FOREACH_PROP_ELEM_SEP_VARGS(idx, extra_transfers, PPI_SEQ_EXTRA_OP,   \
							 (,), _task)))}

#define PPI_SEQ_EXTRA_OPS_LEN(idx) DT_PROP_LEN_OR(DT_DRV_INST(idx), extra_transfers, 0)

/* Depends on GPPI being ready for use. */
#define INIT_PRIO                                                                                  \
	COND_CODE_1(IS_ENABLED(CONFIG_IRONSIDE_SE_CALL),                                           \
		    (UTIL_INC(UTIL_INC(CONFIG_IRONSIDE_SE_CALL_INIT_PRIORITY))), (0))

#define PPI_SEQ_COMMON_DEFINE(tag, is_spi, idx, _task, _end_event, _nrfx_dev, _nrfx_cfg,           \
			      _extra_init, _deinit_fn)                                             \
	PINCTRL_DT_DEFINE(DT_DRV_INST(idx));                                                       \
	static struct ppi_seq_notifier ppi_seq_notifier_##tag##idx;                                \
	static struct ppi_seq_i2c_spi_data ppi_seq_data_##tag##idx;                                \
	static const struct ppi_seq_extra_op ppi_seq_extra_ops_##tag##idx[] =                      \
		PPI_SEQ_EXTRA_OPS_INIT(idx, _task);                                                \
	static const struct ppi_seq_i2c_spi_config ppi_seq_config_##tag##idx = {                   \
		.ppi_seq_config = IS_PPI_SEQ_CONFIG(idx, &ppi_seq_notifier_##tag##idx, _task,      \
						    ppi_seq_extra_ops_##tag##idx,                  \
						    PPI_SEQ_EXTRA_OPS_LEN(idx)),                   \
		.dev_config =                                                                      \
			{                                                                          \
				.generic = (void *)_nrfx_cfg,                                      \
			},                                                                         \
		.nrfx_dev =                                                                        \
			{                                                                          \
				.generic = (void *)_nrfx_dev,                                      \
			},                                                                         \
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(DT_DRV_INST(idx)),                               \
		.spi = is_spi,                                                                     \
		.end_event = _end_event,                                                           \
		.reg = (void *)DT_INST_REG_ADDR(idx),                                              \
		.timer_notifier_reg = PPI_SEQ_TIMER_NOTIFIER_REG(idx)};                            \
	static int ppi_seq_init_##tag##idx(const struct device *dev)                               \
	{                                                                                          \
		PPI_SEQ_COMMON_IRQ_INIT(tag, idx)                                                  \
		COND_CODE_0(_extra_init, (), (_extra_init(dev);))                                  \
		return ppi_seq_##tag##_init(dev);                                                  \
	}                                                                                          \
	DEVICE_DT_INST_DEINIT_DEFINE(idx, ppi_seq_init_##tag##idx, _deinit_fn, NULL,               \
				     &ppi_seq_data_##tag##idx, &ppi_seq_config_##tag##idx,         \
				     POST_KERNEL, INIT_PRIO, NULL);

#define PPI_SEQ_I2C_DEFINE(idx)                                                                    \
	static struct k_sem ppi_seq_twim_sem_##idx;                                                \
	static nrfx_twim_t ppi_seq_twim_##idx;                                                     \
	static const nrfx_twim_config_t ppi_seq_twim_nrfx_config_##idx = PPI_SEQ_I2C_CONFIG(idx);  \
	static const struct ppi_seq_i2c_config ppi_seq_i2c_dev_config_##idx = {                    \
		.twim = &ppi_seq_twim_nrfx_config_##idx,                                           \
		.frequency = DT_INST_PROP(idx, clock_frequency),                                   \
		.sem = &ppi_seq_twim_sem_##idx};                                                   \
	static void ppi_seq_extra_init_i2c_##idx(const struct device *dev)                         \
	{                                                                                          \
		IRQ_CONNECT(DT_IRQN(DT_DRV_INST(idx)),                                             \
			    DT_IRQ_BY_IDX(DT_DRV_INST(idx), 0, priority), nrfx_twim_irq_handler,   \
			    &ppi_seq_twim_##idx, 0);                                               \
	}                                                                                          \
	PPI_SEQ_COMMON_DEFINE(i2c, false, idx, PPI_SEQ_I2C_TASK(idx), PPI_SEQ_I2C_END_EVENT(idx),  \
			      &ppi_seq_twim_##idx, &ppi_seq_i2c_dev_config_##idx,                  \
			      ppi_seq_extra_init_i2c_##idx, ppi_seq_i2c_deinit)

#define PPI_SEQ_SPI_DEFINE(idx)                                                                    \
	static nrfx_spim_t ppi_seq_spim_##idx;                                                     \
	static const nrfx_spim_config_t ppi_seq_spim_nrfx_config_##idx = PPI_SEQ_SPI_CONFIG(idx);  \
	PPI_SEQ_COMMON_DEFINE(spi, true, idx, PPI_SEQ_SPI_TASK(idx), PPI_SEQ_SPI_END_EVENT(idx),   \
			      &ppi_seq_spim_##idx, &ppi_seq_spim_nrfx_config_##idx, 0,             \
			      ppi_seq_spi_deinit)

#define DT_DRV_COMPAT nordic_ppi_seq_i2c
DT_INST_FOREACH_STATUS_OKAY(PPI_SEQ_I2C_DEFINE)

#undef DT_DRV_COMPAT
#define DT_DRV_COMPAT nordic_ppi_seq_spi
DT_INST_FOREACH_STATUS_OKAY(PPI_SEQ_SPI_DEFINE)
