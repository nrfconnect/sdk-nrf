/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_nrf_sqspi

#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#include <softperipheral_regif.h>
#include <nrf_sqspi.h>
#if defined(CONFIG_SOC_SERIES_NRF54L)
#include <hal/nrf_memconf.h>
#include <hal/nrf_oscillators.h>
#include <hal/nrf_spu.h>
#endif
#include <dmm.h>

LOG_MODULE_REGISTER(mspi_sqspi, CONFIG_MSPI_LOG_LEVEL);

#define VPR_NODE DT_NODELABEL(cpuflpr_vpr)

struct mspi_sqspi_data {
	const struct mspi_dev_id *dev_id;
	nrf_sqspi_dev_cfg_t sqspi_dev_cfg;
	struct k_sem finished;
	/* For synchronization of API calls made from different contexts. */
	struct k_sem ctx_lock;
	/* For locking of controller configuration. */
	struct k_sem cfg_lock;
	bool suspended;
};

struct mspi_sqspi_config {
	nrf_sqspi_t sqspi;
	const struct pinctrl_dev_config *pcfg;
	const struct gpio_dt_spec *ce_gpios;
	uint8_t ce_gpios_len;
	void *mem_reg;
};

static void done_callback(const nrf_sqspi_t *sqspi, nrf_sqspi_evt_t *event,
			  void *context)
{
	ARG_UNUSED(sqspi);
	struct mspi_sqspi_data *dev_data = context;

	if (event->type == NRF_SQSPI_EVT_XFER_DONE) {
		if (event->data.xfer_done == NRF_SQSPI_RESULT_OK) {
			k_sem_give(&dev_data->finished);
		}
	}
}

static int api_config(const struct mspi_dt_spec *spec)
{
	ARG_UNUSED(spec);

	return -ENOTSUP;
}

