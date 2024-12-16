#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_BT_FAST_PAIR_LOCATOR_TAG_FP_ADV)
  zephyr_library_sources(fp_adv.c)
  zephyr_linker_sources(DATA_SECTIONS fp_adv_trigger.ld)
endif()
