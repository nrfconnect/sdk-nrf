/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Host–RPU transport for the nRF71 Wi-Fi driver.
 *
 * Zephyr ICMsg binding, host TX/RX IPC protocol, and FMAC transport API.
 * Single IPC endpoint (ipc0) for both TX and RX:
 * - Host sends (addr, size, ack_addr) for TX.
 * - RPU writes completion to ack_addr directly, no IPC ACK.
 * - UMAC sends (event addr, size, ring) for RX; host frees via tail write.
 */

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/ipc/ipc_service.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <common/log_cfg.h>
#include <common/mem_mgmt.h>
#include <common/lock_mgmt.h>

#include <common/fmac_structs_common.h>
#include <common/wifi_ipc.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

#define GET_IPC_INSTANCE(dev) (dev)
typedef struct device ipc_device_wrapper_t;

typedef struct nrf_wifi_ipc_ring_info {
	uint32_t tail_addr;
	uint32_t base;
	uint32_t size;
	bool padded;
} nrf_wifi_ipc_ring_info_t;

typedef struct nrf_wifi_ipc_buf_desc {
	uint32_t addr;
	uint32_t size;
	uint32_t *ack_addr;
	nrf_wifi_ipc_ring_info_t ring;
} nrf_wifi_ipc_buf_desc_t;

typedef enum {
	NRF_WIFI_IPC_STATUS_OK = 0,
	NRF_WIFI_IPC_STATUS_INIT_ERR,
	NRF_WIFI_IPC_STATUS_FREEQ_UNINIT_ERR,
	NRF_WIFI_IPC_STATUS_FREEQ_EMPTY,
	NRF_WIFI_IPC_STATUS_FREEQ_INVALID,
	NRF_WIFI_IPC_STATUS_FREEQ_FULL,
	NRF_WIFI_IPC_STATUS_BUSYQ_NOTREADY,
	NRF_WIFI_IPC_STATUS_BUSYQ_FULL,
	NRF_WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR,
} nrf_wifi_ipc_status_t;

typedef void (*nrf_wifi_ipc_rx_cb_t)(const void *data, size_t len, void *priv);

typedef struct {
	const ipc_device_wrapper_t *ipc_inst;
	struct ipc_ept ipc_ep;
	struct ipc_ept_cfg ipc_ep_cfg;
	nrf_wifi_ipc_rx_cb_t recv_cb;
	void *priv;
	volatile bool ipc_ready;
	bool ipc_bound;
} nrf_wifi_ipc_busyq_t;

typedef struct {
	nrf_wifi_ipc_busyq_t busy_q;
	bool send_ack;
} nrf_wifi_ipc_t;

typedef enum {
	IPC_INSTANCE_CMD_CTRL = 0,
	IPC_INSTANCE_CMD_TX,
	IPC_INSTANCE_EVT,
	IPC_INSTANCE_RX
} nrf_wifi_ipc_instances_t;

typedef enum {
	IPC_EPT_UMAC = 0,
	IPC_EPT_LMAC
} nrf_wifi_ipc_epts_t;

typedef struct ipc_ctx {
	nrf_wifi_ipc_instances_t inst;
	nrf_wifi_ipc_epts_t ept;
} ipc_ctx_t;

static void nrf_wifi_ipc_event_free(const nrf_wifi_ipc_buf_desc_t *event_info);

/* ---- Zephyr ICMsg service binding ---- */

static K_SEM_DEFINE(nrf_wifi_ipc_bind_sem, 0, 1);

static void nrf_wifi_ipc_signal_bound(void)
{
	k_sem_give(&nrf_wifi_ipc_bind_sem);
}

static int nrf_wifi_ipc_wait_bound(nrf_wifi_ipc_t *context)
{
	if (context->busy_q.ipc_ready) {
		return 0;
	}

	if (k_sem_take(&nrf_wifi_ipc_bind_sem,
		       K_MSEC(CONFIG_NRF71_IPC_BIND_TIMEOUT_MS)) != 0 &&
	    !context->busy_q.ipc_ready) {
		LOG_ERR("IPC endpoint not bound after %d ms (Wi-Fi core up? "
			"CONFIG_SOC_NRF71_WIFI_BOOT=y for TF-M builds)",
			CONFIG_NRF71_IPC_BIND_TIMEOUT_MS);
		return -ETIMEDOUT;
	}

	return 0;
}

