/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net_buf.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>
#if defined(CONFIG_SHELL)
#include <zephyr/shell/shell.h>
#endif

#include "dect_mdm_rx_pool.h"

LOG_MODULE_DECLARE(dect_mdm, CONFIG_DECT_MDM_LOG_LEVEL);

#if defined(CONFIG_DECT_MDM_RX_PRIVATE_POOL)

NET_PKT_SLAB_DEFINE(dect_mdm_rx_pkts, CONFIG_DECT_MDM_RX_PRIVATE_PKT_COUNT);

NET_BUF_POOL_FIXED_DEFINE(dect_mdm_rx_bufs,
			  CONFIG_DECT_MDM_RX_PRIVATE_BUF_COUNT,
			  CONFIG_DECT_MDM_RX_PRIVATE_BUF_SIZE,
			  CONFIG_NET_PKT_BUF_USER_DATA_SIZE, NULL);

struct net_pkt *dect_mdm_rx_pkt_alloc(struct net_if *iface, size_t len)
{
	const size_t buf_size = (size_t)CONFIG_DECT_MDM_RX_PRIVATE_BUF_SIZE;
	struct net_pkt *pkt;
	size_t left = len;

	/* DLC RX callback may run in ISR context, so K_NO_WAIT is mandatory. */
	pkt = net_pkt_alloc_from_slab(&dect_mdm_rx_pkts, K_NO_WAIT);
	if (!pkt) {
		LOG_ERR("DECT RX pool: pkt slab exhausted (len %u, free %u/%u)", (unsigned int)len,
			k_mem_slab_num_free_get(&dect_mdm_rx_pkts),
			dect_mdm_rx_pkts.info.num_blocks);
		return NULL;
	}

	net_pkt_set_iface(pkt, iface);
	net_pkt_set_family(pkt, AF_UNSPEC);

	while (left > 0) {
		struct net_buf *buf = net_buf_alloc(&dect_mdm_rx_bufs, K_NO_WAIT);

		if (!buf) {
			LOG_ERR("DECT RX pool: buf pool exhausted (need %u of %u, total %u)",
				(unsigned int)left, (unsigned int)len,
				dect_mdm_rx_bufs.buf_count);
			net_pkt_unref(pkt);
			return NULL;
		}
		net_pkt_append_buffer(pkt, buf);
		left = (left > buf_size) ? (left - buf_size) : 0;
	}

	net_pkt_cursor_init(pkt);
	return pkt;
}

#else /* !CONFIG_DECT_MDM_RX_PRIVATE_POOL */

struct net_pkt *dect_mdm_rx_pkt_alloc(struct net_if *iface, size_t len)
{
	return net_pkt_rx_alloc_with_buffer(iface, len, AF_UNSPEC, 0, K_NO_WAIT);
}

#endif /* CONFIG_DECT_MDM_RX_PRIVATE_POOL */

#if defined(CONFIG_SHELL) && defined(CONFIG_DECT_MDM_RX_PRIVATE_POOL)

static int cmd_dect_rx_pool(const struct shell *sh, size_t argc, char **argv)
{
	ARG_UNUSED(argc);
	ARG_UNUSED(argv);

	const uint32_t pkt_total = dect_mdm_rx_pkts.info.num_blocks;
	const uint32_t pkt_free  = k_mem_slab_num_free_get(&dect_mdm_rx_pkts);

	shell_print(sh, "DECT private RX pool:");
	shell_print(sh, "Address\t\tTotal\tFree\tMaxUsed\tName");
#if defined(CONFIG_MEM_SLAB_TRACE_MAX_UTILIZATION)
	shell_print(sh, "%p\t%u\t%u\t%u\tdect_mdm_rx_pkts (slab)",
		    (void *)&dect_mdm_rx_pkts, pkt_total, pkt_free,
		    dect_mdm_rx_pkts.info.max_used);
#else
	shell_print(sh, "%p\t%u\t%u\t-\tdect_mdm_rx_pkts (slab)",
		    (void *)&dect_mdm_rx_pkts, pkt_total, pkt_free);
#endif

#if defined(CONFIG_NET_BUF_POOL_USAGE)
	shell_print(sh, "%p\t%u\t%ld\t%u\tdect_mdm_rx_bufs (bufs, %u B)",
		    (void *)&dect_mdm_rx_bufs,
		    dect_mdm_rx_bufs.buf_count,
		    atomic_get(&dect_mdm_rx_bufs.avail_count),
		    dect_mdm_rx_bufs.max_used,
		    (unsigned int)CONFIG_DECT_MDM_RX_PRIVATE_BUF_SIZE);
#else
	shell_print(sh, "%p\t%u\t-\t-\tdect_mdm_rx_bufs (bufs, %u B)",
		    (void *)&dect_mdm_rx_bufs,
		    dect_mdm_rx_bufs.buf_count,
		    (unsigned int)CONFIG_DECT_MDM_RX_PRIVATE_BUF_SIZE);
	shell_print(sh, "(set CONFIG_NET_BUF_POOL_USAGE=y for buf availability)");
#endif

	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(sub_dect_mdm,
	SHELL_CMD(rx_pool, NULL,
		  "Print DECT private RX pool stats (slab/bufs).",
		  cmd_dect_rx_pool),
	SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(dect_mdm, &sub_dect_mdm, "DECT modem driver", NULL);

#endif /* CONFIG_SHELL && CONFIG_DECT_MDM_RX_PRIVATE_POOL */
