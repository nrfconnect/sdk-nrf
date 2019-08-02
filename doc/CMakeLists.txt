#
# Copyright (c) 2019 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#
cmake_minimum_required(VERSION 3.13.1)
project(nrf-connect-sdk-doc LANGUAGES)

set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})
get_filename_component(NRF_BASE ${CMAKE_CURRENT_LIST_DIR}../ DIRECTORY)
set(ENV{NRF_BASE} ${NRF_BASE})
if(NOT DEFINED ENV{MCUBOOT_BASE})
 get_filename_component(MCUBOOT_BASE ${CMAKE_CURRENT_LIST_DIR}/../../mcuboot/ REALPATH)
 set(ENV{MCUBOOT_BASE} ${MCUBOOT_BASE})
endif()
if(NOT DEFINED ENV{NRFXLIB_BASE})
 get_filename_component(NRFXLIB_BASE ${CMAKE_CURRENT_LIST_DIR}/../../nrfxlib/ REALPATH)
 set(ENV{NRFXLIB_BASE} ${NRFXLIB_BASE})
endif()

message(STATUS "ZEPHYR_BASE: ${ZEPHYR_BASE}")
message(STATUS "NRF_BASE: $ENV{NRF_BASE}")
message(STATUS "MCUBOOT_BASE: $ENV{MCUBOOT_BASE}")
message(STATUS "NRFXLIB_BASE: $ENV{NRFXLIB_BASE}")

find_package(PythonInterp 3.4)
set(DOXYGEN_SKIP_DOT True)
find_package(Doxygen REQUIRED)

find_program(
  SPHINXBUILD
  sphinx-build
  )
if(${SPHINXBUILD} STREQUAL SPHINXBUILD-NOTFOUND)
  message(FATAL_ERROR "The 'sphinx-build' command was not found. Make sure you have Sphinx installed.")
endif()

set(ZEPHYR_BINARY_DIR ${CMAKE_BINARY_DIR}/zephyr)
set(NRF_BINARY_DIR ${CMAKE_BINARY_DIR}/nrf)
set(MCUBOOT_BINARY_DIR ${CMAKE_BINARY_DIR}/mcuboot)
set(NRFXLIB_BINARY_DIR ${CMAKE_BINARY_DIR}/nrfxlib)
set(NRF_DOC_DIR ${NRF_BASE}/doc/nrf)
set(MCUBOOT_DOC_DIR ${MCUBOOT_BASE}/docs)
set(HTML_DIR ${CMAKE_BINARY_DIR}/html)
# Create the combined HTML output folder
file(MAKE_DIRECTORY ${HTML_DIR})

# Create a mbedtls config file with all settings on
file(STRINGS ${NRFXLIB_BASE}/nrf_security/configs/nrf-config.h.template MBEDTLS_TEMPLATE_CONFIG)
string(REGEX REPLACE
       "#cmakedefine ([-_A-Z0-9]*)"
       "#define \\1\n#define CONFIG_GLUE_\\1\n#define CONFIG_CC310_\\1\n#define CONFIG_VANILLA_\\1"
       MBEDTLS_CONFIG "${MBEDTLS_TEMPLATE_CONFIG}"
)
string(REGEX REPLACE ";" "\n" MBEDTLS_CONFIG "${MBEDTLS_CONFIG}")
file(WRITE ${NRFXLIB_BINARY_DIR}/mbedtls_doxygen_config.h ${MBEDTLS_CONFIG})

# Build Zephyr documentation
set(SPHINXOPTS -c ${NRF_BASE}/doc/zephyr)
# Place the generated HTML in the common Sphinx HTML output folder
set(SPHINX_OUTPUT_DIR ${HTML_DIR}/zephyr)
add_subdirectory(${ZEPHYR_BASE}/doc ${ZEPHYR_BINARY_DIR})

add_custom_target(
  zephyr
)

add_dependencies(zephyr html)

# Common code
set(TARGET_LIST zephyr nrf mcuboot nrfxlib)

