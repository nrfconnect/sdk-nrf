add_custom_command(
  OUTPUT ${PROJECT_BINARY_DIR}/include/generated/ncs_version.h
  COMMAND ${CMAKE_COMMAND} -DZEPHYR_BASE=${ZEPHYR_BASE}
    -DOUT_FILE=${PROJECT_BINARY_DIR}/include/generated/ncs_version.h
    -DVERSION_TYPE=NCS
    -DVERSION_FILE=${NRF_DIR}/VERSION
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
