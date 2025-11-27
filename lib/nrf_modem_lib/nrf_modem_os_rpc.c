/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <nrf_errno.h>
#include <nrf_modem_os_rpc.h>

#include <zephyr/cache.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/mbox.h>
#include <zephyr/ipc/icmsg.h>
#include <zephyr/ipc/pbuf.h>
#if defined(CONFIG_IRONSIDE_SE_CALL)
#include <ironside/se/api.h>
#endif

#define DCACHE_LINE_SIZE (CONFIG_DCACHE_LINE_SIZE)
BUILD_ASSERT(DCACHE_LINE_SIZE == 32,
	     "Unexpected data cache line size " STRINGIFY(DCACHE_LINE_SIZE) ", expected 32");

/** IronSide SE boot report local domain context for cellcore. */
struct boot_report_cellcore_ldc {
	uint32_t ipc_buf_addr;
	uint32_t ipc_buf_size;
	uint32_t loader_addr;
	uint32_t rfu;
};

/** Structure to hold pbuf configuration and data. */
struct nrf_modem_pbuf {
	struct pbuf_cfg pb_cfg;
	struct pbuf pb;
};

/**
 * Structure nrf_modem_os_rpc was only declared in nrf_modem_os_rpc.h.
 * Define the members of the struct here. Populating it with the required IPC data.
 */
struct nrf_modem_os_rpc {
	/** ICMsg internal data. */
	struct icmsg_data_t data;
	/** ICMsg configuration. */
	struct icmsg_config_t conf;
	/** ICMsg callbacks to nrf_modem. */
	struct ipc_service_cb cb;
	/** TX pbuf. */
	struct nrf_modem_pbuf tx;
	/** RX pbuf. */
	struct nrf_modem_pbuf rx;
};

/**
 * Structure nrf_modem_os_rpc_signal was only declared in nrf_modem_os_rpc.h.
 * Define the members of the struct here. Populating it with the required mbox data.
 */
struct nrf_modem_os_rpc_signal {
	/** MBOX instance data. */
	struct mbox_dt_spec mbox;
	/** Callback to nrf_modem. */
	nrf_modem_os_rpc_signal_cb_t recv;
	/** Private context data usable by nrf_modem. */
	void *priv;
};

/**
 * Macro to initialize an instance of struct nrf_modem_os_rpc.
 *
 * @param _inst RPC instance to be initialized.
 * @param _dcache_line_size cache line size in bytes.
 *
 * @return Initializer list for initializing the instance.
 */
#define NRF_MODEM_OS_RPC_INIT(_inst, _dcache_line_size)                                            \
	{                                                                                          \
		.data.tx_pb = &(_inst).tx.pb,                                                      \
		.data.rx_pb = &(_inst).rx.pb,                                                      \
		.tx.pb.cfg = &(_inst).tx.pb_cfg,                                                   \
		.rx.pb.cfg = &(_inst).rx.pb_cfg,                                                   \
		.tx.pb_cfg.dcache_alignment = (_dcache_line_size),                                 \
		.rx.pb_cfg.dcache_alignment = (_dcache_line_size),                                 \
	}

/**
 * Define and initialize the RPC instances used by nrf_modem.
 * These are declared extern in nrf_modem_os_rpc.h.
 */
struct nrf_modem_os_rpc inst_ctrl = NRF_MODEM_OS_RPC_INIT(inst_ctrl, DCACHE_LINE_SIZE);
struct nrf_modem_os_rpc inst_data = NRF_MODEM_OS_RPC_INIT(inst_data, DCACHE_LINE_SIZE);

/**
 * Define and initialize the signaling instances used by nrf_modem.
 * These are declared extern in nrf_modem_os_rpc.h.
 */
struct nrf_modem_os_rpc_signal inst_app_fault;
struct nrf_modem_os_rpc_signal inst_modem_fault;
struct nrf_modem_os_rpc_signal inst_modem_trace;
struct nrf_modem_os_rpc_signal inst_modem_sysoff;

uintptr_t nrf_modem_os_rpc_sigdev_app_get(void)
{
	const struct device *app_bellboard = DEVICE_DT_GET(DT_NODELABEL(cpuapp_bellboard));

	return (uintptr_t)app_bellboard;
}

uintptr_t nrf_modem_os_rpc_sigdev_modem_get(void)
{
	const struct device *modem_bellboard = DEVICE_DT_GET(DT_NODELABEL(cpucell_bellboard));

	return (uintptr_t)modem_bellboard;
}

int nrf_modem_os_rpc_cellcore_boot(void)
{
#if defined(CONFIG_IRONSIDE_SE_CALL)
	struct boot_report_cellcore_ldc params;

	params.ipc_buf_addr = DT_REG_ADDR(DT_NODELABEL(cpuapp_cpucell_ipc_shm_ctrl));
	params.ipc_buf_size = CONFIG_NRF_MODEM_LIB_SHMEM_CTRL_SIZE;
	params.loader_addr = 0;
	params.rfu = 0;

	uint8_t *msg = (uint8_t *)&params;
	size_t msg_size = sizeof(params);

	/* Don't wait as this is not yet supported. */
	bool cpu_wait = false;

	return ironside_se_cpuconf(NRF_PROCESSOR_CELLCORE, NULL, cpu_wait, msg, msg_size);
#else
	/* Without IronSide SE, cellcore is booted by the SDFW. */
	return 0;
#endif
}

