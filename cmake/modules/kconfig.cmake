#
# Copyright (c) 2022 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include_guard(GLOBAL)

if(CONFIG_NCS_IS_VARIANT_IMAGE)
  set(AUTOCONF_H ${PROJECT_BINARY_DIR}/include/generated/zephyr/autoconf.h)
  set(DOTCONFIG  ${PROJECT_BINARY_DIR}/.config)

  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${DOTCONFIG})

  if(CONFIG_NCS_VARIANT_MERGE_KCONFIG)
    # A variant build should reuse same .config and thus autoconf.h, therefore
    # copy files from original and bypass Kconfig invocation.
    set(preload_autoconf_h ${PRELOAD_BINARY_DIR}/zephyr/include/generated/zephyr/autoconf.h)
    set(preload_dotconfig  ${PRELOAD_BINARY_DIR}/zephyr/.config)

    file(COPY ${preload_dotconfig}  DESTINATION ${PROJECT_BINARY_DIR})
    file(COPY ${preload_autoconf_h} DESTINATION ${PROJECT_BINARY_DIR}/include/generated/zephyr)

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${preload_autoconf_h})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${preload_dotconfig})

    import_kconfig("CONFIG" ${DOTCONFIG})
  else()
    # First generate autoconf.h as well as .config files.
    include(${ZEPHYR_BASE}/cmake/modules/kconfig.cmake)
  endif()

  file(APPEND ${AUTOCONF_H} "#define CONFIG_NCS_IS_VARIANT_IMAGE 1")
else()
  include(${ZEPHYR_BASE}/cmake/modules/kconfig.cmake)
endif()
