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
  elseif(CONFIG_BOOT_IMAGE_ACCESS_HOOK_CUSTOM)
    zephyr_library()
    if(IS_ABSOLUTE ${CONFIG_BOOT_IMAGE_ACCESS_HOOK_CUSTOM_PATH})
	    set(HOOK_FILE ${CONFIG_BOOT_IMAGE_ACCESS_HOOK_CUSTOM_PATH})
    elseif((DEFINED CONF_DIR) AND
	    (EXISTS ${CONF_DIR}/${CONFIG_BOOT_IMAGE_ACCESS_HOOK_CUSTOM_PATH}))
	    set(HOOK_FILE ${CONF_DIR}/${CONFIG_BOOT_IMAGE_ACCESS_HOOK_CUSTOM_PATH})
    endif()
    zephyr_library_sources(${CONFIG_BOOT_IMAGE_ACCESS_HOOK_CUSTOM_PATH})
    zephyr_library_link_libraries(MCUBOOT_BOOTUTIL)
  endif()
endif()
