#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

set(CONFIG_MCUBOOT_COMPRESSED_IMAGE_SUPPORT_ENABLED y)
set(ncs_compress_test_compress_hex y)
include(${ZEPHYR_NRF_MODULE_DIR}/cmake/sysbuild/image_signing.cmake)
set_property(GLOBAL APPEND PROPERTY extra_post_build_commands COMMAND
             ${CMAKE_OBJCOPY} -I ihex -O ihex --change-addresses 0x91000
             ${ZEPHYR_BINARY_DIR}/${KERNEL_NAME}.signed.hex)
