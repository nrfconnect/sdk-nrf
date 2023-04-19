#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fp_storage)

if(CONFIG_BT_FAST_PAIR_STORAGE_MANAGER)
  zephyr_linker_sources(SECTIONS fp_storage_manager.ld)
  target_sources(fp_storage PRIVATE fp_storage_manager.c)
endif()
if(CONFIG_BT_FAST_PAIR_STORAGE_AK)
  target_sources(fp_storage PRIVATE fp_storage_ak.c)
endif()
if(CONFIG_BT_FAST_PAIR_STORAGE_EXT_PN)
  target_sources(fp_storage PRIVATE fp_storage_pn.c)
endif()

target_include_directories(fp_storage PUBLIC include)
target_include_directories(fp_storage PUBLIC ../include/common)
if(CONFIG_BT_FAST_PAIR_STORAGE_EXPOSE_PRIV_API)
  target_include_directories(fp_storage PUBLIC include_priv)
else()
  target_include_directories(fp_storage PRIVATE include_priv)
endif()
