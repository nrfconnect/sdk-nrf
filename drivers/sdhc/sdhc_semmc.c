/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_nrf_semmc

#include <zephyr/drivers/sdhc.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/logging/log.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/util.h>

#include <nrf_semmc.h>
#if defined(CONFIG_SOC_SERIES_NRF54L)
#include <hal/nrf_memconf.h>
#include <hal/nrf_spu.h>
#endif
#include <dmm.h>

LOG_MODULE_REGISTER(sdhc_semmc, CONFIG_SDHC_LOG_LEVEL);

#define VPR_NODE DT_NODELABEL(cpuflpr_vpr)

#define FREQUENCY_MIN  KHZ(100)
#if defined(CONFIG_SOC_SERIES_NRF54L)
#define FREQUENCY_MAX  MHZ(32)
#else
#define FREQUENCY_MAX  MHZ(80)
#endif

struct sdhc_semmc_data {
	struct k_sem finished;
	/* For synchronization of API calls made from different contexts. */
	struct k_sem ctx_lock;
	/* For locking of controller configuration. */
	struct k_sem cfg_lock;
	bool suspended;

	nrf_semmc_config_t semmc_config;
	uint32_t response_buf[NRF_SEMMC_RESPONSE_SIZE];
};

struct sdhc_semmc_config {
	nrf_semmc_t semmc;
	const struct pinctrl_dev_config *pcfg;
	void *mem_reg;
};

static void event_handler(nrf_semmc_event_t const *event, void *context)
{
	const struct device *dev = context;
	struct sdhc_semmc_data *dev_data = dev->data;

	if (event->type == NRF_SEMMC_EVT_XFER_DONE) {
		k_sem_give(&dev_data->finished);
	}
}

static int api_reset(const struct device *dev)
{
	return 0;
}

static int send_request(const struct device *dev,
			nrf_semmc_cmd_desc_t *semmc_cmd,
			nrf_semmc_transfer_desc_t *semmc_transfer,
			int timeout_ms)
{
	struct sdhc_semmc_data *dev_data = dev->data;
	const struct sdhc_semmc_config *dev_config = dev->config;
	nrf_semmc_transfer_desc_t no_transfer = {
		.buffer = NULL,
	};
	k_timeout_t timeout;
	nrf_semmc_error_t err;
	int rc;

	if (timeout_ms == SDHC_TIMEOUT_FOREVER) {
		timeout = K_FOREVER;
	} else {
		timeout = K_MSEC(timeout_ms);
	}

	if (semmc_transfer == NULL) {
		semmc_transfer = &no_transfer;
	}

	k_sem_reset(&dev_data->finished);

	err = nrf_semmc_cmd(&dev_config->semmc, semmc_cmd,
			    &dev_data->semmc_config,
			    semmc_transfer, 1, 0);
	if (err != NRF_SEMMC_SUCCESS) {
		LOG_ERR("nrf_semmc_cmd() failed (%d), CMD%d",
			err, semmc_cmd->cmd);
		return -EIO;
	}

	rc = k_sem_take(&dev_data->finished, timeout);
	if (rc < 0) {
		LOG_ERR("Request for CMD%d timed out",
			semmc_cmd->cmd);

		(void)nrf_semmc_abort(&dev_config->semmc);

		return -ETIMEDOUT;
	}

	if (semmc_cmd->err != NRF_SEMMC_SUCCESS) {
		LOG_ERR("Request for CMD%d failed: %d",
			semmc_cmd->cmd, semmc_cmd->err);
		return -EIO;
	}

	return 0;
}

static int _api_request(const struct device *dev,
			struct sdhc_command *cmd,
			struct sdhc_data *data)
{
	struct sdhc_semmc_data *dev_data = dev->data;
	const struct sdhc_semmc_config *dev_config = dev->config;
	nrf_semmc_cmd_desc_t semmc_cmd = {
		.cmd = (uint8_t)cmd->opcode,
		.arg = cmd->arg,
		.resp_buffer = dev_data->response_buf,
	};
	int rc;

	switch (cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) {
	case SD_RSP_TYPE_NONE:
		semmc_cmd.resp_type = NRF_SEMMC_RESP_NONE;
		break;
	case SD_RSP_TYPE_R1:
		semmc_cmd.resp_type = NRF_SEMMC_RESP_R1;
		break;
	case SD_RSP_TYPE_R1b:
		semmc_cmd.resp_type = NRF_SEMMC_RESP_R1B;
		break;
	case SD_RSP_TYPE_R2:
		semmc_cmd.resp_type = NRF_SEMMC_RESP_R2;
		break;
	case SD_RSP_TYPE_R3:
		semmc_cmd.resp_type = NRF_SEMMC_RESP_R3;
		break;
	case SD_RSP_TYPE_R4:
		semmc_cmd.resp_type = NRF_SEMMC_RESP_R4;
		break;
	case SD_RSP_TYPE_R5:
		semmc_cmd.resp_type = NRF_SEMMC_RESP_R5;
		break;
	default:
		LOG_ERR("Unsupported response type: 0x%02X",
			cmd->response_type);
		return -EINVAL;
	}

