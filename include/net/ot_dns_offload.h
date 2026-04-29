/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef OT_DNS_OFFLOAD_H_
#define OT_DNS_OFFLOAD_H_

/**
 * @file
 * @brief Zephyr DNS offload using OpenThread DNS client.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register Zephyr DNS offload using OpenThread DNS client.
 *
 * Installs a Zephyr @c socket_dns_offload implementation via
 * @c socket_offload_dns_register whose @c getaddrinfo handler resolves
 * @p node using @c otDnsClientResolveIp4Address first (IPv4 A records as
 * NAT64-mapped IPv6 when supported), then @c otDnsClientResolveAddress
 * (AAAA). Successful lookups return @c NET_AF_INET6 sockaddr entries only.
 *
 * @retval 0         Registration succeeded.
 * @retval -EALREADY A handler was already registered.
 *
 * @note Requires @kconfig{CONFIG_OPENTHREAD_ZEPHYR_DNS_OFFLOAD}.
 */
int ot_dns_offload_register(void);

#ifdef __cplusplus
}
#endif

#endif /* OT_DNS_OFFLOAD_H_ */
