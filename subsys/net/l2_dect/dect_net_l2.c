/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/**
 * @file dect_net_l2.c
 * @brief DECT NR+ L2 Networking Layer
 *
 * Implements DECT NR+-aware IPv6 networking layer that integrates with Zephyr networking stack.
 * Provides intelligent packet routing based on DECT cluster topology and RD ID addressing.
 *
 * Key Features:
 * - IPv6 link-local routing using DECT RD IDs embedded in addresses
 * - Association management for parent/child device relationships
 * - Multicast forwarding to all associated children
 * - Integration with Zephyr L2 networking API
 * - Thread-safe operation in RX thread context
 *
 * Call Chain:
 * Application → Zephyr Socket → IPv6 Stack → DECT L2 → DECT MAC → nRF Modem
 *
 * Thread Context:
 * - dect_net_l2_recv(): Called in RX thread (dect_nrf91_rx_th)
 * - dect_net_l2_send(): Called in application thread context
 * - Association callbacks: Called from MAC layer (various contexts)
 */
#include <zephyr/net/net_core.h>
#include <zephyr/net/net_if.h>

#include "ipv6.h"

#include <zephyr/net/net_pkt.h>
#include <zephyr/net/mld.h>
#include <zephyr/net/dns_sd.h>

#include <net/dect/dect_utils.h>

#include <net/dect/dect_net_l2.h>
#include <net/dect/dect_net_l2_mgmt.h>

#include "dect_net_l2_ipv6.h"
#include "dect_net_l2_sink.h"
#include "dect_net_l2_internal.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(net_l2_dect, CONFIG_NET_L2_DECT_LOG_LEVEL);

#include "net_private.h" /* For net_sprint_ipv6_addr */

/**************************************************************************************************/

static struct dect_net_l2_association_data
	child_associations[CONFIG_DECT_CLUSTER_MAX_CHILD_ASSOCIATION_COUNT];
static struct dect_net_l2_association_data parent_associations[1];

/**
 * @brief Check if association exists with target device
 *
 * Searches both child and parent association tables for existing connection
 * to specified DECT device by its long RD ID.
 *
 * @param target_long_rd_id DECT device long RD identifier
 * @return true if association exists, false otherwise
 */
static bool dect_net_l2_association_exists(uint32_t target_long_rd_id)
{
	for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
		if (child_associations[i].in_use &&
		    child_associations[i].target_long_rd_id == target_long_rd_id) {
			return true;
		}
	}
	for (int i = 0; i < ARRAY_SIZE(parent_associations); i++) {
		if (parent_associations[i].in_use &&
		    parent_associations[i].target_long_rd_id == target_long_rd_id) {
			return true;
		}
	}
	return false;
}

/**
 * @brief Get parent device long RD ID for routing
 *
 * Returns the long RD ID of parent device in DECT cluster topology.
 * Used for upstream packet routing in multi-hop networks.
 * FT devices have no parent (they are cluster coordinators).
 *
 * @param parent_long_rd_id_out Output parameter for parent ID
 * @param device_type Current device type (FT, PP, etc.)
 * @return true if parent exists, false if no parent (FT device)
 */
static bool dect_net_l2_association_parent_id_get(uint32_t *parent_long_rd_id_out,
						  dect_device_type_t device_type)
{
	if (device_type & DECT_DEVICE_TYPE_FT) {
		*parent_long_rd_id_out = 0;
		return false;
	}

	for (int i = 0; i < ARRAY_SIZE(parent_associations); i++) {
		if (parent_associations[i].in_use) {
			*parent_long_rd_id_out = parent_associations[i].target_long_rd_id;
			return true;
		}
	}
	return false;
}

static struct dect_net_l2_association_data *dect_net_l2_association_ref_get(
	uint32_t target_long_rd_id)
{
	for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
		if (child_associations[i].in_use &&
		    child_associations[i].target_long_rd_id == target_long_rd_id) {
			return &child_associations[i];
		}
	}
	for (int i = 0; i < ARRAY_SIZE(parent_associations); i++) {
		if (parent_associations[i].in_use &&
		    parent_associations[i].target_long_rd_id == target_long_rd_id) {
			return &parent_associations[i];
		}
	}
	return NULL;
}

static uint16_t dect_net_l2_association_count_get(void)
{
	uint16_t count = 0;

	for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
		if (child_associations[i].in_use) {
			count++;
		}
	}
	for (int i = 0; i < ARRAY_SIZE(parent_associations); i++) {
		if (parent_associations[i].in_use) {
			count++;
		}
	}
	return count;
}

