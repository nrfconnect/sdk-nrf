#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_include_directories(
    include
)

zephyr_library()

if(CONFIG_SSF_SUIT_SERVICE_ENABLED)
  zephyr_library_link_libraries(suit_utils)
endif()

zephyr_library_sources(
  src/suitfu_mgmt.c
)


zephyr_library_sources_ifdef(CONFIG_MGMT_SUITFU_GRP_IMG src/suitfu_mgmt_img.c)

zephyr_library_sources_ifdef(CONFIG_MGMT_SUITFU_GRP_SUIT src/suitfu_mgmt_suit.c)
zephyr_library_sources_ifdef(CONFIG_MGMT_SUITFU_GRP_SUIT src/suitfu_mgmt_suit_os.c)
zephyr_library_sources_ifdef(CONFIG_MGMT_SUITFU_GRP_SUIT_MFSTS_STATE src/suitfu_mgmt_suit_mfsts_state.c)
zephyr_library_sources_ifdef(CONFIG_MGMT_SUITFU_GRP_SUIT_CAND_ENV_UPLOAD src/suitfu_mgmt_suit_cand_env_upload.c)
zephyr_library_sources_ifdef(CONFIG_MGMT_SUITFU_GRP_SUIT_IMAGE_FETCH src/suitfu_mgmt_suit_image_fetch.c)
