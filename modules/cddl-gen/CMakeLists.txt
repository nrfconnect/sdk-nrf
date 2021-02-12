#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(NOT CONFIG_CDDL_GEN)
  return()
endif()

add_subdirectory(${ZEPHYR_CDDL_GEN_MODULE_DIR} cddl-gen)
