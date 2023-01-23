#
# Copyright (c) 2022 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_BOOT_IMAGE_ACCESS_HOOKS)
  if(CONFIG_BOOT_IMAGE_ACCESS_HOOK_NRF5340)
    zephyr_library()
    zephyr_library_sources(nrf53_hooks.c)
    zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
  endif()
endif()
