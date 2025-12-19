#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# DECT MAC library directory - container for DECT MAC libraries
# Currently contains utils and l2_shell libraries

# Include utils library
add_subdirectory_ifdef(CONFIG_DECT_UTILS_LIB utils)

# Include l2_shell library
add_subdirectory_ifdef(CONFIG_DECT_L2_SHELL_LIB l2_shell)
