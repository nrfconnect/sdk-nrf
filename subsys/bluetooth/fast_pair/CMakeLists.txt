#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fast_pair)

zephyr_library_include_directories(include)
zephyr_library_include_directories(include/common)
# This directory is included to make a workaround of Android issue with sending old RPA address
# during Key-based Pairing write.
zephyr_library_include_directories(${ZEPHYR_BASE}/subsys/bluetooth)

zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_ADVERTISING fp_advertising.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_AUTH fp_auth.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_GATT_SERVICE fp_gatt_service.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_KEYS fp_keys.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_REGISTRATION_DATA fp_registration_data.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_BATTERY fp_battery.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_STORAGE_INTEGRATION fp_storage_integration.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_BOND_MANAGER fp_bond_manager.c)

zephyr_linker_sources_ifdef(CONFIG_BT_FAST_PAIR_ACTIVATION SECTIONS fp_activation.ld)
if(CONFIG_BT_FAST_PAIR_ACTIVATION)
  zephyr_iterable_section(NAME fp_activation_module KVMA RAM_REGION GROUP RODATA_REGION)
endif()
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_ACTIVATION fp_activation.c)

if(CONFIG_BT_FAST_PAIR_CRYPTO)
  add_subdirectory(fp_crypto)
  target_link_libraries(fast_pair PRIVATE fp_crypto)
endif()

if(CONFIG_BT_FAST_PAIR_STORAGE)
  add_subdirectory(fp_storage)
  target_link_libraries(fast_pair PRIVATE fp_storage)
endif()

if(CONFIG_BT_FAST_PAIR_FHN OR CONFIG_BT_FAST_PAIR_FMDN)
  add_subdirectory(fhn)
  target_link_libraries(fast_pair PRIVATE fhn)
endif()

if(CONFIG_BT_FAST_PAIR_FMDN)
  zephyr_library_compile_options(
    -include ${CMAKE_CURRENT_SOURCE_DIR}/fhn/include_priv/fp_fhn_kconfig_compat.h)
endif()

add_subdirectory_ifdef(CONFIG_BT_FAST_PAIR_ADV_MANAGER adv_manager)
