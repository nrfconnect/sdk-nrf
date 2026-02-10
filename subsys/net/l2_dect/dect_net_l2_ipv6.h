/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file dect_net_l2_ipv6.h
 * @brief DECT NR+ L2 IPv6 Address Management
 *
 * Internal module for managing IPv6 addressing in DECT NR+ cluster topology.
 * Handles link-local and global address assignment, neighbor discovery,
 * and prefix delegation for parent/child associations.
 */

#ifndef DECT_NET_L2_IPV6_UTIL_H_
#define DECT_NET_L2_IPV6_UTIL_H_

#include "dect_net_l2_internal.h"

#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Setup IPv6 addressing after parent association created
 *
 * Stores prefix config, removes old link-local addr, creates new link-local
 * and global addresses, adds parent as neighbor in cache.
 *
 * @param list_item Association data structure to update
 * @param iface Network interface
 * @param parent_long_rd_id Parent device long RD identifier
 * @param ipv6_prefix_config IPv6 prefix configuration from parent
 */
void dect_net_l2_ipv6_addressing_parent_added_handle(struct dect_net_l2_association_data *list_item,
	struct net_if *iface, uint32_t parent_long_rd_id,
	struct dect_net_ipv6_prefix_config *ipv6_prefix_config);

/**
 * @brief Setup IPv6 addressing after child association created
 *
 * If first child: updates own link-local and global addresses.
 * Adds child as neighbor with both local and global IPv6 addresses.
 *
 * @param ass_list_item Association data structure to update
 * @param iface Network interface
 * @param child_long_rd_id Child device long RD identifier
 * @param first_child true if this is the first child association
 */
void dect_net_l2_ipv6_addressing_child_added_handle(
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface,
	uint32_t child_long_rd_id, bool first_child);

/**
 * @brief Clean up IPv6 addressing after child association removed
 *
 * Removes child's local and global IPv6 addresses from neighbor cache.
 * Clears association IPv6 address flags.
 *
 * @param ass_list_item Association data structure to clean up
 * @param iface Network interface
 * @param child_long_rd_id Child device long RD identifier
 */
void dect_net_l2_ipv6_addressing_child_removed_handle(
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface,
	uint32_t child_long_rd_id);

/**
 * @brief Remove child's global IPv6 address from neighbor cache only
 *
 * Removes only global IPv6 neighbor entry, leaves local address intact.
 * Used during prefix changes when only global addressing needs update.
 *
 * @param ass_list_item Association data structure to update
 * @param iface Network interface
 */
void dect_net_l2_ipv6_global_addressing_child_removed_handle(
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface);

/**
 * @brief Add child's new global IPv6 address to neighbor cache
 *
 * Creates and adds new global IPv6 neighbor entry using current prefix.
 * Used after prefix changes to update child's global addressing.
 *
 * @param ctx L2 context with current prefix configuration
 * @param ass_list_item Association data structure to update
 * @param iface Network interface
 */
void dect_net_l2_ipv6_global_addressing_child_changed_handle(struct dect_net_l2_context *ctx,
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface);

/**
 * @brief Clean up IPv6 addressing after parent association removed
 *
 * Removes parent from neighbor cache, removes own link-local address,
 * recreates original link-local address, removes global address.
 * Reverts to standalone addressing.
 *
 * @param ass_list_item Association data structure to clean up
 * @param iface Network interface
 * @param parent_long_rd_id Parent device long RD identifier
 */
void dect_net_l2_ipv6_parent_addressing_removed_handle(
	struct dect_net_l2_association_data *ass_list_item, struct net_if *iface,
	uint32_t parent_long_rd_id);

/**
 * @brief Replace global IPv6 addressing with new sink prefix
 *
 * Gets sink prefix from L2 sink module, removes all old global addresses,
 * creates and adds new global address using sink prefix.
 *
 * @param dect_iface DECT network interface
 */
void dect_net_l2_ipv6_global_addressing_replace(struct net_if *dect_iface);

/**
 * @brief Handle parent IPv6 prefix configuration changes
 *
 * Compares old vs new prefix and handles three cases:
 * - Prefix added: calls parent_added_handle()
 * - Prefix removed: removes global addr and neighbor entries
 * - Prefix changed: removes old, adds new prefix
 *
 * @param list_item Association data structure to update
 * @param iface Network interface
 * @param parent_long_rd_id Parent device long RD identifier
 * @param ipv6_prefix_config New IPv6 prefix configuration from parent
 */
void dect_net_l2_ipv6_addressing_parent_changed_handle(
	struct dect_net_l2_association_data *list_item, struct net_if *iface,
	uint32_t parent_long_rd_id, struct dect_net_ipv6_prefix_config *ipv6_prefix_config);

/**
 * @brief Handle sink IPv6 prefix changes and update child addressing
 *
 * Compares old vs new sink prefix and handles three cases:
 * - Prefix added: calls global_addressing_replace() and prefix_replace()
 * - Prefix removed: removes prefix and global addresses
 * - Prefix changed: replaces addressing with new prefix
 *
 * @param iface Network interface
 * @param ipv6_prefix_config New IPv6 prefix configuration for sink
 * @return true if global addressing changed and children need updates
 */
bool dect_net_l2_ipv6_addressing_sink_changed_handle(struct net_if *iface,
	struct dect_net_ipv6_prefix_config *ipv6_prefix_config);

#ifdef __cplusplus
}
#endif

#endif /* DECT_NET_L2_IPV6_UTIL_H_ */
