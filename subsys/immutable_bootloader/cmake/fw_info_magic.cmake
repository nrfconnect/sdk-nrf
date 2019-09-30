#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

# This file creates variables with the magic numbers used for firmware metadata
# as comma-separated lists of numbers.

math(EXPR
  MAGIC_COMPATIBILITY_VALIDATION_INFO
  "(${CONFIG_FW_VALIDATION_INFO_VERSION}) |
   (${CONFIG_FW_HARDWARE_ID} << 8) |
   (${CONFIG_FW_VALIDATION_INFO_CRYPTO_ID} << 16) |
   (${CONFIG_FW_CUSTOM_COMPATIBILITY_ID} << 24)"
  )

set(POINTER_MAGIC         "${CONFIG_FW_MAGIC_COMMON},${CONFIG_FW_MAGIC_POINTER},${MAGIC_COMPATIBILITY_VALIDATION_INFO}")
set(VALIDATION_INFO_MAGIC "${CONFIG_FW_MAGIC_COMMON},${CONFIG_FW_MAGIC_VALIDATION_INFO},${MAGIC_COMPATIBILITY_VALIDATION_INFO}")

zephyr_compile_definitions(
  POINTER_MAGIC=${POINTER_MAGIC}
  VALIDATION_INFO_MAGIC=${VALIDATION_INFO_MAGIC}
  )
