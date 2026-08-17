/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Network buffer management functions for the nRF71 driver.
 */

#include <string.h>

#include <common/mem_mgmt.h>
#include <common/nbuf_mgmt.h>

#include <zephyr/logging/log.h>
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_pkt.h>
#include <zephyr/net/ethernet.h>

LOG_MODULE_DECLARE(wifi_nrf, CONFIG_WIFI_NRF71_LOG_LEVEL);

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

void *nrf_wifi_nbuf_alloc(unsigned int size)
{
	struct nrf_wifi_nwb *nbuff;

	nbuff = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, sizeof(*nbuff));

	if (!nbuff) {
		return NULL;
	}

	nbuff->priv = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, size);

	if (!nbuff->priv) {
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, nbuff);
		return NULL;
	}

	nbuff->data = nbuff->priv;
	nbuff->tail = nbuff->data;
	nbuff->end = (unsigned char *)nbuff->priv + size;
	nbuff->len = 0;
	nbuff->headroom = 0;
	nbuff->next = NULL;

	return nbuff;
}

void nrf_wifi_nbuf_free(void *nbuf)
{
	struct nrf_wifi_nwb *nwb = nbuf;

	if (!nwb) {
		return;
	}

#ifdef CONFIG_NRF_WIFI_ZERO_COPY_TX
	if (nwb->pkt) {
		net_pkt_unref(nwb->pkt);
		nwb->pkt = NULL;
	}
#endif /* CONFIG_NRF_WIFI_ZERO_COPY_TX */

	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, nwb->priv);
	nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, nwb);
}

void nrf_wifi_nbuf_headroom_res(void *nbuf, unsigned int size)
{
	struct nrf_wifi_nwb *nwb = nbuf;

	nwb->data += size;
	nwb->tail += size;
	nwb->headroom += size;
}

unsigned int nrf_wifi_nbuf_headroom_get(void *nbuf)
{
	return ((struct nrf_wifi_nwb *)nbuf)->headroom;
}

void nrf_wifi_nbuf_tailroom_res(void *nbuf, unsigned int size)
{
	struct nrf_wifi_nwb *nwb = nbuf;

	NET_ASSERT(nwb->end - nwb->tail >= (int)size,
		   "nbuf tailroom reserve (%u) exceeds free space", size);

	nwb->end -= size;
}

unsigned int nrf_wifi_nbuf_data_size(void *nbuf)
{
	return ((struct nrf_wifi_nwb *)nbuf)->len;
}

void *nrf_wifi_nbuf_data_get(void *nbuf)
{
	return ((struct nrf_wifi_nwb *)nbuf)->data;
}

void *nrf_wifi_nbuf_data_put(void *nbuf, unsigned int size)
{
	struct nrf_wifi_nwb *nwb = nbuf;
	unsigned char *data = nwb->tail;

	NET_ASSERT(nwb->tail + size <= nwb->end,
		   "nbuf data put (%u) overruns reserved tailroom", size);

	nwb->tail += size;
	nwb->len += size;

	return data;
}

void *nrf_wifi_nbuf_data_push(void *nbuf, unsigned int size)
{
	struct nrf_wifi_nwb *nwb = nbuf;

	nwb->data -= size;
	nwb->headroom -= size;
	nwb->len += size;

	return nwb->data;
}

void *nrf_wifi_nbuf_data_pull(void *nbuf, unsigned int size)
{
	struct nrf_wifi_nwb *nwb = nbuf;

	nwb->data += size;
	nwb->headroom += size;
	nwb->len -= size;

	return nwb->data;
}

unsigned char nrf_wifi_nbuf_get_priority(void *nbuf)
{
	return ((struct nrf_wifi_nwb *)nbuf)->priority;
}

unsigned char nrf_wifi_nbuf_get_chksum_done(void *nbuf)
{
	return ((struct nrf_wifi_nwb *)nbuf)->chksum_done;
}

void nrf_wifi_nbuf_set_chksum_done(void *nbuf, unsigned char chksum_done)
{
	((struct nrf_wifi_nwb *)nbuf)->chksum_done = (bool)chksum_done;
}

#if defined(CONFIG_NRF71_RAW_DATA_TX)
void *nrf_wifi_nbuf_set_raw_tx_hdr(void *nbuf, unsigned short raw_hdr_len)
{
	struct nrf_wifi_nwb *nwb = nbuf;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return NULL;
	}

	nwb->raw_tx_hdr = nrf_wifi_nbuf_data_get(nwb);
	if (!nwb->raw_tx_hdr) {
		LOG_ERR("%s: Unable to set raw Tx header in network buffer", __func__);
		return NULL;
	}

	nrf_wifi_nbuf_data_pull(nwb, raw_hdr_len);

	return nwb->raw_tx_hdr;
}

void *nrf_wifi_nbuf_get_raw_tx_hdr(void *nbuf)
{
	struct nrf_wifi_nwb *nwb = nbuf;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return NULL;
	}

	return nwb->raw_tx_hdr;
}

bool nrf_wifi_nbuf_is_raw_tx(void *nbuf)
{
	struct nrf_wifi_nwb *nwb = nbuf;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return false;
	}

	return (nwb->raw_tx_hdr != NULL);
}
#endif /* CONFIG_NRF71_RAW_DATA_TX */

