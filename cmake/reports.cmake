#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

add_custom_target(
  partition_manager_report
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/../nrf/scripts/partition_manager_report.py
  --input ${CMAKE_BINARY_DIR}/partitions.yml
  "$<$<NOT:$<TARGET_EXISTS:partition_manager>>:--quiet>"
  )

set_property(TARGET zephyr_property_target
             APPEND PROPERTY rom_report_DEPENDENCIES
             partition_manager_report
	     )
