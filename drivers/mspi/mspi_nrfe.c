/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_nrfe_mspi_controller

#include <zephyr/kernel.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/pm/device.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_nrfe, CONFIG_MSPI_LOG_LEVEL);

#include <hal/nrf_gpio.h>
#include <drivers/mspi/nrfe_mspi.h>

#define MSPI_NRFE_NODE	   DT_DRV_INST(0)
#define MAX_TX_MSG_SIZE	   (DT_REG_SIZE(DT_NODELABEL(sram_tx)))
#define MAX_RX_MSG_SIZE	   (DT_REG_SIZE(DT_NODELABEL(sram_rx)))
#define IPC_TIMEOUT_MS	   100
#define EP_SEND_TIMEOUT_MS 10

#define SDP_MPSI_PINCTRL_DEV_CONFIG_INIT(node_id)                                                  \
	{                                                                                          \
		.reg = PINCTRL_REG_NONE,                                                           \
		.states = Z_PINCTRL_STATES_NAME(node_id),                                          \
		.state_cnt = ARRAY_SIZE(Z_PINCTRL_STATES_NAME(node_id)),                           \
	}

#define SDP_MSPI_PINCTRL_DT_DEFINE(node_id)                                                        \
	LISTIFY(DT_NUM_PINCTRL_STATES(node_id), Z_PINCTRL_STATE_PINS_DEFINE, (;), node_id);        \
	Z_PINCTRL_STATES_DEFINE(node_id)                                                           \
	Z_PINCTRL_DEV_CONFIG_STATIC Z_PINCTRL_DEV_CONFIG_CONST struct pinctrl_dev_config           \
	Z_PINCTRL_DEV_CONFIG_NAME(node_id) = SDP_MPSI_PINCTRL_DEV_CONFIG_INIT(node_id)

SDP_MSPI_PINCTRL_DT_DEFINE(MSPI_NRFE_NODE);

static struct ipc_ept ep;
static size_t ipc_received;
static uint8_t *ipc_receive_buffer;

static K_SEM_DEFINE(ipc_sem, 0, 1);
static K_SEM_DEFINE(ipc_sem_cfg, 0, 1);
static K_SEM_DEFINE(ipc_sem_xfer, 0, 1);

#define MSPI_CONFIG                                                                                \
	{                                                                                          \
		.channel_num = 0,                                                                  \
		.op_mode = DT_PROP_OR(MSPI_NRFE_NODE, op_mode, MSPI_OP_MODE_CONTROLLER),           \
		.duplex = DT_PROP_OR(MSPI_NRFE_NODE, duplex, MSPI_FULL_DUPLEX),                    \
		.dqs_support = DT_PROP_OR(MSPI_NRFE_NODE, dqs_support, false),                     \
		.num_periph = DT_CHILD_NUM(MSPI_NRFE_NODE),                                        \
		.max_freq = DT_PROP(MSPI_NRFE_NODE, clock_frequency),                              \
		.re_init = true,                                                                   \
		.sw_multi_periph = false,                                                          \
	}

struct mspi_nrfe_data {
	nrfe_mspi_xfer_config_msg_t xfer_config_msg;
};

struct mspi_nrfe_config {
	struct mspi_cfg mspicfg;
	const struct pinctrl_dev_config *pcfg;
};

static const struct mspi_nrfe_config dev_config = {
	.mspicfg = MSPI_CONFIG,
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct mspi_nrfe_data dev_data;

static const char * const opcode_strs[] = {
	[NRFE_MSPI_EP_BOUNDED]   = "EP bound",
	[NRFE_MSPI_CONFIG_PINS]  = "config pins",
	[NRFE_MSPI_CONFIG_DEV]   = "config dev",
	[NRFE_MSPI_CONFIG_XFER]  = "config xfer",
	[NRFE_MSPI_TX]           = "TX",
	[NRFE_MSPI_TXRX]         = "TXRX",
	[NRFE_MSPI_WRONG_OPCODE] = "wrong opcode",
};

static void ep_recv(const void *data, size_t len, void *priv);

static void ep_bound(void *priv)
{
	ipc_received = 0;
	k_sem_give(&ipc_sem);
	LOG_DBG("Endpoint bound.");
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound = ep_bound,
		.received = ep_recv
	},
};