#ifdef CONFIG_NRF_WIFI_ZERO_COPY_TX
static void *nrf_wifi_net_pkt_to_nbuf_zc(struct net_pkt *pkt)
{
	struct nrf_wifi_nwb *nbuff;

	if (!pkt || !pkt->buffer) {
		LOG_DBG("Invalid packet, dropping");
		return NULL;
	}

	if (pkt->buffer->frags) {
		LOG_ERR("Zero-copy only supports single buffer packets");
		return NULL;
	}

	nbuff = nrf_wifi_nbuf_alloc(NRF_WIFI_EXTRA_TX_HEADROOM);
	if (!nbuff) {
		return NULL;
	}

	nrf_wifi_nbuf_headroom_res(nbuff, NRF_WIFI_EXTRA_TX_HEADROOM);

	nbuff->data = pkt->buffer->data;
	nbuff->len = pkt->buffer->len;
	nbuff->priority = net_pkt_priority(pkt);
	nbuff->chksum_done = (bool)net_pkt_is_chksum_done(pkt);
	nbuff->pkt = pkt;
	net_pkt_ref(pkt);

	return nbuff;
}
#endif /* CONFIG_NRF_WIFI_ZERO_COPY_TX */

void *nrf_wifi_net_pkt_to_nbuf(struct net_pkt *pkt)
{
	struct nrf_wifi_nwb *nbuff;
	unsigned char *data;
	unsigned int len;

	if (!pkt) {
		return NULL;
	}

#ifdef CONFIG_NRF_WIFI_ZERO_COPY_TX
	if (pkt->buffer && !pkt->buffer->frags) {
		return nrf_wifi_net_pkt_to_nbuf_zc(pkt);
	}
#endif /* CONFIG_NRF_WIFI_ZERO_COPY_TX */

	len = net_pkt_get_len(pkt);

	nbuff = nrf_wifi_nbuf_alloc(len + NRF_WIFI_EXTRA_TX_HEADROOM + NRF71_TX_MIC_TAILROOM);
	if (!nbuff) {
		return NULL;
	}

	nrf_wifi_nbuf_headroom_res(nbuff, NRF_WIFI_EXTRA_TX_HEADROOM);
	nrf_wifi_nbuf_tailroom_res(nbuff, NRF71_TX_MIC_TAILROOM);

	data = nrf_wifi_nbuf_data_put(nbuff, len);

	net_pkt_read(pkt, data, len);

	nbuff->priority = net_pkt_priority(pkt);
	nbuff->chksum_done = (bool)net_pkt_is_chksum_done(pkt);

	return nbuff;
}

struct net_pkt *nrf_wifi_net_pkt_from_nbuf(void *iface, void *frm)
{
	struct net_pkt *pkt = NULL;
	unsigned char *data;
	unsigned int len;
	struct nrf_wifi_nwb *nwb = frm;

	if (!nwb) {
		return NULL;
	}

	len = nrf_wifi_nbuf_data_size(nwb);
	data = nrf_wifi_nbuf_data_get(nwb);

	pkt = net_pkt_rx_alloc_with_buffer(iface, len, NET_AF_UNSPEC, 0, K_MSEC(100));
	if (!pkt) {
		goto out;
	}

	if (net_pkt_write(pkt, data, len)) {
		net_pkt_unref(pkt);
		pkt = NULL;
	}

out:
	nrf_wifi_nbuf_free(nwb);
	return pkt;
}

#if defined(CONFIG_NRF71_RAW_DATA_RX) || defined(CONFIG_NRF71_PROMISC_DATA_RX)
struct net_pkt *nrf_wifi_net_raw_pkt_from_nbuf(void *iface, void *frm,
					       unsigned short raw_hdr_len,
					       void *raw_rx_hdr,
					       bool pkt_free)
{
	struct net_pkt *pkt = NULL;
	unsigned char *nwb_data;
	unsigned char *data = NULL;
	unsigned int nwb_len;
	unsigned int total_len;
	struct nrf_wifi_nwb *nwb = frm;

	if (!nwb) {
		LOG_ERR("%s: Received network buffer is NULL", __func__);
		return NULL;
	}

	nwb_len = nrf_wifi_nbuf_data_size(nwb);
	nwb_data = nrf_wifi_nbuf_data_get(nwb);
	total_len = raw_hdr_len + nwb_len;

	data = nrf_wifi_mem_zalloc(NRF_WIFI_MEM_POOL_TYPE_DATA, total_len);
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
	memcpy(data + raw_hdr_len, nwb_data, nwb_len);

	if (net_pkt_write(pkt, data, total_len)) {
		net_pkt_unref(pkt);
		pkt = NULL;
	}

out:
	if (data != NULL) {
		nrf_wifi_mem_free(NRF_WIFI_MEM_POOL_TYPE_DATA, data);
	}

	if (pkt_free) {
		nrf_wifi_nbuf_free(nwb);
	}

	return pkt;
}
#endif /* CONFIG_NRF71_RAW_DATA_RX || CONFIG_NRF71_PROMISC_DATA_RX */