static void nrf_wifi_ipc_ep_bound(void *priv)
{
	nrf_wifi_ipc_t *context = (nrf_wifi_ipc_t *)priv;

	context->busy_q.ipc_ready = true;
	nrf_wifi_ipc_signal_bound();
}

/* For single-endpoint TX+RX bind: set both contexts ready when endpoint is bound */
static nrf_wifi_ipc_t *nrf_wifi_ipc_rx_ctx_shared;

static void nrf_wifi_ipc_ep_bound_tx_rx(void *priv)
{
	nrf_wifi_ipc_t *tx_ctx = (nrf_wifi_ipc_t *)priv;

	tx_ctx->busy_q.ipc_ready = true;
	if (nrf_wifi_ipc_rx_ctx_shared != NULL) {
		nrf_wifi_ipc_rx_ctx_shared->busy_q.ipc_ready = true;
	}
	nrf_wifi_ipc_signal_bound();
}

static void nrf_wifi_ipc_recv_callback(const void *data, size_t len, void *priv)
{
	nrf_wifi_ipc_t *context = (nrf_wifi_ipc_t *)priv;

	if (context->busy_q.recv_cb != NULL) {
		context->busy_q.recv_cb(data, len, context->busy_q.priv);
	}

	if (context->send_ack) {
		while (!ipc_service_send(&context->busy_q.ipc_ep, data, len)) {
			/* Retry until success */
		}
	}
}

static void nrf_wifi_ipc_busyq_init(nrf_wifi_ipc_busyq_t *busyq,
				    const ipc_device_wrapper_t *ipc_inst,
				    nrf_wifi_ipc_rx_cb_t rx_cb,
				    void *priv)
{
	busyq->ipc_inst = ipc_inst;
	busyq->ipc_ep_cfg.cb.bound = nrf_wifi_ipc_ep_bound;
	busyq->recv_cb = rx_cb;
	/* Never clear a bind that already completed: the ICMsg endpoint stays
	 * bound for the lifetime of the Wi-Fi core, and its bound callback only
	 * fires once. Clearing this on a re-bind would latch the endpoint as
	 * not-ready forever and wedge every subsequent send.
	 */
	if (!busyq->ipc_bound) {
		busyq->ipc_ready = false;
	}
	busyq->priv = priv;
	busyq->ipc_ep_cfg.cb.received = nrf_wifi_ipc_recv_callback;
}

static nrf_wifi_ipc_status_t nrf_wifi_ipc_busyq_register(nrf_wifi_ipc_t *context)
{
	int ret;
	const struct device *ipc_instance = GET_IPC_INSTANCE(context->busy_q.ipc_inst);

	if (context->busy_q.ipc_bound) {
		/* Already bound, only the RX consumer was re-armed. This holds while the
		 * Wi-Fi core stays powered; interface down/up never crosses it.
		 *
		 * TODO(WZN-10457): a power cycle reboots the core and resets the peer
		 * ICMsg state, so the bind goes stale. Once power management can power
		 * the core down, clear ipc_bound (and ipc_ready) on that leg so the next
		 * bring-up re-opens the instance and re-runs the handshake here.
		 */
		LOG_DBG("IPC busy queue already registered");
		return NRF_WIFI_IPC_STATUS_OK;
	}

	k_sem_reset(&nrf_wifi_ipc_bind_sem);

	ret = ipc_service_open_instance(ipc_instance);
	if (ret < 0) {
		return NRF_WIFI_IPC_STATUS_INIT_ERR;
	}

	context->busy_q.ipc_ep_cfg.name = "ep";
	context->busy_q.ipc_ep_cfg.priv = context;

	ret = ipc_service_register_endpoint(ipc_instance, &context->busy_q.ipc_ep,
					    &context->busy_q.ipc_ep_cfg);
	if (ret < 0 && ret != -EALREADY) {
		return NRF_WIFI_IPC_STATUS_INIT_ERR;
	}

	if (nrf_wifi_ipc_wait_bound(context) != 0) {
		return NRF_WIFI_IPC_STATUS_INIT_ERR;
	}

	context->busy_q.ipc_bound = true;

	LOG_INF("IPC busy queue registered");

	return NRF_WIFI_IPC_STATUS_OK;
}

