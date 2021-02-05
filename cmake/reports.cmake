#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(pm_config
  "$<$<TARGET_EXISTS:partition_manager>:$<TARGET_PROPERTY:partition_manager,PM_CONFIG_FILES>>")

set(pm_depends
  "$<$<TARGET_EXISTS:partition_manager>:$<TARGET_PROPERTY:partition_manager,PM_DEPENDS>>")

# The script executed by this command will exit immediately if the
# '--quiet' argument is provided. This way, it is valid to pass empty
# values for the other arguments. We need this hack since the partition
# manager target is created at the end of the configure stage, so we can
# not know at which point whether or not it exist.
add_custom_target(
  partition_manager_report
  COMMAND
  ${PYTHON_EXECUTABLE}
  ${ZEPHYR_BASE}/../nrf/scripts/partition_manager_report.py
  --input ${pm_config}
  "$<$<NOT:$<TARGET_EXISTS:partition_manager>>:--quiet>"
  DEPENDS
  ${pm_depends}
  COMMAND_EXPAND_LISTS
  )

set_property(TARGET zephyr_property_target
             APPEND PROPERTY rom_report_DEPENDENCIES
             partition_manager_report
             )