static int _api_dev_config(const struct device *dev,
			   const enum mspi_dev_cfg_mask param_mask,
			   const struct mspi_dev_cfg *cfg)
{
	struct mspi_sqspi_data *dev_data = dev->data;

	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		if (cfg->endian != MSPI_XFER_BIG_ENDIAN) {
			LOG_ERR("Only big endian transfers are supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CE_POL) {
		if (cfg->ce_polarity != MSPI_CE_ACTIVE_LOW) {
			LOG_ERR("Only active low CE is supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) {
		if (cfg->mem_boundary) {
			LOG_ERR("Auto CE break is not supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) {
		if (cfg->time_to_break) {
			LOG_ERR("Auto CE break is not supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		switch (cfg->io_mode) {
		default:
		case MSPI_IO_MODE_SINGLE:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_SINGLE;
			break;
		case MSPI_IO_MODE_DUAL:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_DUAL_2_2_2;
			break;
		case MSPI_IO_MODE_DUAL_1_1_2:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_DUAL_1_1_2;
			break;
		case MSPI_IO_MODE_DUAL_1_2_2:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_DUAL_1_2_2;
			break;
		case MSPI_IO_MODE_QUAD:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_QUAD_4_4_4;
			break;
		case MSPI_IO_MODE_QUAD_1_1_4:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_QUAD_1_1_4;
			break;
		case MSPI_IO_MODE_QUAD_1_4_4:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_QUAD_1_4_4;
			break;
		case MSPI_IO_MODE_OCTAL:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_OCTAL_8_8_8;
			break;
		case MSPI_IO_MODE_OCTAL_1_1_8:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_OCTAL_1_1_8;
			break;
		case MSPI_IO_MODE_OCTAL_1_8_8:
			dev_data->sqspi_dev_cfg.mspi_lines = NRF_SQSPI_SPI_LINES_OCTAL_1_8_8;
			break;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		switch (cfg->cpp) {
		default:
		case MSPI_CPP_MODE_0:
			dev_data->sqspi_dev_cfg.spi_cpolpha = NRF_SQSPI_SPI_CPOLPHA_0;
			break;
		case MSPI_CPP_MODE_1:
			dev_data->sqspi_dev_cfg.spi_cpolpha = NRF_SQSPI_SPI_CPOLPHA_1;
			break;
		case MSPI_CPP_MODE_2:
			dev_data->sqspi_dev_cfg.spi_cpolpha = NRF_SQSPI_SPI_CPOLPHA_2;
			break;
		case MSPI_CPP_MODE_3:
			dev_data->sqspi_dev_cfg.spi_cpolpha = NRF_SQSPI_SPI_CPOLPHA_3;
			break;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		if (cfg->freq < 1 ||
		    cfg->freq > SP_VPR_BASE_FREQ_HZ) {
			LOG_ERR("Invalid frequency: %u, MIN: %u, MAX: %u",
				cfg->freq, 1, SP_VPR_BASE_FREQ_HZ);
			return -EINVAL;
		}

		dev_data->sqspi_dev_cfg.sck_freq_khz = cfg->freq / 1000;
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		/* TODO: add support for DDR */
		if (cfg->data_rate != MSPI_DATA_RATE_SINGLE) {
			LOG_ERR("Only single data rate is supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		/* TODO: add support for DQS */
		if (cfg->dqs_enable) {
			LOG_ERR("DQS line is not supported.");
			return -ENOTSUP;
		}
	}

	return 0;
}

static int api_dev_config(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask,
			  const struct mspi_dev_cfg *cfg)
{
	struct mspi_sqspi_data *dev_data = dev->data;
	int rc;

	if (dev_id != dev_data->dev_id) {
		rc = k_sem_take(&dev_data->cfg_lock,
				K_MSEC(CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE));
		if (rc < 0) {
			LOG_ERR("Failed to switch controller to device");
			return -EBUSY;
		}

		dev_data->dev_id = dev_id;

		if (param_mask == MSPI_DEVICE_CONFIG_NONE) {
			return 0;
		}
	}

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	rc = _api_dev_config(dev, param_mask, cfg);

	k_sem_give(&dev_data->ctx_lock);

	if (rc < 0) {
		dev_data->dev_id = NULL;
		k_sem_give(&dev_data->cfg_lock);
	}

	return rc;
}

static int api_get_channel_status(const struct device *dev, uint8_t ch)
{
	ARG_UNUSED(ch);

	struct mspi_sqspi_data *dev_data = dev->data;

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	dev_data->dev_id = NULL;
	k_sem_give(&dev_data->cfg_lock);

	k_sem_give(&dev_data->ctx_lock);

	return 0;
}

static int perform_xfer(const struct device *dev,
			const nrf_sqspi_xfer_t *sqspi_xfer,
			k_timeout_t timeout)
{
	const struct mspi_sqspi_config *dev_config = dev->config;
	struct mspi_sqspi_data *dev_data = dev->data;
	int rc = 0;
	nrfx_err_t err;

	if (dev_data->dev_id->ce.port) {
		rc = gpio_pin_set_dt(&dev_data->dev_id->ce, 1);
		if (rc < 0) {
			LOG_ERR("Failed to activate CE line (%d)", rc);
			return rc;
		}
	}

	err = nrf_sqspi_xfer(&dev_config->sqspi, sqspi_xfer, 1, 0);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrf_sqspi_xfer() failed: %08x", err);
		return -EIO;
	}

	rc = k_sem_take(&dev_data->finished, timeout);
	if (rc < 0) {
		rc = -ETIMEDOUT;
	}

	if (dev_data->dev_id->ce.port) {
		int rc2;

		/* Do not use `rc` to not overwrite potential timeout error. */
		rc2 = gpio_pin_set_dt(&dev_data->dev_id->ce, 0);
		if (rc2 < 0) {
			LOG_ERR("Failed to deactivate CE line (%d)", rc2);
			return rc2;
		}
	}

	return rc;
}

static int process_packet(const struct device *dev,
			  const struct mspi_xfer_packet *packet,
			  const struct mspi_xfer *xfer,
			  k_timeout_t timeout)
{
	const struct mspi_sqspi_config *dev_config = dev->config;
	nrf_sqspi_xfer_t sqspi_xfer = {
		/* Use TX direction when there is no data to transfer. */
		.dir = NRF_SQSPI_XFER_DIR_TX,
		.cmd = packet->cmd,
		.address = packet->address,
		.data_length = packet->num_bytes,
		.cmd_length = 8 * xfer->cmd_length,
		.addr_length = 8 * xfer->addr_length,
	};
	int rc;

	if (packet->num_bytes) {
		if (packet->dir == MSPI_TX) {
			sqspi_xfer.dir = NRF_SQSPI_XFER_DIR_TX;
			sqspi_xfer.dummy_length = xfer->tx_dummy;
			rc = dmm_buffer_out_prepare(dev_config->mem_reg,
						    packet->data_buf,
						    packet->num_bytes,
						    &sqspi_xfer.p_data);
		} else {
			sqspi_xfer.dir = NRF_SQSPI_XFER_DIR_RX;
			sqspi_xfer.dummy_length = xfer->rx_dummy;
			rc = dmm_buffer_in_prepare(dev_config->mem_reg,
						   packet->data_buf,
						   packet->num_bytes,
						   &sqspi_xfer.p_data);
		}
		if (rc < 0) {
			LOG_ERR("Failed to allocate DMM buffer (%d)", rc);
			return rc;
		}
	} else if (xfer->cmd_length == 0 &&
		   xfer->addr_length == 0) {
		/* Nothing to be transferred, skip this packet. */
		return 0;
	}

	rc = perform_xfer(dev, &sqspi_xfer, timeout);

	if (packet->num_bytes) {
		/* No need to check the error codes here. These calls could only
		 * fail if an invalid memory region was specified, but since the
		 * same one was used in the corresponding *_prepare() call above
		 * and that call has succeeded, we know the region is valid.
		 */
		if (packet->dir == MSPI_TX) {
			(void)dmm_buffer_out_release(dev_config->mem_reg,
						     sqspi_xfer.p_data);
		} else {
			(void)dmm_buffer_in_release(dev_config->mem_reg,
						    packet->data_buf,
						    packet->num_bytes,
						    sqspi_xfer.p_data);
		}
	}

	return rc;
}

static int _api_transceive(const struct device *dev,
			   const struct mspi_xfer *req)
{
	const struct mspi_sqspi_config *dev_config = dev->config;
	struct mspi_sqspi_data *dev_data = dev->data;
	k_timeout_t timeout = K_MSEC(req->timeout);
	uint32_t done;
	nrfx_err_t err;
	int rc;

	err = nrf_sqspi_dev_cfg(&dev_config->sqspi, &dev_data->sqspi_dev_cfg,
				done_callback, dev_data);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrf_sqspi_dev_cfg() failed: %08x", err);
		return -EIO;
	}

	for (done = 0; done < req->num_packet; ++done) {
		rc = process_packet(dev, &req->packets[done], req, timeout);
		if (rc < 0) {
			return rc;
		}
	}

	return 0;
}

static int api_transceive(const struct device *dev,
			  const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	struct mspi_sqspi_data *dev_data = dev->data;
	int rc, rc2;

	if (dev_id != dev_data->dev_id) {
		LOG_ERR("Controller is not configured for this device");
		return -EINVAL;
	}

	/* TODO: add support for asynchronous transfers */
	if (req->async) {
		LOG_ERR("Asynchronous transfers are not supported");
		return -ENOTSUP;
	}

	rc = pm_device_runtime_get(dev);
	if (rc < 0) {
		LOG_ERR("pm_device_runtime_get() failed: %d", rc);
		return rc;
	}

	(void)k_sem_take(&dev_data->ctx_lock, K_FOREVER);

	if (dev_data->suspended) {
		rc = -EFAULT;
	} else {
		rc = _api_transceive(dev, req);
	}

	k_sem_give(&dev_data->ctx_lock);

	rc2 = pm_device_runtime_put(dev);
	if (rc2 < 0) {
		LOG_ERR("pm_device_runtime_put() failed: %d", rc2);
		rc = (rc < 0 ? rc : rc2);
	}

	return rc;
}

static int dev_pm_action_cb(const struct device *dev,
			    enum pm_device_action action)
{
	struct mspi_sqspi_data *dev_data = dev->data;
	const struct mspi_sqspi_config *dev_config = dev->config;
	int rc;

	if (action == PM_DEVICE_ACTION_RESUME) {
		rc = pinctrl_apply_state(dev_config->pcfg,
					 PINCTRL_STATE_DEFAULT);
		if (rc < 0) {
			LOG_ERR("Cannot apply default pins state (%d)", rc);
			return rc;
		}

		nrf_sqspi_activate(&dev_config->sqspi);
#if defined(CONFIG_SOC_SERIES_NRF54L)
		nrf_memconf_ramblock_ret_enable_set(NRF_MEMCONF,
			1, MEMCONF_POWER_RET_MEM0_Pos, false);
#endif

		dev_data->suspended = false;

		return 0;
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE) &&
	    action == PM_DEVICE_ACTION_SUSPEND) {
		rc = pinctrl_apply_state(dev_config->pcfg,
					 PINCTRL_STATE_SLEEP);
		if (rc < 0) {
			LOG_ERR("Cannot apply sleep pins state (%d)", rc);
			return rc;
		}

		if (k_sem_take(&dev_data->ctx_lock, K_NO_WAIT) != 0) {
			LOG_ERR("Controller in use, cannot be suspended");
			return -EBUSY;
		}

#if defined(CONFIG_SOC_SERIES_NRF54L)
		nrf_memconf_ramblock_ret_enable_set(NRF_MEMCONF,
			1, MEMCONF_POWER_RET_MEM0_Pos, true);
#endif
		nrf_sqspi_deactivate(&dev_config->sqspi);

		k_sem_give(&dev_data->ctx_lock);

		return 0;
	}

	return -ENOTSUP;
}

static int dev_init(const struct device *dev)
{
	struct mspi_sqspi_data *dev_data = dev->data;
	const struct mspi_sqspi_config *dev_config = dev->config;
	const struct gpio_dt_spec *ce_gpio;
	static const nrf_sqspi_cfg_t sqspi_cfg = {
		.skip_gpio_cfg = true,
		.skip_pmux_cfg = true,
	};
	nrf_sqspi_data_fmt_t sqspi_data_fmt = {
		.cmd_bit_order = NRF_SQSPI_DATA_FMT_BIT_ORDER_MSB_FIRST,
		.addr_bit_order = NRF_SQSPI_DATA_FMT_BIT_ORDER_MSB_FIRST,
		.data_bit_order = NRF_SQSPI_DATA_FMT_BIT_ORDER_MSB_FIRST,
		.data_bit_reorder_unit = 8,
		.data_container = 32,
		.data_swap_unit = 8,
		.data_padding = NRF_SQSPI_DATA_FMT_PAD_RAW,
	};
	int rc;
	nrfx_err_t err;

	k_sem_init(&dev_data->finished, 0, 1);
	k_sem_init(&dev_data->ctx_lock, 1, 1);
	k_sem_init(&dev_data->cfg_lock, 1, 1);

	dev_data->sqspi_dev_cfg.protocol = NRF_SQSPI_PROTO_SPI_C;
	dev_data->sqspi_dev_cfg.sample_sync = NRF_SQSPI_SAMPLE_SYNC_DELAY;
	dev_data->sqspi_dev_cfg.sample_delay_cyc = 1;

#if defined(CONFIG_SOC_SERIES_NRF54L)
	nrf_oscillators_pll_freq_set(NRF_OSCILLATORS,
				     NRF_OSCILLATORS_PLL_FREQ_128M);
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	nrf_spu_periph_perm_secattr_set(NRF_SPU00,
		nrf_address_slave_get(DT_REG_ADDR(DT_NODELABEL(cpuflpr_vpr))),
		true);
#endif
#endif /* defined(CONFIG_SOC_SERIES_NRF54L) */

	IRQ_CONNECT(DT_IRQN(VPR_NODE), DT_IRQ(VPR_NODE, priority),
		    nrfx_isr, nrf_sqspi_irq_handler, 0);

	err = nrf_sqspi_init(&dev_config->sqspi, &sqspi_cfg);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrf_sqspi_init() failed: %08x", err);
		return -EIO;
	}

	err = nrf_sqspi_dev_data_fmt_set(&dev_config->sqspi, &sqspi_data_fmt);
	if (err != NRFX_SUCCESS) {
		LOG_ERR("nrf_sqspi_dev_data_fmt_set() failed: %08x", err);
		return -EIO;
	}

	for (ce_gpio = dev_config->ce_gpios;
	     ce_gpio < &dev_config->ce_gpios[dev_config->ce_gpios_len];
	     ce_gpio++) {
		if (!device_is_ready(ce_gpio->port)) {
			LOG_ERR("CE GPIO port %s is not ready",
				ce_gpio->port->name);
			return -ENODEV;
		}

		rc = gpio_pin_configure_dt(ce_gpio, GPIO_OUTPUT_INACTIVE);
		if (rc < 0) {
			return rc;
		}
	}

	if (IS_ENABLED(CONFIG_PM_DEVICE_RUNTIME)) {
		rc = pinctrl_apply_state(dev_config->pcfg, PINCTRL_STATE_SLEEP);
		if (rc < 0) {
			LOG_ERR("Cannot apply sleep pins state (%d)", rc);
			return rc;
		}
	}

	return pm_device_driver_init(dev, dev_pm_action_cb);
}

static DEVICE_API(mspi, drv_api) = {
	.config             = api_config,
	.dev_config         = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive         = api_transceive,
};

#define FOREACH_CE_GPIOS_ELEM(inst)					\
	DT_INST_FOREACH_PROP_ELEM_SEP(inst, ce_gpios,			\
				      GPIO_DT_SPEC_GET_BY_IDX, (,))
#define MSPI_SQSPI_CE_GPIOS(inst)					\
	.ce_gpios = (const struct gpio_dt_spec [])			\
		{ FOREACH_CE_GPIOS_ELEM(inst) },			\
	.ce_gpios_len = DT_INST_PROP_LEN(inst, ce_gpios),

#define MSPI_SQSPI_INST(inst)						\
	PM_DEVICE_DT_INST_DEFINE(inst, dev_pm_action_cb);		\
	PINCTRL_DT_DEFINE(VPR_NODE);					\
	static struct mspi_sqspi_data dev##inst##_data;			\
	static const struct mspi_sqspi_config dev##inst##_config = {	\
		.sqspi = {						\
			.p_reg = (void *)DT_INST_REG_ADDR(inst),	\
			.drv_inst_idx = 0,				\
		},							\
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(VPR_NODE),		\
		.mem_reg = DMM_DEV_TO_REG(DT_DRV_INST(inst)),		\
	IF_ENABLED(DT_INST_NODE_HAS_PROP(inst, ce_gpios),		\
		(MSPI_SQSPI_CE_GPIOS(inst)))				\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
		dev_init, PM_DEVICE_DT_INST_GET(inst),			\
		&dev##inst##_data, &dev##inst##_config,			\
		POST_KERNEL, CONFIG_MSPI_INIT_PRIORITY,			\
		&drv_api);

MSPI_SQSPI_INST(0);
