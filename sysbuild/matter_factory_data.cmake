#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_ZEPHYR_CONNECTEDHOMEIP_MODULE AND SB_CONFIG_MATTER AND
  SB_CONFIG_MATTER_FACTORY_DATA_GENERATE AND NOT SB_CONFIG_PARTITION_MANAGER
)
  ExternalZephyrProject_Add(
    APPLICATION matter_factory_data
    SOURCE_DIR ${ZEPHYR_NRF_MODULE_DIR}/applications/matter_factory_data
  )

  set_config_string(matter_factory_data CONFIG_APPLICATION_NAME ${DEFAULT_IMAGE})
  sysbuild_add_dependencies(CONFIGURE matter_factory_data ${DEFAULT_IMAGE})
endif()
