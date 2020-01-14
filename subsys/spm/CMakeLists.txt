#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_sources(spm.c)
zephyr_sources_ifdef(CONFIG_SPM_SECURE_SERVICES secure_services.c)
zephyr_linker_sources(SECTIONS secure_services.ld)


if(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)
  share(
    "list(APPEND ${IMAGE_NAME}BUILD_BYPRODUCTS
    ${CMAKE_BINARY_DIR}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME})")
endif()
