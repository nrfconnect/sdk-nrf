/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file dect_utils.h
 * @brief DECT NR+ utility functions
 *
 * This header provides misc utility functions for DECT NR+ operations.
 */

#ifndef DECT_UTILS_H
#define DECT_UTILS_H

#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_pkt.h>
/**
 * @brief Extract destination long RD ID from socket link-layer address
 *
 * @param dst_addr Pointer to the destination socket link-layer address structure
 * @return The extracted 32-bit long RD (Radio Device) ID. 0 if extraction fails.
 */
uint32_t dect_utils_lib_dst_long_rd_id_get_from_dst_sock_ll_addr(struct sockaddr_ll *dst_addr);

/**
 * @brief Extract destination long RD ID from network packet destination address
 *
 * @param pkt Pointer to the network packet structure
 * @return The extracted 32-bit long RD (Radio Device) ID from packet destination.
 *         0 if extraction fails.
 */
uint32_t dect_utils_lib_dst_long_rd_id_get_from_pkt_dst_addr(struct net_pkt *pkt);

/**
 * @brief Set network link address from long RD ID
 *
 * Configures a network link address structure using the provided long RD ID.
 *
 * @param lladdr Pointer to the network link address structure to be set
 * @param long_rd_id The 32-bit long RD ID to use for address configuration
 * @return true if the address was successfully set, false otherwise
 */
bool dect_utils_lib_net_linkaddr_set_from_long_rd_id(struct net_linkaddr *lladdr,
						     uint32_t long_rd_id);

/**
 * @brief Create IPv6 Interface Identifier (IID) from link address
 *
 * Generates an IPv6 Interface Identifier for the given address using the
 * provided link-layer address.
 * This is NR+ version of zephyr net_ipv6_addr_create_iid().
 * Note: no toggling of universal/local bit
 *
 * @param addr Pointer to the IPv6 address structure where IID will be stored
 * @param lladdr Pointer to the link-layer address to derive IID from
 */
void dect_utils_lib_net_ipv6_addr_create_iid(struct in6_addr *addr, struct net_linkaddr *lladdr);

/**
 * @brief Extract long RD ID from IPv6 address
 *
 * Extracts the long RD ID from an IPv6 address structure.
 *
 * @param addr Pointer to the IPv6 address structure
 * @return The extracted 32-bit long RD ID. 0 if extraction fails.
 */
uint32_t dect_utils_lib_long_rd_id_from_ipv6_addr(struct in6_addr *addr);

/**
 * @brief Extract sink RD ID from IPv6 address
 *
 * Extracts the sink RD ID from an IPv6 address structure.
 *
 * @param addr Pointer to the IPv6 address structure
 * @return The extracted 32-bit sink RD ID. 0 if extraction fails.
 */
uint32_t dect_utils_lib_sink_rd_id_from_ipv6_addr(struct in6_addr *addr);

/**
 * @brief Create IPv6 address from sink and long RD IDs
 *
 * Constructs an IPv6 address using a 64-bit prefix, sink RD ID, and own RD ID.
 * This is typically used for creating IPv6 addresses in DECT NR+ network topology.
 *
 * @param prefix_64 The 64-bit IPv6 prefix
 * @param sink_rd_id The sink Radio Device ID (32-bit)
 * @param own_rd_id The own Radio Device ID (32-bit)
 * @param addr Pointer to the IPv6 address structure where result will be stored
 * @return true if the address was successfully created, false otherwise
 */
bool dect_utils_lib_net_ipv6_addr_create_from_sink_and_long_rd_id(struct in6_addr prefix_64,
								  uint32_t sink_rd_id,
								  uint32_t own_rd_id,
								  struct in6_addr *addr);

/**
 * @brief Convert dBm power value to PHY transmission power
 *
 * Converts a power value in dBm (decibels relative to milliwatt) to the
 * corresponding PHY layer transmission power representation.
 *
 * @param pwr_dBm Power value in dBm (signed 8-bit)
 * @return Corresponding PHY transmission power value (unsigned 8-bit)
 */
uint8_t dect_utils_lib_dbm_to_phy_tx_power(int8_t pwr_dBm);

/**
 * @brief Convert PHY transmission power to dBm value
 *
 * Converts a PHY layer transmission power value to the corresponding
 * power value in dBm (decibels relative to milliwatt).
 *
 * @param phy_power PHY transmission power value (unsigned 8-bit)
 * @return Corresponding power value in dBm (signed 8-bit)
 */
int8_t dect_utils_lib_phy_tx_power_to_dbm(uint8_t phy_power);

/**
 * @brief Validate 32-bit network ID
 *
 * Validates whether the provided 32-bit network ID is valid according to
 * DECT NR+ network ID specifications and constraints.
 *
 * @param network_id The 32-bit network ID to validate
 * @return true if the network ID is valid, false otherwise
 */
bool dect_utils_lib_32bit_network_id_validate(uint32_t network_id);

#endif /* DECT_UTILS_H */