/**************************************************************************************************/

/**
 * @brief Fill association data for status reporting
 *
 * Populates status information structure with current association details
 * including IPv6 addresses for both parent and child associations.
 * Used by management layer to report network topology status.
 *
 * @param iface Network interface
 * @param status_info_out Output structure to fill with association data
 */
void dect_net_l2_status_info_fill_association_data(
	struct net_if *iface, struct dect_status_info *status_info_out)
{
	if (status_info_out == NULL) {
		return;
	}
	struct dect_net_l2_association_data *ass_data = NULL;

	/* Fill parent data if available */
	for (int i = 0; i < status_info_out->parent_count; i++) {
		ass_data = dect_net_l2_association_ref_get(
			status_info_out->parent_associations[i].long_rd_id);
		if (ass_data) {
			status_info_out->parent_associations[i].local_ipv6_addr =
				ass_data->local_ipv6_addr;
			status_info_out->parent_associations[i].global_ipv6_addr_set =
				ass_data->global_ipv6_addr_set;
			if (ass_data->global_ipv6_addr_set) {
				status_info_out->parent_associations[i].global_ipv6_addr =
					ass_data->global_ipv6_addr;
			}
		}
	}
	/* Fill child data if available */
	for (int i = 0; i < status_info_out->child_count; i++) {
		ass_data = dect_net_l2_association_ref_get(
			status_info_out->child_associations[i].long_rd_id);
		if (ass_data) {
			status_info_out->child_associations[i].local_ipv6_addr =
				ass_data->local_ipv6_addr;
			status_info_out->child_associations[i].global_ipv6_addr_set =
				ass_data->global_ipv6_addr_set;
			if (ass_data->global_ipv6_addr_set) {
				status_info_out->child_associations[i].global_ipv6_addr =
					ass_data->global_ipv6_addr;
			}
		}
	}
}

/**
 * @brief Fill sink IPv6 prefix data for status reporting
 *
 * Populates status information structure with current sink IPv6 prefix configuration.
 * Used by management layer to report FT device's global prefix delegation status.
 * Compares L2 sink prefix with driver prefix and reports any mismatches.
 *
 * Key functions:
 * - Retrieves current L2 sink IPv6 prefix configuration
 * - Validates consistency between L2 and driver prefix settings
 * - Reports network interface and prefix length information
 * - Logs warnings if L2 and driver prefixes don't match
 *
 * @param iface Network interface
 * @param status_info_out Output structure to fill with sink prefix data
 */
void dect_net_l2_status_info_fill_sink_data(
	struct net_if *iface, struct dect_status_info *status_info_out)
{
	if (status_info_out == NULL) {
		return;
	}
	struct dect_net_l2_sink_ipv6_prefix l2_sink_prefix;
	struct in6_addr driver_ipv6_prefix = status_info_out->br_global_ipv6_addr_prefix;
	bool driver_ipv6_prefix_set = status_info_out->br_global_ipv6_addr_prefix_set;

	if (dect_net_l2_sink_ipv6_prefix_get(&l2_sink_prefix)) {
		struct in6_addr l2_sink_ipv6_prefix = l2_sink_prefix.prefix;

		/* Print warning if not the same as driver */
		if (!driver_ipv6_prefix_set ||
		    !net_ipv6_addr_cmp(&l2_sink_ipv6_prefix, &driver_ipv6_prefix)) {
			LOG_WRN("SINK: IPv6 prefix %s/%d does not match driver prefix %s/%d",
				net_sprint_ipv6_addr(&l2_sink_ipv6_prefix),
				(sizeof(struct in6_addr) / 2) * 8,
				net_sprint_ipv6_addr(&driver_ipv6_prefix),
				(sizeof(struct in6_addr) / 2) * 8);
		}
		/* Anyways, we report L2 sink information */
		status_info_out->br_net_iface = l2_sink_prefix.iface;
		status_info_out->br_global_ipv6_addr_prefix = l2_sink_ipv6_prefix;
		status_info_out->br_global_ipv6_addr_prefix_len = l2_sink_prefix.len;
		status_info_out->br_global_ipv6_addr_prefix_set = true;
	}
}

/**************************************************************************************************/

