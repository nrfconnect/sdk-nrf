/*
 * Copyright (c) 2024, Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#define DT_DRV_COMPAT nordic_hpf_mspi_controller

#include <zephyr/kernel.h>
#include <zephyr/drivers/mspi.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/pm/device.h>
#include <zephyr/pm/device_runtime.h>
#include <zephyr/sys/atomic.h>
#include <hal/nrf_vpr.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mspi_hpf, CONFIG_MSPI_LOG_LEVEL);

#include <drivers/mspi/hpf_mspi.h>

#define MSPI_HPF_NODE		     DT_DRV_INST(0)
#define MAX_TX_MSG_SIZE		     (DT_REG_SIZE(DT_NODELABEL(sram_tx)))
#define MAX_RX_MSG_SIZE		     (DT_REG_SIZE(DT_NODELABEL(sram_rx)))
#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
/* Unaligned data is bounced through a buffer on the stack.
 * Flash page program can be max 256 bytes in this case.
 */
#define BOUNCE_BUF_SIZE		     256
#else
/* The transfer packet message is built on the stack before ICMsg copies it into the TX region. */
#define MAX_COPY_MSG_SIZE	     MAX_TX_MSG_SIZE
#define MAX_STACK_MSG_SIZE	     1024
/* ICMsg drops a received message that does not fit its read buffer. */
#define MAX_REPLY_MSG_SIZE	     MIN(MAX_RX_MSG_SIZE, CONFIG_PBUF_RX_READ_BUF_SIZE)
#endif
/* Alignment the FLPR requires for data passed by reference. */
#define DATA_BUF_ALIGNMENT	     sizeof(uint32_t)
#define HPF_MSPI_IPC_CONFIG_TIMEOUT_MS 100
#define IPC_BOUND_TIMEOUT_MS	     100
#define IPC_BOUND_RETRY_COUNT	     10
#define IPC_BOUND_RETRY_DELAY_MS	     10
#define EP_SEND_TIMEOUT_MS	     10
#define EXTREME_DRIVE_FREQ_THRESHOLD 32000000
#define CNT0_TOP_CALCULATE(freq)     (NRFX_CEIL_DIV(SystemCoreClock, freq * 2) - 1)
#define DATA_LINE_INDEX(pinctr_fun)  (pinctr_fun - NRF_FUN_HPF_MSPI_DQ0)
#define DATA_PIN_UNUSED              UINT8_MAX
#define FLPR_VPR		     ((NRF_VPR_Type *)DT_REG_ADDR(DT_NODELABEL(cpuflpr_vpr)))

#if defined(CONFIG_SOC_NRF54L15) || defined(CONFIG_SOC_NRF54LM20A) || \
	defined(CONFIG_SOC_NRF54LM20B)

#define HPF_MSPI_PORT_NUMBER	2 /* Physical port number */
#define HPF_MSPI_SCK_PIN_NUMBER 1 /* Physical pin number on port 2 */

#define HPF_MSPI_DATA_LINE_CNT_MAX 8
#define HPF_MSPI_CS_LINE_CNT_MAX 5
#define MAX_MSPI_DUMMY_CLOCKS 59
#elif defined(CONFIG_SOC_NRF54LV10A) || defined(CONFIG_SOC_NRF54LC10A)
#define HPF_MSPI_PORT_NUMBER	1  /* Physical port number */
#define HPF_MSPI_SCK_PIN_NUMBER 16 /* Physical pin number on port 1 */

#define HPF_MSPI_DATA_LINE_CNT_MAX 8
#define HPF_MSPI_CS_LINE_CNT_MAX 5
#define MAX_MSPI_DUMMY_CLOCKS 59

#else
#error "Unsupported SoC for HPF MSPI"
#endif

#ifdef CONFIG_PINCTRL_STORE_REG
#define HPF_MPSI_PINCTRL_DEV_CONFIG_INIT(node_id)                                                  \
	{                                                                                          \
		.reg = PINCTRL_REG_NONE,                                                           \
		.states = Z_PINCTRL_STATES_NAME(node_id),                                          \
		.state_cnt = ARRAY_SIZE(Z_PINCTRL_STATES_NAME(node_id)),                           \
	}
#else
#define HPF_MPSI_PINCTRL_DEV_CONFIG_INIT(node_id)                                                  \
	{                                                                                          \
		.states = Z_PINCTRL_STATES_NAME(node_id),                                          \
		.state_cnt = ARRAY_SIZE(Z_PINCTRL_STATES_NAME(node_id)),                           \
	}
#endif

#define HPF_MSPI_PINCTRL_DT_DEFINE(node_id)                                                        \
	LISTIFY(DT_NUM_PINCTRL_STATES(node_id), Z_PINCTRL_STATE_PINS_DEFINE, (;), node_id);        \
	Z_PINCTRL_STATES_DEFINE(node_id)                                                           \
	Z_PINCTRL_DEV_CONFIG_STATIC Z_PINCTRL_DEV_CONFIG_CONST struct pinctrl_dev_config           \
	Z_PINCTRL_DEV_CONFIG_NAME(node_id) = HPF_MPSI_PINCTRL_DEV_CONFIG_INIT(node_id)

HPF_MSPI_PINCTRL_DT_DEFINE(MSPI_HPF_NODE);

static struct ipc_ept ep;
#ifndef CONFIG_MSPI_HPF_IPC_NO_COPY
/* ICMsg passes a received message in a buffer that is valid only during the receive callback,
 * so the reply data is copied there straight into the buffer of the pending RX packet.
 * The receive callback runs in an interrupt or in a cooperative thread, so the thread that sends
 * a packet cannot preempt it. Clearing rx_dest before that thread returns is therefore enough to
 * keep a late reply from writing into the buffer afterwards.
 */
