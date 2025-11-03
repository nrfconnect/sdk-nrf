#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_MCUMGR_GRP_IMG_NRF)
  zephyr_library_amend()
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
