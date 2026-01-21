#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Adding a module to b0 is needed as a way to inject source code into
# the non-default sysbuild image (in this case b0).
set(b0_EXTRA_ZEPHYR_MODULES ${CMAKE_CURRENT_LIST_DIR}/modules/b0_cleanup_ram_test_prepare  CACHE INTERNAL "")
