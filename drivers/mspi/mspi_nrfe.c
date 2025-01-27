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
#if !defined(CONFIG_MULTITHREADING)
#include <zephyr/sys/atomic.h>
#endif
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_nrfe, CONFIG_MSPI_LOG_LEVEL);

#include <hal/nrf_gpio.h>
#include <drivers/mspi/nrfe_mspi.h>

#define MSPI_NRFE_NODE	   DT_DRV_INST(0)
#define MAX_TX_MSG_SIZE	   (DT_REG_SIZE(DT_NODELABEL(sram_tx)))
#define MAX_RX_MSG_SIZE	   (DT_REG_SIZE(DT_NODELABEL(sram_rx)))
#define CONFIG_TIMEOUT_MS  100
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

#if defined(CONFIG_MULTITHREADING)
static K_SEM_DEFINE(ipc_sem, 0, 1);
static K_SEM_DEFINE(ipc_sem_cfg, 0, 1);
static K_SEM_DEFINE(ipc_sem_xfer, 0, 1);
#else
static atomic_t ipc_atomic_sem = ATOMIC_INIT(0);
#endif

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
	struct mspi_xfer xfer;
	struct mspi_dev_id dev_id;
	struct mspi_dev_cfg dev_cfg;
};

static struct mspi_nrfe_data dev_data;

struct mspi_nrfe_config {
	struct mspi_cfg mspicfg;
	const struct pinctrl_dev_config *pcfg;
};

static const struct mspi_nrfe_config dev_config = {
	.mspicfg = MSPI_CONFIG,
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static void ipc_recv_clbk(const void *data, size_t len);

static void ep_bound(void *priv)
{
	ipc_received = 0;
#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&ipc_sem);
#else
	atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_EP_BOUNDED);
#endif
	LOG_DBG("Ep bounded");
}

static void ep_recv(const void *data, size_t len, void *priv)
{
	(void)priv;

	ipc_recv_clbk(data, len);
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {
		.bound = ep_bound,
		.received = ep_recv,
	},
};

/**
 * @brief IPC receive callback function.
 *
 * This function is called by the IPC stack when a message is received from the
 * other core. The function checks the opcode of the received message and takes
 * appropriate action.
 *
 * @param data Pointer to the received message.
 * @param len Length of the received message.
 */
static void ipc_recv_clbk(const void *data, size_t len)
{
	nrfe_mspi_flpr_response_t *response = (nrfe_mspi_flpr_response_t *)data;

	switch (response->opcode) {
	case NRFE_MSPI_CONFIG_PINS: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_CONFIG_PINS);
#endif
		break;
	}
	case NRFE_MSPI_CONFIG_CTRL: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_CONFIG_CTRL);
#endif
		break;
	}
	case NRFE_MSPI_CONFIG_DEV: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_CONFIG_DEV);
#endif
		break;
	}
	case NRFE_MSPI_CONFIG_XFER: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_CONFIG_XFER);
#endif
		break;
	}
	case NRFE_MSPI_TX: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_xfer);
#else
		atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_TX);
#endif
		break;
	}
	case NRFE_MSPI_TXRX: {
		if (len > 0) {
			ipc_received = len - 1;
			ipc_receive_buffer = (uint8_t *)&response->data;
		}
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_xfer);
#else
		atomic_set_bit(&ipc_atomic_sem, NRFE_MSPI_TXRX);
#endif
		break;
	}
	default: {
		LOG_ERR("Invalid response opcode: %d", response->opcode);
		break;
	}
	}

	LOG_DBG("Received msg with opcode: %d", response->opcode);
}

/**
 * @brief Send data to the flpr with the given opcode.
 *
 * @param opcode The opcode of the message to send.
 * @param data The data to send.
 * @param len The length of the data to send.
 *
 * @return 0 on success, -ENOMEM if there is no space in the buffer,
 *         -ETIMEDOUT if the transfer timed out.
 */
