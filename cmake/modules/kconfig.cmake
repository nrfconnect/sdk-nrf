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

  if(SB_CONFIG_PARTITION_MANAGER)
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
    dt_chosen(code_partition PROPERTY "zephyr,code-partition")
    dt_reg_addr(code_partition_offset PATH "${code_partition}" REQUIRED)
    dt_reg_size(code_partition_size PATH "${code_partition}" REQUIRED)

    set(preload_autoconf_h ${PRELOAD_BINARY_DIR}/zephyr/include/generated/zephyr/autoconf.h)
    set(preload_dotconfig  ${PRELOAD_BINARY_DIR}/zephyr/.config)

    file(STRINGS ${preload_autoconf_h} autoconf_content)
    file(STRINGS ${preload_dotconfig} dotconfig_content)

    # Modify the CONFIG_FLASH_LOAD_OFFSET and CONFIG_FLASH_LOAD_SIZE for both the .config and autoconf.h files.
    # If partition manager is not used, these values should be taken from the device tree.
    # Additionally, convert primary slot dependencies to secondary slot dependencies.
    set(dotconfig_variant_content)
    foreach(line IN LISTS dotconfig_content)
      if("${line}" MATCHES "^CONFIG_FLASH_LOAD_OFFSET=.*$")
        string(REGEX REPLACE "CONFIG_FLASH_LOAD_OFFSET=(.*)" "CONFIG_FLASH_LOAD_OFFSET=${code_partition_offset}" line ${line})
      endif()

      if("${line}" MATCHES "^CONFIG_FLASH_LOAD_SIZE=.*$")
        string(REGEX REPLACE "CONFIG_FLASH_LOAD_SIZE=(.*)" "CONFIG_FLASH_LOAD_SIZE=${code_partition_size}" line ${line})
      endif()

      if("${line}" MATCHES "(--dependencies|-d).*\([0-9, ]+primary[0-9., ]+\)")
        string(REGEX REPLACE "primary" "secondary" line ${line})
      endif()

      list(APPEND dotconfig_variant_content "${line}\n")
    endforeach()

    set(autoconf_variant_content)
    foreach(line IN LISTS autoconf_content)
      if("${line}" MATCHES "^#define CONFIG_FLASH_LOAD_OFFSET .*$")
        string(REGEX REPLACE "#define CONFIG_FLASH_LOAD_OFFSET (.*)" "#define CONFIG_FLASH_LOAD_OFFSET ${code_partition_offset}" line ${line})
      endif()

      if("${line}" MATCHES "^#define CONFIG_FLASH_LOAD_SIZE .*$")
        string(REGEX REPLACE "#define CONFIG_FLASH_LOAD_SIZE (.*)" "#define CONFIG_FLASH_LOAD_SIZE ${code_partition_size}" line ${line})
      endif()

      if("${line}" MATCHES "(--dependencies|-d).*\([0-9, ]+primary[0-9., ]+\)")
        string(REGEX REPLACE "primary" "secondary" line ${line})
      endif()

      list(APPEND autoconf_variant_content "${line}\n")
    endforeach()

    set(dest_autoconf_h ${PROJECT_BINARY_DIR}/include/generated/zephyr/autoconf.h)
    set(dest_dotconfig  ${PROJECT_BINARY_DIR}/.config)

    file(WRITE ${dest_autoconf_h} ${autoconf_variant_content})
    file(WRITE ${dest_dotconfig} ${dotconfig_variant_content})

    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${preload_autoconf_h})
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${preload_dotconfig})

    import_kconfig("CONFIG" ${DOTCONFIG})

  endif()

  file(APPEND ${AUTOCONF_H} "#define CONFIG_NCS_IS_VARIANT_IMAGE 1")
else()
  include(${ZEPHYR_BASE}/cmake/modules/kconfig.cmake)
endif()
