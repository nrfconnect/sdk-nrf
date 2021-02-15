#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_SPM_SERVICE_NS_HANDLER_FROM_SPM_FAULT OR CONFIG_RESET_ON_FATAL_ERROR)
  zephyr_library()
  zephyr_library_sources(fatal_error.c)
endif()
