# Copyright (c) 2026 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

include(${ZEPHYR_BASE}/boards/common/nrfutil.board.cmake)

if(CONFIG_SOC_NRF9251_CPUAPP)
  set(jlink_script_file ${CMAKE_CURRENT_LIST_DIR}/support/nrf9251_cpuapp.JLinkScript)
  board_runner_args(jlink "--device=CORTEX-M33" "--speed=4000" "--tool-opt=-jlinkscriptfile ${jlink_script_file}")
elseif(CONFIG_SOC_NRF9251_CPUPPR)
  set(jlink_script_file ${CMAKE_CURRENT_LIST_DIR}/support/nrf9251_cpuppr.JLinkScript)
  # Workaround: Use device nRF54L15_RV32 until something else is defined.
  board_runner_args(jlink "--device=nRF54L15_RV32" "--speed=4000" "--tool-opt=-jlinkscriptfile ${jlink_script_file}")
elseif(CONFIG_SOC_NRF9251_CPUFLPR)
  set(jlink_script_file ${CMAKE_CURRENT_LIST_DIR}/support/nrf9251_cpuflpr.JLinkScript)
  # Workaround: Use device nRF54L15_RV32 until something else is defined.
  board_runner_args(jlink "--device=nRF54L15_RV32" "--speed=4000" "--tool-opt=-jlinkscriptfile ${jlink_script_file}")
endif()

include(${ZEPHYR_BASE}/boards/common/jlink.board.cmake)
