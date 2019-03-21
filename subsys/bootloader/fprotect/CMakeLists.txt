#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# Find out which flash protection should be used
if (CONFIG_HAS_HW_NRF_SPU)
  set(FPROTECT_SRC fprotect_spu.c)
elseif (CONFIG_HAS_HW_NRF_ACL)
  set(FPROTECT_SRC fprotect_acl.c)
elseif (CONFIG_HAS_HW_NRF_BPROT OR CONFIG_HAS_HW_NRF_MPU)
  set(FPROTECT_SRC fprotect_bprot.c)
else()
  message(FATAL_ERROR "No fprotect backend selected.")
endif()

zephyr_library()
zephyr_library_sources(${FPROTECT_SRC})
