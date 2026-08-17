/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing OS specific definitions for the
 * Zephyr OS layer of the Wi-Fi driver.
 */

#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/__assert.h>
#include <zephyr/net/net_core.h>
#include <common/mem_mgmt.h>
#include "ipc_if.h"
#include <zephyr/sys/math_extras.h>

#include "shim.h"
#include "work.h"
#include "osal_ops.h"
#include "common/hal_structs_common.h"

LOG_MODULE_REGISTER(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

/*
 * Tailroom reserved after the payload in every copied TX data buffer.
 *
 * The Wi-Fi crypto hardware on nrf71 does not compute the TKIP Michael
 * MIC, so once a TKIP key is installed UMAC generates the 8-byte MIC in
 * software and writes it right after the payload. Host and firmware share
 * the TX buffer and operate on it independently, so the firmware just
 * needs the room to exist past the payload. CCMP/GCMP MICs are produced
 * by the HW crypto engine and never land in this buffer.
 */
#define NRF71_TX_MIC_TAILROOM 8

struct zep_shim_intr_priv *intr_priv;

static void *zep_shim_spinlock_alloc(void)
{
	struct k_mutex *lock = NULL;

	lock = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*lock));
	if (!lock) {
		LOG_ERR("%s: Unable to allocate memory for spinlock", __func__);
		return NULL;
	}

	return lock;
}

static void zep_shim_spinlock_free(void *lock)
{
	if (lock) {
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, lock);
	}
}

static void zep_shim_spinlock_init(void *lock)
{
	k_mutex_init(lock);
}

static void zep_shim_spinlock_take(void *lock)
{
	k_mutex_lock(lock, K_FOREVER);
}

static void zep_shim_spinlock_rel(void *lock)
{
	k_mutex_unlock(lock);
}

static void zep_shim_spinlock_irq_take(void *lock, unsigned long *flags)
{
	ARG_UNUSED(flags);
	k_mutex_lock(lock, K_FOREVER);
}

static void zep_shim_spinlock_irq_rel(void *lock, unsigned long *flags)
{
	ARG_UNUSED(flags);
	k_mutex_unlock(lock);
}

static int zep_shim_pr_dbg(const char *fmt, va_list args)
{
	static char buf[80];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_DBG("%s", buf);

	return 0;
}

static int zep_shim_pr_info(const char *fmt, va_list args)
{
	static char buf[80];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_INF("%s", buf);

	return 0;
}

static int zep_shim_pr_err(const char *fmt, va_list args)
{
	static char buf[256];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_ERR("%s", buf);

	return 0;
}

struct nwb {
	unsigned char *data;
	unsigned char *tail;
	unsigned char *end;
	int len;
	int headroom;
	void *next;
	void *priv;
	int iftype;
	void *ifaddr;
	void *dev;
	int hostbuffer;
	void *cleanup_ctx;
	void (*cleanup_cb)();
	unsigned char priority;
	bool chksum_done;
#ifdef CONFIG_NRF71_RAW_DATA_TX
	void *raw_tx_hdr;
#endif /* CONFIG_NRF71_RAW_DATA_TX */
#ifdef CONFIG_NRF_WIFI_ZERO_COPY_TX
	struct net_pkt *pkt;
#endif
};

static void *zep_shim_nbuf_alloc(unsigned int size)
{
	struct nwb *nbuff;

	nbuff = (struct nwb *)nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, sizeof(struct nwb));

	if (!nbuff) {
		return NULL;
	}

	nbuff->priv = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, size);

	if (!nbuff->priv) {
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, nbuff);
		return NULL;
	}

	nbuff->data = (unsigned char *)nbuff->priv;
	nbuff->tail = nbuff->data;
	nbuff->end = (unsigned char *)nbuff->priv + size;
	nbuff->len = 0;
	nbuff->headroom = 0;
	nbuff->next = NULL;

	return nbuff;
}

