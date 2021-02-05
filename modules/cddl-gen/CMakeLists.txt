#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if(NOT CONFIG_CDDL_GEN)
  return()
endif()

add_subdirectory(${ZEPHYR_CDDL_GEN_MODULE_DIR} cddl-gen)
