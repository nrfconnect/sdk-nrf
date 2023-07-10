/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @brief Add IPv4 address for the default PDP context to the network interface.
 *
 *  @param iface Pointer to network interface.
 *
 *  @retval 0 on success.
 *  @retval -EINVAL if the passed in network interface pointer was NULL.
 *  @retval -ENODATA if no IPv4 address was obtained from the modem.
 *  @retval -EFAULT if an error occurred when obtaining the IPv4 address.
 *  @retval -ENODEV if the passed in interface pointer was not valid.
 */
int lte_ipv4_addr_add(const struct net_if *iface);

/** @brief Add IPv6 address for the default PDP context to the network interface.
 *
 *  @param iface Pointer to network interface.
 *
 *  @retval 0 on success.
 *  @retval -EINVAL if the passed in network interface pointer was NULL.
 *  @retval -ENODATA if no IPv6 address was obtained from the modem.
 *  @retval -EFAULT if an error occurred when obtaining the IPv6 address.
 *  @retval -ENODEV if the passed in interface pointer was not valid.
 */
int lte_ipv6_addr_add(const struct net_if *iface);

/** @brief Remove IPv4 address that was previously associated with the default PDP context
 *         from the network interface.
 *
 *  @param iface Pointer to network interface.
 *
 *  @retval 0 on success or there was no IPv4 address to remove.
 *  @retval -EINVAL if the passed in network interface pointer was NULL.
 *  @retval -EFAULT if the IPv4 address failed to be removed.
 */
int lte_ipv4_addr_remove(const struct net_if *iface);

/** @brief Remove IPv6 address that was previously associated with the default PDP context
 *         from the network interface.
 *
 *  @param iface Pointer to network interface.
 *
 *  @retval 0 on success or there was no IPv6 address to remove.
 *  @retval -EINVAL if the passed in network interface pointer was NULL.
 *  @retval -EFAULT if the IPv6 address failed to be removed.
 */
int lte_ipv6_addr_remove(const struct net_if *iface);
