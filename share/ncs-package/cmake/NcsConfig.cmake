# Relative directory of NRF dir as seen from ZephyrExtension package file
set(NRF_RELATIVE_DIR "../../..")
set(NCS_RELATIVE_DIR "../../../..")

set(NCS_TOOLCHAIN_MINIMUM_REQUIRED 1.5.0)

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

if(NOT NO_BOILERPLATE)
  if(NCS_TOOLCHAIN_VERSION)
    set(NCS_TOOLCHAIN_MINIMUM_REQUIRED ${NCS_TOOLCHAIN_VERSION})
    set(EXACT "EXACT")
  endif()

  if(NOT ${NCS_TOOLCHAIN_MINIMUM_REQUIRED} STREQUAL NONE)
    find_package(NcsToolchain ${NCS_TOOLCHAIN_MINIMUM_REQUIRED} ${EXACT} QUIET)
    if(${NcsToolchain_FOUND})
      message("-- Using NCS Toolchain ${NcsToolchain_VERSION} for building. (${NcsToolchain_DIR})")
      set(GIT_EXECUTABLE           ${NCS_TOOLCHAIN_GIT}     CACHE FILEPATH "NCS Toolchain Git")

      set(DTC                      ${NCS_TOOLCHAIN_DTC}     CACHE FILEPATH "NCS Toolchain DTC")
      set(GPERF                    ${NCS_TOOLCHAIN_GPERF}   CACHE FILEPATH "NCS Toolchain gperf")
      set(WEST                     ${NCS_TOOLCHAIN_WEST}    CACHE FILEPATH "NCS Toolchain West")
      set(Python3_EXECUTABLE       ${NCS_TOOLCHAIN_PYTHON}  CACHE FILEPATH "NCS Toolchain Python")
      set(PYTHON_EXECUTABLE        ${NCS_TOOLCHAIN_PYTHON}  CACHE FILEPATH "NCS Toolchain Python")
      set(PYTHON_PREFER            ${NCS_TOOLCHAIN_PYTHON}  CACHE FILEPATH "NCS Toolchain Python")

      set(ZEPHYR_TOOLCHAIN_VARIANT ${NCS_TOOLCHAIN_VARIANT}        CACHE STRING "NCS Toolchain Variant")
      set(GNUARMEMB_TOOLCHAIN_PATH ${NCS_GNUARMEMB_TOOLCHAIN_PATH} CACHE PATH "NCS GNU ARM emb path")

      if(${CMAKE_GENERATOR} STREQUAL Ninja)
        set(CMAKE_MAKE_PROGRAM ${NCS_TOOLCHAIN_NINJA} CACHE INTERNAL "NCS Toolchain ninja")
      endif()

      # If NCS_TOOLCHAIN_ENV_PATH is set, then we must ensure to prepend it
      # to ENV{PATH}. This is especially needed for west to work properly
      # in windows with MinGW64.
      if(NCS_TOOLCHAIN_ENV_PATH)
        set(ENV{PATH} "${NCS_TOOLCHAIN_ENV_PATH};$ENV{PATH}")
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
