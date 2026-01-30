#
# Copyright (c) 2021 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(matter-lock)

# Enable GNU STD support.
include(${ZEPHYR_CONNECTEDHOMEIP_MODULE_DIR}/config/nrfconnect/app/enable-gnu-std.cmake)

# NORDIC SDK APP START

# Include all source files that are located in the Matter common directory.
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/source_common.cmake)

# Include Data Model utils
include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/data_model.cmake)

include(${ZEPHYR_NRF_MODULE_DIR}/samples/matter/common/cmake/zap_helpers.cmake)
ncs_get_zap_parent_dir(ZAP_PARENT_DIR)

target_include_directories(app PRIVATE
  src
  ${ZAP_PARENT_DIR}
)

target_sources(app PRIVATE
  src/app_task.cpp
  src/bolt_lock_manager.cpp
  src/main.cpp
  src/zcl_callbacks.cpp
  src/access/access_data_types.cpp
  src/access/access_manager_credentials.cpp
  src/access/access_manager_users.cpp
  src/access/access_manager.cpp
)

target_sources_ifdef(CONFIG_LOCK_ACCESS_STORAGE_PERSISTENT_STORAGE app PRIVATE
  src/access/access_storage.cpp
)

target_sources_ifdef(CONFIG_LOCK_ACCESS_STORAGE_PROTECTED_STORAGE app PRIVATE
  src/access/access_storage_psa_ps.cpp
)

target_sources_ifdef(CONFIG_LOCK_SCHEDULES app PRIVATE
  src/access/access_manager_schedules.cpp
)

target_sources_ifdef(CONFIG_LOCK_PRINT_STORAGE_STATUS app PRIVATE
  src/access/access_storage_print.cpp
)

target_sources_ifdef(CONFIG_THREAD_WIFI_SWITCHING app PRIVATE
  src/thread_wifi_switch.cpp
)

# Do not treat warnings as errors while the variable may be uninitialized for this sample data model.
# In the door-lock-server implementation, there is a warning that the C++ "optional"
# variable may be uninitialized, but actually, it is wrongly interpreted by the Zephyr toolchain.
target_compile_options(matter-data-model PRIVATE -Wno-error=maybe-uninitialized)

ncs_configure_data_model()

# NORDIC SDK APP END
