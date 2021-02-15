#
# Copyright (c) 2019-2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_ARM_FIRMWARE_USES_SECURE_ENTRY_FUNCS)
  zephyr_sources(secure_services_ns.c)
  zephyr_sources_ifdef(CONFIG_SPM_NS_DEBUG_LOGS_DUMP spm_ns_logs_dump.c)

  add_dependencies(zephyr_interface spm_subimage)
endif()
