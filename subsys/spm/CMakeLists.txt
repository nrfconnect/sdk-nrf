#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_IS_SPM)
zephyr_sources(spm.c)
zephyr_sources_ifdef(CONFIG_SPM_SECURE_SERVICES secure_services.c)
zephyr_sources_ifdef(CONFIG_SPM_SHARE_CONSOLE_UART spm_uart.c)
zephyr_linker_sources(SECTIONS secure_services.ld)

if(CONFIG_ARM_FIRMWARE_HAS_SECURE_ENTRY_FUNCS)
  share("list(APPEND ${IMAGE_NAME}BUILD_BYPRODUCTS
        ${CMAKE_BINARY_DIR}/${CONFIG_ARM_ENTRY_VENEERS_LIB_NAME})")
        share("set(${IMAGE_NAME}CONFIG_FPU ${CONFIG_FPU})")
endif()
elseif(CONFIG_SPM)
  include(${CMAKE_BINARY_DIR}/spm/shared_vars.cmake)
  if(NOT "${CONFIG_FPU}" STREQUAL "${spm_CONFIG_FPU}")
    add_custom_target(spm_fail
                      COMMAND ${PYTHON_EXECUTABLE}
                      -c \"print('Mismatch between fpu configurations in application and spm, CONFIG_FPU=${CONFIG_FPU}, spm_CONFIG_FPU=${spm_CONFIG_FPU}')\; exit(1)\"
                      )
    # This dependency is added to ensure build is failing as early as possible:
    add_dependencies(zephyr_interface spm_fail)
  endif()
endif()