static void ep_recv(const void *data, size_t len, void *priv)
{
	nrfe_mspi_flpr_response_msg_t *response = (nrfe_mspi_flpr_response_msg_t *)data;

	switch (response->opcode) {
	case NRFE_MSPI_CONFIG_PINS:
	case NRFE_MSPI_CONFIG_DEV:
	case NRFE_MSPI_CONFIG_XFER:
		k_sem_give(&ipc_sem_cfg);
		break;
	case NRFE_MSPI_TXRX:
		if (len > 0) {
			ipc_received = len - sizeof(nrfe_mspi_opcode_t);
			ipc_receive_buffer = (uint8_t *)&response->data;
		}
	case NRFE_MSPI_TX:
		k_sem_give(&ipc_sem_xfer);
		break;
	default:
		LOG_ERR("Invalid response opcode: %d", response->opcode);
		break;
	}
}

static int ipc_data_send(nrfe_mspi_opcode_t opcode, const void *data, size_t len)
{
	int rc;

	LOG_DBG("Sending msg with opcode: %s, size: %d", opcode_strs[opcode], len);
	uint32_t start = k_uptime_get_32();

	do {
		rc = ipc_service_send(&ep, data, len);
		if ((k_uptime_get_32() - start) > EP_SEND_TIMEOUT_MS) {
			break;
		}
	} while (rc == -ENOMEM); /* No space in the buffer. Retry. */

	return rc;
}

static int wait_for_response(nrfe_mspi_opcode_t opcode)
{
	int ret = 0;

	switch (opcode) {
	case NRFE_MSPI_CONFIG_PINS:
	case NRFE_MSPI_CONFIG_DEV:
	case NRFE_MSPI_CONFIG_XFER:
		ret = k_sem_take(&ipc_sem_cfg, K_MSEC(IPC_TIMEOUT_MS));
		break;
	case NRFE_MSPI_TX:
	case NRFE_MSPI_TXRX:
		ret = k_sem_take(&ipc_sem_xfer, K_MSEC(IPC_TIMEOUT_MS));
		break;
	default:
		break;
	}

	if (ret < 0) {
		return -ETIMEDOUT;
	}
	return 0;
}

static int send_and_wait(nrfe_mspi_opcode_t opcode, const void *data, size_t len)
{
	int rc;

#ifdef CONFIG_MSPI_NRFE_IPC_NO_COPY
	(void)len;
	void *data_ptr = (void *)data;

	rc = ipc_data_send(opcode, &data_ptr, sizeof(void *));
#else
	rc = ipc_data_send(opcode, data, len);
#endif

	if (rc < 0) {
		LOG_ERR("Data transfer failed: %d", rc);
		return rc;
	}

	rc = wait_for_response(opcode);
	if (rc < 0) {
		LOG_ERR("Data transfer: %s response timeout: %d!", opcode_strs[opcode], rc);
	}

	return rc;
}


