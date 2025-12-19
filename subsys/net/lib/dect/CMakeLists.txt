#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# DECT MAC library directory - container for DECT MAC libraries
# Currently contains utils and l2_shell libraries

# Include utils library
add_subdirectory_ifdef(CONFIG_DECT_UTILS_LIB utils)
