#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(${ZEPHYR_NRF_MODULE_DIR}/sysbuild/image_flasher.cmake)

add_image_flasher(NAME provision_raw_data HEX_FILE "${CMAKE_BINARY_DIR}/${DEFAULT_IMAGE}/zephyr/provisioned_raw.hex")

