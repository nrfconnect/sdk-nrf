/*
 * Copyright (c) 2019 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <nrfx_ipc.h>
#include <nrf_modem.h>
#include <nrf_modem_at.h>
#include <zephyr/init.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/toolchain/common.h>
#include <modem/nrf_modem_lib.h>
#include <modem/nrf_modem_lib_trace.h>
#include <pm_config.h>

#ifndef CONFIG_TRUSTED_EXECUTION_NONSECURE
#error  nrf_modem_lib must be run as non-secure firmware.\
	Are you building for the correct board ?
#endif /* CONFIG_TRUSTED_EXECUTION_NONSECURE */

LOG_MODULE_DECLARE(nrf_modem, CONFIG_NRF_MODEM_LIB_LOG_LEVEL);

/* Interrupt used for communication with the network layer. */
#define NRF_MODEM_IPC_IRQ DT_IRQ_BY_IDX(DT_NODELABEL(ipc), 0, irq)
BUILD_ASSERT(IPC_IRQn == NRF_MODEM_IPC_IRQ, "NRF_MODEM_IPC_IRQ mismatch");

/* The heap implementation in `nrf_modem_os.c` require some overhead
 * to allow allocating up to `NRF_MODEM_LIB_SHMEM_TX_SIZE` bytes.
 */
#define NRF_MODEM_LIB_SHMEM_TX_HEAP_OVERHEAD_SIZE 128

struct shutdown_thread {
	sys_snode_t node;
	struct k_sem sem;
};

static sys_slist_t shutdown_threads;
static bool first_time_init;
static struct k_mutex slist_mutex;

static int init_ret;
static enum nrf_modem_mode init_mode;

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
	.fault_handler = nrf_modem_fault_handler
};

static const struct nrf_modem_bootloader_init_params bootloader_init_params = {
	.ipc_irq_prio = CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO,
	.shmem.base = PM_NRF_MODEM_LIB_SRAM_ADDRESS,
	.shmem.size = PM_NRF_MODEM_LIB_SRAM_SIZE,
	.fault_handler = nrf_modem_fault_handler
};

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

static int _nrf_modem_lib_init(const struct device *unused)
{
	int err;
	(void) err;

	if (!first_time_init) {
		sys_slist_init(&shutdown_threads);
		k_mutex_init(&slist_mutex);
		first_time_init = true;
	}

	/* Setup the network IRQ used by the Modem library.
	 * Note: No call to irq_enable() here, that is done through nrf_modem_init().
	 */
	IRQ_CONNECT(NRF_MODEM_IPC_IRQ, CONFIG_NRF_MODEM_LIB_IPC_IRQ_PRIO,
		    nrfx_isr, nrfx_ipc_irq_handler, 0);

	init_ret = nrf_modem_init(&init_params);

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_LOG_FW_VERSION_UUID)) {
		log_fw_version_uuid();
	}

	k_mutex_lock(&slist_mutex, K_FOREVER);
	if (sys_slist_peek_head(&shutdown_threads) != NULL) {
		struct shutdown_thread *thread, *next_thread;

		/* Wake up all sleeping threads. */
		SYS_SLIST_FOR_EACH_CONTAINER_SAFE(&shutdown_threads, thread,
					     next_thread, node) {
			k_sem_give(&thread->sem);
		}
	}
	k_mutex_unlock(&slist_mutex);

	LOG_DBG("Modem library has initialized, ret %d", init_ret);
	STRUCT_SECTION_FOREACH(nrf_modem_lib_init_cb, e) {
		LOG_DBG("Modem init callback: %p", e->callback);
		e->callback(init_ret, e->context);
	}

	if (IS_ENABLED(CONFIG_NRF_MODEM_LIB_SYS_INIT)) {
		/* nrf_modem_init() returns values from a different namespace
		 * than Zephyr's. Make sure to return something in Zephyr's
		 * namespace, in this case 0, when called during SYS_INIT.
		 * Non-zero values in SYS_INIT are currently ignored.
		 */
		return 0;
	}

	return init_ret;
}

void nrf_modem_lib_shutdown_wait(void)
{
	struct shutdown_thread thread;

	k_sem_init(&thread.sem, 0, 1);

	k_mutex_lock(&slist_mutex, K_FOREVER);
	sys_slist_append(&shutdown_threads, &thread.node);
	k_mutex_unlock(&slist_mutex);

	(void)k_sem_take(&thread.sem, K_FOREVER);

	k_mutex_lock(&slist_mutex, K_FOREVER);
	sys_slist_find_and_remove(&shutdown_threads, &thread.node);
	k_mutex_unlock(&slist_mutex);
}

int nrf_modem_lib_init(enum nrf_modem_mode mode)
{
	init_mode = mode;
	if (mode == NORMAL_MODE) {
		return _nrf_modem_lib_init(NULL);
	} else {
		return nrf_modem_bootloader_init(&bootloader_init_params);
	}
}

int nrf_modem_lib_get_init_ret(void)
{
	return init_ret;
}

int nrf_modem_lib_shutdown(void)
{
	LOG_DBG("Shutting down modem library");
	STRUCT_SECTION_FOREACH(nrf_modem_lib_shutdown_cb, e) {
		LOG_DBG("Modem shutdown callback: %p", e->callback);
		e->callback(e->context);
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

#if defined(CONFIG_NRF_MODEM_LIB_SYS_INIT)
/* Initialize during SYS_INIT */
SYS_INIT(_nrf_modem_lib_init, POST_KERNEL, 0);
#endif
