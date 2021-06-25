#
# Copyright (c) 2021 Georgi Valkov
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

target_sources(
	app PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/os/int.c
	${CMAKE_CURRENT_SOURCE_DIR}/os/int.s
	${CMAKE_CURRENT_SOURCE_DIR}/os/os_zephyr.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/generic_uart.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/nrf52840_uart.c
	${CMAKE_CURRENT_SOURCE_DIR}/src/nrf52840_uart_dma.c
)

# add this to the top level project
# include_directories(src/uart/include)
