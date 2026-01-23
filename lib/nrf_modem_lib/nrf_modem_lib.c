/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <modem/nrf_modem_lib.h>
#include <modem/nrf_modem_lib_trace.h>
#include <nrf_modem.h>
#include <nrf_modem_at.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>

#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(nrf_modem, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

#define AT_CFUN_READ "AT+CFUN?"
#define AT_CFUN0_VAL 0
#define AT_CFUN4_VAL 4

static void nrf_modem_lib_dfu_handler(uint32_t dfu_res);

/* The heap implementation in `nrf_modem_os.c` require some overhead
 * so we can actually only allocate a bit less then `CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE`.
 */
#define NRF_MODEM_LIB_SHMEM_TX_HEAP_OVERHEAD_SIZE 128

#ifdef CONFIG_PARTITION_MANAGER_ENABLED
#include <nrfx_ipc.h>
#include <pm_config.h>

#ifndef CONFIG_TRUSTED_EXECUTION_NONSECURE
#error  nrf_modem_lib must be run as non-secure firmware.\
	Are you building for the correct board ?
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

/* Interrupt used for communication with the network layer. */
#define NRF_MODEM_IPC_IRQ DT_IRQ_BY_IDX(DT_NODELABEL(ipc), 0, irq)
BUILD_ASSERT(IPC_IRQn == NRF_MODEM_IPC_IRQ, "NRF_MODEM_IPC_IRQ mismatch");


static const struct nrf_modem_init_params init_params = {
	.ipc_irq_prio = CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO,
	.shmem.ctrl = {
		.base = PM_NRF_MODEM_LIB_CTRL_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_CTRL_SIZE,
	},
	.shmem.tx = {
		.base = PM_NRF_MODEM_LIB_TX_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE -
			NRF_MODEM_LIB_SHMEM_TX_HEAP_OVERHEAD_SIZE,
	},
	.shmem.rx = {
		.base = PM_NRF_MODEM_LIB_RX_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE,
	},
#if CONFIG_NRF_MODEM_LIB_TRACE
	.shmem.trace = {
		.base = PM_NRF_MODEM_LIB_TRACE_ADDRESS,
		.size = CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE,
	},
#endif
	.fault_handler = nrf_modem_fault_handler,
	.dfu_handler = nrf_modem_lib_dfu_handler,
};

static const struct nrf_modem_bootloader_init_params bootloader_init_params = {
	.ipc_irq_prio = CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO,
	.shmem.base = PM_NRF_MODEM_LIB_SRAM_ADDRESS,
	.shmem.size = PM_NRF_MODEM_LIB_SRAM_SIZE,
	.fault_handler = nrf_modem_fault_handler
};
#else /* !CONFIG_PARTITION_MANAGER_ENABLED (use Device Tree) */

struct {
	uint8_t ctrl[CONFIG_NRF_MODEM_LIB_SHMEM_CTRL_SIZE];
	uint8_t tx[CONFIG_NRF_MODEM_LIB_SHMEM_TX_SIZE];
	uint8_t rx[CONFIG_NRF_MODEM_LIB_SHMEM_RX_SIZE];
#if CONFIG_NRF_MODEM_LIB_TRACE
	uint8_t trace[CONFIG_NRF_MODEM_LIB_SHMEM_TRACE_SIZE];
#endif
} __aligned(32) nrf_modem_lib_shmem
#ifdef CONFIG_SOC_SERIES_NRF91
__attribute__((section(".shared_region")))
#endif
;

const uint8_t *nrf_modem_lib_shmem_tx = nrf_modem_lib_shmem.tx;
#ifdef CONFIG_IRONSIDE_SE_CALL
const uint8_t *nrf_modem_lib_shmem_ctrl = nrf_modem_lib_shmem.ctrl;
#endif

#if CONFIG_SOC_SERIES_NRF91
#include <nrfx_ipc.h>

#ifndef CONFIG_TRUSTED_EXECUTION_NONSECURE
#error  nrf_modem_lib must be run as non-secure firmware. Are you building for the correct board ?
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

/* Interrupt used for communication with the network layer. */
#define NRF_MODEM_IPC_IRQ DT_IRQ_BY_IDX(DT_NODELABEL(ipc), 0, irq)
BUILD_ASSERT(IPC_IRQn == NRF_MODEM_IPC_IRQ, "NRF_MODEM_IPC_IRQ mismatch");

static const struct nrf_modem_bootloader_init_params bootloader_init_params = {
	.ipc_irq_prio = CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO,
	.shmem.base = (uint32_t)&nrf_modem_lib_shmem,
	.shmem.size = sizeof(nrf_modem_lib_shmem),
	.fault_handler = nrf_modem_fault_handler
};
#endif

static const struct nrf_modem_init_params init_params = {
	.ipc_irq_prio = CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO,
	.shmem.ctrl = {
		.base = (uint32_t)nrf_modem_lib_shmem.ctrl,
		.size = sizeof(nrf_modem_lib_shmem.ctrl),
	},
	.shmem.tx = {
		.base = (uint32_t)nrf_modem_lib_shmem.tx,
		.size = sizeof(nrf_modem_lib_shmem.tx) -
			NRF_MODEM_LIB_SHMEM_TX_HEAP_OVERHEAD_SIZE,
	},
	.shmem.rx = {
		.base = (uint32_t)nrf_modem_lib_shmem.rx,
		.size = sizeof(nrf_modem_lib_shmem.rx),
	},
#if CONFIG_NRF_MODEM_LIB_TRACE
	.shmem.trace = {
		.base = (uint32_t)nrf_modem_lib_shmem.trace,
		.size = sizeof(nrf_modem_lib_shmem.trace),
	},
#endif
	.fault_handler = nrf_modem_fault_handler,
	.dfu_handler = nrf_modem_lib_dfu_handler,
};