static void zep_shim_nbuf_free(void *nbuf)
{
	if (!nbuf) {
		return;
	}
#ifdef CONFIG_NRF_WIFI_ZERO_COPY_TX
	if (((struct nwb *)nbuf)->pkt) {
		net_pkt_unref(((struct nwb *)nbuf)->pkt);
		((struct nwb *)nbuf)->pkt = NULL;
	}
#endif /* CONFIG_NRF_WIFI_ZERO_COPY_TX */

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, ((struct nwb *)nbuf)->priv);
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, nbuf);
}

static void zep_shim_nbuf_headroom_res(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	nwb->data += size;
	nwb->tail += size;
	nwb->headroom += size;
}

static unsigned int zep_shim_nbuf_headroom_get(void *nbuf)
{
	return ((struct nwb *)nbuf)->headroom;
}

static void zep_shim_nbuf_tailroom_res(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	NET_ASSERT(nwb->end - nwb->tail >= (int)size,
		   "nbuf tailroom reserve (%u) exceeds free space", size);

	nwb->end -= size;
}

static unsigned int zep_shim_nbuf_data_size(void *nbuf)
{
	return ((struct nwb *)nbuf)->len;
}

static void *zep_shim_nbuf_data_get(void *nbuf)
{
	return ((struct nwb *)nbuf)->data;
}

static void *zep_shim_nbuf_data_put(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;
	unsigned char *data = nwb->tail;

	NET_ASSERT(nwb->tail + size <= nwb->end,
		   "nbuf data put (%u) overruns reserved tailroom", size);

	nwb->tail += size;
	nwb->len += size;

	return data;
}

static void *zep_shim_nbuf_data_push(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	nwb->data -= size;
	nwb->headroom -= size;
	nwb->len += size;

	return nwb->data;
}

static void *zep_shim_nbuf_data_pull(void *nbuf, unsigned int size)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	nwb->data += size;
	nwb->headroom += size;
	nwb->len -= size;

	return nwb->data;
}

static unsigned char zep_shim_nbuf_get_priority(void *nbuf)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	return nwb->priority;
}

static unsigned char zep_shim_nbuf_get_chksum_done(void *nbuf)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	return nwb->chksum_done;
}

static void zep_shim_nbuf_set_chksum_done(void *nbuf, unsigned char chksum_done)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	nwb->chksum_done = (bool)chksum_done;
}

#ifdef CONFIG_NRF71_RAW_DATA_TX
static void *zep_shim_nbuf_set_raw_tx_hdr(void *nbuf, unsigned short raw_hdr_len)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return NULL;
	}

	nwb->raw_tx_hdr = zep_shim_nbuf_data_get(nwb);
	if (!nwb->raw_tx_hdr) {
		LOG_ERR("%s: Unable to set raw Tx header in network buffer", __func__);
		return NULL;
	}

	zep_shim_nbuf_data_pull(nwb, raw_hdr_len);

	return nwb->raw_tx_hdr;
}

static void *zep_shim_nbuf_get_raw_tx_hdr(void *nbuf)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return NULL;
	}

	return nwb->raw_tx_hdr;
}

static bool zep_shim_nbuf_is_raw_tx(void *nbuf)
{
	struct nwb *nwb = (struct nwb *)nbuf;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return false;
	}

	return (nwb->raw_tx_hdr != NULL);
}
#endif /* CONFIG_NRF71_RAW_DATA_TX */




#include <zephyr/net/ethernet.h>

#ifdef CONFIG_NRF_WIFI_ZERO_COPY_TX
void *net_pkt_to_nbuf_zc(struct net_pkt *pkt)
{
	struct nwb *nbuff;

	if (!pkt || !pkt->buffer) {
		LOG_DBG("Invalid packet, dropping");
		return NULL;
	}

	/* Check if packet has more than one fragment */
	if (pkt->buffer->frags) {
		LOG_ERR("Zero-copy only supports single buffer packets");
		return NULL;
	}

	nbuff = zep_shim_nbuf_alloc(NRF_WIFI_EXTRA_TX_HEADROOM); /* Just for headers */
	if (!nbuff) {
		return NULL;
	}

	zep_shim_nbuf_headroom_res(nbuff, NRF_WIFI_EXTRA_TX_HEADROOM);

	/* Zero-copy: point to the single data buffer */
	/* TODO: Use API for proper cursor access? */
	nbuff->data = pkt->buffer->data;
	nbuff->len = pkt->buffer->len;

	nbuff->priority = net_pkt_priority(pkt);
	nbuff->chksum_done = (bool)net_pkt_is_chksum_done(pkt);

	nbuff->pkt = pkt;
	/* Ref the packet so that it is not freed */
	net_pkt_ref(pkt);

	return nbuff;
}
#endif /* CONFIG_NRF_WIFI_ZERO_COPY_TX */

