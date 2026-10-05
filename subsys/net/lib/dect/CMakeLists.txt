#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# DECT MAC library directory - container for DECT MAC libraries
# Currently contains utils, l2_shell, tether_ipv6, and icmp ping libraries

# Include utils library
add_subdirectory_ifdef(CONFIG_DECT_UTILS_LIB utils)

# Include l2_shell library
add_subdirectory_ifdef(CONFIG_DECT_L2_SHELL_LIB l2_shell)

add_subdirectory_ifdef(CONFIG_DECT_TETHER_IPV6_LIB tether_ipv6)

add_subdirectory_ifdef(CONFIG_DECT_ICMP_PING_LIB ping)
