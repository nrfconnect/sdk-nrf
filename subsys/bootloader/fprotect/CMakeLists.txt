#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

if (CONFIG_SB_FLASH_PROTECT)
  # Find out which flash protection should be used
  if (CONFIG_HAS_HW_NRF_SPU)
    set(FPROTECT_SRC fprotect_spu.c)
  elseif (CONFIG_HAS_HW_NRF_ACL)
    set(FPROTECT_SRC fprotect_acl.c)
  elseif (CONFIG_HAS_HW_BPROT)
    set(FPROTECT_SRC fprotect_bprot.c)
  else()
    message(FATAL_ERROR "No fprotect backend selected.")
  endif()

  add_library(
    fprotect
    ${FPROTECT_SRC})
  target_include_directories(fprotect PRIVATE BEFORE ${ZEPHYR_BASE}/lib/libc/minimal/include)
  target_link_libraries(fprotect PRIVATE zephyr_interface)
  add_dependencies(fprotect offsets_h)
endif ()