void *net_pkt_to_nbuf(struct net_pkt *pkt)
{
	struct nwb *nbuff;
	unsigned char *data;
	unsigned int len;

	if (!pkt) {
		return NULL;
	}

#ifdef CONFIG_NRF_WIFI_ZERO_COPY_TX
	/* For zero-copy, check if packet has single buffer */
	if (pkt->buffer && !pkt->buffer->frags) {
		return net_pkt_to_nbuf_zc(pkt);
	}
#endif /* CONFIG_NRF_WIFI_ZERO_COPY_TX */

	len = net_pkt_get_len(pkt);

	nbuff = zep_shim_nbuf_alloc(len + 100 + NRF71_TX_MIC_TAILROOM);

	if (!nbuff) {
		return NULL;
	}

	zep_shim_nbuf_headroom_res(nbuff, 100);
	zep_shim_nbuf_tailroom_res(nbuff, NRF71_TX_MIC_TAILROOM);

	data = zep_shim_nbuf_data_put(nbuff, len);

	net_pkt_read(pkt, data, len);

	nbuff->priority = net_pkt_priority(pkt);
	nbuff->chksum_done = (bool)net_pkt_is_chksum_done(pkt);

	return nbuff;
}

void *net_pkt_from_nbuf(void *iface, void *frm)
{
	struct net_pkt *pkt = NULL;
	unsigned char *data;
	unsigned int len;
	struct nwb *nwb = frm;

	if (!nwb) {
		return NULL;
	}

	len = zep_shim_nbuf_data_size(nwb);

	data = zep_shim_nbuf_data_get(nwb);

	pkt = net_pkt_rx_alloc_with_buffer(iface, len, NET_AF_UNSPEC, 0, K_MSEC(100));

	if (!pkt) {
		goto out;
	}

	if (net_pkt_write(pkt, data, len)) {
		net_pkt_unref(pkt);
		pkt = NULL;
		goto out;
	}

out:
	zep_shim_nbuf_free(nwb);
	return pkt;
}

#if defined(CONFIG_NRF71_RAW_DATA_RX) || defined(CONFIG_NRF71_PROMISC_DATA_RX)
void *net_raw_pkt_from_nbuf(void *iface, void *frm,
			    unsigned short raw_hdr_len,
			    void *raw_rx_hdr,
			    bool pkt_free)
{
	struct net_pkt *pkt = NULL;
	unsigned char *nwb_data;
	unsigned char *data =  NULL;
	unsigned int nwb_len;
	unsigned int total_len;
	struct nwb *nwb = frm;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return NULL;
	}

	nwb_len = zep_shim_nbuf_data_size(nwb);
	nwb_data = zep_shim_nbuf_data_get(nwb);
	total_len = raw_hdr_len + nwb_len;

	data = (unsigned char *)nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, total_len);
	if (!data) {
		LOG_ERR("%s: Unable to allocate memory for sniffer data packet", __func__);
		goto out;
	}

	pkt = net_pkt_rx_alloc_with_buffer(iface, total_len, NET_AF_PACKET, ETH_P_ALL, K_MSEC(100));
	if (!pkt) {
		LOG_ERR("%s: Unable to allocate net packet buffer", __func__);
		goto out;
	}

	memcpy(data, raw_rx_hdr, raw_hdr_len);
	memcpy((data+raw_hdr_len), nwb_data, nwb_len);

	if (net_pkt_write(pkt, data, total_len)) {
		net_pkt_unref(pkt);
		pkt = NULL;
		goto out;
	}