/**
 * @brief DECT L2 receive handler - processes incoming packets from network stack
 *
 * Called by Zephyr networking core when packets are received on DECT interface.
 * Runs in RX thread context (e.g. dect_nrf91_rx_th), not ISR context.
 *
 * Functions:
 * - IPv6 link-local routing: Routes packets directly to associated DECT devices
 * - Multicast forwarding: Forwards link-local multicast to all children
 * - DECT topology awareness: Uses RD IDs embedded in IPv6 addresses for routing
 *
 * @param iface Network interface (DECT interface)
 * @param pkt Received network packet (already parsed by network stack)
 * @return NET_OK if packet handled/forwarded, NET_CONTINUE to pass up stack
 */
static enum net_verdict dect_net_l2_recv(struct net_if *iface, struct net_pkt *pkt)
{
	LOG_DBG("iface %p recv %d bytes from ipv6 addr %s", iface, net_pkt_get_len(pkt),
		net_sprint_ipv6_addr((struct in6_addr *)NET_IPV6_HDR(pkt)->src));

	/* Check if this possible ipv6 packet is targeted to one of our children,
	 * then we route directly from here
	 * OR
	 * In case of multicast, we forward to all children (except the one who sent this)
	 */
	uint8_t vtc_vhl = NET_IPV6_HDR(pkt)->vtc & 0xf0;
	int ret;

	if (vtc_vhl != 0x60) {
		goto exit;
	}

	if (net_ipv6_is_ll_addr((struct in6_addr *)NET_IPV6_HDR(pkt)->dst)) {
		uint32_t target_long_rd_id = dect_utils_lib_long_rd_id_from_ipv6_addr(
			(struct in6_addr *)NET_IPV6_HDR(pkt)->dst);

		if (dect_net_l2_association_exists(target_long_rd_id)) {
			const struct dect_nr_hal_api *api = net_if_get_device(iface)->api;

			if (!api) {
				LOG_ERR("Link local: no api for iface %p", iface);
				goto exit;
			}

			net_pkt_set_family(pkt, AF_INET6);

			/* Set ll addr for driver level to know destination long rd id */
			target_long_rd_id = htonl(target_long_rd_id);
			net_pkt_lladdr_dst(pkt)->len = sizeof(target_long_rd_id);
			memcpy(net_pkt_lladdr_dst(pkt)->addr, &target_long_rd_id,
			       net_pkt_lladdr_dst(pkt)->len);

			ret = net_l2_send(api->send, net_if_get_device(iface), iface, pkt);
			if (!ret) {
				ret = net_pkt_get_len(pkt);
				LOG_DBG("%s (iface %p): forwarded to long rd id %u "
					"(%d bytes)",
					(__func__), iface, ntohl(target_long_rd_id), ret);
			} else {
				LOG_ERR("%s: iface %p forwarding send error %d to "
					"long rd id %u",
					(__func__), iface, ret, ntohl(target_long_rd_id));
			}
			net_pkt_unref(pkt);

			return NET_OK; /* We handled this one... */
		}
	} else if (net_ipv6_is_addr_mcast_link((struct in6_addr *)NET_IPV6_HDR(pkt)->dst)) {
		/* link local scope multicast address (FFx2::) -> forward to all children */
		uint32_t source_long_rd_id = dect_utils_lib_long_rd_id_from_ipv6_addr(
			(struct in6_addr *)NET_IPV6_HDR(pkt)->src);

		LOG_DBG("%s: IPv6 multicast packet to link local scope address", (__func__));

		/* Forward to all children */
		for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
			if (child_associations[i].in_use &&
			    child_associations[i].target_long_rd_id != source_long_rd_id) {
				const struct dect_nr_hal_api *api = net_if_get_device(iface)->api;

				if (!api) {
					LOG_ERR("Multicast link local: no api for iface %p", iface);
					goto exit;
				}

				struct net_pkt *pkt_cpy = net_pkt_clone(pkt, K_NO_WAIT);
				uint32_t target_long_rd_id =
					child_associations[i].target_long_rd_id;

				if (pkt_cpy == NULL) {
					LOG_ERR("Cannot clone pkt");
					continue;
				}

				net_pkt_set_forwarding(pkt_cpy, true);
				net_pkt_set_orig_iface(pkt_cpy, pkt->iface);
				net_pkt_set_family(pkt_cpy, AF_INET6);
				net_pkt_set_iface(pkt_cpy, iface);

				/* Set ll addr for driver level to know destination long rd
				 * id
				 */
				target_long_rd_id = htonl(target_long_rd_id);
				net_pkt_lladdr_dst(pkt_cpy)->len = sizeof(target_long_rd_id);
				memcpy(net_pkt_lladdr_dst(pkt_cpy)->addr, &target_long_rd_id,
				       net_pkt_lladdr_dst(pkt_cpy)->len);

				ret = net_l2_send(api->send, net_if_get_device(iface), iface,
						  pkt_cpy);
				if (ret) {
					LOG_ERR("%s: iface %p send error %d for multicast "
						"forward",
						(__func__), iface, ret);
					ret = net_pkt_get_len(pkt_cpy);
					net_pkt_unref(pkt_cpy);
				} else {
					ret = net_pkt_get_len(pkt_cpy);
					net_pkt_unref(pkt_cpy);
					LOG_DBG("%s (iface %p): multicast forwarded to "
						"long rd id %u (%d bytes)",
						(__func__), iface, ntohl(target_long_rd_id), ret);
				}
			}
		}

