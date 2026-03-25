#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(manufacturing_app)

# ---------------------------------------------------------------------------
# Key and digest header generation
# ---------------------------------------------------------------------------
include(cmake/keys.cmake)

# ---------------------------------------------------------------------------
# Include path for generated headers
# ---------------------------------------------------------------------------
target_include_directories(app PRIVATE
  src
  ${CMAKE_BINARY_DIR}/generated
)

# ---------------------------------------------------------------------------
# Expose the keys/ directory path as a string literal available at runtime,
# ---------------------------------------------------------------------------
file(RELATIVE_PATH _keys_rel ${WEST_TOPDIR} ${CMAKE_CURRENT_SOURCE_DIR}/keys)
target_compile_definitions(app PRIVATE
  SAMPLE_KEYS_DIR="WEST_TOPDIR/${_keys_rel}"
)

# ---------------------------------------------------------------------------
# Application sources
# ---------------------------------------------------------------------------
target_sources(app PRIVATE
  src/main.c
  src/recovery.c
  src/kmu_check.c
  src/image_validation.c
  src/key_provisioning.c
  src/erase_protection.c
  src/huk_provisioning.c
  src/product_tests.c
  src/asset_provisioning.c
)
