/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef DECT_ICMP_PING_H__
#define DECT_ICMP_PING_H__

/**
 * @file dect_icmp_ping.h
 * @brief DECT ICMPv6 ping shell library (registers the ``ping`` shell command).
 */

#include <net/dect/dect_net_lib_shell.h>

#ifdef __cplusplus
extern "C" {
#endif

struct k_poll_signal;

/**
 * @defgroup dect_icmp_ping DECT ICMPv6 ping library
 * @brief IPv6 ICMP echo shell command for DECT NR+ interfaces (Zephyr @c net_icmp).
 *
 * @details Enable with @kconfig{CONFIG_DECT_ICMP_PING_LIB}. The library registers the
 * top-level ``ping`` shell command (hostname resolution, count, interval, timeout, payload,
 * RTT statistics). Default interface name is set by
 * @kconfig{CONFIG_DECT_ICMP_PING_DEFAULT_IFACE} when ``-I`` is not used.
 *
 * Applications that need timestamped output or a global abort (for example
 * :ref:`dect_shell`) call @ref dect_icmp_ping_init() and @ref dect_icmp_ping_set_abort_signal()
 * at startup; otherwise the shell uses normal @c shell_print / @c shell_error output.
 *
 * @{
 */

/**
 * @brief Initialize the ping library with optional custom print functions.
 *
 * If not called, ping uses shell_print/error/warn on the active shell session.
 *
 * @param print_fns Print callbacks, or NULL to use shell defaults only.
 *
 * @return 0 on success.
 */
int dect_icmp_ping_init(const struct dect_net_lib_shell_print_fns *print_fns);

/**
 * @brief Optional abort signal (for example, sample-wide kill on button press).
 *
 * @param signal Poll signal raised to stop an in-progress ping, or NULL to disable abort.
 */
void dect_icmp_ping_set_abort_signal(struct k_poll_signal *signal);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DECT_ICMP_PING_H__ */