out:
	if (data != NULL) {
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, data);
	}

	if (pkt_free) {
		zep_shim_nbuf_free(nwb);
	}

	return pkt;
}
#endif /* CONFIG_NRF71_RAW_DATA_RX || CONFIG_NRF71_PROMISC_DATA_RX */

static void *zep_shim_llist_node_alloc(void)
{
	struct zep_shim_llist_node *llist_node = NULL;

	llist_node = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, sizeof(*llist_node));

	if (!llist_node) {
		LOG_ERR("%s: Unable to allocate memory for linked list node", __func__);
		return NULL;
	}

	sys_dnode_init(&llist_node->head);

	return llist_node;
}

static void *zep_shim_ctrl_llist_node_alloc(void)
{
	struct zep_shim_llist_node *llist_node = NULL;

	llist_node = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*llist_node));

	if (!llist_node) {
		LOG_ERR("%s: Unable to allocate memory for linked list node", __func__);
		return NULL;
	}

	sys_dnode_init(&llist_node->head);

	return llist_node;
}

static void zep_shim_llist_node_free(void *llist_node)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, llist_node);
}

static void zep_shim_ctrl_llist_node_free(void *llist_node)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, llist_node);
}

static void *zep_shim_llist_node_data_get(void *llist_node)
{
	struct zep_shim_llist_node *zep_llist_node = NULL;

	zep_llist_node = (struct zep_shim_llist_node *)llist_node;

	return zep_llist_node->data;
}

static void zep_shim_llist_node_data_set(void *llist_node, void *data)
{
	struct zep_shim_llist_node *zep_llist_node = NULL;

	zep_llist_node = (struct zep_shim_llist_node *)llist_node;

	zep_llist_node->data = data;
}

static void *zep_shim_llist_alloc(void)
{
	struct zep_shim_llist *llist = NULL;

	llist = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, sizeof(*llist));

	if (!llist) {
		LOG_ERR("%s: Unable to allocate memory for linked list", __func__);
	}

	return llist;
}

static void *zep_shim_ctrl_llist_alloc(void)
{
	struct zep_shim_llist *llist = NULL;

	llist = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*llist));

	if (!llist) {
		LOG_ERR("%s: Unable to allocate memory for linked list", __func__);
	}

	return llist;
}

static void zep_shim_llist_free(void *llist)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, llist);
}

static void zep_shim_ctrl_llist_free(void *llist)
{
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, llist);
}

static void zep_shim_llist_init(void *llist)
{
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;

	sys_dlist_init(&zep_llist->head);
}

static void zep_shim_llist_add_node_tail(void *llist, void *llist_node)
{
	struct zep_shim_llist *zep_llist = NULL;
	struct zep_shim_llist_node *zep_node = NULL;

	zep_llist = (struct zep_shim_llist *)llist;
	zep_node = (struct zep_shim_llist_node *)llist_node;

	sys_dlist_append(&zep_llist->head, &zep_node->head);

	zep_llist->len += 1;
}

static void zep_shim_llist_add_node_head(void *llist, void *llist_node)
{
	struct zep_shim_llist *zep_llist = NULL;
	struct zep_shim_llist_node *zep_node = NULL;

	zep_llist = (struct zep_shim_llist *)llist;
	zep_node = (struct zep_shim_llist_node *)llist_node;

	sys_dlist_prepend(&zep_llist->head, &zep_node->head);

	zep_llist->len += 1;
}

static void *zep_shim_llist_get_node_head(void *llist)
{
	struct zep_shim_llist_node *zep_head_node = NULL;
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;

	if (!zep_llist->len) {
		return NULL;
	}

	zep_head_node = (struct zep_shim_llist_node *)sys_dlist_peek_head(&zep_llist->head);

	return zep_head_node;
}

static void *zep_shim_llist_get_node_nxt(void *llist, void *llist_node)
{
	struct zep_shim_llist_node *zep_node = NULL;
	struct zep_shim_llist_node *zep_nxt_node = NULL;
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;
	zep_node = (struct zep_shim_llist_node *)llist_node;

	zep_nxt_node = (struct zep_shim_llist_node *)sys_dlist_peek_next(&zep_llist->head,
									 &zep_node->head);

	return zep_nxt_node;
}

