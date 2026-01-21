#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library_named(nrf_compress)
zephyr_library_sources(src/implementation.c)
zephyr_linker_sources(SECTIONS sections.ld)
zephyr_iterable_section(NAME nrf_compress_implementation KVMA RAM_REGION GROUP RODATA_REGION)

if(CONFIG_NRF_COMPRESS_LZMA OR CONFIG_NRF_COMPRESS_ARM_THUMB)
  zephyr_library_include_directories(lzma)
endif()

if(CONFIG_NRF_COMPRESS_LZMA)
  zephyr_library_sources(lzma/LzmaDec.c src/lzma.c)

  if(CONFIG_NRF_COMPRESS_LZMA_VERSION_LZMA2)
    zephyr_library_sources(lzma/Lzma2Dec.c)
  endif()
endif()

if(CONFIG_NRF_COMPRESS_ARM_THUMB)
  zephyr_library_sources(lzma/armthumb.c src/arm_thumb.c)
endif()