static int mspi_ipc_data_send(enum nrfe_mspi_opcode opcode, const void *data, size_t len)
{
	int rc;

	LOG_DBG("Sending msg with opcode: %d", (uint8_t)opcode);
#if defined(CONFIG_SYS_CLOCK_EXISTS)
	uint32_t start = k_uptime_get_32();
#else
	uint32_t repeat = EP_SEND_TIMEOUT_MS;
#endif
#if !defined(CONFIG_MULTITHREADING)
	atomic_clear_bit(&ipc_atomic_sem, opcode);
#endif

	do {
		rc = ipc_service_send(&ep, data, len);
#if defined(CONFIG_SYS_CLOCK_EXISTS)
		if ((k_uptime_get_32() - start) > EP_SEND_TIMEOUT_MS) {
#else
		repeat--;
		if ((rc < 0) && (repeat == 0)) {
#endif
			break;
		};
	} while (rc == -ENOMEM); /* No space in the buffer. Retry. */

	return rc;
}

/**
 * @brief Waits for a response from the peer with the given opcode.
 *
 * @param opcode The opcode of the response to wait for.
 * @param timeout The timeout in milliseconds.
 *
 * @return 0 on success, -ETIMEDOUT if the operation timed out.
 */
static int nrfe_mspi_wait_for_response(enum nrfe_mspi_opcode opcode, uint32_t timeout)
{
#if defined(CONFIG_MULTITHREADING)
	int ret = 0;

	switch (opcode) {
	case NRFE_MSPI_CONFIG_PINS:
	case NRFE_MSPI_CONFIG_CTRL:
	case NRFE_MSPI_CONFIG_DEV:
	case NRFE_MSPI_CONFIG_XFER: {
		ret = k_sem_take(&ipc_sem_cfg, K_MSEC(timeout));
		break;
	}
	case NRFE_MSPI_TX:
	case NRFE_MSPI_TXRX:
		ret = k_sem_take(&ipc_sem_xfer, K_MSEC(timeout));
		break;
	default:
		break;
	}

	if (ret < 0) {
		return -ETIMEDOUT;
	}
#else
#if defined(CONFIG_SYS_CLOCK_EXISTS)
	uint32_t start = k_uptime_get_32();
#else
	uint32_t repeat = timeout * 1000; /* Convert ms to us */
#endif
	while (!atomic_test_and_clear_bit(&ipc_atomic_sem, opcode)) {
#if defined(CONFIG_SYS_CLOCK_EXISTS)
		if ((k_uptime_get_32() - start) > timeout) {
			return -ETIMEDOUT;
		};
#else
		repeat--;
		if (!repeat) {
			return -ETIMEDOUT;
		};
#endif
		k_sleep(K_USEC(1));
	}
#endif /* CONFIG_MULTITHREADING */
	return 0;
}

/**
 * @brief Send a data struct to the FLPR core using the IPC service.
 *
 * The function sends a data structure to the FLPR core,
 * inserting a byte at the beginning responsible for the opcode.
 *
 * @param opcode The NRFE MSPI opcode.
 * @param data The data to send.
 * @param len The length of the data to send.
 *
 * @return 0 on success, negative errno code on failure.
 */
static int send_with_opcode(enum nrfe_mspi_opcode opcode, const void *data, size_t len)
{
	uint8_t buffer[len + 1];

	buffer[0] = (uint8_t)opcode;
	memcpy(&buffer[1], data, len);

	return mspi_ipc_data_send(opcode, buffer, sizeof(buffer));
}

/**
 * @brief Send a configuration struct to the FLPR core using the IPC service.
 *
 * @param opcode The configuration packet opcode to send.
 * @param config The data to send.
 * @param len The length of the data to send.
 *
 * @return 0 on success, negative errno code on failure.
 */
static int send_config(enum nrfe_mspi_opcode opcode, const void *config, size_t len)
{
	int rc;

	rc = send_with_opcode(opcode, config, len);
	if (rc < 0) {
		LOG_ERR("Configuration send failed: %d", rc);
		return rc;
	}

	rc = nrfe_mspi_wait_for_response(opcode, CONFIG_TIMEOUT_MS);
	if (rc < 0) {
		LOG_ERR("FLPR config: %d response timeout: %d!", opcode, rc);
	}

	return rc;
}

/**
 * @brief Configures the MSPI controller based on the provided spec.
 *
 * This function configures the MSPI controller according to the provided
 * spec. It checks if the spec is valid and sends the configuration to
 * the FLPR.
 *
 * @param spec The MSPI spec to use for configuration.
 *
 * @return 0 on success, negative errno code on failure.
 */
static int api_config(const struct mspi_dt_spec *spec)
{
	int ret;
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_nrfe_config *drv_cfg = spec->bus->config;

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
	nrfe_mspi_pinctrl_soc_pin_t pins_cfg;

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
		pins_cfg.pin[i] = drv_cfg->pcfg->states[state_id].pins[i];
	}

	/* Send pinout configuration to FLPR */
	ret = send_config(NRFE_MSPI_CONFIG_PINS, (const void *)pins_cfg.pin, sizeof(pins_cfg));
	if (ret < 0) {
		return ret;
	}

	/* Send controller configuration to FLPR */
	return send_config(NRFE_MSPI_CONFIG_CTRL, (const void *)config, sizeof(struct mspi_cfg));
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

/**
 * @brief Configure a device on the MSPI bus.
 *
 * @param dev MSPI controller device.
 * @param dev_id Device ID to configure.
 * @param param_mask Bitmask of parameters to configure.
 * @param cfg Device configuration.
 *
 * @return 0 on success, negative errno code on failure.
 */
static int api_dev_config(const struct device *dev, const struct mspi_dev_id *dev_id,
			  const enum mspi_dev_cfg_mask param_mask, const struct mspi_dev_cfg *cfg)
{
	const struct mspi_nrfe_config *drv_cfg = dev->config;
	struct mspi_nrfe_data *drv_data = dev->data;
	int rc;

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

	memcpy((void *)&drv_data->dev_cfg, (void *)cfg, sizeof(drv_data->dev_cfg));
	drv_data->dev_id = *dev_id;

	return send_config(NRFE_MSPI_CONFIG_DEV, (void *)cfg, sizeof(struct mspi_dev_cfg));
}

static int api_get_channel_status(const struct device *dev, uint8_t ch)
{
	return 0;
}

/**
 * @brief Send a transfer packet to the eMSPI controller.
 *
 * @param dev eMSPI controller device
 * @param packet Transfer packet containing the data to be transferred
 * @param timeout Timeout in milliseconds
 *
 * @retval 0 on success
 * @retval -ENOTSUP if the packet is not supported
 * @retval -ENOMEM if there is no space in the buffer
 * @retval -ETIMEDOUT if the transfer timed out
 */
static int xfer_packet(struct mspi_xfer_packet *packet, uint32_t timeout)
{
	int rc;
	uint32_t struct_size = sizeof(struct mspi_xfer_packet);
	uint32_t len = struct_size + packet->num_bytes + 1;
	uint8_t buffer[len];
	enum nrfe_mspi_opcode opcode = (packet->dir == MSPI_RX) ? NRFE_MSPI_TXRX : NRFE_MSPI_TX;

	buffer[0] = (uint8_t)opcode;
	memcpy((void *)&buffer[1], (void *)packet, struct_size);
	memcpy((void *)(&buffer[1] + struct_size), (void *)packet->data_buf, packet->num_bytes);

	rc = mspi_ipc_data_send(opcode, buffer, len);
	if (rc < 0) {
		LOG_ERR("Packet transfer error: %d", rc);
	}

	rc = nrfe_mspi_wait_for_response(opcode, timeout);
	if (rc < 0) {
		LOG_ERR("FLPR Xfer response timeout: %d", rc);
		return rc;
	}

	/* Wait for the transfer to complete and receive data. */
	if ((packet->dir == MSPI_RX) && (ipc_receive_buffer != NULL) && (ipc_received > 0)) {
		memcpy((void *)packet->data_buf, (void *)ipc_receive_buffer, ipc_received);
		packet->num_bytes = ipc_received;

		/* Clear the receive buffer pointer and size */
		ipc_receive_buffer = NULL;
		ipc_received = 0;
	}

	return rc;
}

/**
 * @brief Initiates the transfer of the next packet in an MSPI transaction.
 *
 * This function prepares and starts the transmission of the next packet
 * specified in the MSPI transfer configuration. It checks if the packet
 * size is within the allowable limits before initiating the transfer.
 *
 * @param xfer Pointer to the mspi_xfer structure.
 * @param packets_done Number of packets that have already been processed.
 *
 * @retval 0 If the packet transfer is successfully started.
 * @retval -EINVAL If the packet size exceeds the maximum transmission size.
 */
static int start_next_packet(struct mspi_xfer *xfer, uint32_t packets_done)
{
	struct mspi_xfer_packet *packet = (struct mspi_xfer_packet *)&xfer->packets[packets_done];

	if (packet->num_bytes >= MAX_TX_MSG_SIZE) {
		LOG_ERR("Packet size to large: %u. Increase SRAM data region.", packet->num_bytes);
		return -EINVAL;
	}

	return xfer_packet(packet, xfer->timeout);
}

/**
 * @brief Send a multi-packet transfer request to the host.
 *
 * This function sends a multi-packet transfer request to the host and waits
 * for the host to complete the transfer. This function does not support
 * asynchronous transfers.
 *
 * @param dev Pointer to the device structure.
 * @param dev_id Pointer to the device identification structure.
 * @param req Pointer to the xfer structure.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If asynchronous transfers are requested.
 * @retval -EIO If an I/O error occurs.
 */
static int api_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	(void)dev_id;
	struct mspi_nrfe_data *drv_data = dev->data;
	uint32_t packets_done = 0;
	int rc;

	/* TODO: add support for asynchronous transfers */
	if (req->async) {
		return -ENOTSUP;
	}

	if (req->num_packet == 0 || !req->packets ||
	    req->timeout > CONFIG_MSPI_COMPLETION_TIMEOUT_TOLERANCE) {
		return -EFAULT;
	}

	drv_data->xfer = *req;

	rc = send_config(NRFE_MSPI_CONFIG_XFER, (void *)&drv_data->xfer, sizeof(struct mspi_xfer));
	if (rc < 0) {
		LOG_ERR("Send xfer config error: %d", rc);
		return rc;
	}

	while (packets_done < drv_data->xfer.num_packet) {
		rc = start_next_packet(&drv_data->xfer, packets_done);
		if (rc < 0) {
			LOG_ERR("Start next packet error: %d", rc);
			return rc;
		}
		++packets_done;
	}

	return 0;
}