static void zep_shim_llist_del_node(void *llist, void *llist_node)
{
	struct zep_shim_llist_node *zep_node = NULL;
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;
	zep_node = (struct zep_shim_llist_node *)llist_node;

	sys_dlist_remove(&zep_node->head);

	zep_llist->len -= 1;
}

static unsigned int zep_shim_llist_len(void *llist)
{
	struct zep_shim_llist *zep_llist = NULL;

	zep_llist = (struct zep_shim_llist *)llist;

	return zep_llist->len;
}

static void *zep_shim_work_alloc(int type)
{
	return work_alloc(type);
}

static void zep_shim_work_free(void *item)
{
	work_free(item);
}

static void zep_shim_work_init(void *item, void (*callback)(unsigned long data),
				  unsigned long data)
{
	work_init(item, callback, data);
}

static void zep_shim_work_schedule(void *item)
{
	work_schedule(item);
}

static void zep_shim_work_kill(void *item)
{
	work_kill(item);
}

static unsigned long zep_shim_time_get_curr_us(void)
{
	return k_ticks_to_us_floor64(k_uptime_ticks());
}

static unsigned int zep_shim_time_elapsed_us(unsigned long start_time_us)
{
	unsigned long curr_time_us = 0;

	curr_time_us = zep_shim_time_get_curr_us();

	return curr_time_us - start_time_us;
}

static unsigned long zep_shim_time_get_curr_ms(void)
{
	return k_uptime_get();
}

static unsigned int zep_shim_time_elapsed_ms(unsigned long start_time_ms)
{
	unsigned long curr_time_ms = 0;

	curr_time_ms = zep_shim_time_get_curr_ms();

	return curr_time_ms - start_time_ms;
}

static enum nrf_wifi_status zep_shim_bus_qspi_dev_init(void *os_qspi_dev_ctx)
{
	ARG_UNUSED(os_qspi_dev_ctx);

	return NRF_WIFI_STATUS_SUCCESS;
}

static void zep_shim_bus_qspi_dev_deinit(void *priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	volatile struct rpu_dev *dev = qspi_priv->qspi_dev;

	dev->deinit();
}

static int ipc_send_msg(unsigned int msg_type, void *msg, unsigned int len)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct rpu_dev *dev = rpu_dev();
	int ret;
	ipc_ctx_t ctx;

	switch (msg_type) {
	case NRF_WIFI_HAL_MSG_TYPE_CMD_CTRL:
		ctx.inst = IPC_INSTANCE_CMD_CTRL;
		ctx.ept = IPC_EPT_UMAC;
		break;
	case NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_TX:
		ctx.inst = IPC_INSTANCE_CMD_TX;
		ctx.ept = IPC_EPT_UMAC;
		break;
	case NRF_WIFI_HAL_MSG_TYPE_CMD_DATA_RX:
		ctx.inst = IPC_INSTANCE_RX;
		ctx.ept = IPC_EPT_LMAC;
		break;
	default:
		nrf_wifi_osal_log_err("%s: Invalid msg_type (%d)", __func__, msg_type);
		goto out;
	};

	ret = dev->send(ctx, msg, len);
	if (ret < 0) {
		nrf_wifi_osal_log_err("%s: Sending message to RPU failed\n", __func__);
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

static void *zep_shim_bus_qspi_dev_add(void *os_qspi_priv, void *osal_qspi_dev_ctx)
{
	struct zep_shim_bus_qspi_priv *zep_qspi_priv = os_qspi_priv;
	struct rpu_dev *dev = rpu_dev();

	dev->init();
	zep_qspi_priv->qspi_dev = dev;
	zep_qspi_priv->dev_added = true;

	return zep_qspi_priv;
}

static void zep_shim_bus_qspi_dev_rem(void *priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev = qspi_priv->qspi_dev;

	ARG_UNUSED(dev);

}

static void *zep_shim_bus_qspi_init(void)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_CTRL, sizeof(*qspi_priv));

	if (!qspi_priv) {
		LOG_ERR("%s: Unable to allocate memory for qspi_priv", __func__);
		goto out;
	}
