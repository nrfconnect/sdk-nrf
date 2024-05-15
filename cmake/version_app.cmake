#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(${APP_VERSION_NUMBER})
  find_package(Git QUIET)
  if(GIT_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-parse --absolute-git-dir
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE app_git_dir
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE stderr
      RESULT_VARIABLE return_code)
    if(return_code)
      message(WARNING "APP_VERSION: git rev-parse failed: ${stderr}")
    else()
      if(NOT "${stderr}" STREQUAL "")
        message(WARNING "APP_VERSION: git rev-parse warned: ${stderr}")
      else()
        set(APP_GIT_INDEX ${app_git_dir}/index)
      endif()
    endif()
  endif()

  if(DEFINED APP_GIT_INDEX)
    add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/include/generated/app_commit.h
      COMMAND ${CMAKE_COMMAND} -DZEPHYR_BASE=${ZEPHYR_BASE} -DNRF_DIR=${NRF_DIR}
        -DOUT_FILE=${PROJECT_BINARY_DIR}/include/generated/app_commit.h
        -DCOMMIT_TYPE=APP
        -DCOMMIT_PATH=${CMAKE_SOURCE_DIR}
        -P ${ZEPHYR_NRF_MODULE_DIR}/cmake/gen_commit_h.cmake
        DEPENDS ${CMAKE_SOURCE_DIR}/VERSION ${APP_GIT_INDEX}
    )
    add_custom_target(app_commit_h DEPENDS ${PROJECT_BINARY_DIR}/include/generated/app_commit.h)
    add_dependencies(version_h app_commit_h)

    # Add a compile definition so that the commit can be printed
    zephyr_compile_definitions(NCS_APPLICATION_BOOT_BANNER_GIT_REPO)
  endif()
endif()
