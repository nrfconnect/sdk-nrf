/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MOSH_NET_UTILS_H
#define MOSH_NET_UTILS_H

char *net_utils_sckt_addr_ntop(const struct sockaddr *addr);
int net_utils_sa_family_from_ip_string(const char *src);
int net_utils_socket_pdn_id_set(int fd, uint32_t pdn_id);
bool net_utils_ip_string_is_valid(const char *src);

#endif /* MOSH_NET_UTILS_H */