		/* ... and pass also to us */
	}
exit:
	return NET_CONTINUE;
}

/**
 * @brief DECT L2 send handler - transmits packets through DECT driver
 *
 * Called by Zephyr networking core when applications send packets on DECT interface.
 * Extracts target RD ID from IPv6 destination address and forwards to MAC layer.
 *
 * Process:
 * 1. Extract target long RD ID from IPv6 destination address
 * 2. Look up association information for routing
 * 3. Call DECT MAC driver send function with target ID
 * 4. Handle transmission errors and cleanup
 *
 * @param iface Network interface (DECT interface)
 * @param pkt Network packet to transmit
 * @return 0 on success, negative error code on failure
 */
static int dect_net_l2_send(struct net_if *iface, struct net_pkt *pkt)
{
	const struct dect_nr_hal_api *api = net_if_get_device(iface)->api;

	int ret = -1;
	uint32_t target_long_rd_id = 0;

	if (!api) {
		ret = -ENOENT;
		goto error;
	}

	if (!api->send) {
		ret = -ENOTSUP;
		goto error;
	}

	struct net_context *context;
	struct dect_net_l2_context *l2_ctx = net_if_l2_data(iface);
	dect_device_type_t device_type = l2_ctx->device_type;

	if (IS_ENABLED(CONFIG_NET_IPV6) && net_pkt_family(pkt) == AF_INET6) {
		uint32_t parent_long_rd_id = 0;

		target_long_rd_id = dect_utils_lib_long_rd_id_from_ipv6_addr(
			(struct in6_addr *)NET_IPV6_HDR(pkt)->dst);

		if (!dect_net_l2_association_exists(target_long_rd_id)) {
			if (!dect_net_l2_association_parent_id_get(&parent_long_rd_id,
								   device_type)) {
				if (!((device_type & DECT_DEVICE_TYPE_FT) &&
				    net_ipv6_is_addr_mcast_link(
					    (struct in6_addr *)NET_IPV6_HDR(pkt)->dst))) {
					LOG_WRN("%s: IPv6: no parent and no association with "
						"target long RD ID %u (parsed from ipv dst addr "
						"%s) - drop",
						(__func__), target_long_rd_id,
						net_sprint_ipv6_addr(
							(struct in6_addr *)NET_IPV6_HDR(pkt)->dst));
					ret = -EINVAL;
					goto error;
				}

				/* FT device: forward multicasts to all children */
				LOG_DBG("%s: FT device: forward multicast to all children",
					(__func__));
				for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
					if (child_associations[i].in_use == false) {
						continue;
					}
					struct net_pkt *pkt_cpy =
						net_pkt_clone(pkt, K_NO_WAIT);
					uint32_t target_long_rd_id =
						child_associations[i]
							.target_long_rd_id;
					if (pkt_cpy == NULL) {
						LOG_ERR("Cannot clone pkt");
						continue;
					}
					net_pkt_set_forwarding(pkt_cpy, true);
					net_pkt_set_orig_iface(pkt_cpy, pkt->iface);
					net_pkt_set_family(pkt_cpy, AF_INET6);
					net_pkt_set_iface(pkt_cpy, iface);
					/* Set ll addr for driver level to know
					 * destination long rd id
					 */
					target_long_rd_id =
						htonl(target_long_rd_id);
					net_pkt_lladdr_dst(pkt_cpy)->len =
						sizeof(target_long_rd_id);
					memcpy(net_pkt_lladdr_dst(pkt_cpy)->addr,
					       &target_long_rd_id,
					       net_pkt_lladdr_dst(pkt_cpy)->len);
					ret = net_l2_send(api->send,
							  net_if_get_device(iface),
							  iface, pkt_cpy);
					if (ret) {
						LOG_ERR("%s: iface %p send error "
							"%d for multicast "
							"forward",
							(__func__), iface, ret);
						net_pkt_unref(pkt_cpy);
					} else {
						ret = net_pkt_get_len(pkt_cpy);
						net_pkt_unref(pkt_cpy);
						LOG_DBG("%s (iface %p): multicast "
							"forwarded to "
							"long rd id %u (%d bytes)",
							(__func__), iface,
							ntohl(target_long_rd_id),
							ret);
					}
				}
				if (ret < 0) {
					LOG_WRN("%s: IPv6: multicast: no parent and no "
						"association with "
						"target long RD ID %u (parsed from ipv dst "
						"addr %s) - drop",
						(__func__), target_long_rd_id,
						net_sprint_ipv6_addr(
							(struct in6_addr *)NET_IPV6_HDR(pkt)
								->dst));
					ret = -EINVAL;
					goto error;
				}
				net_pkt_unref(pkt);
				return ret;
			}
			/* else: use parent long RD ID */
			target_long_rd_id = parent_long_rd_id;
		}
		LOG_DBG("IPv6 send: target_long_rd_id %u to ipv6 addr %s", target_long_rd_id,
			net_sprint_ipv6_addr((struct in6_addr *)NET_IPV6_HDR(pkt)->dst));

		/* Set ll addr for driver level to know destination long rd id */
		target_long_rd_id = htonl(target_long_rd_id);
		net_pkt_lladdr_dst(pkt)->len = sizeof(target_long_rd_id);
		memcpy(net_pkt_lladdr_dst(pkt)->addr, &target_long_rd_id,
		       net_pkt_lladdr_dst(pkt)->len);

		goto send;
	} else if (IS_ENABLED(CONFIG_NET_SOCKETS_PACKET) && net_pkt_family(pkt) == AF_PACKET) {
		enum net_sock_type socket_type;

		context = net_pkt_context(pkt);
		if (!context) {
			LOG_DBG("AF_PACKET: No context in the packet");
			ret = -EINVAL;
			goto error;
		}

		socket_type = net_context_get_type(context);
		if (socket_type == SOCK_DGRAM) {
			struct sockaddr_ll *dst_addr = (struct sockaddr_ll *)&context->remote;

			target_long_rd_id =
				dect_utils_lib_dst_long_rd_id_get_from_dst_sock_ll_addr(
					dst_addr);
			if (dect_net_l2_association_exists(target_long_rd_id)) {
				struct sockaddr_ll_ptr *src_addr =
					(struct sockaddr_ll_ptr *)&context->local;

				/* Set destination to pkt for the driver level */
				__ASSERT_NO_MSG(dst_addr->sll_halen <=
						sizeof(net_pkt_lladdr_dst(pkt)->addr));

				memcpy(net_pkt_lladdr_dst(pkt)->addr, dst_addr->sll_addr,
				       dst_addr->sll_halen);
				net_pkt_lladdr_dst(pkt)->len = dst_addr->sll_halen;

				__ASSERT_NO_MSG(src_addr->sll_halen <=
						sizeof(net_pkt_lladdr_src(pkt)->addr));
				memcpy(net_pkt_lladdr_src(pkt)->addr, src_addr->sll_addr,
				       src_addr->sll_halen);
				net_pkt_lladdr_src(pkt)->len = src_addr->sll_halen;

				goto send;
			} else {
				LOG_WRN("No association with target %u - drop", target_long_rd_id);
				ret = -EINVAL;
				goto error;
			}

			/* Send the packet as it is */
			goto send;
		} else {
			/* Others not supported */
			LOG_ERR("Socket type %d not supported", socket_type);
			ret = -EINVAL;
			goto error;
		}
	} else {
		/* Others not yet supported */
		LOG_ERR("Packet type %d not supported", net_pkt_family(pkt));
		ret = -EINVAL;
		goto error;
	}