BUILD_ASSERT(!IS_ENABLED(CONFIG_SMP) &&
		     (!IS_ENABLED(CONFIG_MULTITHREADING) ||
		      IS_ENABLED(CONFIG_IPC_SERVICE_BACKEND_ICMSG_WQ_ENABLE) ||
		      (CONFIG_SYSTEM_WORKQUEUE_PRIORITY < 0)),
	     "Reply handling requires a single CPU and ICMsg calling back from a cooperative "
	     "thread");
static uint8_t *volatile rx_dest;
static volatile size_t rx_dest_len;
static volatile size_t ipc_received;
#endif
static volatile uint32_t *cpuflpr_error_ctx_ptr =
	(uint32_t *)DT_REG_ADDR(DT_NODELABEL(cpuflpr_error_code));

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
		.op_mode = DT_ENUM_IDX_OR(MSPI_HPF_NODE, op_mode, MSPI_OP_MODE_CONTROLLER),        \
		.duplex = DT_ENUM_IDX_OR(MSPI_HPF_NODE, duplex, MSPI_HALF_DUPLEX),                 \
		.dqs_support = DT_PROP_OR(MSPI_HPF_NODE, dqs_support, false),                      \
		.num_periph = DT_CHILD_NUM(MSPI_HPF_NODE),                                         \
		.max_freq = DT_PROP(MSPI_HPF_NODE, clock_frequency),                               \
		.re_init = true,                                                                   \
		.sw_multi_periph = false,                                                          \
	}

struct mspi_hpf_data {
	hpf_mspi_xfer_config_msg_t xfer_config_msg;
};

struct mspi_hpf_config {
	struct mspi_cfg mspicfg;
	const struct pinctrl_dev_config *pcfg;
};

static const struct mspi_hpf_config dev_config = {
	.mspicfg = MSPI_CONFIG,
	.pcfg = PINCTRL_DT_INST_DEV_CONFIG_GET(0),
};

static struct mspi_hpf_data dev_data;

/* Set once the FLPR has been halted after a response timeout. */
static atomic_t flpr_halted = ATOMIC_INIT(0);

static void ep_recv(const void *data, size_t len, void *priv);

static void ep_bound(void *priv)
{
#if defined(CONFIG_MULTITHREADING)
	k_sem_give(&ipc_sem);
#else
	atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED);
#endif
	LOG_DBG("Ep bounded");
}

static struct ipc_ept_cfg ep_cfg = {
	.cb = {.bound = ep_bound, .received = ep_recv},
};

const char *z_riscv_mcause_str(uint32_t cause)
{
	static const char *const mcause_str[17] = {
		[0] = "Instruction address misaligned",
		[1] = "Instruction Access fault",
		[2] = "Illegal instruction",
		[3] = "Breakpoint",
		[4] = "Load address misaligned",
		[5] = "Load access fault",
		[6] = "Store/AMO address misaligned",
		[7] = "Store/AMO access fault",
		[8] = "Environment call from U-mode",
		[9] = "Environment call from S-mode",
		[10] = "Unknown",
		[11] = "Environment call from M-mode",
		[12] = "Instruction page fault",
		[13] = "Load page fault",
		[14] = "Unknown",
		[15] = "Store/AMO page fault",
		[16] = "Unknown",
	};

	return mcause_str[MIN(cause, ARRAY_SIZE(mcause_str) - 1)];
}

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
static void ep_recv(const void *data, size_t len, void *priv)
{
	hpf_mspi_flpr_response_msg_t *response = (hpf_mspi_flpr_response_msg_t *)data;

	switch (response->opcode) {
#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
	case HPF_MSPI_CONFIG_TIMER_PTR: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_CONFIG_TIMER_PTR);
#endif
		break;
	}
#endif
	case HPF_MSPI_CONFIG_PINS: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_CONFIG_PINS);
#endif
		break;
	}
	case HPF_MSPI_CONFIG_DEV: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_CONFIG_DEV);
#endif
		break;
	}
	case HPF_MSPI_CONFIG_XFER: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_cfg);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_CONFIG_XFER);
#endif
		break;
	}
	case HPF_MSPI_TX: {
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_xfer);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_TX);
#endif
		break;
	}
	case HPF_MSPI_TXRX: {
#ifndef CONFIG_MSPI_HPF_IPC_NO_COPY
		uint8_t *dest = rx_dest;

		ipc_received = (len >= sizeof(hpf_mspi_opcode_t)) ? len - sizeof(hpf_mspi_opcode_t)
								   : 0;
		if ((dest != NULL) && (ipc_received == rx_dest_len)) {
			memcpy(dest, &response->data, ipc_received);
		}
#endif
#if defined(CONFIG_MULTITHREADING)
		k_sem_give(&ipc_sem_xfer);
#else
		atomic_set_bit(&ipc_atomic_sem, HPF_MSPI_TXRX);
#endif
		break;
	}
	case HPF_MSPI_HPF_APP_HARD_FAULT: {

		const uint32_t mcause_exc_mask = 0xfff;
		volatile uint32_t cause = cpuflpr_error_ctx_ptr[0];
		volatile uint32_t pc = cpuflpr_error_ctx_ptr[1];
		volatile uint32_t bad_addr = cpuflpr_error_ctx_ptr[2];
		volatile uint32_t *ctx = (volatile uint32_t *)cpuflpr_error_ctx_ptr[3];

		LOG_ERR(">>> HPF APP FATAL ERROR: %s", z_riscv_mcause_str(cause & mcause_exc_mask));
		LOG_ERR("Faulting instruction address (mepc): 0x%08x", pc);
		LOG_ERR("mcause: 0x%08x, mtval: 0x%08x, ra: 0x%08x", cause, bad_addr, ctx[0]);
		LOG_ERR("    t0: 0x%08x,    t1: 0x%08x, t2: 0x%08x", ctx[1], ctx[2], ctx[3]);

		LOG_ERR("HPF application halted...");
		break;
	}
	default: {
		LOG_ERR("Invalid response opcode: %d", response->opcode);
		break;
	}
	}

	LOG_HEXDUMP_DBG((uint8_t *)data, len, "Received msg:");
}

