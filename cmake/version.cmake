math(EXPR NCS_VERSION_CODE "(${NCS_VERSION_MAJOR} << 16) + (${NCS_VERSION_MINOR} << 8)  + (${NCS_VERSION_PATCH})")
math(EXPR NCS_VERSION_NUMBER ${NCS_VERSION_CODE} OUTPUT_FORMAT HEXADECIMAL)

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/include/generated/ncs_version.h
  COMMAND ${CMAKE_COMMAND} -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DOUT_FILE=${PROJECT_BINARY_DIR}/include/generated/ncs_version.h
    -DVERSION_TYPE=NCS
    # We don't want Zephyr to parse the NCS VERSION file as it is not conforming to Zephyr VERSION
    # format. Pointing to a non-existing file allows us full control from CMake instead.
    -DVERSION_FILE=${NRF_DIR}/VERSIONFILEDUMMY
    -DNCS_VERSION_CODE=${NCS_VERSION_CODE}
    -DNCS_VERSION_NUMBER=${NCS_VERSION_NUMBER}
    -DNCS_VERSION_MAJOR=${NCS_VERSION_MAJOR}
    -DNCS_VERSION_MINOR=${NCS_VERSION_MINOR}
    -DNCS_PATCHLEVEL=${NCS_VERSION_PATCH}
    -DNCS_VERSION_STRING=${NCS_VERSION}
    -P ${ZEPHYR_BASE}/cmake/gen_version_h.cmake
    DEPENDS ${NRF_DIR}/VERSION ${git_dependency}
)
add_custom_target(ncs_version_h DEPENDS ${PROJECT_BINARY_DIR}/include/generated/ncs_version.h)
add_dependencies(version_h ncs_version_h)

find_package(Git QUIET)
if(GIT_FOUND)
  # The gitdir folder will not be under NRF_DIR when using git worktrees. Use git rev-parse to find
  # the absolute path to the git directory.
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-parse --absolute-git-dir
    WORKING_DIRECTORY                ${NRF_DIR}
    OUTPUT_VARIABLE                  ABSOLUTE_GIT_DIR
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
    ERROR_VARIABLE                   stderr
    RESULT_VARIABLE                  return_code
  )

  if(return_code)
    message(STATUS "git rev-parse failed: ${stderr}")
  elseif(NOT "${stderr}" STREQUAL "")
    message(STATUS "git rev-parse warned: ${stderr}")
  endif()
else()
  set(ABSOLUTE_GIT_DIR ${NRF_DIR}/".git")
endif()

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/include/generated/ncs_commit.h
  COMMAND ${CMAKE_COMMAND} -DZEPHYR_BASE=${ZEPHYR_BASE} -DNRF_DIR=${NRF_DIR}
    -DOUT_FILE=${PROJECT_BINARY_DIR}/include/generated/ncs_commit.h
    -DCOMMIT_TYPE=NCS
    -DCOMMIT_PATH=${NRF_DIR}
    -P ${ZEPHYR_NRF_MODULE_DIR}/cmake/gen_commit_h.cmake
    DEPENDS ${NRF_DIR}/VERSION ${ABSOLUTE_GIT_DIR} ${ABSOLUTE_GIT_DIR}/index
)
add_custom_target(ncs_commit_h DEPENDS ${PROJECT_BINARY_DIR}/include/generated/ncs_commit.h)
add_dependencies(version_h ncs_commit_h)

add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/include/generated/zephyr_commit.h
  COMMAND ${CMAKE_COMMAND} -DZEPHYR_BASE=${ZEPHYR_BASE} -DNRF_DIR=${NRF_DIR}
    -DOUT_FILE=${PROJECT_BINARY_DIR}/include/generated/zephyr_commit.h
    -DCOMMIT_TYPE=ZEPHYR
    -DCOMMIT_PATH=${ZEPHYR_BASE}
    -P ${ZEPHYR_NRF_MODULE_DIR}/cmake/gen_commit_h.cmake
    DEPENDS ${ZEPHYR_BASE}/VERSION ${ZEPHYR_BASE}/.git ${git_dependency}
)
add_custom_target(zephyr_commit_h DEPENDS ${PROJECT_BINARY_DIR}/include/generated/zephyr_commit.h)
add_dependencies(version_h zephyr_commit_h)
