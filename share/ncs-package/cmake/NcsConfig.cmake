# Relative directory of NRF dir as seen from ZephyrExtension package file
set(NRF_RELATIVE_DIR "../../..")
set(NCS_RELATIVE_DIR "../../../..")

# Set the current NRF_DIR
# The use of get_filename_component ensures that the final path variable will not contain `../..`.
get_filename_component(NRF_DIR ${CMAKE_CURRENT_LIST_DIR}/${NRF_RELATIVE_DIR} ABSOLUTE)
get_filename_component(NCS_DIR ${CMAKE_CURRENT_LIST_DIR}/${NCS_RELATIVE_DIR} ABSOLUTE)

file(STRINGS ${NRF_DIR}/VERSION NCS_VERSION LIMIT_COUNT 1 LENGTH_MINIMUM 5)
string(REGEX MATCH "([^\.]*)\.([^\.]*)\.([^-]*)[-]?(.*)" OUT_VAR ${NCS_VERSION})

set(NCS_VERSION_MAJOR ${CMAKE_MATCH_1})
set(NCS_VERSION_MINOR ${CMAKE_MATCH_2})
set(NCS_VERSION_PATCH ${CMAKE_MATCH_3})
set(NCS_VERSION_EXTRA ${CMAKE_MATCH_4})

file(STRINGS ${NRF_DIR}/scripts/ncs-toolchain-version-minimum.txt
     toolchain_string LIMIT_COUNT 1 REGEX "^nrf-connect-sdk-toolchain="
)

string(REGEX MATCH "=([^ \t]*)" OUT_VAR "${toolchain_string}")
set(NCS_TOOLCHAIN_MINIMUM_REQUIRED ${CMAKE_MATCH_1})

if(NOT NO_BOILERPLATE)
  if(NCS_TOOLCHAIN_VERSION)
    set(NCS_TOOLCHAIN_MINIMUM_REQUIRED ${NCS_TOOLCHAIN_VERSION})
    set(EXACT "EXACT")
  endif()

  if(NOT "${NCS_TOOLCHAIN_MINIMUM_REQUIRED}" STREQUAL "NONE")
    find_package(NcsToolchain ${NCS_TOOLCHAIN_MINIMUM_REQUIRED} ${EXACT} QUIET)
    if(${NcsToolchain_FOUND})
      message("-- Using NCS Toolchain ${NcsToolchain_VERSION} for building. (${NcsToolchain_DIR})")

      set(CUSTOM_COMMAND_PATH ${NCS_TOOLCHAIN_BIN_PATH} $ENV{PATH})
      cmake_path(CONVERT "${CUSTOM_COMMAND_PATH}" TO_NATIVE_PATH_LIST CUSTOM_COMMAND_PATH)
      string(REPLACE ";" "\\\;" CUSTOM_COMMAND_PATH "${CUSTOM_COMMAND_PATH}")

      set(CUSTOM_COMMAND_ENV       ${CMAKE_COMMAND} -E env "PATH=${CUSTOM_COMMAND_PATH}")
      set(GIT_EXECUTABLE           ${NCS_TOOLCHAIN_GIT}     CACHE FILEPATH "NCS Toolchain Git")

      set(DTC                      ${NCS_TOOLCHAIN_DTC}     CACHE FILEPATH "NCS Toolchain DTC")
      set(GPERF                    ${NCS_TOOLCHAIN_GPERF}   CACHE FILEPATH "NCS Toolchain gperf")
      set(WEST                     ${NCS_TOOLCHAIN_WEST}    CACHE FILEPATH "NCS Toolchain West")
      set(Python3_EXECUTABLE       ${NCS_TOOLCHAIN_PYTHON}  CACHE FILEPATH "NCS Toolchain Python")
      set(PYTHON_EXECUTABLE        ${NCS_TOOLCHAIN_PYTHON}  CACHE FILEPATH "NCS Toolchain Python")

      if(DEFINED NCS_TOOLCHAIN_PROTOC)
        set(PROTOC                     ${CUSTOM_COMMAND_ENV} ${NCS_TOOLCHAIN_PROTOC} CACHE STRING "NCS Toolchain protoc")
        set(PROTOBUF_PROTOC_EXECUTABLE ${CUSTOM_COMMAND_ENV} ${NCS_TOOLCHAIN_PROTOC} CACHE STRING "NCS Toolchain protoc")
      endif()

      set(ZEPHYR_TOOLCHAIN_VARIANT ${NCS_TOOLCHAIN_VARIANT}        CACHE STRING "NCS Toolchain Variant")
      if("${ZEPHYR_TOOLCHAIN_VARIANT}" STREQUAL "zephyr")
        set(ZEPHYR_SDK_INSTALL_DIR ${NCS_ZEPHYR_SDK_INSTALL_DIR} CACHE PATH "NCS Zephyr SDK install dir")
      endif()
      set(GNUARMEMB_TOOLCHAIN_PATH ${NCS_GNUARMEMB_TOOLCHAIN_PATH} CACHE PATH "NCS GNU ARM emb path")

      if("${CMAKE_GENERATOR}" STREQUAL "Ninja")
        set(CMAKE_MAKE_PROGRAM ${NCS_TOOLCHAIN_NINJA} CACHE INTERNAL "NCS Toolchain ninja")
      endif()

      if("${CMAKE_HOST_SYSTEM_NAME}" STREQUAL "Windows")
        set(env_list_delimeter ";")
      else()
        set(env_list_delimeter ":")
      endif()

      # If NCS_TOOLCHAIN_ENV_PATH is set, then we must ensure to prepend it
      # to ENV{PATH}. This is especially needed for west to work properly
      # in windows with MinGW64.
      if(NCS_TOOLCHAIN_ENV_PATH)
        set(ENV{PATH} "${NCS_TOOLCHAIN_ENV_PATH}${env_list_delimeter}$ENV{PATH}")
      endif()

      # If the NCS toolchain specifies a dedicated PYTHONPATH,
      # we must ensure all calls uses that.
      if(NCS_TOOLCHAIN_PYTHONPATH)
        set(ENV{PYTHONPATH} "${NCS_TOOLCHAIN_PYTHONPATH}")
      endif()
    endif()
  endif()

  include(${NRF_DIR}/cmake/boilerplate.cmake NO_POLICY_SCOPE)
endif()