	/* For commands that read and write multiple blocks (CMD18 and CMD25,
	 * respectively), the SD stack expects that they will be automatically
	 * preceded by the host with CMD23 that sets the number of blocks (see
	 * note in the implementation of card_read() in sd_ops.c).
	 */
	if (cmd->opcode == SD_READ_MULTIPLE_BLOCK ||
	    cmd->opcode == SD_WRITE_MULTIPLE_BLOCK) {
		nrf_semmc_cmd_desc_t blk_cnt_cmd = {
			.cmd = SD_SET_BLOCK_COUNT,
			.arg = data->blocks,
			.resp_type = NRF_SEMMC_RESP_R1,
			.resp_buffer = semmc_cmd.resp_buffer,
		};

		rc = send_request(dev, &blk_cnt_cmd, NULL, cmd->timeout_ms);
		if (rc < 0) {
			return rc;
		}
	}

	if (data) {
		nrf_semmc_transfer_desc_t transfer = {
			.block_size = data->block_size,
			.num_blocks = data->blocks,
		};
		uint32_t buf_size = data->blocks * data->block_size;
		bool out_buf = cmd->opcode == SD_WRITE_SINGLE_BLOCK ||
			       cmd->opcode == SD_WRITE_MULTIPLE_BLOCK;

		if (out_buf) {
			rc = dmm_buffer_out_prepare(dev_config->mem_reg,
						    data->data,
						    buf_size,
						    &transfer.buffer);
		} else {
			rc = dmm_buffer_in_prepare(dev_config->mem_reg,
						   data->data,
						   buf_size,
						   &transfer.buffer);
		}
		if (rc < 0) {
			LOG_ERR("Failed to prepare DMM buffer (%d)", rc);
			return rc;
		}

		rc = send_request(dev, &semmc_cmd, &transfer, data->timeout_ms);

		if (out_buf) {
			(void)dmm_buffer_out_release(dev_config->mem_reg,
						     transfer.buffer);
		} else {
			(void)dmm_buffer_in_release(dev_config->mem_reg,
						    data->data,
						    buf_size,
						    transfer.buffer);
		}
	} else {
		rc = send_request(dev, &semmc_cmd, NULL, cmd->timeout_ms);
	}

	if (rc < 0) {
		return rc;
	}

	if (semmc_cmd.resp_type != NRF_SEMMC_RESP_NONE) {
		BUILD_ASSERT(sizeof(cmd->response) == sizeof(semmc_cmd.response));

		memcpy(cmd->response, semmc_cmd.response, sizeof(cmd->response));
	}

	return 0;
}

static int api_request(const struct device *dev,
		       struct sdhc_command *cmd,
		       struct sdhc_data *data)
{
	struct sdhc_semmc_data *dev_data = dev->data;
	int rc, rc2;

	/* During common SD initialization, CMD8 (SEND_IF_COND) is requested
	 * with expected response R7. Since this driver only supports the MMC
	 * protocol, indicate that such command in not supported.
	 */
	if ((cmd->response_type & SDHC_NATIVE_RESPONSE_MASK) == SD_RSP_TYPE_R7) {
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
		int attempts = 1 + cmd->retries;

		do {
			rc = _api_request(dev, cmd, data);
			if (rc >= 0) {
				break;
			}
		} while (--attempts > 0);
	}

	k_sem_give(&dev_data->ctx_lock);

	rc2 = pm_device_runtime_put(dev);
	if (rc2 < 0) {
		LOG_ERR("pm_device_runtime_put() failed: %d", rc2);
		rc = (rc < 0 ? rc : rc2);
	}

	return rc;
}

static int api_set_io(const struct device *dev, struct sdhc_io *ios)
{
	struct sdhc_semmc_data *dev_data = dev->data;
	nrf_semmc_config_t new_config = dev_data->semmc_config;

	if (ios->clock) {
		if (ios->clock < FREQUENCY_MIN ||
		    ios->clock > FREQUENCY_MAX) {
			return -EINVAL;
		}

		new_config.clk_freq_hz = ios->clock;
	}

	if (ios->bus_width) {
		switch (ios->bus_width) {
		case SDHC_BUS_WIDTH1BIT:
			new_config.bus_width = NRF_SEMMC_BUS_WIDTH_1;
			break;
		case SDHC_BUS_WIDTH4BIT:
			new_config.bus_width = NRF_SEMMC_BUS_WIDTH_4;
			break;
		case SDHC_BUS_WIDTH8BIT:
			new_config.bus_width = NRF_SEMMC_BUS_WIDTH_8;
			break;
		default:
			LOG_ERR("Unsupported bus width: %d", ios->bus_width);
			return -EINVAL;
		}
	}

	dev_data->semmc_config = new_config;

	return  0;
}

