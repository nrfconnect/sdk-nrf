/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @brief Header containing network buffer management function declarations for the nRF71 driver.
 */

#ifndef __NBUF_MGMT_H__
#define __NBUF_MGMT_H__

#include <stdbool.h>

#include <zephyr/net/net_pkt.h>

/** Extra headroom reserved for Wi-Fi TX headers. */
#define NRF_WIFI_EXTRA_TX_HEADROOM 100

/** Driver network buffer wrapper around a payload region. */
struct nrf_wifi_nwb {
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
#endif /* CONFIG_NRF_WIFI_ZERO_COPY_TX */
};

/**
 * @brief Allocate a network buffer from the data pool.
 *
 * @param size Size in bytes of the payload area.
 *
 * @return Pointer to the network buffer on success, NULL on failure.
 */
void *nrf_wifi_nbuf_alloc(unsigned int size);

/**
 * @brief Free a network buffer allocated by @ref nrf_wifi_nbuf_alloc.
 *
 * @param nbuf Pointer to the network buffer to free. No operation if NULL.
 */
void nrf_wifi_nbuf_free(void *nbuf);

/**
 * @brief Reserve headroom at the start of the network buffer data area.
 *
 * @param nbuf Pointer to a network buffer.
 * @param size Number of headroom bytes to reserve.
 */
void nrf_wifi_nbuf_headroom_res(void *nbuf, unsigned int size);

/**
 * @brief Get the size of reserved headroom in a network buffer.
 *
 * @param nbuf Pointer to a network buffer.
 *
 * @return Headroom size in bytes.
 */
unsigned int nrf_wifi_nbuf_headroom_get(void *nbuf);

/**
 * @brief Reserve tailroom at the end of the network buffer data area.
 *
 * @param nbuf Pointer to a network buffer.
 * @param size Number of tailroom bytes to reserve.
 */
void nrf_wifi_nbuf_tailroom_res(void *nbuf, unsigned int size);

/**
 * @brief Get the size of data in a network buffer.
 *
 * @param nbuf Pointer to a network buffer.
 *
 * @return Data size in bytes.
 */
unsigned int nrf_wifi_nbuf_data_size(void *nbuf);

/**
 * @brief Get a pointer to the start of the network buffer data area.
 *
 * @param nbuf Pointer to a network buffer.
 *
 * @return Pointer to the data area.
 */
void *nrf_wifi_nbuf_data_get(void *nbuf);

/**
 * @brief Extend the tail of the network buffer data area.
 *
 * @param nbuf Pointer to a network buffer.
 * @param size Number of bytes to add at the tail.
 *
 * @return Pointer to the start of the newly added region.
 */
void *nrf_wifi_nbuf_data_put(void *nbuf, unsigned int size);

/**
 * @brief Extend the head of the network buffer data area.
 *
 * @param nbuf Pointer to a network buffer.
 * @param size Number of bytes to add at the head.
 *
 * @return Pointer to the start of the extended data area.
 */
void *nrf_wifi_nbuf_data_push(void *nbuf, unsigned int size);

/**
 * @brief Shrink the head of the network buffer data area.
 *
 * @param nbuf Pointer to a network buffer.
 * @param size Number of bytes to remove from the head.
 *
 * @return Pointer to the new start of the data area.
 */
void *nrf_wifi_nbuf_data_pull(void *nbuf, unsigned int size);

/**
 * @brief Get the priority of a network buffer.
 *
 * @param nbuf Pointer to a network buffer.
 *
 * @return Packet priority.
 */
unsigned char nrf_wifi_nbuf_get_priority(void *nbuf);

/**
 * @brief Get the checksum-done flag of a network buffer.
 *
 * @param nbuf Pointer to a network buffer.
 *
 * @return Non-zero if the checksum is marked done, zero otherwise.
 */
unsigned char nrf_wifi_nbuf_get_chksum_done(void *nbuf);

/**
 * @brief Set the checksum-done flag of a network buffer.
 *
 * @param nbuf Pointer to a network buffer.
 * @param chksum_done Checksum-done flag value.
 */
void nrf_wifi_nbuf_set_chksum_done(void *nbuf, unsigned char chksum_done);

#if defined(CONFIG_NRF71_RAW_DATA_TX) || defined(__DOXYGEN__)
/**
 * @brief Reserve space for a raw TX header in a network buffer.
 *
 * @param nbuf Pointer to a network buffer.
 * @param raw_hdr_len Length of the raw TX header in bytes.
 *
 * @return Pointer to the raw TX header on success, NULL on failure.
 */
void *nrf_wifi_nbuf_set_raw_tx_hdr(void *nbuf, unsigned short raw_hdr_len);

/**
 * @brief Get the raw TX header from a network buffer.
 *
 * @param nbuf Pointer to a network buffer.
 *
 * @return Pointer to the raw TX header on success, NULL on failure.
 */
void *nrf_wifi_nbuf_get_raw_tx_hdr(void *nbuf);

/**
 * @brief Check whether a network buffer carries a raw TX header.
 *
 * @param nbuf Pointer to a network buffer.
 *
 * @return true if the buffer has a raw TX header, false otherwise.
 */
bool nrf_wifi_nbuf_is_raw_tx(void *nbuf);
#endif /* CONFIG_NRF71_RAW_DATA_TX || __DOXYGEN__ */

/**
 * @brief Convert a Zephyr @c net_pkt into a driver network buffer.
 *
 * @param pkt Pointer to the network packet to convert.
 *
 * @return Pointer to a driver network buffer on success, NULL on failure.
 */
void *nrf_wifi_net_pkt_to_nbuf(struct net_pkt *pkt);

/**
 * @brief Convert a driver network buffer into a Zephyr @c net_pkt.
 *
 * The network buffer is freed before this function returns.
 *
 * @param iface Network interface used to allocate the packet.
 * @param frm Pointer to the driver network buffer.
 *
 * @return Pointer to a network packet on success, NULL on failure.
 */
struct net_pkt *nrf_wifi_net_pkt_from_nbuf(void *iface, void *frm);

#if defined(CONFIG_NRF71_RAW_DATA_RX) || defined(CONFIG_NRF71_PROMISC_DATA_RX)
/**
 * @brief Build a raw RX @c net_pkt from a driver network buffer and header.
 *
 * @param iface Network interface used to allocate the packet.
 * @param frm Pointer to the driver network buffer.
 * @param raw_hdr_len Length of the raw RX header in bytes.
 * @param raw_rx_hdr Pointer to the raw RX header data.
 * @param pkt_free If true, free @p frm before returning.
 *
 * @return Pointer to a network packet on success, NULL on failure.
 */
struct net_pkt *nrf_wifi_net_raw_pkt_from_nbuf(void *iface,
					       void *frm,
					       unsigned short raw_hdr_len,
					       void *raw_rx_hdr,
					       bool pkt_free);
#endif /* CONFIG_NRF71_RAW_DATA_RX || CONFIG_NRF71_PROMISC_DATA_RX */

#endif /* __NBUF_MGMT_H__ */
