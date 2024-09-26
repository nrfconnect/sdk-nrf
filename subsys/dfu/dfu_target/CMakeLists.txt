#
# Copyright (c) 2020 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_include_directories(include)

zephyr_library_sources(
  src/dfu_target.c
  )

zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_STREAM
  src/dfu_target_stream.c
  )
zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_MODEM_DELTA
  src/dfu_target_modem_delta.c
  )
zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_FULL_MODEM
  src/dfu_target_full_modem.c
  )
zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_MCUBOOT
  src/dfu_target_mcuboot.c
  )
zephyr_library_sources_ifdef(CONFIG_DFU_TARGET_SMP
  src/dfu_target_smp.c
  )
zephyr_library_sources(src/dfu_stream_flatten.c)
if (CONFIG_DFU_TARGET_SUIT)
  zephyr_library_sources(src/dfu_target_suit.c)
  if (CONFIG_SSF_SUIT_SERVICE_ENABLED)
    zephyr_library_link_libraries(suit_utils)
  endif()
endif()
