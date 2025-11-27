#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(SB_CONFIG_TEST_SYNCHRONIZE_CORES)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_TEST_SYNCHRONIZE_CORES y)
  set_config_bool(${SB_CONFIG_NETCORE_IMAGE_NAME} CONFIG_TEST_SYNCHRONIZE_CORES y)
else()
  set_config_bool(${DEFAULT_IMAGE} CONFIG_TEST_SYNCHRONIZE_CORES n)

  if(SB_CONFIG_NETCORE_REMOTE)
    set_config_bool(${SB_CONFIG_NETCORE_IMAGE_NAME} CONFIG_TEST_SYNCHRONIZE_CORES n)
  endif()
endif()
