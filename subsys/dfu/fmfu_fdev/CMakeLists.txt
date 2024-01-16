#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources(src/fmfu_fdev.c)

if (CONFIG_FMFU_CDDL_DECODER_GENERATE)
  add_subdirectory(cddl)
else()
  zephyr_library_sources(src/modem_update_decode.c)
  zephyr_include_directories(include)
endif()
