#
# Copyright (c) 2025 - 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_MCUMGR_GRP_IMG_NRF)
  # Amend the image management group library
  set(ZEPHYR_CURRENT_LIBRARY "mcumgr_grp_img_mgmt")

  zephyr_library_sources(
    src/img_mgmt.c
    src/img_mgmt_state.c
  )

  set_source_files_properties(
    ${ZEPHYR_BASE}/subsys/mgmt/mcumgr/grp/img_mgmt/src/img_mgmt.c
    DIRECTORY ${ZEPHYR_BASE}/subsys/mgmt/mcumgr/grp/img_mgmt/
    PROPERTIES HEADER_FILE_ONLY ON
  )

  set_source_files_properties(
    ${ZEPHYR_BASE}/subsys/mgmt/mcumgr/grp/img_mgmt/src/img_mgmt_state.c
    DIRECTORY ${ZEPHYR_BASE}/subsys/mgmt/mcumgr/grp/img_mgmt/
    PROPERTIES HEADER_FILE_ONLY ON
  )
endif()

if(CONFIG_MCUMGR_GRP_IMG_NRF5340_BOOTUTIL_HOOK)
  # Amend the image management group library
  set(ZEPHYR_CURRENT_LIBRARY "mcumgr_grp_img_mgmt")

  zephyr_include_directories(
    ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/bootutil/include
    ${ZEPHYR_MCUBOOT_MODULE_DIR}/boot/zephyr/include
  )

  zephyr_library_sources(bootutil_hooks/nrf53_hooks.c)
endif()
