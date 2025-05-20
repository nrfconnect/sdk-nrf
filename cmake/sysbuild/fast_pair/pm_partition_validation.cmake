#
# Copyright (c) 2025 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT "${FP_PARTITION_NAME}" STREQUAL "${FP_PARTITION_PM_CONFIG_NAME}")
  message(FATAL_ERROR
          "\nThe `bt_fast_pair` partition is not defined in the Partition Manager yaml "
          "file. Please define the Fast Pair partition to use the automatic provisioning "
          "data generation with the SB_CONFIG_BT_FAST_PAIR_PROV_DATA Kconfig. The partition "
          "must be statically defined with the pm_static.yml file when the "
          "CONFIG_BT_FAST_PAIR Kconfig is not enabled in the default image.")
endif()