#if CONFIG_PM_DEVICE
/**
 * @brief Callback function to handle power management actions.
 *
 * This function is responsible for handling power management actions
 * such as suspend and resume for the given device. It performs the
 * necessary operations when the device is requested to transition
 * between different power states.
 *
 * @param dev Pointer to the device structure.
 * @param action The power management action to be performed.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If the action is not supported.
 */
static int dev_pm_action_cb(const struct device *dev, enum pm_device_action action)
{
	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		/* TODO: Handle PM suspend state */
		break;
	case PM_DEVICE_ACTION_RESUME:
		/* TODO: Handle PM resume state */
		break;
	default:
		return -ENOTSUP;
	}

	return 0;
}
#endif

/**
 * @brief Initialize the MSPI NRFE driver.
 *
 * This function initializes the MSPI NRFE driver. It is responsible for
 * setting up the hardware and registering the IPC endpoint for the
 * driver.
 *
 * @param dev Pointer to the device structure for the MSPI NRFE driver.
 *
 * @retval 0 If successful.
 * @retval -errno If an error occurs.
 */
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
#if defined(CONFIG_MULTITHREADING)
	k_sem_take(&ipc_sem, K_FOREVER);
#else
	while (!atomic_test_and_clear_bit(&ipc_atomic_sem, NRFE_MSPI_EP_BOUNDED)) {
	}
#endif

	ret = api_config(&spec);
	if (ret < 0) {
		return ret;
	}

#if CONFIG_PM_DEVICE
	ret = pm_device_driver_init(dev, dev_pm_action_cb);
	if (ret < 0) {
		return ret;
	}
#endif
	return ret;
}

static const struct mspi_driver_api drv_api = {
	.config = api_config,
	.dev_config = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive = api_transceive,
};

PM_DEVICE_DT_INST_DEFINE(0, dev_pm_action_cb);

DEVICE_DT_INST_DEFINE(0, nrfe_mspi_init, PM_DEVICE_DT_INST_GET(0), &dev_data, &dev_config,
		      POST_KERNEL, CONFIG_MSPI_NRFE_INIT_PRIORITY, &drv_api);
