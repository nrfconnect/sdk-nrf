#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(fast_pair_adv_manager)

if(CONFIG_BT_FAST_PAIR_FMDN)
  zephyr_library_compile_options(
    -include ${CMAKE_CURRENT_SOURCE_DIR}/../fhn/include_priv/fp_fhn_kconfig_compat.h)
endif()

zephyr_library_sources(core.c)
zephyr_library_sources_ifdef(CONFIG_BT_FAST_PAIR_ADV_MANAGER_USE_CASE_LOCATOR_TAG locator_tag.c)

zephyr_linker_sources(DATA_SECTIONS trigger.ld)

target_include_directories(fast_pair_adv_manager PRIVATE include_priv)
