#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(coremark)

zephyr_include_directories(${ZEPHYR_COREMARK_MODULE_DIR} .)

zephyr_library_sources(${ZEPHYR_COREMARK_MODULE_DIR}/core_main.c
			${ZEPHYR_COREMARK_MODULE_DIR}/core_matrix.c
			${ZEPHYR_COREMARK_MODULE_DIR}/core_state.c
			${ZEPHYR_COREMARK_MODULE_DIR}/core_util.c
			${ZEPHYR_COREMARK_MODULE_DIR}/core_list_join.c
			core_portme.c)