static int api_get_card_present(const struct device *dev)
{
	return  1;
}

static int api_card_busy(const struct device *dev)
{
	const struct sdhc_semmc_config *dev_config = dev->config;

	return nrf_semmc_is_busy(&dev_config->semmc);
}

static int api_get_host_props(const struct device *dev,
			      struct sdhc_host_props *props)
{
	memset(props, 0, sizeof(*props));

	props->f_min = FREQUENCY_MIN;
	props->f_max = FREQUENCY_MAX;

	props->host_caps.bus_4_bit_support = 1;
	props->host_caps.vol_180_support = 1;

	return 0;
}

static int dev_pm_action_cb(const struct device *dev,
			    enum pm_device_action action)
{
	struct sdhc_semmc_data *dev_data = dev->data;
	const struct sdhc_semmc_config *dev_config = dev->config;
	int rc;
	nrf_semmc_error_t err;

	if (action == PM_DEVICE_ACTION_RESUME) {
		rc = pinctrl_apply_state(dev_config->pcfg,
					 PINCTRL_STATE_DEFAULT);
		if (rc < 0) {
			LOG_ERR("Cannot apply default pins state (%d)", rc);
			return rc;
		}

		err = nrf_semmc_enable(&dev_config->semmc);
		if (err != NRF_SEMMC_SUCCESS) {
			LOG_ERR("nrf_semmc_enable() failed: %08x", err);
			return -EIO;
		}
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
		err = nrf_semmc_disable(&dev_config->semmc);
		if (err == NRF_SEMMC_SUCCESS) {
			dev_data->suspended = true;
		} else {
			LOG_ERR("nrf_semmc_disable() failed: %08x", err);
		}

		k_sem_give(&dev_data->ctx_lock);

		return err == NRF_SEMMC_SUCCESS ? 0 : -EIO;
	}

	return -ENOTSUP;
}

static int dev_init(const struct device *dev)
{
	struct sdhc_semmc_data *dev_data = dev->data;
	const struct sdhc_semmc_config *dev_config = dev->config;
	int rc;
	nrf_semmc_error_t err;

	dev_data->semmc_config.clk_freq_hz = KHZ(400);
	dev_data->semmc_config.bus_width = NRF_SEMMC_BUS_WIDTH_1;

	k_sem_init(&dev_data->finished, 0, 1);
	k_sem_init(&dev_data->ctx_lock, 1, 1);
	k_sem_init(&dev_data->cfg_lock, 1, 1);

#if defined(CONFIG_SOC_SERIES_NRF54L)
	NRF_GPIOHSPADCTRL->BIAS = 2;
#if !defined(CONFIG_TRUSTED_EXECUTION_NONSECURE)
	nrf_spu_periph_perm_secattr_set(NRF_SPU00,
		nrf_address_slave_get(DT_REG_ADDR(DT_NODELABEL(cpuflpr_vpr))),
		true);
#endif
#endif /* defined(CONFIG_SOC_SERIES_NRF54L) */

	IRQ_CONNECT(DT_IRQN(VPR_NODE), DT_IRQ(VPR_NODE, priority),
		    nrfx_isr, nrf_semmc_irq_handler, 0);

	err = nrf_semmc_init(&dev_config->semmc, event_handler, (void *)dev);
	if (err != NRF_SEMMC_SUCCESS) {
		LOG_ERR("nrf_semmc_init() failed: %08x", err);
		return -EIO;
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

static DEVICE_API(sdhc, drv_api) = {
	.reset = api_reset,
	.request = api_request,
	.set_io = api_set_io,
	.get_card_present = api_get_card_present,
	.card_busy = api_card_busy,
	.get_host_props = api_get_host_props,
};

#define SDHC_SEMMC_INST(inst)						\
	PM_DEVICE_DT_INST_DEFINE(inst, dev_pm_action_cb);		\
	PINCTRL_DT_DEFINE(VPR_NODE);					\
	static struct sdhc_semmc_data dev##inst##_data;			\
	static const struct sdhc_semmc_config dev##inst##_config = {	\
		.semmc = {						\
			.p_reg = (void *)DT_INST_REG_ADDR(inst),	\
			.drv_inst_idx = 0,				\
		},							\
		.pcfg = PINCTRL_DT_DEV_CONFIG_GET(VPR_NODE),		\
		.mem_reg = DMM_DEV_TO_REG(DT_DRV_INST(inst)),		\
	};								\
	DEVICE_DT_INST_DEFINE(inst,					\
		dev_init, PM_DEVICE_DT_INST_GET(inst),			\
		&dev##inst##_data, &dev##inst##_config,			\
		POST_KERNEL, CONFIG_SDHC_INIT_PRIORITY,			\
		&drv_api);

SDHC_SEMMC_INST(0);