send:
	ret = net_l2_send(api->send, net_if_get_device(iface), iface, pkt);
	if (ret) {
		goto error;
	}
	ret = net_pkt_get_len(pkt);
	net_pkt_unref(pkt);

	LOG_DBG("iface %p sent %d bytes (caller %p)", iface, ret, __builtin_return_address(0));

	return ret;
error:
	LOG_ERR("%s: iface %p send error %d", (__func__), iface, ret);
	return ret;
}

static int dect_net_l2_enable(struct net_if *iface, bool state)
{
	LOG_DBG("%s: iface %p %s", (__func__), iface, state ? "up" : "down");

	return 0;
}

/**
 * @brief Get DECT L2 interface flags
 *
 * Returns current interface state flags (UP/DOWN) for Zephyr networking core.
 * Used by network stack to determine interface capabilities and status.
 *
 * @param iface Network interface
 * @return Current interface flags (NET_L2_MULTICAST typically)
 */
static enum net_l2_flags dect_net_l2_flags(struct net_if *iface)
{
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);

	return ctx->flags;
}

/**
 * @brief DECT L2 layer registration with Zephyr networking stack
 *
 * Registers DECT as a custom L2 protocol with Zephyr networking core.
 */
NET_L2_INIT(DECT_L2, dect_net_l2_recv, dect_net_l2_send, dect_net_l2_enable, dect_net_l2_flags);