#endif /* CONFIG_PARTITION_MANAGER_ENABLED */

#if CONFIG_NRF_MODEM_LIB_TRACE
extern void nrf_modem_lib_trace_init(void);
#endif /* CONFIG_NRF_MODEM_LIB_TRACE */

static void log_fw_version_uuid(void)
{
	int err;
	size_t off;
	char fw_version_buf[32];
	char *fw_version_end;
	char fw_uuid_buf[64];
	char *fw_uuid;
	char *fw_uuid_end;

	err = nrf_modem_at_cmd(fw_version_buf, sizeof(fw_version_buf), "AT+CGMR");
	if (err == 0) {
		/* Get first string before "\r\n"
		 * which corresponds to FW version string.
		 */
		fw_version_end = strstr(fw_version_buf, "\r\n");
		off = fw_version_end - fw_version_buf - 1;
		fw_version_buf[off + 1] = '\0';
		LOG_INF("Modem FW version: %s", fw_version_buf);
	} else {
		LOG_ERR("Unable to obtain modem FW version (ERR: %d, ERR TYPE: %d)",
			nrf_modem_at_err(err), nrf_modem_at_err_type(err));
	}

	err = nrf_modem_at_cmd(fw_uuid_buf, sizeof(fw_uuid_buf), "AT%%XMODEMUUID");
	if (err == 0) {
		/* Get string that starts with " " after "%XMODEMUUID:",
		 * then move to the next string before "\r\n"
		 * which corresponds to the FW UUID string.
		 */
		fw_uuid = strstr(fw_uuid_buf, " ");
		fw_uuid++;
		fw_uuid_end = strstr(fw_uuid_buf, "\r\n");
		off = fw_uuid_end - fw_uuid_buf - 1;
		fw_uuid_buf[off + 1] = '\0';
		LOG_INF("Modem FW UUID: %s", fw_uuid);
	} else {
		LOG_ERR("Unable to obtain modem FW UUID (ERR: %d, ERR TYPE: %d)\n",
			nrf_modem_at_err(err), nrf_modem_at_err_type(err));
	}
}

static void nrf_modem_lib_dfu_handler(uint32_t dfu_res)
{
	LOG_DBG("Modem library update result %d", dfu_res);
	STRUCT_SECTION_FOREACH(nrf_modem_lib_dfu_cb, e) {
		LOG_DBG("Modem dfu callback: %p", e->callback);
		e->callback(dfu_res, e->context);
	}
}

int nrf_modem_lib_init(void)
{
	int err;

#ifdef CONFIG_SOC_SERIES_NRF91
	/* Setup the network IRQ used by the Modem library.
	 * Note: No call to irq_enable() here, that is done through nrf_modem_init().
	 */
	IRQ_CONNECT(NRF_MODEM_IPC_IRQ, CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO,
		    nrfx_isr, nrfx_ipc_irq_handler, 0);
#endif /* CONFIG_SOC_SERIES_NRF91 */

	err = nrf_modem_init(&init_params);
	if (err) {
		LOG_ERR("Modem library initialization failed, err %d", err);

		/* Jump to the init callbacks and pass them the returned error. */
		goto init_callbacks;
	}

#if CONFIG_NRF_MODEM_LIB_TRACE
	/* We enable tracing as early as possible because the modem can only store a given
	 * amount of traces internally before they are dropped.
	 */
	nrf_modem_lib_trace_init();
#endif /* CONFIG_NRF_MODEM_LIB_TRACE */

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_LOG_FW_VERSION_UUID)) {
		log_fw_version_uuid();
	}

init_callbacks:
	STRUCT_SECTION_FOREACH(nrf_modem_lib_init_cb, e) {
		LOG_DBG("Modem init callback: %p", e->callback);
		e->callback(err, e->context);
	}

	return err;
}

int nrf_modem_lib_bootloader_init(void)
{
#ifdef CONFIG_SOC_SERIES_NRF91
	return nrf_modem_bootloader_init(&bootloader_init_params);
#else
	return -ENOSYS;
#endif
}

int nrf_modem_lib_shutdown(void)
{
	int ret;
	uint16_t mode;

	LOG_DBG("Shutting down modem library");
	STRUCT_SECTION_FOREACH(nrf_modem_lib_shutdown_cb, e) {
		LOG_DBG("Modem shutdown callback: %p", e->callback);
		e->callback(e->context);
	}

	/* The application must disable both transmit and receive RF circuits, and deactivate
	 *  LTE and GNSS services, before calling nrf_modem_shutdown.
	 */
	ret = nrf_modem_at_scanf(AT_CFUN_READ, "+CFUN: %hu", &mode);
	if (ret == 1 && (mode != AT_CFUN0_VAL && mode != AT_CFUN4_VAL)) {
		LOG_WRN("Application should set minimal functional mode (CFUN=0) before "
			"shutting down modem library");
		nrf_modem_at_printf("AT+CFUN=0");
	}

	nrf_modem_shutdown();

#if CONFIG_NRF_MODEM_LIB_TRACE
	/* Before returning, make sure that there isn't any
	 * pending trace data when the library is
	 * re-initialized.
	 */
	nrf_modem_lib_trace_processing_done_wait(K_FOREVER);
#endif

	return 0;
}
