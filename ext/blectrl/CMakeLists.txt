#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: BSD-5-Clause-Nordic
#

zephyr_include_directories(.)

add_library(blectlr STATIC IMPORTED GLOBAL)

if(CONFIG_SOC_NRF52840)
set_target_properties(blectlr PROPERTIES IMPORTED_LOCATION
	${NRF_DIR}/ext/blectrl/libblectlr_s140_nrf52.a)
elseif(CONFIG_SOC_NRF52832)
set_target_properties(blectlr PROPERTIES IMPORTED_LOCATION
	${NRF_DIR}/ext/blectrl/libblectlr_s132_nrf52.a)
elseif(CONFIG_SOC_NRF52810)
set_target_properties(blectlr PROPERTIES IMPORTED_LOCATION
	${NRF_DIR}/ext/blectrl/libblectlr_s112_nrf52.a)
endif()

zephyr_link_libraries(blectlr)
