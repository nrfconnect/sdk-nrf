#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(mock_nrf_modem_at.c)
# Manually handled mock.
set(mock_function_scanf nrf_modem_at_scanf)
set(mock_function_printf nrf_modem_at_printf)
zephyr_library_link_libraries("-Wl,--defsym,${mock_function_scanf}=__cmock_${mock_function_scanf}")
zephyr_library_link_libraries("-Wl,--defsym,${mock_function_printf}=__cmock_${mock_function_printf}")
