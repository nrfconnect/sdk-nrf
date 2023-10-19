/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
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

#include "rpu_hw_if.h"
#include "shim.h"
#include "zephyr_work.h"
#include "timer.h"
#include "osal_ops.h"
#include "qspi_if.h"

LOG_MODULE_REGISTER(wifi_nrf, CONFIG_WIFI_NRF700X_LOG_LEVEL);

struct zep_shim_intr_priv *intr_priv;

static void *zep_shim_mem_alloc(size_t size)
{
	size = (size + 4) & 0xfffffffc;
	return k_malloc(size);
}

static void *zep_shim_mem_zalloc(size_t size)
{
	size = (size + 4) & 0xfffffffc;
	return k_calloc(size, sizeof(char));
}

static void *zep_shim_mem_cpy(void *dest, const void *src, size_t count)
{
	return memcpy(dest, src, count);
}

static void *zep_shim_mem_set(void *start, int val, size_t size)
{
	return memset(start, val, size);
}

static int zep_shim_mem_cmp(const void *addr1,
			    const void *addr2,
			    size_t size)
{
	return memcmp(addr1, addr2, size);
}

static unsigned int zep_shim_qspi_read_reg32(void *priv, unsigned long addr)
{
	unsigned int val;
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev;

	dev = qspi_priv->qspi_dev;

	if (addr < 0x0C0000) {
		dev->hl_read(addr, &val, 4);
	} else {
		dev->read(addr, &val, 4);
	}

	return val;
}

static void zep_shim_qspi_write_reg32(void *priv, unsigned long addr, unsigned int val)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev;

	dev = qspi_priv->qspi_dev;

	dev->write(addr, &val, 4);
}

static void zep_shim_qspi_cpy_from(void *priv, void *dest, unsigned long addr, size_t count)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev;

	dev = qspi_priv->qspi_dev;

	if (count % 4 != 0) {
		count = (count + 4) & 0xfffffffc;
	}

	if (addr < 0x0C0000) {
		dev->hl_read(addr, dest, count);
	} else {
		dev->read(addr, dest, count);
	}
}

static void zep_shim_qspi_cpy_to(void *priv, unsigned long addr, const void *src, size_t count)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = priv;
	struct qspi_dev *dev;

	dev = qspi_priv->qspi_dev;

	if (count % 4 != 0) {
		count = (count + 4) & 0xfffffffc;
	}

	dev->write(addr, src, count);
}

static void *zep_shim_spinlock_alloc(void)
{
	struct k_sem *lock = NULL;

	lock = k_malloc(sizeof(*lock));

	if (!lock) {
		LOG_ERR("%s: Unable to allocate memory for spinlock\n", __func__);
	}

	return lock;
}

static void zep_shim_spinlock_free(void *lock)
{
	k_free(lock);
}

static void zep_shim_spinlock_init(void *lock)
{
	k_sem_init(lock, 1, 1);
}

static void zep_shim_spinlock_take(void *lock)
{
	k_sem_take(lock, K_FOREVER);
}

static void zep_shim_spinlock_rel(void *lock)
{
	k_sem_give(lock);
}

static void zep_shim_spinlock_irq_take(void *lock, unsigned long *flags)
{
	k_sem_take(lock, K_FOREVER);
}

static void zep_shim_spinlock_irq_rel(void *lock, unsigned long *flags)
{
	k_sem_give(lock);
}

static int zep_shim_pr_dbg(const char *fmt, va_list args)
{
	char buf[80];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_DBG("%s", buf);

	return 0;
}

static int zep_shim_pr_info(const char *fmt, va_list args)
{
	char buf[80];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_INF("%s", buf);

	return 0;
}

static int zep_shim_pr_err(const char *fmt, va_list args)
{
	char buf[256];

	vsnprintf(buf, sizeof(buf), fmt, args);

	LOG_ERR("%s", buf);

	return 0;
}

struct nwb {
	unsigned char *data;
	unsigned char *tail;
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
};

static void *zep_shim_nbuf_alloc(unsigned int size)
{
	struct nwb *nwb;

	nwb = (struct nwb *)k_calloc(sizeof(struct nwb), sizeof(char));

	if (!nwb)
		return NULL;

	nwb->priv = k_calloc(size, sizeof(char));

	if (!nwb->priv) {
		k_free(nwb);
		return NULL;
	}

	nwb->data = (unsigned char *)nwb->priv;
	nwb->tail = nwb->data;
	nwb->len = 0;
	nwb->headroom = 0;
	nwb->next = NULL;

	return nwb;
}

