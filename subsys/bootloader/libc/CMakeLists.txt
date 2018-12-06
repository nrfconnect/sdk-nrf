#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

set(STD_LIB_C ${ZEPHYR_BASE}/lib/libc/minimal)
set(STD_SOURCE ${STD_LIB_C}/source)
set(STD_INCLUDE ${STD_LIB_C}/include)

add_library(libc ${ZEPHYR_BASE}/misc/empty_file.c)
target_link_libraries(libc PRIVATE zephyr_interface)
target_link_libraries(libc PRIVATE gcc)
add_dependencies(libc offsets_h)

target_include_directories(libc
  PRIVATE
  ${STD_INCLUDE}
  ${STD_INCLUDE}/bits
  ${STD_INCLUDE}/sys
  )

target_sources(libc
  PRIVATE
  ${STD_SOURCE}/stdlib/atoi.c
  ${STD_SOURCE}/stdlib/strtol.c
  ${STD_SOURCE}/stdlib/strtoul.c
  ${STD_SOURCE}/stdlib/malloc.c
  ${STD_SOURCE}/string/strncasecmp.c
  ${STD_SOURCE}/string/strstr.c
  ${STD_SOURCE}/string/string.c
  ${STD_SOURCE}/stdout/prf.c
  ${STD_SOURCE}/stdout/stdout_console.c
  ${STD_SOURCE}/stdout/sprintf.c
  ${STD_SOURCE}/stdout/fprintf.c
)