nrf_wifi_ipc_status_t nrf_wifi_ipc_bind_ipc_service(nrf_wifi_ipc_t *context,
					    const ipc_device_wrapper_t *ipc_inst,
					    nrf_wifi_ipc_rx_cb_t rx_cb,
					    void *priv)
{
	nrf_wifi_ipc_busyq_init(&context->busy_q, ipc_inst, rx_cb, priv);

	return nrf_wifi_ipc_busyq_register(context);
}

nrf_wifi_ipc_status_t nrf_wifi_ipc_bind_ipc_service_tx_rx(nrf_wifi_ipc_t *tx_context,
						  nrf_wifi_ipc_t *rx_context,
						  const ipc_device_wrapper_t *ipc_inst,
						  nrf_wifi_ipc_rx_cb_t rx_cb,
						  void *priv)
{
	nrf_wifi_ipc_status_t ret;

	nrf_wifi_ipc_busyq_init(&rx_context->busy_q, ipc_inst, NULL, priv);
	nrf_wifi_ipc_rx_ctx_shared = rx_context;
	nrf_wifi_ipc_busyq_init(&tx_context->busy_q, ipc_inst, rx_cb, priv);
	tx_context->busy_q.ipc_ep_cfg.cb.bound = nrf_wifi_ipc_ep_bound_tx_rx;

	ret = nrf_wifi_ipc_busyq_register(tx_context);
	nrf_wifi_ipc_rx_ctx_shared = NULL;

	if (ret == NRF_WIFI_IPC_STATUS_OK) {
		/* Only the TX context owns the endpoint registration, but both
		 * share its bound state.
		 */
		rx_context->busy_q.ipc_bound = true;
	}

	return ret;
}

static nrf_wifi_ipc_status_t nrf_wifi_ipc_busyq_send(nrf_wifi_ipc_t *context,
						    const void *data,
						    size_t len)
{
	nrf_wifi_ipc_busyq_t *busyq = &context->busy_q;

	if (!busyq->ipc_ready) {
		LOG_DBG("IPC service is not ready");
		return NRF_WIFI_IPC_STATUS_BUSYQ_NOTREADY;
	}

	LOG_DBG("IPC send: len %d", len);

	int ret = ipc_service_send(&busyq->ipc_ep, (const void *)data, len);

	if (ret == -ENOMEM) {
		LOG_ERR("IPC send: ENOMEM");
		return NRF_WIFI_IPC_STATUS_BUSYQ_FULL;
	} else if (ret < 0) {
		LOG_ERR("IPC send: Critical IPC failure: %d", ret);
		return NRF_WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR;
	}

	return NRF_WIFI_IPC_STATUS_OK;
}

nrf_wifi_ipc_status_t nrf_wifi_ipc_host_tx_init(nrf_wifi_ipc_t *context, uint32_t addr_freeq)
{
	context->send_ack = false;

	return NRF_WIFI_IPC_STATUS_OK;
}

nrf_wifi_ipc_status_t nrf_wifi_ipc_host_rx_init(nrf_wifi_ipc_t *context, uint32_t addr_freeq)
{
	context->send_ack = false;

	return NRF_WIFI_IPC_STATUS_OK;
}

nrf_wifi_ipc_status_t nrf_wifi_ipc_host_tx_send(nrf_wifi_ipc_t *context,
					const void *data,
					size_t len,
					uint32_t *ack_addr)
{
	nrf_wifi_ipc_buf_desc_t msg_info = { 0 };

	msg_info.addr = (uint32_t)data;
	msg_info.size = len;
	msg_info.ack_addr = ack_addr;

	return nrf_wifi_ipc_busyq_send(context, &msg_info, sizeof(msg_info));
}