out:
	return qspi_priv;
}

static void zep_shim_bus_qspi_deinit(void *os_qspi_priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = os_qspi_priv;

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_CTRL, qspi_priv);
}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
/* nRF71 flat driver: QSPI RPU power save not supported; no-ops. */
static int zep_shim_bus_qspi_ps_sleep(void *os_qspi_priv)
{
	ARG_UNUSED(os_qspi_priv);
	return 0;
}

static int zep_shim_bus_qspi_ps_wake(void *os_qspi_priv)
{
	ARG_UNUSED(os_qspi_priv);
	return 0;
}

static int zep_shim_bus_qspi_ps_status(void *os_qspi_priv)
{
	ARG_UNUSED(os_qspi_priv);
	return 0;
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

static void zep_shim_bus_qspi_dev_host_map_get(void *os_qspi_dev_ctx,
					       struct nrf_wifi_osal_host_map *host_map)
{
	if (!os_qspi_dev_ctx || !host_map) {
		LOG_ERR("%s: Invalid parameters", __func__);
		return;
	}

	host_map->addr = 0;
}


static enum nrf_wifi_status zep_shim_bus_qspi_intr_reg(void *os_dev_ctx, void *callbk_data,
						       int (*callbk_fn)(void *callbk_data))
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int ret = -1;

	ARG_UNUSED(os_dev_ctx);

	ret = ipc_register_rx_cb(callbk_fn, callbk_data);
	if (ret) {
		LOG_ERR("%s: ipc_register_rx_cb failed\n", __func__);
		goto out;
	}
	status = NRF_WIFI_STATUS_SUCCESS;
out:
	return status;
}

static void zep_shim_bus_qspi_intr_unreg(void *os_qspi_dev_ctx)
{
	ARG_UNUSED(os_qspi_dev_ctx);

	/* Detach the event consumer before the HAL context is torn down. The IPC
	 * endpoint itself stays bound: it is the control plane and its lifetime
	 * follows the Wi-Fi core, not the interface.
	 */
	ipc_unregister_rx_cb();
}

