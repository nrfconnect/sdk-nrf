#
# Copyright (c) 2023 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# This CMakeLists.txt is executed both by the Zephyr build system and
# by the TF-M build system and must thus be portable to both.

if(NOT CONFIG_PSA_CRYPTO_DRIVER_CRACEN)
  # These sources are only added when CRACEN is used and it is only
  # supported to use CRACEN as a PSA_CRYPTO_DRIVER.
  return()
endif()

if(CONFIG_BUILD_WITH_TFM)
  # CONFIG_BUILD_WITH_TFM is not set for the TF-M image, so this must
  # be the non-secure image and the non-secure image should not have
  # cracen sources so we return.
  return()
endif()

# This CMakeListst.txt will add sources to the pre-existing CMake
# library called ${mbedcrypto_target}.
if(NOT TARGET "${mbedcrypto_target}")
  message(FATAL_ERROR "Expected a library named '${mbedcrypto_target}' to exist")
endif()

set(cracen_driver_sources)

set(cracen_driver_include_dirs
  ${CMAKE_CURRENT_LIST_DIR}/ext/common/include/
)

# Add more CRACEN sources and include dirs from the ext directory
include(ext/sxsymcrypt/sxsymcrypt.cmake)
include(ext/silexpk/silexpk.cmake)
include(ext/sicrypto/sicrypto.cmake)
include(ext/cracenpsa/cracenpsa.cmake)

target_sources(
  ${mbedcrypto_target}
  PRIVATE
  ${cracen_driver_sources}
)

target_include_directories(
  ${mbedcrypto_target}
  PUBLIC
  ${cracen_driver_include_dirs}
)

# The CRACEN sources have not been written to comply with this gcc
# warning
target_compile_options(
  ${mbedcrypto_target}
  PRIVATE
  -Wno-pointer-sign
)

if(BUILD_FROM_TFM)
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
zephyr_library_sources(ext/sxsymcrypt/src/platform/baremetal/interrupts.c)
target_link_libraries(cracen_interrupts
  PRIVATE
  ${mbedcrypto_target}
  )