static inline void pbuf_configure(struct pbuf_cfg *pb_cfg, uintptr_t mem_addr, size_t size)
{
	const uint32_t wr_idx_offset = MAX(pb_cfg->dcache_alignment, _PBUF_IDX_SIZE);

	pb_cfg->rd_idx_loc = (uint32_t *)(mem_addr);
	pb_cfg->wr_idx_loc = (uint32_t *)(mem_addr + wr_idx_offset);
	pb_cfg->len = (uint32_t)((uint32_t)size - wr_idx_offset - _PBUF_IDX_SIZE);
	pb_cfg->data_loc = (uint8_t *)(mem_addr + wr_idx_offset + _PBUF_IDX_SIZE);
}

int nrf_modem_os_rpc_open(struct nrf_modem_os_rpc *instance,
			  const struct nrf_modem_os_rpc_config *config)
{
	if (instance == NULL || config == NULL) {
		return -NRF_EINVAL;
	}

	pbuf_configure(&instance->tx.pb_cfg, config->tx.addr, config->tx.size);
	pbuf_configure(&instance->rx.pb_cfg, config->rx.addr, config->rx.size);

	instance->conf.mbox_tx.dev = (struct device *)config->tx.sigdev;
	instance->conf.mbox_rx.dev = (struct device *)config->rx.sigdev;
	instance->conf.mbox_tx.channel_id = config->tx.ch;
	instance->conf.mbox_rx.channel_id = config->rx.ch;
	instance->conf.unbound_mode = ICMSG_UNBOUND_MODE_DISABLE;

	instance->cb.bound = config->cb.bound;
	instance->cb.received = config->cb.received;

	return icmsg_open(&instance->conf, &instance->data, &instance->cb, config->cb.priv);
}

int nrf_modem_os_rpc_send(struct nrf_modem_os_rpc *instance, const void *msg, size_t len)
{
	int ret;

	ret = icmsg_send(&instance->conf, &instance->data, msg, len);
	if (ret < 0) {
		switch (ret) {
		case -EBUSY:
		case -EINVAL:
			return -NRF_EBUSY;
		case -ENOBUFS:
		case -ENOMEM:
			return -NRF_ENOMEM;
		default:
			return ret;
		}
	}

	return 0;
}

int nrf_modem_os_rpc_close(struct nrf_modem_os_rpc *instance)
{
	return icmsg_close(&instance->conf, &instance->data);
}

int nrf_modem_os_rpc_rx_suspend(struct nrf_modem_os_rpc *instance)
{
	return mbox_set_enabled_dt(&instance->conf.mbox_rx, false);
}

int nrf_modem_os_rpc_rx_resume(struct nrf_modem_os_rpc *instance)
{
	return mbox_set_enabled_dt(&instance->conf.mbox_rx, true);
}

static void mbox_common_callback(const struct device *dev, mbox_channel_id_t ch, void *ctx,
				 struct mbox_msg *data)
{
	struct nrf_modem_os_rpc_signal *inst = (struct nrf_modem_os_rpc_signal *)ctx;

	ARG_UNUSED(dev);
	ARG_UNUSED(data);

	if (inst->recv != NULL) {
		inst->recv(ch, inst->priv);
	}
}

int nrf_modem_os_rpc_signal_init(struct nrf_modem_os_rpc_signal *instance,
				 struct nrf_modem_os_rpc_signal_config *conf)
{
	int err;

	instance->mbox.dev = (struct device *)conf->sigdev;
	instance->mbox.channel_id = (mbox_channel_id_t)conf->ch;
	instance->priv = conf->priv;
	instance->recv = conf->recv;

	if (instance->recv == NULL) {
		return 0;
	}

	err = mbox_register_callback_dt(&instance->mbox, mbox_common_callback, (void *)instance);
	if (err) {
		goto errout;
	}

	err = mbox_set_enabled_dt(&instance->mbox, true);
	if (err) {
		goto errout;
	}

	return 0;

errout:
	instance->recv = NULL;
	return err;
}

int nrf_modem_os_rpc_signal_send(struct nrf_modem_os_rpc_signal *instance)
{
	if (instance->recv != NULL) {
		return -ENOSYS;
	}

	return mbox_send_dt(&instance->mbox, NULL);
}

int nrf_modem_os_rpc_signal_deinit(struct nrf_modem_os_rpc_signal *instance)
{
	if (instance->recv == NULL) {
		return 0;
	}

	return mbox_set_enabled_dt(&instance->mbox, false);
}

int nrf_modem_os_rpc_cache_data_flush(void *addr, size_t size)
{
#if CONFIG_DCACHE
	/* Separate heaps are used for data payloads to and from the modem.
	 * Cache flush is only used on the tx heap. Therefore, cache coherency is
	 * maintained even when start address and size are not aligned with cache lines.
	 */
	const uintptr_t addr_aligned = ROUND_DOWN((uintptr_t)addr, CONFIG_DCACHE_LINE_SIZE);

	size = ROUND_UP((uintptr_t)addr + size, CONFIG_DCACHE_LINE_SIZE) - addr_aligned;

	return sys_cache_data_flush_range((void *)addr_aligned, size);
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	return 0;
#endif
}

int nrf_modem_os_rpc_cache_data_invalidate(void *addr, size_t size)
{
#if CONFIG_DCACHE
	/* Separate heaps are used for data payloads to and from the modem.
	 * Cache invalidation is only used on the rx heap. Therefore, cache coherency is
	 * maintained even when start address and size are not aligned with cache lines.
	 */
	const uintptr_t addr_aligned = ROUND_DOWN((uintptr_t)addr, CONFIG_DCACHE_LINE_SIZE);

	size = ROUND_UP((uintptr_t)addr + size, CONFIG_DCACHE_LINE_SIZE) - addr_aligned;

	return sys_cache_data_invd_range((void *)addr_aligned, size);
#else
	ARG_UNUSED(addr);
	ARG_UNUSED(size);
	return 0;
#endif
}
