#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Attach this subtree to the upstream bluetooth drivers library when it exists,
# preserving the same target and compilation order as Zephyr's own sources.
# If there is no such target, create the library out-of-tree.
if(TARGET drivers__bluetooth)
    zephyr_library_amend(${ZEPHYR_BASE}/drivers/bluetooth)
else()
    # Note: When the target is created here, we won’t automatically inherit any in-tree properties (include dirs, options)
    # Zephyr would have set on it. We need to add whatever we need explicitly in our module.
    zephyr_library_named(drivers__bluetooth)
endif()
add_subdirectory_ifdef(CONFIG_BT_DRIVERS hci)
