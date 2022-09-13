#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_UART_HOST dfu_target_uart_host.c)

zephyr_include_directories(${NRF_DIR}/subsys/dfu/include)
add_subdirectory(${NRF_DIR}/subsys/dfu/dfu_target/src/dfu_target_uart_cddl dfu_uart_cbor)