static void zep_shim_nbuf_free(void *nbuf)
{
	struct nwb *nwb;

	nwb = nbuf;

	k_free(((struct nwb *)nbuf)->priv);

	k_free(nbuf);
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

#include <zephyr/net/ethernet.h>
#include <zephyr/net/net_core.h>

void *net_pkt_to_nbuf(struct net_pkt *pkt)
{
	struct nwb *nwb;
	unsigned char *data;
	unsigned int len;

	len = net_pkt_get_len(pkt);

	nwb = zep_shim_nbuf_alloc(len + 100);

	if (!nwb) {
		return NULL;
	}

	zep_shim_nbuf_headroom_res(nwb, 100);

	data = zep_shim_nbuf_data_put(nwb, len);

	net_pkt_read(pkt, data, len);

	nwb->priority = net_pkt_priority(pkt);

	return nwb;
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

	pkt = net_pkt_rx_alloc_with_buffer(iface, len, AF_UNSPEC, 0, K_MSEC(100));

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

static void *zep_shim_llist_node_alloc(void)
{
	struct zep_shim_llist_node *llist_node = NULL;

	llist_node = k_calloc(sizeof(*llist_node), sizeof(char));

	if (!llist_node) {
		LOG_ERR("%s: Unable to allocate memory for linked list node\n", __func__);
		return NULL;
	}

	sys_dnode_init(&llist_node->head);

	return llist_node;
}

static void zep_shim_llist_node_free(void *llist_node)
{
	k_free(llist_node);
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

	llist = k_calloc(sizeof(*llist), sizeof(char));

	if (!llist) {
		LOG_ERR("%s: Unable to allocate memory for linked list\n", __func__);
	}

	return llist;
}

static void zep_shim_llist_free(void *llist)
{
	k_free(llist);
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
	return work_free(item);
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
	return k_uptime_get() * USEC_PER_MSEC;
}

static unsigned int zep_shim_time_elapsed_us(unsigned long start_time_us)
{
	unsigned long curr_time_us = 0;

	curr_time_us = zep_shim_time_get_curr_us();

	return curr_time_us - start_time_us;
}

static enum nrf_wifi_status zep_shim_bus_qspi_dev_init(void *os_qspi_dev_ctx)
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	struct qspi_dev *dev = NULL;

	dev = os_qspi_dev_ctx;

	status = NRF_WIFI_STATUS_SUCCESS;

	return status;
}

static void zep_shim_bus_qspi_dev_deinit(void *os_qspi_dev_ctx)
{
	struct qspi_dev *dev = NULL;

	dev = os_qspi_dev_ctx;

	dev->deinit();
}

static void *zep_shim_bus_qspi_dev_add(void *os_qspi_priv, void *osal_qspi_dev_ctx)
{
	struct zep_shim_bus_qspi_priv *zep_qspi_priv = NULL;
	struct qspi_dev *qdev = NULL;

	zep_qspi_priv = os_qspi_priv;

	rpu_enable();

	qdev = qspi_dev();

	zep_qspi_priv->qspi_dev = qdev;
	zep_qspi_priv->dev_added = true;

	return zep_qspi_priv;
}

static void zep_shim_bus_qspi_dev_rem(void *os_qspi_dev_ctx)
{
	struct qspi_dev *dev = NULL;

	dev = os_qspi_dev_ctx;

	/* TODO: Make qspi_dev a dynamic instance and remove it here */
	rpu_disable();
}

static void *zep_shim_bus_qspi_init(void)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = k_calloc(sizeof(*qspi_priv), sizeof(char));

	if (!qspi_priv) {
		LOG_ERR("%s: Unable to allocate memory for qspi_priv\n", __func__);
		goto out;
	}
out:
	return qspi_priv;
}

static void zep_shim_bus_qspi_deinit(void *os_qspi_priv)
{
	struct zep_shim_bus_qspi_priv *qspi_priv = NULL;

	qspi_priv = os_qspi_priv;

	k_free(qspi_priv);
}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
static int zep_shim_bus_qspi_ps_sleep(void *os_qspi_priv)
{
	rpu_sleep();

	return 0;
}

static int zep_shim_bus_qspi_ps_wake(void *os_qspi_priv)
{
	rpu_wakeup();

	return 0;
}