/* ---- Host TX/RX IPC protocol ---- */

#define NUM_INSTANCES 1
#define NUM_ENDPOINTS 1

struct device *ipc_instances[NUM_INSTANCES];
struct ipc_ept ept[NUM_ENDPOINTS];
struct ipc_ept_cfg ept_cfg[NUM_ENDPOINTS];

/* Single endpoint: TX (Host->UMAC) and RX (UMAC->Host) on same channel */
static nrf_wifi_ipc_t nrf_wifi_ipc_host_tx;
static nrf_wifi_ipc_t nrf_wifi_ipc_host_rx;

/** Per-IPC-instance (ipc0) state bound to one FMAC device context at a time. */
struct nrf_wifi_ipc_inst {
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx;
	void *cmd_lock;
	void *rx_lock;
	bool rx_active;
};

static struct nrf_wifi_ipc_inst ipc_inst;

static struct nrf_wifi_ipc_inst *ipc_inst_bound(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	if (!fmac_dev_ctx || ipc_inst.fmac_dev_ctx != fmac_dev_ctx) {
		return NULL;
	}

	return &ipc_inst;
}

#define IPC_TX_ACK_SLOTS 64

static K_MUTEX_DEFINE(host_tx_ack_lock);
static uint32_t host_tx_ack_slots[IPC_TX_ACK_SLOTS];
static uintptr_t host_tx_pending_bufs[IPC_TX_ACK_SLOTS];

static void host_tx_reclaim_completed(void)
{
	int i;

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		uint32_t completed_addr;

		completed_addr = host_tx_ack_slots[i];

		if ((completed_addr == 0U) || (host_tx_pending_bufs[i] == 0U)) {
			continue;
		}

		if (completed_addr != (uint32_t)host_tx_pending_bufs[i]) {
			LOG_WRN("Unexpected TX completion addr 0x%x in slot %d",
				completed_addr, i);
			continue;
		}

		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL,
				  (void *)host_tx_pending_bufs[i]);
		host_tx_pending_bufs[i] = 0U;
		host_tx_ack_slots[i] = 0U;
	}

	k_mutex_unlock(&host_tx_ack_lock);
}

static uint32_t *host_tx_ack_slot_alloc(const void *data)
{
	int i;

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		if (host_tx_pending_bufs[i] != 0U) {
			continue;
		}

		host_tx_ack_slots[i] = 0U;
		host_tx_pending_bufs[i] = (uintptr_t)data;
		k_mutex_unlock(&host_tx_ack_lock);

		return &host_tx_ack_slots[i];
	}

	k_mutex_unlock(&host_tx_ack_lock);

	return NULL;
}

static void host_tx_ack_slot_free(uint32_t *ack_addr)
{
	int i;

	if (ack_addr == NULL) {
		return;
	}

	k_mutex_lock(&host_tx_ack_lock, K_FOREVER);

	for (i = 0; i < IPC_TX_ACK_SLOTS; i++) {
		if (&host_tx_ack_slots[i] != ack_addr) {
			continue;
		}

		host_tx_ack_slots[i] = 0U;
		host_tx_pending_bufs[i] = 0U;
		break;
	}

	k_mutex_unlock(&host_tx_ack_lock);
}