/**
 * @brief Initialize DECT L2 interface with initial configuration
 *
 * Called by DECT NR+ device driver during interface setup to configure L2 networking layer.
 * Sets up IPv6 addressing, device identity, and initial network parameters.
 *
 * Key initialization:
 * - IPv6-only operation with no neighbor discovery (direct DECT routing)
 * - Link-local address generation from MAC address
 * - Network ID and transmitter ID from initial settings
 * - Interface marked dormant until association established
 *
 * @param iface Network interface to initialize
 * @param initial_settings DECT configuration (network ID, device type, etc.)
 */
void dect_net_l2_init(struct net_if *iface, struct dect_settings *initial_settings)
{
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);
	struct net_if_addr *ifaddr;
	struct in6_addr iid;

	LOG_DBG("Initializing DECT L2 %p for iface %d (%p)", ctx, net_if_get_by_iface(iface),
		iface);

	net_if_flag_set(iface, NET_IF_IPV6);
	net_if_flag_set(iface, NET_IF_IPV6_NO_ND);
	net_if_dormant_on(iface);
	ctx->network_id = initial_settings->identities.network_id;
	ctx->transmitter_long_rd_id = initial_settings->identities.transmitter_long_rd_id;
	ctx->device_type = initial_settings->device_type;
	ctx->ipv6_prefix_cfg.prefix_len = 0; /* No prefix by default */

	/* Add link-local address */
	dect_utils_lib_net_ipv6_addr_create_iid(&iid, net_if_get_link_addr(iface));
	ifaddr = net_if_ipv6_addr_add(iface, &iid, NET_ADDR_AUTOCONF, 0);
	if (!ifaddr) {
		LOG_ERR("%s: cannot add address to interface %p", (__func__), iface);
	} else {
		/* As DAD is disabled in dect net iface,
		 * we need to mark the address as a preferred one.
		 */
		ifaddr->addr_state = NET_ADDR_PREFERRED;
		ctx->local_ipv6_addr = iid;
	}
}

static void dect_net_l2_join_ipv6_mdns_group(struct net_if *iface)
{
	struct in6_addr ipv6mr_multiaddr;
	int ret;

	/* Well known IPv6 ff02::fb address */
	net_ipv6_addr_create(&ipv6mr_multiaddr, 0xff02, 0, 0, 0, 0, 0, 0, 0x00fb);

	ret = net_ipv6_mld_join(iface, &ipv6mr_multiaddr);
	if (ret < 0) {
		LOG_DBG("Iface %p, cannot join mDNS (ff02::fb) IPv6 multicast group (%d)", iface,
			ret);
	} else {
		LOG_DBG("Iface %p, joined mDNS (ff02::fb) IPv6 multicast group", iface);
	}
}

/**************************************************************************************************/

/**
 * @brief Handle parent association creation in DECT NR+ cluster
 *
 * Called by DECT MAC/driver when association with parent device is established.
 * Sets up routing, IPv6 addressing, and activates network interface.
 *
 * Key actions:
 * - Add parent to association table
 * - Configure IPv6 addressing with parent-provided prefix
 * - Bring interface up (exit dormant state)
 * - Generate management event for application layer
 *
 * @param iface Network interface
 * @param target_long_rd_id Parent device long RD identifier
 * @param ipv6_prefix_config IPv6 prefix configuration from parent
 */
void dect_net_l2_parent_association_created(
	struct net_if *iface, uint32_t target_long_rd_id,
	struct dect_net_ipv6_prefix_config *ipv6_prefix_config)
{
	bool found = false;