static int zep_shim_bus_qspi_ps_status(void *os_qspi_priv)
{
	return rpu_sleep_status();
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

static void zep_shim_bus_qspi_dev_host_map_get(void *os_qspi_dev_ctx,
					       struct nrf_wifi_osal_host_map *host_map)
{
	if (!os_qspi_dev_ctx || !host_map) {
		LOG_ERR("%s: Invalid parameters\n", __func__);
		return;
	}

	host_map->addr = 0;
}

static void irq_work_handler(struct k_work *work)
{
	int ret = 0;

	ret = intr_priv->callbk_fn(intr_priv->callbk_data);

	if (ret) {
		LOG_ERR("%s: Interrupt callback failed\n", __func__);
	}
}


extern struct k_work_q zep_wifi_intr_q;

static void zep_shim_irq_handler(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	k_work_schedule_for_queue(&zep_wifi_intr_q, &intr_priv->work, K_NO_WAIT);
}

static enum nrf_wifi_status zep_shim_bus_qspi_intr_reg(void *os_dev_ctx, void *callbk_data,
						       int (*callbk_fn)(void *callbk_data))
{
	enum nrf_wifi_status status = NRF_WIFI_STATUS_FAIL;
	int ret = -1;

	ARG_UNUSED(os_dev_ctx);

	intr_priv = k_calloc(sizeof(*intr_priv), sizeof(char));

	if (!intr_priv) {
		LOG_ERR("%s: Unable to allocate memory for intr_priv\n", __func__);
		goto out;
	}

	intr_priv->callbk_data = callbk_data;
	intr_priv->callbk_fn = callbk_fn;

	k_work_init_delayable(&intr_priv->work, irq_work_handler);

	ret = rpu_irq_config(&intr_priv->gpio_cb_data, zep_shim_irq_handler);

	if (ret) {
		LOG_ERR("%s: request_irq failed\n", __func__);
		k_free(intr_priv);
		intr_priv = NULL;
		goto out;
	}

	status = NRF_WIFI_STATUS_SUCCESS;

out:
	return status;
}

static void zep_shim_bus_qspi_intr_unreg(void *os_qspi_dev_ctx)
{
	ARG_UNUSED(os_qspi_dev_ctx);

	k_work_cancel_delayable(&intr_priv->work);

	rpu_irq_remove(&intr_priv->gpio_cb_data);

	k_free(intr_priv);
	intr_priv = NULL;
}

#ifdef CONFIG_NRF_WIFI_LOW_POWER
static void *zep_shim_timer_alloc(void)
{
	struct timer_list *timer = NULL;

	timer = k_malloc(sizeof(*timer));

	if (!timer)
		LOG_ERR("%s: Unable to allocate memory for work\n", __func__);

	return timer;
}

static void zep_shim_timer_init(void *timer, void (*callback)(unsigned long), unsigned long data)
{
	((struct timer_list *)timer)->function = callback;
	((struct timer_list *)timer)->data = data;

	init_timer(timer);
}

static void zep_shim_timer_free(void *timer)
{
	k_free(timer);
}

static void zep_shim_timer_schedule(void *timer, unsigned long duration)
{
	mod_timer(timer, duration);
}

static void zep_shim_timer_kill(void *timer)
{
	del_timer_sync(timer);
}
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

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
		LOG_ERR("%s: Invalid assertion operation\n", __func__);
	}
}

static unsigned int zep_shim_strlen(const void *str)
{
	return strlen(str);
}

static const struct nrf_wifi_osal_ops nrf_wifi_os_zep_ops = {
	.mem_alloc = zep_shim_mem_alloc,
	.mem_zalloc = zep_shim_mem_zalloc,
	.mem_free = k_free,
	.mem_cpy = zep_shim_mem_cpy,
	.mem_set = zep_shim_mem_set,
	.mem_cmp = zep_shim_mem_cmp,

	.qspi_read_reg32 = zep_shim_qspi_read_reg32,
	.qspi_write_reg32 = zep_shim_qspi_write_reg32,
	.qspi_cpy_from = zep_shim_qspi_cpy_from,
	.qspi_cpy_to = zep_shim_qspi_cpy_to,

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
	.llist_node_free = zep_shim_llist_node_free,
	.llist_node_data_get = zep_shim_llist_node_data_get,
	.llist_node_data_set = zep_shim_llist_node_data_set,

	.llist_alloc = zep_shim_llist_alloc,
	.llist_free = zep_shim_llist_free,
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

	.tasklet_alloc = zep_shim_work_alloc,
	.tasklet_free = zep_shim_work_free,
	.tasklet_init = zep_shim_work_init,
	.tasklet_schedule = zep_shim_work_schedule,
	.tasklet_kill = zep_shim_work_kill,

	.sleep_ms = k_msleep,
	.delay_us = k_usleep,
	.time_get_curr_us = zep_shim_time_get_curr_us,
	.time_elapsed_us = zep_shim_time_elapsed_us,

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
	.timer_alloc = zep_shim_timer_alloc,
	.timer_init = zep_shim_timer_init,
	.timer_free = zep_shim_timer_free,
	.timer_schedule = zep_shim_timer_schedule,
	.timer_kill = zep_shim_timer_kill,

	.bus_qspi_ps_sleep = zep_shim_bus_qspi_ps_sleep,
	.bus_qspi_ps_wake = zep_shim_bus_qspi_ps_wake,
	.bus_qspi_ps_status = zep_shim_bus_qspi_ps_status,
#endif /* CONFIG_NRF_WIFI_LOW_POWER */

	.assert = zep_shim_assert,
	.strlen = zep_shim_strlen,
};

const struct nrf_wifi_osal_ops *get_os_ops(void)
{
	return &nrf_wifi_os_zep_ops;
}
