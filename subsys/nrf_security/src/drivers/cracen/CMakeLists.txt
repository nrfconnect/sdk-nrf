#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This CMakeLists.txt is executed both by the Zephyr build system and
# by the TF-M build system and must thus be portable to both.

# This CMakeListst.txt will add sources to the pre-existing CMake
# library called ${mbedcrypto_target}.
if(NOT TARGET "${mbedcrypto_target}")
  message(FATAL_ERROR "Expected a library named '${mbedcrypto_target}' to exist")
endif()

set(cracen_driver_sources)

set(cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/common/include/
)

# Add more CRACEN sources and include dirs from the ext directory
include(${NRF_SECURITY_ROOT}/src/utils/nrf_security_utils.cmake)
include(sxsymcrypt/sxsymcrypt.cmake)
include(silexpk/silexpk.cmake)
include(sicrypto/sicrypto.cmake)
include(cracenpsa/cracenpsa.cmake)

if(BUILD_INSIDE_TFM)
target_sources(
  ${mbedcrypto_target}
  PRIVATE
  ${nrf_security_utils_sources}
)
else()
  zephyr_library()
  zephyr_library_sources(${nrf_security_utils_sources})
endif()

target_sources(
  ${mbedcrypto_target}
  PRIVATE
  ${cracen_driver_sources}
)

target_include_directories(
  ${mbedcrypto_target}
  PUBLIC
  ${cracen_driver_include_dirs}
  ${nrf_security_utils_include_dirs}
)

# The CRACEN sources have not been written to comply with this gcc
# warning
target_compile_options(
  ${mbedcrypto_target}
  PRIVATE
  -Wno-pointer-sign
)

if(BUILD_INSIDE_TFM)
  # Add an empty interface library called zephyr_interface so that we
  # can use the zephyr_library_* API's in both the TF-M and Zephyr
  # build.
  add_library(zephyr_interface INTERFACE)

  target_compile_options(
	mbedcrypto_base
	PUBLIC
	-imacros ${ZEPHYR_AUTOCONF}
  )

  target_link_libraries(${mbedcrypto_target}
	PUBLIC
	tfm_sprt

	# The CRACEN sources in mbedcrypto_target use functions from the
	# cracen_interrupts library.
	#
	# cracen_interrupts is not in the mbedcrypto_target library itself
	# like the other CRACEN sources because use of the Zephyr syscall
	# k_event would not work from the mbedcrypto_target library for
	# some reason.
	cracen_interrupts
  )
endif()

zephyr_library_named(cracen_interrupts)
zephyr_library_sources(sxsymcrypt/src/platform/baremetal/interrupts.c)
target_link_libraries(cracen_interrupts
  PRIVATE
  ${mbedcrypto_target}
  )