/**
 * @brief Waits for a response from the peer with the given opcode.
 *
 * @param opcode The opcode of the response to wait for.
 * @param timeout The timeout in milliseconds.
 *
 * @return 0 on success, -ETIMEDOUT if the operation timed out.
 */
static int hpf_mspi_wait_for_response(hpf_mspi_opcode_t opcode, uint32_t timeout)
{
#if defined(CONFIG_MULTITHREADING)
	int ret = 0;

	switch (opcode) {
	case HPF_MSPI_CONFIG_TIMER_PTR:
		ret = k_sem_take(&ipc_sem, K_MSEC(timeout));
		break;
	case HPF_MSPI_CONFIG_PINS:
	case HPF_MSPI_CONFIG_DEV:
	case HPF_MSPI_CONFIG_XFER: {
		ret = k_sem_take(&ipc_sem_cfg, K_MSEC(timeout));
		break;
	}
	case HPF_MSPI_TX:
	case HPF_MSPI_TXRX:
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

static int hpf_mspi_wait_for_endpoint_bound(void)
{
#if defined(CONFIG_MULTITHREADING)
	int ret = k_sem_take(&ipc_sem, K_MSEC(IPC_BOUND_TIMEOUT_MS));

	return (ret < 0) ? -ETIMEDOUT : 0;
#else
#if defined(CONFIG_SYS_CLOCK_EXISTS)
	uint32_t start = k_uptime_get_32();
#else
	uint32_t repeat = IPC_BOUND_TIMEOUT_MS * 1000; /* Convert ms to us */
#endif

	while (!atomic_test_and_clear_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED)) {
#if defined(CONFIG_SYS_CLOCK_EXISTS)
		if ((k_uptime_get_32() - start) > IPC_BOUND_TIMEOUT_MS) {
			return -ETIMEDOUT;
		}
#else
		repeat--;
		if (repeat == 0U) {
			return -ETIMEDOUT;
		}
#endif
		k_sleep(K_USEC(1));
	}

	return 0;
#endif
}

static int hpf_mspi_register_endpoint_with_retry(const struct device *ipc_instance)
{
	int ret;

	for (int attempt = 0; attempt < IPC_BOUND_RETRY_COUNT; attempt++) {
#if defined(CONFIG_MULTITHREADING)
		k_sem_reset(&ipc_sem);
#else
		atomic_clear_bit(&ipc_atomic_sem, HPF_MSPI_EP_BOUNDED);
#endif

		ret = ipc_service_register_endpoint(ipc_instance, &ep, &ep_cfg);
		if (ret < 0) {
			LOG_ERR("ipc_service_register_endpoint() failure");
			return ret;
		}

		ret = hpf_mspi_wait_for_endpoint_bound();
		if (ret == 0) {
			return 0;
		}

		ret = ipc_service_deregister_endpoint(&ep);
		if (ret < 0) {
			LOG_ERR("ipc_service_deregister_endpoint() failure");
			return ret;
		}

		k_sleep(K_MSEC(IPC_BOUND_RETRY_DELAY_MS));
	}

	return -ETIMEDOUT;
}

/**
 * @brief Stop the FLPR after it failed to respond in time.
 */
static void hpf_mspi_halt_flpr(void)
{
	/* CPURUN only takes effect on a core reset, so reset the core after clearing it. */
	nrf_vpr_cpurun_set(FLPR_VPR, false);
	nrf_vpr_debugif_dmcontrol_mask_set(
		FLPR_VPR,
		(VPR_DEBUGIF_DMCONTROL_NDMRESET_Active << VPR_DEBUGIF_DMCONTROL_NDMRESET_Pos) |
			(VPR_DEBUGIF_DMCONTROL_DMACTIVE_Enabled
			 << VPR_DEBUGIF_DMCONTROL_DMACTIVE_Pos));
	nrf_vpr_debugif_dmcontrol_mask_set(
		FLPR_VPR,
		(VPR_DEBUGIF_DMCONTROL_NDMRESET_Inactive << VPR_DEBUGIF_DMCONTROL_NDMRESET_Pos) |
			(VPR_DEBUGIF_DMCONTROL_DMACTIVE_Disabled
			 << VPR_DEBUGIF_DMCONTROL_DMACTIVE_Pos));

	atomic_set(&flpr_halted, 1);

#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
	/* The FLPR stops the fault timer only when it completes a transfer. */
	(void)counter_stop(DEVICE_DT_GET(DT_NODELABEL(fault_timer)));
#endif

	/* Hand the pins back from the stopped FLPR, if a sleep state is defined. */
	(void)pinctrl_apply_state(dev_config.pcfg, PINCTRL_STATE_SLEEP);

	LOG_ERR("FLPR stopped after response timeout, driver disabled");
}

/**
 * @brief Send data to the FLPR core using the IPC service, and wait for FLPR response.
 *
 * The length of the data is ignored in no copy mode, where only the pointer
 * to the data is sent.
 *
 * @param opcode The configuration packet opcode to send.
 * @param data The data to send.
 * @param len The length of the data to send.
 * @param timeout_ms Maximum time to wait for the FLPR response, in milliseconds.
 *
 * @retval 0 on success.
 * @retval -EIO if the FLPR was stopped after an earlier response timeout.
 * @retval -ETIMEDOUT if the FLPR did not respond in time. The FLPR is stopped before returning.
 * @retval -errno other negative errno code on failure.
 */
static int send_data(hpf_mspi_opcode_t opcode, const void *data, size_t len,
		     uint32_t timeout_ms)
{
	LOG_DBG("Sending msg with opcode: %d", (uint8_t)opcode);

	int rc;

	if (atomic_get(&flpr_halted)) {
		return -EIO;
	}
#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
	(void)len;
	void *data_ptr = (void *)data;
#endif

#if defined(CONFIG_SYS_CLOCK_EXISTS)
	uint32_t start = k_uptime_get_32();
#else
	uint32_t repeat = EP_SEND_TIMEOUT_MS;
#endif
#if !defined(CONFIG_MULTITHREADING)
	atomic_clear_bit(&ipc_atomic_sem, opcode);
#endif

	do {
#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
		rc = ipc_service_send(&ep, &data_ptr, sizeof(void *));
#else
		rc = ipc_service_send(&ep, data, len);
#endif
#if defined(CONFIG_SYS_CLOCK_EXISTS)
		if ((k_uptime_get_32() - start) > EP_SEND_TIMEOUT_MS) {
#else
		repeat--;
		if ((rc < 0) && (repeat == 0)) {
#endif
			break;
		};
	} while (rc == -ENOMEM); /* No space in the buffer. Retry. */

	if (rc < 0) {
		LOG_ERR("Data transfer failed: %d", rc);
		return rc;
	}

	rc = hpf_mspi_wait_for_response(opcode, timeout_ms);
	if (rc < 0) {
		hpf_mspi_halt_flpr();
		LOG_ERR("Data transfer: %d response timeout: %d!", opcode, rc);
	}

	return rc;
}

static int check_pin_assignments(const struct pinctrl_state *state)
{
	uint8_t data_pins[HPF_MSPI_DATA_LINE_CNT_MAX];
	uint8_t data_pins_cnt = 0;
	uint8_t cs_pins[HPF_MSPI_PIN_COUNT];
	uint8_t cs_pins_cnt = 0;
	uint32_t psel = 0;
	uint32_t pin_fun = 0;

	for (uint8_t i = 0; i < HPF_MSPI_DATA_LINE_CNT_MAX; i++) {
		data_pins[i] = DATA_PIN_UNUSED;
	}

	for (uint8_t i = 0; i < state->pin_cnt; i++) {
		psel = NRF_GET_PIN(state->pins[i]);
		if (NRF_PIN_NUMBER_TO_PORT(psel) != HPF_MSPI_PORT_NUMBER) {
			LOG_ERR("Wrong port number. Only %d port is supported.",
				HPF_MSPI_PORT_NUMBER);
			return -ENOTSUP;
		}
		pin_fun = NRF_GET_FUN(state->pins[i]);
		switch (pin_fun) {
		case NRF_FUN_HPF_MSPI_DQ0:
		case NRF_FUN_HPF_MSPI_DQ1:
		case NRF_FUN_HPF_MSPI_DQ2:
		case NRF_FUN_HPF_MSPI_DQ3:
		case NRF_FUN_HPF_MSPI_DQ4:
		case NRF_FUN_HPF_MSPI_DQ5:
		case NRF_FUN_HPF_MSPI_DQ6:
		case NRF_FUN_HPF_MSPI_DQ7:
			if (data_pins[DATA_LINE_INDEX(pin_fun)] != DATA_PIN_UNUSED) {
				LOG_ERR("This pin is assigned to an already taken data line: "
					"%d.%d.",
					NRF_PIN_NUMBER_TO_PORT(psel), NRF_PIN_NUMBER_TO_PIN(psel));
				return -EINVAL;
			}
			data_pins[DATA_LINE_INDEX(pin_fun)] = NRF_PIN_NUMBER_TO_PIN(psel);
			data_pins_cnt++;
			break;
		case NRF_FUN_HPF_MSPI_CS0:
		case NRF_FUN_HPF_MSPI_CS1:
		case NRF_FUN_HPF_MSPI_CS2:
		case NRF_FUN_HPF_MSPI_CS3:
		case NRF_FUN_HPF_MSPI_CS4:
			cs_pins[cs_pins_cnt] = NRF_PIN_NUMBER_TO_PIN(psel);
			cs_pins_cnt++;
			break;
		case NRF_FUN_HPF_MSPI_SCK:
			if (NRF_PIN_NUMBER_TO_PIN(psel) != HPF_MSPI_SCK_PIN_NUMBER) {
				LOG_ERR("Clock signal only supported on pin %d.%d",
					HPF_MSPI_PORT_NUMBER, HPF_MSPI_SCK_PIN_NUMBER);
				return -ENOTSUP;
			}
			break;
		default:
			LOG_ERR("Not supported pin function: %d", pin_fun);
			return -ENOTSUP;
		}
	}

	if (cs_pins_cnt == 0) {
		LOG_ERR("No CS pin defined.");
		return -EINVAL;
	}

	for (uint8_t i = 0; i < cs_pins_cnt; i++) {
		for (uint8_t j = 0; j < data_pins_cnt; j++) {
			if (cs_pins[i] == data_pins[j]) {
				LOG_ERR("CS pin cannot be the same as any data line pin.");
				return -EINVAL;
			}
		}
		if (cs_pins[i] == HPF_MSPI_SCK_PIN_NUMBER) {
			LOG_ERR("CS pin cannot be the same CLK pin.");
			return -EINVAL;
		}
	}

	return 0;
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
	const struct mspi_cfg *config = &spec->config;
	const struct mspi_hpf_config *drv_cfg = spec->bus->config;
	hpf_mspi_pinctrl_soc_pin_msg_t mspi_pin_config;
	int ret;

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

	if (config->duplex != MSPI_HALF_DUPLEX) {
		LOG_ERR("Only half-duplex mode is supported.");
		return -ENOTSUP;
	}

	if (config->num_ce_gpios > HPF_MSPI_CS_LINE_CNT_MAX) {
		LOG_ERR("Invalid number of CE GPIOs: %d", config->num_ce_gpios);
		return -EINVAL;
	}

	if (config->num_periph > HPF_MSPI_CS_LINE_CNT_MAX) {
		LOG_ERR("Invalid MSPI peripheral number.");
		return -EINVAL;
	}

	if (config->num_ce_gpios != 0 &&
	    config->num_ce_gpios != config->num_periph) {
		LOG_ERR("Invalid number of ce_gpios vs num_periph.");
		return -EINVAL;
	}

	/* Create pinout configuration */
	uint8_t state_id;

	for (state_id = 0; state_id < drv_cfg->pcfg->state_cnt; state_id++) {
		if (drv_cfg->pcfg->states[state_id].id == PINCTRL_STATE_DEFAULT) {
			break;
		}
	}

	if (drv_cfg->pcfg->states[state_id].pin_cnt > HPF_MSPI_PIN_COUNT) {
		LOG_ERR("Too many pins defined. Max: %d", HPF_MSPI_PIN_COUNT);
		return -ENOTSUP;
	}

	if (drv_cfg->pcfg->states[state_id].id != PINCTRL_STATE_DEFAULT) {
		LOG_ERR("Pins default state not found.");
		return -ENOTSUP;
	}

	ret = check_pin_assignments(&drv_cfg->pcfg->states[state_id]);

	if (ret < 0) {
		return ret;
	}

	for (uint8_t i = 0; i < drv_cfg->pcfg->states[state_id].pin_cnt; i++) {
		mspi_pin_config.pin[i] = drv_cfg->pcfg->states[state_id].pins[i];
	}
	mspi_pin_config.opcode = HPF_MSPI_CONFIG_PINS;
	mspi_pin_config.pins_count = drv_cfg->pcfg->states[state_id].pin_cnt;

	/* Send pinout configuration to FLPR */
	return send_data(HPF_MSPI_CONFIG_PINS, (const void *)&mspi_pin_config,
			 sizeof(hpf_mspi_pinctrl_soc_pin_msg_t),
			 HPF_MSPI_IPC_CONFIG_TIMEOUT_MS);
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

static int check_pins_for_io_mode(const struct pinctrl_state *state, enum mspi_io_mode io_mode)
{
	bool d0_defined = false;
	bool d1_defined = false;
	uint8_t data_pins_cnt = 0;

	switch (io_mode) {
	case MSPI_IO_MODE_SINGLE: {
		for (uint8_t i = 0; i < state->pin_cnt; i++) {
			if (NRF_GET_FUN(state->pins[i]) == NRF_FUN_HPF_MSPI_DQ0) {
				d0_defined = true;
			} else if (NRF_GET_FUN(state->pins[i]) == NRF_FUN_HPF_MSPI_DQ1) {
				d1_defined = true;
			}
		}
		if (!d0_defined || !d1_defined) {
			LOG_ERR("IO SINGLE mode requires definitions of D0 and D1 pins.");
			return -EINVAL;
		}
		break;
	}
	case MSPI_IO_MODE_QUAD:
	case MSPI_IO_MODE_QUAD_1_1_4:
	case MSPI_IO_MODE_QUAD_1_4_4: {
		for (uint8_t i = 0; i < state->pin_cnt; i++) {
			switch (NRF_GET_FUN(state->pins[i])) {
			case NRF_FUN_HPF_MSPI_DQ0:
			case NRF_FUN_HPF_MSPI_DQ1:
			case NRF_FUN_HPF_MSPI_DQ2:
			case NRF_FUN_HPF_MSPI_DQ3:
				data_pins_cnt++;
				break;
			default:
				break;
			}
		}
		if (data_pins_cnt < 4) {
			LOG_ERR("Not enough data pins for QUAD mode: %d", data_pins_cnt);
			return -EINVAL;
		}
		break;
	}
	default:
		break;
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
	const struct mspi_hpf_config *drv_cfg = dev->config;
	int rc;
	hpf_mspi_dev_config_msg_t mspi_dev_config_msg;

	if (param_mask == MSPI_DEVICE_CONFIG_NONE) {
		return 0;
	}

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

		uint8_t state_id;

		for (state_id = 0; state_id < drv_cfg->pcfg->state_cnt; state_id++) {
			if (drv_cfg->pcfg->states[state_id].id == PINCTRL_STATE_DEFAULT) {
				break;
			}
		}

		if ((cfg->freq >= EXTREME_DRIVE_FREQ_THRESHOLD) &&
		    (NRF_GET_DRIVE(drv_cfg->pcfg->states[state_id].pins[0]) != NRF_DRIVE_E0E1)) {
			LOG_ERR("Invalid pin drive for this frequency: %u, expected: %u",
				NRF_GET_DRIVE(drv_cfg->pcfg->states[state_id].pins[0]),
				NRF_DRIVE_E0E1);
			return -EINVAL;
		}

		if (cfg->freq > drv_cfg->mspicfg.max_freq) {
			LOG_ERR("Invalid frequency: %u, MAX: %u", cfg->freq,
				drv_cfg->mspicfg.max_freq);
			return -EINVAL;
		}

		if (CNT0_TOP_CALCULATE(cfg->freq) > UINT16_MAX) {
			LOG_ERR("Invalid frequency: %u. MIN: %u", cfg->freq,
				NRFX_CEIL_DIV(drv_cfg->mspicfg.max_freq, UINT16_MAX));
			return -EINVAL;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_IO_MODE) {
		rc = check_io_mode(cfg->io_mode);
		if (rc < 0) {
			return rc;
		}
		rc = check_pins_for_io_mode(&drv_cfg->pcfg->states[PINCTRL_STATE_DEFAULT],
					    cfg->io_mode);
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

	if (param_mask & MSPI_DEVICE_CONFIG_CPP) {
		if ((cfg->cpp != MSPI_CPP_MODE_0) && (cfg->cpp != MSPI_CPP_MODE_3)) {
			LOG_ERR("Only CPP modes 0 and 3 are supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_ENDIAN) {
		if (cfg->endian != MSPI_XFER_BIG_ENDIAN) {
			LOG_ERR("Only big endian mode is supported.");
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_CE_NUM) {
		if (cfg->ce_num > HPF_MSPI_CS_LINE_CNT_MAX) {
			LOG_ERR("CE number %d not supported.", cfg->ce_num);
			return -EINVAL;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_RX_DUMMY) {
		if (cfg->rx_dummy > MAX_MSPI_DUMMY_CLOCKS) {
			LOG_ERR("Value of RX dummy clock is too big. Max: %d is supported.",
				MAX_MSPI_DUMMY_CLOCKS);
			return -ENOTSUP;
		}
	}

	if (param_mask & MSPI_DEVICE_CONFIG_TX_DUMMY) {
		if (cfg->tx_dummy > MAX_MSPI_DUMMY_CLOCKS) {
			LOG_ERR("Value of TX dummy clock is too big. Max: %d is supported.",
				MAX_MSPI_DUMMY_CLOCKS);
			return -ENOTSUP;
		}
	}

	mspi_dev_config_msg.opcode = HPF_MSPI_CONFIG_DEV;
	mspi_dev_config_msg.device_index = dev_id->dev_idx;
	mspi_dev_config_msg.dev_config.io_mode = cfg->io_mode;
	mspi_dev_config_msg.dev_config.cpp = cfg->cpp;
	mspi_dev_config_msg.dev_config.ce_polarity = cfg->ce_polarity;
	mspi_dev_config_msg.dev_config.cnt0_value = CNT0_TOP_CALCULATE(cfg->freq);
	mspi_dev_config_msg.dev_config.ce_index = cfg->ce_num;

	return send_data(HPF_MSPI_CONFIG_DEV, (void *)&mspi_dev_config_msg,
			 sizeof(hpf_mspi_dev_config_msg_t),
			 HPF_MSPI_IPC_CONFIG_TIMEOUT_MS);
}

static int api_get_channel_status(const struct device *dev, uint8_t ch)
{
	return 0;
}

#ifndef CONFIG_MSPI_HPF_IPC_NO_COPY
BUILD_ASSERT(MAX_COPY_MSG_SIZE <= MAX_STACK_MSG_SIZE,
	     "TX region is too large for the transfer packet message buffer on the stack. Size "
	     "sram_tx for the largest TX packet only; RX data uses sram_rx.");
BUILD_ASSERT(CONFIG_PBUF_RX_READ_BUF_SIZE >= sizeof(hpf_mspi_opcode_t),
	     "ICMsg read buffer cannot hold a response opcode");
#endif

/**
 * @brief Check whether a packet can be sent to the FLPR.
 *
 * @param packet Transfer packet to check.
 *
 * @retval 0 if the packet can be sent
 * @retval -EINVAL if the packet is too large
 */
static int check_packet_size(const struct mspi_xfer_packet *packet)
{
#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
	if (!IS_ALIGNED(packet->data_buf, DATA_BUF_ALIGNMENT) &&
	    (packet->num_bytes > BOUNCE_BUF_SIZE)) {
		LOG_ERR("Unaligned packet of %u bytes exceeds the %u byte bounce buffer. Use a "
			"word-aligned buffer or declare packet-data-limit.",
			packet->num_bytes, BOUNCE_BUF_SIZE);
		return -EINVAL;
	}
#else
	if ((packet->dir == MSPI_TX) &&
	    (packet->num_bytes > MAX_COPY_MSG_SIZE - sizeof(hpf_mspi_xfer_packet_msg_t))) {
		LOG_ERR("Packet of %u bytes does not fit the %u byte TX region. Declare "
			"packet-data-limit or increase the TX region.",
			packet->num_bytes, (uint32_t)MAX_COPY_MSG_SIZE);
		return -EINVAL;
	}

	if ((packet->dir == MSPI_RX) &&
	    (packet->num_bytes > MAX_REPLY_MSG_SIZE - sizeof(hpf_mspi_opcode_t))) {
		LOG_ERR("Reply of %u bytes does not fit the %u byte message (RX region: %u bytes, "
			"CONFIG_PBUF_RX_READ_BUF_SIZE: %u bytes). Declare packet-data-limit.",
			packet->num_bytes, (uint32_t)MAX_REPLY_MSG_SIZE, (uint32_t)MAX_RX_MSG_SIZE,
			CONFIG_PBUF_RX_READ_BUF_SIZE);
		return -EINVAL;
	}
#endif

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
 * @retval -EINVAL if the packet does not fit the memory shared with the FLPR
 * @retval -ENOTSUP if the packet is not supported
 * @retval -ENOMEM if there is no space in the buffer
 * @retval -ETIMEDOUT if the transfer timed out
 */
static int send_packet(const struct mspi_xfer_packet *packet, uint32_t timeout)
{
	int rc;
	hpf_mspi_opcode_t opcode = (packet->dir == MSPI_RX) ? HPF_MSPI_TXRX : HPF_MSPI_TX;
#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
	/* Only a pointer to the message is sent. */
	hpf_mspi_xfer_packet_msg_t msg;
	hpf_mspi_xfer_packet_msg_t *xfer_packet = &msg;
	uint8_t bounce_buf[BOUNCE_BUF_SIZE] __aligned(DATA_BUF_ALIGNMENT);
	bool bounce = !IS_ALIGNED(packet->data_buf, DATA_BUF_ALIGNMENT);
#else
	uint8_t buffer[MAX_COPY_MSG_SIZE] __aligned(__alignof(hpf_mspi_xfer_packet_msg_t));
	hpf_mspi_xfer_packet_msg_t *xfer_packet = (hpf_mspi_xfer_packet_msg_t *)buffer;
#endif
	size_t len;

	rc = check_packet_size(packet);
	if (rc < 0) {
		return rc;
	}

	xfer_packet->opcode = opcode;
	xfer_packet->command = packet->cmd;
	xfer_packet->address = packet->address;
	xfer_packet->num_bytes = packet->num_bytes;

#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
	if (bounce) {
		if (packet->dir == MSPI_TX) {
			memcpy(bounce_buf, packet->data_buf, packet->num_bytes);
		}
		xfer_packet->data = bounce_buf;
	} else {
		xfer_packet->data = packet->data_buf;
	}
	len = sizeof(*xfer_packet);
#else
	if (packet->dir == MSPI_TX) {
		memcpy(xfer_packet->data, packet->data_buf, packet->num_bytes);
		len = sizeof(*xfer_packet) + packet->num_bytes;
	} else {
		len = sizeof(*xfer_packet);
		ipc_received = 0;
		rx_dest_len = packet->num_bytes;
		rx_dest = packet->data_buf;
	}
#endif

	rc = send_data(xfer_packet->opcode, xfer_packet, len, timeout);

	if (packet->dir == MSPI_RX) {
#ifdef CONFIG_MSPI_HPF_IPC_NO_COPY
		/* The FLPR wrote the data by reference, into the bounce buffer if it was used. */
		if (bounce) {
			memcpy(packet->data_buf, bounce_buf, packet->num_bytes);
		}
#else
		/* The reply data was copied in ep_recv(). A reply of the wrong length is not
		 * copied, so treat it as an error.
		 */
		rx_dest = NULL;
		if ((rc == 0) && (ipc_received != packet->num_bytes)) {
			rc = -EIO;
		}
#endif
	}

	return rc;
}

/**
 * @brief Send a multi-packet transfer request to the host.
 *
 * This function sends a multi-packet transfer request to the host and waits
 * for the host to complete the transfer. This function does not support
 * asynchronous transfers.
 * The FLPR is stopped in case of timeout, so it no longer accesses
 * the request buffers, and the driver is disabled.
 *
 * @param dev Pointer to the device structure.
 * @param dev_id Pointer to the device identification structure.
 * @param req Pointer to the xfer structure.
 *
 * @retval 0 If successful.
 * @retval -ENOTSUP If the requested transfer configuration is not supported.
 * @retval -EIO General input / output error, failed to send over the bus, or the driver is
 *              disabled after an earlier timeout.
 * @retval -ETIMEDOUT If the FLPR did not complete the transfer in time.
 */
static int api_transceive(const struct device *dev, const struct mspi_dev_id *dev_id,
			  const struct mspi_xfer *req)
{
	struct mspi_hpf_data *drv_data = dev->data;
	uint32_t packets_done = 0;
	int rc;

	/* TODO: add support for asynchronous transfers */
	if ((req->async) || (req->xfer_mode != MSPI_PIO) ||
	    (req->tx_dummy > MAX_MSPI_DUMMY_CLOCKS) || (req->rx_dummy > MAX_MSPI_DUMMY_CLOCKS)) {
		return -ENOTSUP;
	}

	if (req->num_packet == 0 || !req->packets) {
		return -EFAULT;
	}

	drv_data->xfer_config_msg.opcode = HPF_MSPI_CONFIG_XFER;
	drv_data->xfer_config_msg.xfer_config.device_index = dev_id->dev_idx;
	drv_data->xfer_config_msg.xfer_config.command_length = req->cmd_length;
	drv_data->xfer_config_msg.xfer_config.address_length = req->addr_length;
	drv_data->xfer_config_msg.xfer_config.hold_ce = req->hold_ce;
	drv_data->xfer_config_msg.xfer_config.tx_dummy = req->tx_dummy;
	drv_data->xfer_config_msg.xfer_config.rx_dummy = req->rx_dummy;

	rc = pm_device_runtime_get(dev);
	if (rc < 0) {
		return rc;
	}

	rc = send_data(HPF_MSPI_CONFIG_XFER, (void *)&drv_data->xfer_config_msg,
		       sizeof(hpf_mspi_xfer_config_msg_t), req->timeout);

	if (rc < 0) {
		LOG_ERR("Send xfer config error: %d", rc);
		goto release;
	}

	while (packets_done < req->num_packet) {
		rc = send_packet(&req->packets[packets_done], req->timeout);
		if (rc < 0) {
			LOG_ERR("Start next packet error: %d", rc);
			goto release;
		}
		++packets_done;
	}

release:
	(void)pm_device_runtime_put(dev);

	return rc;
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
	const struct mspi_hpf_config *drv_cfg = dev->config;

	switch (action) {
	case PM_DEVICE_ACTION_SUSPEND:
		return pinctrl_apply_state(drv_cfg->pcfg, PINCTRL_STATE_SLEEP);
	case PM_DEVICE_ACTION_RESUME:
		/* Keep the pins away from a stopped FLPR. */
		if (atomic_get(&flpr_halted)) {
			return 0;
		}
		return pinctrl_apply_state(drv_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	default:
		return -ENOTSUP;
	}
}
#endif

#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
static void flpr_fault_handler(const struct device *dev, void *user_data)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(user_data);
	const struct device *const flpr_fault_timer = DEVICE_DT_GET(DT_NODELABEL(fault_timer));

	counter_stop(flpr_fault_timer);

	LOG_ERR("HPF fault detected.");
}
#endif

/**
 * @brief Initialize the MSPI HPF driver.
 *
 * This function initializes the MSPI HPF driver. It is responsible for
 * setting up the hardware and registering the IPC endpoint for the
 * driver.
 *
 * @param dev Pointer to the device structure for the MSPI HPF driver.
 *
 * @retval 0 If successful.
 * @retval -errno If an error occurs.
 */
static int hpf_mspi_init(const struct device *dev)
{
	int ret;
	const struct device *ipc_instance = DEVICE_DT_GET(DT_NODELABEL(ipc0));
	const struct mspi_hpf_config *drv_cfg = dev->config;
	const struct mspi_dt_spec spec = {
		.bus = dev,
		.config = drv_cfg->mspicfg,
	};

#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
	const struct device *const flpr_fault_timer = DEVICE_DT_GET(DT_NODELABEL(fault_timer));
	const struct counter_top_cfg top_cfg = {
		.callback = flpr_fault_handler,
		.user_data = NULL,
		.flags = 0,
		.ticks = counter_us_to_ticks(flpr_fault_timer, CONFIG_MSPI_HPF_FAULT_TIMEOUT)
	};
#endif

	ret = pinctrl_apply_state(drv_cfg->pcfg, PINCTRL_STATE_DEFAULT);
	if (ret) {
		return ret;
	}

	ret = ipc_service_open_instance(ipc_instance);
	if ((ret < 0) && (ret != -EALREADY)) {
		LOG_ERR("ipc_service_open_instance() failure");
		return ret;
	}

	ret = hpf_mspi_register_endpoint_with_retry(ipc_instance);
	if (ret == -ETIMEDOUT) {
		LOG_ERR("Endpoint bind timeout: %d", ret);
		return ret;
	} else if (ret < 0) {
		LOG_ERR("Endpoint registration/deregistration failed: %d", ret);
		return ret;
	}

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

#if defined(CONFIG_MSPI_HPF_FAULT_TIMER)
	/* Configure timer as HPF `watchdog` */
	if (!device_is_ready(flpr_fault_timer)) {
		LOG_ERR("FLPR timer not ready");
		return -1;
	}

	ret = counter_set_top_value(flpr_fault_timer, &top_cfg);
	if (ret < 0) {
		LOG_ERR("counter_set_top_value() failure");
		return ret;
	}

	/* Send timer address to FLPR */
	hpf_mspi_flpr_timer_msg_t timer_data = {
		.opcode = HPF_MSPI_CONFIG_TIMER_PTR,
		.timer_ptr = (NRF_TIMER_Type *)DT_REG_ADDR(DT_NODELABEL(fault_timer)),
	};

	ret = send_data(HPF_MSPI_CONFIG_TIMER_PTR, (const void *)&timer_data.opcode,
			sizeof(hpf_mspi_flpr_timer_msg_t), HPF_MSPI_IPC_CONFIG_TIMEOUT_MS);
	if (ret < 0) {
		LOG_ERR("Send timer configuration failure");
		return ret;
	}

	ret = counter_start(flpr_fault_timer);
	if (ret < 0) {
		LOG_ERR("counter_start() failure");
		return ret;
	}
#endif

	return ret;
}

static DEVICE_API(mspi, drv_api) = {
	.config = api_config,
	.dev_config = api_dev_config,
	.get_channel_status = api_get_channel_status,
	.transceive = api_transceive,
};

PM_DEVICE_DT_INST_DEFINE(0, dev_pm_action_cb);

DEVICE_DT_INST_DEFINE(0, hpf_mspi_init, PM_DEVICE_DT_INST_GET(0), &dev_data, &dev_config,
		      POST_KERNEL, CONFIG_MSPI_HPF_INIT_PRIORITY, &drv_api);