	LOG_DBG("dect_net_l2_parent_association_created: iface %p target_long_rd_id %u", iface,
		target_long_rd_id);

	for (int i = 0; i < ARRAY_SIZE(parent_associations); i++) {
		if (!parent_associations[i].in_use) {
			parent_associations[i].in_use = true;
			parent_associations[i].target_long_rd_id = target_long_rd_id;
			dect_net_l2_ipv6_addressing_parent_added_handle(
				&parent_associations[i], iface, target_long_rd_id,
				ipv6_prefix_config);
			dect_mgmt_parent_association_created_evt(iface, target_long_rd_id);

			/* We support only one parent */
			__ASSERT_NO_MSG(dect_net_l2_association_count_get() == 1);

			net_if_dormant_off(iface);

			dect_net_l2_join_ipv6_mdns_group(iface);

			found = true;
			break;
		}
	}
	__ASSERT_NO_MSG(found);
}

/**
 * @brief Handle child association creation in DECT NR+ cluster
 *
 * Called by DECT MAC/driver when child device associates with this device.
 * Adds child to routing table and manages network interface state.
 *
 * Key actions:
 * - Add child to association table
 * - Configure IPv6 addressing for child device
 * - Activate interface if this is first child (for FT devices)
 * - Generate management event for application layer
 *
 * @param iface Network interface
 * @param child_long_rd_id Child device long RD identifier
 */
void dect_net_l2_child_association_created(struct net_if *iface, uint32_t child_long_rd_id)
{
	bool first_child = false;

	LOG_DBG("dect_net_l2_child_association_created: iface %p child_long_rd_id %u", iface,
		child_long_rd_id);

	for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
		if (!child_associations[i].in_use) {
			child_associations[i].in_use = true;
			child_associations[i].target_long_rd_id = child_long_rd_id;

			first_child = (dect_net_l2_association_count_get() == 1);
			dect_net_l2_ipv6_addressing_child_added_handle(
				&child_associations[i], iface, child_long_rd_id, first_child);
			dect_mgmt_child_association_created_evt(iface, child_long_rd_id);

			/* Put the carrier on if this was the first association */
			if (first_child) {
				net_if_dormant_off(iface);
				dect_net_l2_join_ipv6_mdns_group(iface);
			}
			return;
		}
	}
}

static bool dect_net_l2_no_associations(void)
{
	for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
		if (child_associations[i].in_use) {
			return false;
		}
	}
	for (int i = 0; i < ARRAY_SIZE(parent_associations); i++) {
		if (parent_associations[i].in_use) {
			return false;
		}
	}
	return true;
}

/**
 * @brief Handle association removal from DECT NR+ cluster
 *
 * Called by DECT MAC/driver when DECT association is terminated.
 * Cleans up routing tables, removes IPv6 addresses, and handles topology changes.
 *
 * Actions:
 * - Remove association from child/parent tables
 * - Clean up IPv6 addressing for removed device
 * - Update routing table entries
 * - Handle interface state changes if last association
 *
 * @param iface Network interface
 * @param long_rd_id DECT device long RD identifier being removed
 * @param cause Reason for association release
 * @param neighbor_initiated true if remote device initiated release
 */
void dect_net_l2_association_removed(
	struct net_if *iface, uint32_t long_rd_id,
	enum dect_association_release_cause cause, bool neighbor_initiated)
{
	LOG_DBG("dect_net_l2_association_removed: iface %p, "
		"long_rd_id %u, release_cause %d", iface, long_rd_id, cause);

	for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
		if (child_associations[i].in_use &&
		    child_associations[i].target_long_rd_id == long_rd_id) {
			child_associations[i].in_use = false;
			dect_net_l2_ipv6_addressing_child_removed_handle(
				&child_associations[i], iface, long_rd_id);
			dect_mgmt_association_released_evt(
				iface, long_rd_id, DECT_NEIGHBOR_ROLE_CHILD,
				neighbor_initiated, cause);

			/* If this was the last association, drop the carrier */
			if (dect_net_l2_no_associations()) {
				net_if_dormant_on(iface);
			}
			return;
		}
	}
	for (int i = 0; i < ARRAY_SIZE(parent_associations); i++) {
		if (parent_associations[i].in_use &&
		    parent_associations[i].target_long_rd_id == long_rd_id) {
			parent_associations[i].in_use = false;

			dect_net_l2_ipv6_parent_addressing_removed_handle(
				&parent_associations[i], iface, long_rd_id);
			dect_mgmt_association_released_evt(
				iface, long_rd_id, DECT_NEIGHBOR_ROLE_PARENT,
				neighbor_initiated, cause);

			/* If this was the last association, drop the carrier */
			if (dect_net_l2_no_associations()) {
				net_if_dormant_on(iface);
			}
			return;
		}
	}
}

