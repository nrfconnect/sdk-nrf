#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

zephyr_include_directories(include)
zephyr_library()
zephyr_library_sources(
  os/supl_os_client.c
  os/lte_params.c
  os/utils.c
)