/* RX path: receive event (addr, size, ring); after handler, free by writing tail to GDRAM */
static void host_rx_recv(const void *data, size_t len, void *priv)
{
	struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx = priv;
	struct nrf_wifi_fmac_priv *fpriv;
	const nrf_wifi_ipc_buf_desc_t *desc = data;
	nrf_wifi_ipc_buf_desc_t msg_info = *desc;
	void *event_data;
	unsigned int event_len = sizeof(event_data);

	LOG_DBG("Host RX IPC received");

	/* The endpoint stays bound across device teardown, so events can arrive
	 * with no consumer attached. Drop them, but always release the ring slot
	 * or the UMAC event ring fills up and wedges the next bring-up.
	 */
	if (!ipc_inst_bound(fmac_dev_ctx) || !fmac_dev_ctx->fpriv) {
		LOG_DBG("Host RX IPC event dropped, no consumer");
		nrf_wifi_ipc_event_free(&msg_info);
		return;
	}

	fpriv = fmac_dev_ctx->fpriv;
	if (!fpriv->rpu_event_cb) {
		nrf_wifi_ipc_event_free(&msg_info);
		return;
	}

	event_data = (void *)msg_info.addr;

	LOG_DBG("Host RX IPC delivering RPU event");
	(void)fpriv->rpu_event_cb(fmac_dev_ctx, event_data, event_len);
	nrf_wifi_ipc_event_free(&msg_info);
}

static void nrf_wifi_ipc_event_free(const nrf_wifi_ipc_buf_desc_t *event_info)
{
	volatile uint32_t *tail = (volatile uint32_t *)event_info->ring.tail_addr;
	uint32_t new_tail;

	if (event_info->ring.padded) {
		/* Ring was padded; tail advances past the event to ring base. */
		new_tail = event_info->size;
	} else {
		new_tail = (*tail + event_info->size) % event_info->ring.size;
	}
	*tail = new_tail;
}

int ipc_init(void)
{
	static bool slots_initialized;

	nrf_wifi_ipc_host_tx_init(&nrf_wifi_ipc_host_tx, 0);
	nrf_wifi_ipc_host_rx_init(&nrf_wifi_ipc_host_rx, 0);

	/* The ack slots track buffers the UMAC may still be reading. Clearing
	 * them on a re-init would leak every in-flight buffer, so initialize
	 * them only once.
	 */
	if (!slots_initialized) {
		for (int i = 0; i < IPC_TX_ACK_SLOTS; i++) {
			host_tx_ack_slots[i] = 0U;
			host_tx_pending_bufs[i] = 0U;
		}
		slots_initialized = true;
	}

	LOG_DBG("IPC host single endpoint (ipc0) TX+RX initialized");
	return 0;
}

int ipc_deinit(void)
{
	/* Reclaim whatever the UMAC has already acknowledged. Buffers still in
	 * flight stay owned by the ack slots and are reclaimed on a later send.
	 */
	host_tx_reclaim_completed();

	/* TODO(WZN-10457): this runs on interface teardown, where the core stays
	 * powered and the IPC endpoint stays bound. When power management gains a
	 * real core power-down, tear the endpoint down here (clear the bind latch
	 * so the next power-up re-runs the handshake) rather than leaving it armed
	 * against a core that has rebooted.
	 */
	return 0;
}

int ipc_recv(ipc_ctx_t ctx, void *data, int len)
{
	return 0;
}

int ipc_send(ipc_ctx_t ctx, const void *data, int len)
{
	int ret = 0;
	nrf_wifi_ipc_status_t status;

	LOG_DBG("IPC send: inst %d, len %d", ctx.inst, len);
	switch (ctx.inst) {
	case IPC_INSTANCE_CMD_CTRL:
	case IPC_INSTANCE_CMD_TX: {
		uint32_t *ack_addr;

		host_tx_reclaim_completed();
		ack_addr = host_tx_ack_slot_alloc(data);
		if (ack_addr == NULL) {
			LOG_ERR("No free TX ack slots");
			return -1;
		}

		/* TX path: send (addr, size, ack_addr) on IPC0 */
		{
			int64_t deadline = k_uptime_get() + CONFIG_NRF71_IPC_SEND_TIMEOUT_MS;

			do {
				status = nrf_wifi_ipc_host_tx_send(&nrf_wifi_ipc_host_tx,
							       data,
							       (size_t)len,
							       ack_addr);
				if (status != NRF_WIFI_IPC_STATUS_BUSYQ_NOTREADY) {
					break;
				}

				if (k_uptime_get() >= deadline) {
					LOG_ERR("IPC host TX timed out after %d ms",
						CONFIG_NRF71_IPC_SEND_TIMEOUT_MS);
					host_tx_ack_slot_free(ack_addr);
					return -ETIMEDOUT;
				}

				k_usleep(CONFIG_NRF71_IPC_SEND_RETRY_INTERVAL_US);
			} while (true);
		}

		if (status != NRF_WIFI_IPC_STATUS_OK) {
			host_tx_ack_slot_free(ack_addr);

			if (status == NRF_WIFI_IPC_STATUS_BUSYQ_CRITICAL_ERR) {
				LOG_ERR("Critical error during IPC host TX transfer");
				return -1;
			}
		}

		ret = (status == NRF_WIFI_IPC_STATUS_OK) ? 0 : -1;
		break;
	}
	case IPC_INSTANCE_RX:
		break;
	case IPC_INSTANCE_EVT:
		break;
	default:
		break;
	}

	LOG_DBG("IPC send completed: %d", ret);
	return ret;
}

