#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This code allows to install additional code from the test_mcuboot_watchdog module to MCUboot.
# This includes an INIT level function that starts the watchdog with parameters configurable
# via Kconfig as well as a hook function that adds a delay allowing the watchdog to fire in case it
# is not fed at the start of the boot process.
set(mcuboot_EXTRA_ZEPHYR_MODULES ${CMAKE_CURRENT_LIST_DIR}/modules/test_mcuboot_watchdog CACHE INTERNAL "")
