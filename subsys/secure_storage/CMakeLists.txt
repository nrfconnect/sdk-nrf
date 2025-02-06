# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

if(CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS AND CONFIG_PARTITION_MANAGER_ENABLED)
  message(FATAL_ERROR "
    CONFIG_SECURE_STORAGE_ITS_STORE_IMPLEMENTATION_ZMS is
    not supported when partition manager is enabled.
    ")
endif()