static int host_rpu_ipc_arm_rx(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	int ret;

	ret = nrf_wifi_ipc_bind_ipc_service_tx_rx(&nrf_wifi_ipc_host_tx, &nrf_wifi_ipc_host_rx,
					      DEVICE_DT_GET(DT_NODELABEL(ipc0)),
					      host_rx_recv, fmac_dev_ctx);
	if (ret != NRF_WIFI_IPC_STATUS_OK) {
		LOG_ERR("Failed to bind IPC host TX+RX (ipc0): %d", ret);
		return -1;
	}

	return 0;
}

static void host_rpu_ipc_disarm_rx(void)
{
	/* Detach the consumer only. The endpoint stays bound for the lifetime of
	 * the Wi-Fi core; events arriving from now on are dropped in
	 * host_rx_recv() with their ring slot released.
	 */
	ipc_inst.fmac_dev_ctx = NULL;
}

/* ---- FMAC host–RPU transport ---- */

enum nrf_wifi_status nrf_wifi_ipc_open(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_fmac_priv *fpriv;
	struct nrf_wifi_ipc_inst *inst;
	int ret;

	if (!fmac_dev_ctx || !fmac_dev_ctx->fpriv) {
		return NRF_WIFI_STATUS_FAIL;
	}

	fpriv = fmac_dev_ctx->fpriv;
	if (!fpriv->rpu_event_cb) {
		return NRF_WIFI_STATUS_FAIL;
	}

	if (ipc_inst.fmac_dev_ctx != NULL && ipc_inst.fmac_dev_ctx != fmac_dev_ctx) {
		LOG_ERR("%s: IPC instance already bound to another device", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	inst = ipc_inst_bound(fmac_dev_ctx);
	if (inst && inst->rx_active) {
		return NRF_WIFI_STATUS_SUCCESS;
	}

	ipc_inst.fmac_dev_ctx = fmac_dev_ctx;

	if (!ipc_inst.cmd_lock) {
		ipc_inst.cmd_lock = nrf_wifi_lock_alloc();
		if (!ipc_inst.cmd_lock) {
			LOG_ERR("%s: Unable to allocate command lock", __func__);
			ipc_inst.fmac_dev_ctx = NULL;
			return NRF_WIFI_STATUS_FAIL;
		}
		nrf_wifi_lock_init(ipc_inst.cmd_lock);

		ipc_inst.rx_lock = nrf_wifi_lock_alloc();
		if (!ipc_inst.rx_lock) {
			LOG_ERR("%s: Unable to allocate RX lock", __func__);
			nrf_wifi_lock_free(ipc_inst.cmd_lock);
			ipc_inst.cmd_lock = NULL;
			ipc_inst.fmac_dev_ctx = NULL;
			return NRF_WIFI_STATUS_FAIL;
		}
		nrf_wifi_lock_init(ipc_inst.rx_lock);

		ret = ipc_init();
		if (ret) {
			LOG_ERR("%s: ipc_init failed", __func__);
			nrf_wifi_lock_free(ipc_inst.rx_lock);
			ipc_inst.rx_lock = NULL;
			nrf_wifi_lock_free(ipc_inst.cmd_lock);
			ipc_inst.cmd_lock = NULL;
			ipc_inst.fmac_dev_ctx = NULL;
			return NRF_WIFI_STATUS_FAIL;
		}
	}

	ret = host_rpu_ipc_arm_rx(fmac_dev_ctx);
	if (ret) {
		LOG_ERR("%s: host_rpu_ipc_arm_rx failed", __func__);
		ipc_inst.fmac_dev_ctx = NULL;
		return NRF_WIFI_STATUS_FAIL;
	}

	nrf_wifi_lock_irq_take(ipc_inst.rx_lock, NULL);
	ipc_inst.rx_active = true;
	nrf_wifi_lock_irq_rel(ipc_inst.rx_lock, NULL);

	return NRF_WIFI_STATUS_SUCCESS;
}

bool nrf_wifi_ipc_is_open(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_ipc_inst *inst = ipc_inst_bound(fmac_dev_ctx);

	return inst != NULL && inst->cmd_lock != NULL;
}

void nrf_wifi_ipc_close(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_ipc_inst *inst = ipc_inst_bound(fmac_dev_ctx);

	if (!inst) {
		return;
	}

	if (inst->rx_lock) {
		nrf_wifi_lock_irq_take(inst->rx_lock, NULL);
		inst->rx_active = false;
		nrf_wifi_lock_irq_rel(inst->rx_lock, NULL);
	}

	host_rpu_ipc_disarm_rx();
	ipc_deinit();

	if (inst->rx_lock) {
		nrf_wifi_lock_free(inst->rx_lock);
		inst->rx_lock = NULL;
	}

	if (inst->cmd_lock) {
		nrf_wifi_lock_free(inst->cmd_lock);
		inst->cmd_lock = NULL;
	}
}

enum nrf_wifi_status nrf_wifi_ipc_cmd_send(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx,
					     void *cmd,
					     unsigned int cmd_size)
{
	struct nrf_wifi_ipc_inst *inst = ipc_inst_bound(fmac_dev_ctx);
	ipc_ctx_t ctx = {
		.inst = IPC_INSTANCE_CMD_CTRL,
		.ept = IPC_EPT_UMAC,
	};
	int ret;

	if (!inst || !inst->cmd_lock) {
		return NRF_WIFI_STATUS_FAIL;
	}

#ifdef NRF_WIFI_CMD_EVENT_LOG
	LOG_INF("%s: caller %p", __func__, __builtin_return_address(0));
#else
	LOG_DBG("%s: caller %p", __func__, __builtin_return_address(0));
#endif

	nrf_wifi_lock_take(inst->cmd_lock);
	ret = ipc_send(ctx, cmd, cmd_size);
	nrf_wifi_lock_rel(inst->cmd_lock);

	if (ret < 0) {
		LOG_ERR("%s: Sending command to RPU failed", __func__);
		return NRF_WIFI_STATUS_FAIL;
	}

	return NRF_WIFI_STATUS_SUCCESS;
}

void nrf_wifi_ipc_rx_lock(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_ipc_inst *inst = ipc_inst_bound(fmac_dev_ctx);
	unsigned long flags = 0;

	if (inst && inst->rx_lock) {
		nrf_wifi_lock_irq_take(inst->rx_lock, &flags);
	}
}

void nrf_wifi_ipc_rx_unlock(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_ipc_inst *inst = ipc_inst_bound(fmac_dev_ctx);
	unsigned long flags = 0;

	if (inst && inst->rx_lock) {
		nrf_wifi_lock_irq_rel(inst->rx_lock, &flags);
	}
}

bool nrf_wifi_ipc_rx_enabled(struct nrf_wifi_fmac_dev_ctx *fmac_dev_ctx)
{
	struct nrf_wifi_ipc_inst *inst = ipc_inst_bound(fmac_dev_ctx);

	if (!inst) {
		return false;
	}

	return inst->rx_active;
}