static void zep_shim_assert(int test_val, int val, enum nrf_wifi_assert_op_type op, char *msg)
{
	switch (op) {
	case NRF_WIFI_ASSERT_EQUAL_TO:
		NET_ASSERT(test_val == val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_NOT_EQUAL_TO:
		NET_ASSERT(test_val != val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_LESS_THAN:
		NET_ASSERT(test_val < val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_LESS_THAN_EQUAL_TO:
		NET_ASSERT(test_val <= val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_GREATER_THAN:
		NET_ASSERT(test_val > val, "%s", msg);
	break;
	case NRF_WIFI_ASSERT_GREATER_THAN_EQUAL_TO:
		NET_ASSERT(test_val >= val, "%s", msg);
	break;
	default:
		LOG_ERR("%s: Invalid assertion operation", __func__);
	}
}

static unsigned int zep_shim_strlen(const void *str)
{
	return strlen(str);
}

const struct nrf_wifi_osal_ops nrf_wifi_os_zep_ops = {
	.spinlock_alloc = zep_shim_spinlock_alloc,
	.spinlock_free = zep_shim_spinlock_free,
	.spinlock_init = zep_shim_spinlock_init,
	.spinlock_take = zep_shim_spinlock_take,
	.spinlock_rel = zep_shim_spinlock_rel,

	.spinlock_irq_take = zep_shim_spinlock_irq_take,
	.spinlock_irq_rel = zep_shim_spinlock_irq_rel,

	.log_dbg = zep_shim_pr_dbg,
	.log_info = zep_shim_pr_info,
	.log_err = zep_shim_pr_err,

	.llist_node_alloc = zep_shim_llist_node_alloc,
	.ctrl_llist_node_alloc = zep_shim_ctrl_llist_node_alloc,
	.llist_node_free = zep_shim_llist_node_free,
	.ctrl_llist_node_free = zep_shim_ctrl_llist_node_free,
	.llist_node_data_get = zep_shim_llist_node_data_get,
	.llist_node_data_set = zep_shim_llist_node_data_set,

	.llist_alloc = zep_shim_llist_alloc,
	.ctrl_llist_alloc = zep_shim_ctrl_llist_alloc,
	.llist_free = zep_shim_llist_free,
	.ctrl_llist_free = zep_shim_ctrl_llist_free,
	.llist_init = zep_shim_llist_init,
	.llist_add_node_tail = zep_shim_llist_add_node_tail,
	.llist_add_node_head = zep_shim_llist_add_node_head,
	.llist_get_node_head = zep_shim_llist_get_node_head,
	.llist_get_node_nxt = zep_shim_llist_get_node_nxt,
	.llist_del_node = zep_shim_llist_del_node,
	.llist_len = zep_shim_llist_len,

	.nbuf_alloc = zep_shim_nbuf_alloc,
	.nbuf_free = zep_shim_nbuf_free,
	.nbuf_headroom_res = zep_shim_nbuf_headroom_res,
	.nbuf_headroom_get = zep_shim_nbuf_headroom_get,
	.nbuf_data_size = zep_shim_nbuf_data_size,
	.nbuf_data_get = zep_shim_nbuf_data_get,
	.nbuf_data_put = zep_shim_nbuf_data_put,
	.nbuf_data_push = zep_shim_nbuf_data_push,
	.nbuf_data_pull = zep_shim_nbuf_data_pull,
	.nbuf_get_priority = zep_shim_nbuf_get_priority,
	.nbuf_get_chksum_done = zep_shim_nbuf_get_chksum_done,
	.nbuf_set_chksum_done = zep_shim_nbuf_set_chksum_done,
#ifdef CONFIG_NRF71_RAW_DATA_TX
	.nbuf_set_raw_tx_hdr = zep_shim_nbuf_set_raw_tx_hdr,
	.nbuf_get_raw_tx_hdr = zep_shim_nbuf_get_raw_tx_hdr,
	.nbuf_is_raw_tx = zep_shim_nbuf_is_raw_tx,
#endif /* CONFIG_NRF71_RAW_DATA_TX */

	.tasklet_alloc = zep_shim_work_alloc,
	.tasklet_free = zep_shim_work_free,
	.tasklet_init = zep_shim_work_init,
	.tasklet_schedule = zep_shim_work_schedule,
	.tasklet_kill = zep_shim_work_kill,

	.sleep_ms = k_msleep,
	.delay_us = k_usleep,
	.time_get_curr_us = zep_shim_time_get_curr_us,
	.time_elapsed_us = zep_shim_time_elapsed_us,
	.time_get_curr_ms = zep_shim_time_get_curr_ms,
	.time_elapsed_ms = zep_shim_time_elapsed_ms,

	.bus_qspi_init = zep_shim_bus_qspi_init,
	.bus_qspi_deinit = zep_shim_bus_qspi_deinit,
	.bus_qspi_dev_add = zep_shim_bus_qspi_dev_add,
	.bus_qspi_dev_rem = zep_shim_bus_qspi_dev_rem,
	.bus_qspi_dev_init = zep_shim_bus_qspi_dev_init,
	.bus_qspi_dev_deinit = zep_shim_bus_qspi_dev_deinit,
	.bus_qspi_dev_intr_reg = zep_shim_bus_qspi_intr_reg,
	.bus_qspi_dev_intr_unreg = zep_shim_bus_qspi_intr_unreg,
	.bus_qspi_dev_host_map_get = zep_shim_bus_qspi_dev_host_map_get,

#ifdef CONFIG_NRF_WIFI_LOW_POWER
	.bus_qspi_ps_sleep = zep_shim_bus_qspi_ps_sleep,
	.bus_qspi_ps_wake = zep_shim_bus_qspi_ps_wake,
	.bus_qspi_ps_status = zep_shim_bus_qspi_ps_status,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */
	.assert = zep_shim_assert,
	.strlen = zep_shim_strlen,
	.ipc_send_msg = ipc_send_msg,
};