foreach(target ${TARGET_LIST})
  set(TARGET_BINARY_DIR ${CMAKE_BINARY_DIR}/${target})
  # Cleanup build output
  add_custom_target(
    clean-${target}
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${TARGET_BINARY_DIR}/doctrees
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${TARGET_BINARY_DIR}/doxygen
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${TARGET_BINARY_DIR}/rst
    COMMAND ${CMAKE_COMMAND} -E remove ${TARGET_BINARY_DIR}/*.log
    COMMAND ${CMAKE_COMMAND} -E remove ${TARGET_BINARY_DIR}/*.warnings
    # Remove generated HTML as well
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${HTML_DIR}/${target}
  )
endforeach()

###################################################
# Everything below this line is nrf-specific
###################################################

set(NRF_SPHINXOPTS -d ${NRF_BINARY_DIR}/doctrees ${NRF_SPHINXOPTS})

set(NRF_DOXYFILE_IN ${NRF_DOC_DIR}/nrf.doxyfile.in)
set(NRF_DOXYFILE_OUT ${NRF_BINARY_DIR}/nrf.doxyfile)
set(NRF_RST_OUT ${NRF_BINARY_DIR}/rst)
set(NRF_CONF_PY ${NRF_DOC_DIR}/conf.py)
set(NRF_DOC_LOG ${NRF_BINARY_DIR}/doc.log)
set(NRF_DOXY_LOG ${NRF_BINARY_DIR}/doxy.log)
set(NRF_SPHINX_LOG ${NRF_BINARY_DIR}/sphinx.log)
set(NRF_DOC_WARN ${NRF_BINARY_DIR}/doc.warnings)
set(NRF_CONTENT_OUTPUTS ${NRF_BINARY_DIR}/extracted-content.txt)

configure_file(${NRF_DOXYFILE_IN} ${NRF_DOXYFILE_OUT} @ONLY)

set(ARGS ${NRF_DOXYFILE_OUT})

add_custom_target(
  nrf-doxy
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS="${ARGS}"
    -DOUTPUT_FILE=${NRF_DOXY_LOG}
    -DERROR_FILE=${NRF_DOXY_LOG}
    -DWORKING_DIRECTORY=${CMAKE_CURRENT_LIST_DIR}
    -P ${ZEPHYR_BASE}/cmake/util/execute_process.cmake
)

set(NRF_EXTRACT_CONTENT_COMMAND
  ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${NRF_BASE}
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/doc/scripts/extract_content.py
  --outputs ${NRF_CONTENT_OUTPUTS}
  --ignore ${CMAKE_CURRENT_BINARY_DIR}
  --sphinx-src-root ${NRF_DOC_DIR}
  "*:doc:${NRF_RST_OUT}"
  "*.rst:applications:${NRF_RST_OUT}"
  "*.rst:samples:${NRF_RST_OUT}"
  "*.rst:boards:${NRF_RST_OUT}"
  "*.rst:include:${NRF_RST_OUT}"
  "*.rst:lib:${NRF_RST_OUT}"
  "*.rst:scripts:${NRF_RST_OUT}"
  "*.rst:applications:${NRF_RST_OUT}/doc/nrf"
  "*.rst:samples:${NRF_RST_OUT}/doc/nrf"
  "*.rst:boards:${NRF_RST_OUT}/doc/nrf"
  "release-notes*.rst:doc:${NRF_RST_OUT}/doc/nrf"
  "changelog.rst:doc:${NRF_RST_OUT}/doc/nrf"
  "*.rst:include:${NRF_RST_OUT}/doc/nrf"
  "*.rst:lib:${NRF_RST_OUT}/doc/nrf"
  "*.rst:scripts:${NRF_RST_OUT}/doc/nrf"
)

add_custom_target(
  nrf-content
  # Copy all files in doc/ to the rst folder
  COMMAND ${NRF_EXTRACT_CONTENT_COMMAND}
  WORKING_DIRECTORY ${NRF_DOC_DIR}
)

if(WIN32)
  set(SEP $<SEMICOLON>)
else()
  set(SEP :)
endif()

add_custom_target(
  nrf-kconfig
  COMMAND ${CMAKE_COMMAND} -E make_directory ${NRF_RST_OUT}/doc/nrf/reference/kconfig
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH="${ZEPHYR_BASE}/scripts/kconfig${SEP}$ENV{PYTHONPATH}"
  srctree=${NRF_BASE}
  ENV_VAR_BOARD_DIR=boards/*/*/
  ENV_VAR_ARCH=*
  KERNELVERSION=${PROJECT_VERSION}
  SRCARCH=x86
  GENERATED_DTS_BOARD_CONF=not_applicable
  CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
  KCONFIG_TURBO_MODE=${KCONFIG_TURBO_MODE}
  KCONFIG_DOC_MODE=1
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/doc/scripts/genrest.py ${NRF_BASE}/Kconfig.nrf ${NRF_RST_OUT}/doc/nrf/reference/kconfig/
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(NRF_SPHINX_BUILD_HTML_COMMAND
  ${CMAKE_COMMAND} -E env
  ZEPHYR_BUILD=${ZEPHYR_BINARY_DIR}
  ZEPHYR_OUTPUT=${HTML_DIR}/zephyr
  NRF_BASE=${NRF_BASE}
  NRF_BUILD=${NRF_BINARY_DIR}
  NRF_OUTPUT=${HTML_DIR}/nrf
  NRF_RST_SRC=${NRF_RST_OUT}/doc/nrf
  MCUBOOT_OUTPUT=${HTML_DIR}/mcuboot
  NRFXLIB_OUTPUT=${HTML_DIR}/nrfxlib
  ${SPHINXBUILD} -w ${NRF_SPHINX_LOG} -N -b html ${NRF_SPHINXOPTS} ${NRF_RST_OUT}/doc/nrf ${HTML_DIR}/nrf
)

add_custom_target(
  nrf-html
  COMMAND ${NRF_SPHINX_BUILD_HTML_COMMAND}
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${NRF_DOC_LOG} ${NRF_DOXY_LOG} ${NRF_SPHINX_LOG}
  # Copy root index file
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/static/html/index.html ${HTML_DIR}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
add_dependencies(nrf-html nrf-doxy nrf-content nrf-kconfig)

add_custom_target(nrf)
add_dependencies(nrf nrf-html)

###################################################
# Everything below this line is mcuboot-specific
###################################################

set(MCUBOOT_SPHINXOPTS -d ${MCUBOOT_BINARY_DIR}/doctrees -c ${NRF_BASE}/doc/mcuboot ${MCUBOOT_SPHINXOPTS})

set(MCUBOOT_RST_OUT ${MCUBOOT_BINARY_DIR}/rst)
set(MCUBOOT_CONF_PY ${NRF_BASE}/doc/mcuboot/conf.py)
set(MCUBOOT_DOC_LOG ${MCUBOOT_BINARY_DIR}/doc.log)
set(MCUBOOT_SPHINX_LOG ${MCUBOOT_BINARY_DIR}/sphinx.log)
set(MCUBOOT_DOC_WARN ${MCUBOOT_BINARY_DIR}/doc.warnings)

set(EXTRACT_CONTENT ${ZEPHYR_BASE}/doc/scripts/extract_content.py)
set(FIX_MARKDOWN ${NRF_BASE}/doc/scripts/fix_markdown.py)

file(GLOB MDFILES ${MCUBOOT_DOC_DIR}/*.md)

set(MCUBOOT_EXTRACT_CONTENT_COMMAND
 ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${NRF_BASE}
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/doc/scripts/extract_content.py
  --ignore ${CMAKE_CURRENT_BINARY_DIR}
  "*:doc/mcuboot:${MCUBOOT_RST_OUT}"
)

add_custom_target(
  mcuboot-content

  # Copy all files from nrf/doc/mcuboot to the rst folder
  COMMAND ${MCUBOOT_EXTRACT_CONTENT_COMMAND}

  # Copy all markdown files from mcuboot/docs to the rst folder
  COMMAND ${CMAKE_COMMAND} -E copy ${MDFILES} ${MCUBOOT_RST_OUT}/doc/mcuboot/

  # Fix problems in markdown files
  COMMAND ${CMAKE_COMMAND} -E env
  ${PYTHON_EXECUTABLE} ${FIX_MARKDOWN} ${MCUBOOT_RST_OUT}/doc/mcuboot

  WORKING_DIRECTORY ${MCUBOOT_DOC_DIR}
)

add_custom_target(
  mcuboot-html
  COMMAND ${CMAKE_COMMAND} -E env
  NRF_BASE=${NRF_BASE}
  ${SPHINXBUILD} -w ${MCUBOOT_SPHINX_LOG} -N -b html ${MCUBOOT_SPHINXOPTS} ${MCUBOOT_RST_OUT}/doc/mcuboot ${HTML_DIR}/mcuboot

  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
add_dependencies(mcuboot-html mcuboot-content)

add_custom_target(mcuboot)
add_dependencies(mcuboot mcuboot-html)



###################################################
# Everything below this line is nrfxlib-specific
###################################################

set(NRFXLIB_SPHINXOPTS -d ${NRFXLIB_BINARY_DIR}/doctrees -c ${NRF_BASE}/doc/nrfxlib ${NRFXLIB_SPHINXOPTS})


set(NRFXLIB_DOXYFILE_IN ${NRF_BASE}/doc/nrfxlib/nrfxlib.doxyfile.in)
set(NRFXLIB_DOXYFILE_OUT ${NRFXLIB_BINARY_DIR}/nrfxlib.doxyfile)
set(NRFXLIB_RST_OUT ${NRFXLIB_BINARY_DIR}/rst)
set(NRFXLIB_CONF_PY ${NRF_BASE}/doc/nrfxlib/conf.py)
set(NRFXLIB_DOC_LOG ${NRFXLIB_BINARY_DIR}/doc.log)
set(NRFXLIB_DOXY_LOG ${NRFXLIB_BINARY_DIR}/doxy.log)
set(NRFXLIB_SPHINX_LOG ${NRFXLIB_BINARY_DIR}/sphinx.log)
set(NRFXLIB_DOC_WARN ${NRFXLIB_BINARY_DIR}/doc.warnings)


configure_file(${NRFXLIB_DOXYFILE_IN} ${NRFXLIB_DOXYFILE_OUT} @ONLY)

set(ARGS ${NRFXLIB_DOXYFILE_OUT})

add_custom_target(
  nrfxlib-doxy
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS="${ARGS}"
    -DOUTPUT_FILE=${NRFXLIB_DOXY_LOG}
    -DERROR_FILE=${NRFXLIB_DOXY_LOG}
    -DWORKING_DIRECTORY=${NRFXLIB_BASE}
    -P ${ZEPHYR_BASE}/cmake/util/execute_process.cmake
)


add_custom_target(
  nrfxlib-kconfig
  COMMAND ${CMAKE_COMMAND} -E make_directory ${NRFXLIB_RST_OUT}/kconfig
  COMMAND ${CMAKE_COMMAND} -E env
  PYTHONPATH="${ZEPHYR_BASE}/scripts/kconfig${SEP}$ENV{PYTHONPATH}"
  srctree=${NRFXLIB_BASE}
  ENV_VAR_BOARD_DIR=boards/*/*/
  ENV_VAR_ARCH=*
  KERNELVERSION=${PROJECT_VERSION}
  SRCARCH=x86
  GENERATED_DTS_BOARD_CONF=not_applicable
  CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
  KCONFIG_TURBO_MODE=${KCONFIG_TURBO_MODE}
  KCONFIG_DOC_MODE=1
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/doc/scripts/genrest.py ${NRFXLIB_BASE}/Kconfig.nrfxlib ${NRFXLIB_RST_OUT}/kconfig
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(EXTRACT_CONTENT ${ZEPHYR_BASE}/doc/scripts/extract_content.py)

set(NRFXLIB_EXTRACT_CONTENT_COMMAND
 ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${NRFXLIB_BASE}
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/doc/scripts/extract_content.py
  --ignore ${CMAKE_CURRENT_BINARY_DIR}
  "*.rst:.:${NRFXLIB_RST_OUT}"
  "*.rst:include:${NRFXLIB_RST_OUT}/kconfig"
)


add_custom_target(
  nrfxlib-content

  # Copy all files from nrfxlib to the rst folder
  COMMAND ${NRFXLIB_EXTRACT_CONTENT_COMMAND}

  WORKING_DIRECTORY ${NRFXLIB_BASE}
)

add_custom_target(
  nrfxlib-html
  COMMAND ${CMAKE_COMMAND} -E env
  NRF_BASE=${NRF_BASE}
  NRFXLIB_BUILD=${NRFXLIB_BINARY_DIR}
  ${SPHINXBUILD} -w ${NRFXLIB_SPHINX_LOG} -N -b html ${NRFXLIB_SPHINXOPTS} ${NRFXLIB_RST_OUT} ${HTML_DIR}/nrfxlib

  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
add_dependencies(nrfxlib-html nrfxlib-content nrfxlib-doxy nrfxlib-kconfig)

add_custom_target(nrfxlib)
add_dependencies(nrfxlib nrfxlib-html)
