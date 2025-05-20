#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
zephyr_library()
if(CONFIG_NFC_PLATFORM)
  zephyr_library_sources(platform.c)
  zephyr_library_sources_ifdef(CONFIG_NFC_THREAD_CALLBACK platform_internal_thread.c)
  zephyr_library_sources_ifndef(CONFIG_NFC_THREAD_CALLBACK platform_internal_irq.c)
endif()