/**
 * @brief Handle DECT NR+ settings changes from device driver / MAC layer
 *
 * Called by DECT MAC/driver when device settings are updated (network ID, device type, etc.).
 * Updates L2 context with new settings that affect routing and addressing behavior.
 * IPv6 addressing updates are deferred until next association creation.
 *
 * Typically called when:
 * - Device type changes (FT ↔ PP transitions)
 * - Network ID reconfiguration
 * - Transmitter ID changes
 *
 * @param iface Network interface
 * @param driver_current_settings Updated DECT settings from MAC layer
 */
void dect_net_l2_settings_changed(
	struct net_if *iface, struct dect_settings *driver_current_settings)
{
	struct dect_net_l2_context *ctx = net_if_l2_data(iface);

	/* Update needed settings in our side to context.
	 * Addressing are updated on next time when 1st association is created.
	 */
	ctx->network_id = driver_current_settings->identities.network_id;
	ctx->transmitter_long_rd_id = driver_current_settings->identities.transmitter_long_rd_id;
	ctx->device_type = driver_current_settings->device_type;
}

/**
 * @brief Handle parent IPv6 configuration changes
 *
 * Called by DECT MAC/driver when parent device updates IPv6 prefix configuration.
 * Triggers IPv6 address reconfiguration for this device and propagates
 * changes to child devices in the DECT cluster topology.
 *
 * Common triggers:
 * - Parent device receives new global prefix from upstream
 * - Parent changes its prefix delegation policy
 * - Network topology changes affecting addressing
 *
 * @param iface Network interface
 * @param parent_long_rd_id Parent device long RD identifier
 * @param ipv6_prefix_config New IPv6 prefix configuration from parent
 */
void dect_net_l2_parent_ipv6_config_changed(
	struct net_if *iface, uint32_t parent_long_rd_id,
	struct dect_net_ipv6_prefix_config *ipv6_prefix_config)
{
	struct dect_net_l2_association_data *assoc_data =
		dect_net_l2_association_ref_get(parent_long_rd_id);

	if (assoc_data) {
		dect_net_l2_ipv6_addressing_parent_changed_handle(
			assoc_data, iface, parent_long_rd_id,
			ipv6_prefix_config);
	}
}

/**
 * @brief Handle sink IPv6 configuration changes
 *
 * Called by L2 sink module when this device (acting as FT/sink) receives new IPv6 prefix
 * configuration from external network. Updates own addressing and propagates
 * changes to all child devices in the DECT cluster.
 *
 * Process:
 * 1. Update own IPv6 addressing with new prefix
 * 2. Remove old global addresses from all children
 * 3. Add new global addresses to all associated children
 * 4. Ensure proper neighbor discovery for updated addresses
 *
 * Typically occurs when:
 * - FT device receives new prefix from network infrastructure
 * - Global prefix delegation changes
 * - Network reconnection with different addressing
 *
 * @param iface Network interface
 * @param ipv6_prefix_config New IPv6 prefix configuration for sink
 */
void dect_net_l2_sink_ipv6_config_changed(
	struct net_if *iface, struct dect_net_ipv6_prefix_config *ipv6_prefix_config)
{
	struct dect_net_l2_context *l2_ctx = net_if_l2_data(iface);
	bool update_global = false;

	__ASSERT_NO_MSG(iface);
	__ASSERT_NO_MSG(ipv6_prefix_config);
	__ASSERT_NO_MSG(l2_ctx);

	/* Update our addressing */
	update_global = dect_net_l2_ipv6_addressing_sink_changed_handle(
		iface, ipv6_prefix_config);

	/* Update children, by first removing all global nbrs */
	for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
		if (child_associations[i].in_use) {
			dect_net_l2_ipv6_global_addressing_child_removed_handle(
				&child_associations[i], iface);
		}
	}

	/* Then update / add global neighbors */
	if (update_global) {
		for (int i = 0; i < ARRAY_SIZE(child_associations); i++) {
			if (child_associations[i].in_use) {
				dect_net_l2_ipv6_global_addressing_child_changed_handle(
					l2_ctx, &child_associations[i], iface);
			}
		}
	}

}
