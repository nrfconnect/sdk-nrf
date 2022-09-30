#
# Copyright (c) 2018 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()
zephyr_library_sources(wifi_conn_mgr.c)

zephyr_library_include_directories(
	${NRF_DIR}/../modules/tee/tf-m/trusted-firmware-m/interface/include/
)