static int xfer_packet(struct mspi_xfer *xfer, uint32_t packets_done)
{
	int rc;
	struct mspi_xfer_packet *packet = (struct mspi_xfer_packet *)&xfer->packets[packets_done];
	nrfe_mspi_opcode_t opcode = (packet->dir == MSPI_RX) ? NRFE_MSPI_TXRX : NRFE_MSPI_TX;

	if (packet->num_bytes >= MAX_TX_MSG_SIZE) {
		LOG_ERR("Packet size to large: %u. Increase SRAM data region.", packet->num_bytes);
		return -EINVAL;
	}

#ifdef CONFIG_MSPI_NRFE_IPC_NO_COPY
	/* Check for alignment problems. */
	uint32_t len = ((uint32_t)packet->data_buf) % sizeof(uint32_t) != 0
			       ? sizeof(nrfe_mspi_xfer_packet_msg_t) + packet->num_bytes
			       : sizeof(nrfe_mspi_xfer_packet_msg_t);
#else
	uint32_t len = sizeof(nrfe_mspi_xfer_packet_msg_t) + packet->num_bytes;
#endif
	uint8_t buffer[len];
	nrfe_mspi_xfer_packet_msg_t *xfer_packet = (nrfe_mspi_xfer_packet_msg_t *)buffer;

	xfer_packet->opcode = opcode;
	xfer_packet->command = packet->cmd;
	xfer_packet->address = packet->address;
	xfer_packet->num_bytes = packet->num_bytes;

#ifdef CONFIG_MSPI_NRFE_IPC_NO_COPY
	/*
	 * Check for alignment problems. With no-copy mode, FLPR expects data buffer to be aligned
	 * to word size.
	 */
	if (((uint32_t)packet->data_buf) % sizeof(uint32_t) != 0) {
		memcpy((void *)(buffer + sizeof(nrfe_mspi_xfer_packet_msg_t)),
		       (void *)packet->data_buf, packet->num_bytes);
		xfer_packet->data = buffer + sizeof(nrfe_mspi_xfer_packet_msg_t);
	} else {
		xfer_packet->data = packet->data_buf;
	}
#else
	memcpy((void *)xfer_packet->data, (void *)packet->data_buf, packet->num_bytes);
#endif

	/* Wait for the transfer to complete and receive data. */
	rc = send_and_wait(xfer_packet->opcode, xfer_packet, len);

	if ((packet->dir == MSPI_RX) && (ipc_receive_buffer != NULL) && (ipc_received > 0)) {
		/*
		 * It is not possible to check whether received data is valid, so packet->num_bytes
		 * should always be equal to ipc_received. If it is not, then something went wrong.
		 */
		if (packet->num_bytes != ipc_received) {
			rc = -EIO;
		} else {
			memcpy((void *)packet->data_buf, (void *)ipc_receive_buffer, ipc_received);
		}

		/* Clear the receive buffer pointer and size */
		ipc_receive_buffer = NULL;
		ipc_received = 0;
	}

	return rc;
}

static int api_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	struct mspi_nrfe_data *drv_data = dev->data;
	uint32_t packet_idx = 0;
	int rc;

	/* TODO: add support for asynchronous transfers */
	if (req->async) {
		return -ENOTSUP;
	}

	if (req->num_packet == 0 || !req->packets ||
	    req->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		return -EFAULT;
	}

	drv_data->xfer_config_msg.opcode = NRFE_MSPI_CONFIG_XFER;
	drv_data->xfer_config_msg.xfer_config.device_index = dev_id->dev_idx;
	drv_data->xfer_config_msg.xfer_config.command_length = req->cmd_length;
	drv_data->xfer_config_msg.xfer_config.address_length = req->addr_length;
	drv_data->xfer_config_msg.xfer_config.hold_ce = req->hold_ce;
	drv_data->xfer_config_msg.xfer_config.tx_dummy = req->tx_dummy;
	drv_data->xfer_config_msg.xfer_config.rx_dummy = req->rx_dummy;

	rc = send_and_wait(NRFE_MSPI_CONFIG_XFER, (void *)&drv_data->xfer_config_msg,
		       sizeof(nrfe_mspi_xfer_config_msg_t));

	if (rc < 0) {
		LOG_ERR("Send xfer config error: %d", rc);
		return rc;
	}

	while (packet_idx < req->num_packet) {
		rc = xfer_packet((struct mspi_xfer *)req, packet_idx);
		if (rc < 0) {
			LOG_ERR("xfer_packet error: %d packet_idx: %d", rc, packet_idx);
			return rc;
		}
		packet_idx++;
	}

	return 0;
}

static int api_config(const struct mspi_dt_spec *spec)
{
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_nrfe_config *drv_cfg = spec->bus->config;
	nrfe_mspi_pinctrl_soc_pin_msg_t mspi_pin_config;

	if (config->op_mode != MSPI_OP_MODE_CONTROLLER) {
		LOG_ERR("Only MSPI controller mode is supported.");
		return -ENOTSUP;
	}

	if (config->dqs_support) {
		LOG_ERR("DQS mode is not supported.");
		return -ENOTSUP;
	}

	if (config->max_freq > drv_cfg->mspicfg.max_freq) {
		LOG_ERR("max_freq is too large.");
		return -ENOTSUP;
	}

	/* Create pinout configuration */
	uint8_t state_id;

	for (state_id = 0; state_id < drv_cfg->pcfg->state_cnt; state_id++) {
		if (drv_cfg->pcfg->states[state_id].id == PINCTRL_STATE_DEFAULT) {
			break;
		}
	}

	if (drv_cfg->pcfg->states[state_id].pin_cnt > NRFE_MSPI_PINS_MAX) {
		LOG_ERR("Too many pins defined. Max: %d", NRFE_MSPI_PINS_MAX);
		return -ENOTSUP;
	}

	if (drv_cfg->pcfg->states[state_id].id != PINCTRL_STATE_DEFAULT) {
		LOG_ERR("Pins default state not found.");
		return -ENOTSUP;
	}

	for (uint8_t i = 0; i < drv_cfg->pcfg->states[state_id].pin_cnt; i++) {
		mspi_pin_config.pin[i] = drv_cfg->pcfg->states[state_id].pins[i];
	}
	mspi_pin_config.opcode = NRFE_MSPI_CONFIG_PINS;

	/* Send pinout configuration to FLPR */
	return send_and_wait(NRFE_MSPI_CONFIG_PINS, (const void *)&mspi_pin_config,
			 sizeof(nrfe_mspi_pinctrl_soc_pin_msg_t));
}

