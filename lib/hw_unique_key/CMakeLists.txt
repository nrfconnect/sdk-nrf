#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

if (CONFIG_CRACEN_HW_PRESENT)
  zephyr_library_sources(hw_unique_key_cracen.c)
else()
  zephyr_library_sources(hw_unique_key.c)

  if (DEFINED CONFIG_HAS_HW_NRF_ACL)
    zephyr_library_sources(hw_unique_key_acl.c)
  else()
    zephyr_library_sources(hw_unique_key_kmu.c)
  endif()

  ncs_add_partition_manager_config(pm.yml.huk)
endif()
