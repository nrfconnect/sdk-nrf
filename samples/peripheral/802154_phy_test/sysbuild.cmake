#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

get_property(PM_DOMAINS GLOBAL PROPERTY PM_DOMAINS)

zephyr_get(EXTRA_ZEPHYR_MODULES)
set(remote_shell_EXTRA_ZEPHYR_MODULES "${EXTRA_ZEPHYR_MODULES};${CMAKE_CURRENT_LIST_DIR}/modules/app_rpc" CACHE INTERNAL "extra modules directories")