static int check_io_mode(enum mspi_io_mode io_mode)
{
	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE:
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4:
		break;
	default:
		LOG_ERR("IO mode %d not supported", io_mode);
		return -ENOTSUP;
	}

	return 0;
}

static int api_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask, const struct mspi_dev_cfg *cfg)
{
	const struct mspi_nrfe_config *drv_cfg = dev->config;
	int rc;
	nrfe_mspi_dev_config_msg_t mspi_dev_config_msg;

	if (param_mask & MSPI_DEVICE_CONFIG_MEM_BOUND) {
		if (cfg->mem_boundary) {
			LOG_ERR("Memory boundary is not supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_BREAK_TIME) {
		if (cfg->time_to_break) {
			LOG_ERR("Transfer break is not supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_FREQUENCY) {
		if (cfg->freq > drv_cfg->mspicfg.max_freq) {
			LOG_ERR("Invalid frequency: %u, MAX: %u", cfg->freq,
				drv_cfg->mspicfg.max_freq);
			return -EINVAL;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		rc = check_io_mode(cfg->io_mode);
		if (rc < 0) {
			return rc;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DATA_RATE) {
		if (cfg->data_rate != MSPI_DATA_RATE_SINGLE) {
			LOG_ERR("Only single data rate is supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_DQS) {
		if (cfg->dqs_enable) {
			LOG_ERR("DQS signal is not supported.");
			return -ENOTSUP;
		}
	}

	mspi_dev_config_msg.opcode = NRFE_MSPI_CONFIG_DEV;
	mspi_dev_config_msg.device_index = dev_id->dev_idx;
	mspi_dev_config_msg.dev_config.io_mode = cfg->io_mode;
	mspi_dev_config_msg.dev_config.cpp = cfg->cpp;
	mspi_dev_config_msg.dev_config.ce_polarity = cfg->ce_polarity;
	mspi_dev_config_msg.dev_config.freq = cfg->freq;
	mspi_dev_config_msg.dev_config.ce_index = cfg->ce_num;

	return send_and_wait(NRFE_MSPI_CONFIG_DEV, (void *)&mspi_dev_config_msg,
			 sizeof(nrfe_mspi_dev_config_msg_t));
}

static int api_get_channel_status(const struct device *dev, uint8_t ch)
{
	return 0;
}

static int nrfe_mspi_init(const struct device *dev)
{
	int ret;
	const struct device *ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	const struct mspi_nrfe_config *drv_cfg = dev->config;
	const struct mspi_dt_spec spec = {
		.bus = dev,
		.config = drv_cfg->mspicfg,
	};

	ret = pinctrl_apply_state(drv_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = ipc_service_open_instance(ipc_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	ret = ipc_service_register_endpoint(ipc_instance, &ep, &ep_cfg);
	if (ret < 0) {
		LOG_ERR("ipc_service_register_endpoint() failure");
		return ret;
	}

	/* Wait for ep to be bounded */
	k_sem_take(&ipc_sem, K_FOREVER);

	ret = api_config(&spec);
	if (ret < 0) {
		return ret;
	}

	return ret;
}

static const struct mspi_driver_api drv_api = {
	.config = api_config,
	.dev_config = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive = api_transceive,
};

DEVICE_DT_INST_DEFINE(0, nrfe_mspi_init, PM_DEVICE_DT_INST_GET(0), &dev_data, &dev_config,
		      POST_KERNEL, CONFIG_MSPI_NRFE_INIT_PRIORITY, &drv_api);
