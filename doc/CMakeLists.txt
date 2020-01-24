# Copyright (c) 2019 Nordic Semiconductor ASA
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic

# Builds combined documentation for all documentation sets: nRF (including
# Doxygen documentation), Zephyr, MCUboot, etc.
#
# We use our own Sphinx configuration files when building the documentation set
# for each repository, instead of reusing configuration files. See e.g.
# doc/nrf/conf.py and doc/zephyr/conf.py.
#
# Intersphinx (http://www.sphinx-doc.org/en/master/usage/extensions/intersphinx.html)
# is used to link documentation sets together. It is configured in the Sphinx
# configuration files.
#
# The *-content targets copy/create .rst files that later get built by Sphinx.

cmake_minimum_required(VERSION 3.13.1)
project(nrf-connect-sdk-doc LANGUAGES)

#
# Set various *_BASE variables pointing to the nrf/, zephyr/, etc.,
# directories. Derive them automatically if they're not set in the environment,
# by assuming that e.g. nrfxlib can be found at ../../nrfxlib/. Also add them
# to the environment if they're not there.
#

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

if(NOT DEFINED ENV{NRFX_BASE})
  get_filename_component(NRFX_BASE ${CMAKE_CURRENT_LIST_DIR}/../../modules/hal/nordic/nrfx REALPATH)
  set(ENV{NRFX_BASE} ${NRFX_BASE})
endif()

message(STATUS "ZEPHYR_BASE: ${ZEPHYR_BASE}")
message(STATUS "NRF_BASE: $ENV{NRF_BASE}")
message(STATUS "MCUBOOT_BASE: $ENV{MCUBOOT_BASE}")
message(STATUS "NRFXLIB_BASE: $ENV{NRFXLIB_BASE}")
message(STATUS "NRFX_BASE: $ENV{NRFX_BASE}")

#
# Find programs we need (Python, Sphinx, and Doxygen)
#

find_package(PythonInterp 3.4)
set(DOXYGEN_SKIP_DOT True) # Skip the otional Dot component
find_package(Doxygen REQUIRED)

find_program(SPHINXBUILD sphinx-build)
if(${SPHINXBUILD} STREQUAL SPHINXBUILD-NOTFOUND)
  message(FATAL_ERROR "The 'sphinx-build' command was not found. Make sure you have Sphinx installed.")
endif()

#
# CMake build directories (build/zephyr, build/nrf, etc.) and the shared HTML
# output directory
#

set(ZEPHYR_BINARY_DIR ${CMAKE_BINARY_DIR}/zephyr)
set(NRF_BINARY_DIR ${CMAKE_BINARY_DIR}/nrf)
set(MCUBOOT_BINARY_DIR ${CMAKE_BINARY_DIR}/mcuboot)
set(NRFXLIB_BINARY_DIR ${CMAKE_BINARY_DIR}/nrfxlib)
set(NRFX_BINARY_DIR ${CMAKE_BINARY_DIR}/nrfx)

# Output directory for the shared Kconfig documentation
set(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/kconfig)

# HTML output directory
set(HTML_DIR ${CMAKE_BINARY_DIR}/html)
file(MAKE_DIRECTORY ${HTML_DIR})

#
# Add the 'zephyr' target for building the Zephyr documentation. We reuse
# doc/CMakeLists.txt from the Zephyr repository, but use our own Sphinx
# configuration from doc/zephyr/conf.py. The generated HTML is placed in the
# common Sphinx HTML output folder.
#

# Parameters to doc/CMakeLists.txt in Zephyr. KCONFIG_OUTPUT is used to find
# objects.inv from the Kconfig docs, to link the Zephyr docs to the Kconfig
# docs.
set(SPHINXOPTS -c ${NRF_BASE}/doc/zephyr)
set(SPHINX_OUTPUT_DIR ${HTML_DIR}/zephyr)
set(KCONFIG_OUTPUT ${HTML_DIR}/kconfig)

# Get access to the 'html' target from doc/CMakeLists.txt in Zephyr
add_subdirectory(${ZEPHYR_BASE}/doc ${ZEPHYR_BINARY_DIR})

add_custom_target(zephyr)

# The 'html' target is from Zephyr
add_dependencies(zephyr html)

#
# Add 'clean-zephyr', 'clean-nrf', etc., targets
#

foreach(target zephyr nrf mcuboot nrfxlib nrfx kconfig)
  set(TARGET_BINARY_DIR ${CMAKE_BINARY_DIR}/${target})
  # Cleanup build output. remove_directory silently ignores missing
  # directories.
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

#
# Add nRF-related targets
#

set(NRF_SPHINXOPTS -d ${NRF_BINARY_DIR}/doctrees ${NRF_SPHINXOPTS})

set(NRF_DOC_DIR ${NRF_BASE}/doc/nrf)

set(NRF_DOXYFILE_IN ${NRF_DOC_DIR}/nrf.doxyfile.in)
set(NRF_DOXYFILE_OUT ${NRF_BINARY_DIR}/nrf.doxyfile)
set(NRF_RST_OUT ${NRF_BINARY_DIR}/rst)
set(NRF_DOC_LOG ${NRF_BINARY_DIR}/doc.log)
set(NRF_DOXY_LOG ${NRF_BINARY_DIR}/doxy.log)
set(NRF_SPHINX_LOG ${NRF_BINARY_DIR}/sphinx.log)
set(NRF_CONTENT_OUTPUTS ${NRF_BINARY_DIR}/extracted-content.txt)

configure_file(${NRF_DOXYFILE_IN} ${NRF_DOXYFILE_OUT} @ONLY)

add_custom_target(
  nrf-doxy
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS="${NRF_DOXYFILE_OUT}"
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
  "*.rst:doc/nrf/drivers:${NRF_RST_OUT}"
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

set(NRF_SPHINX_BUILD_HTML_COMMAND
  ${CMAKE_COMMAND} -E env
  ZEPHYR_BUILD=${ZEPHYR_BINARY_DIR}
  ZEPHYR_OUTPUT=${HTML_DIR}/zephyr
  NRF_BASE=${NRF_BASE}
  NRF_BUILD=${NRF_BINARY_DIR}
  NRF_OUTPUT=${HTML_DIR}/nrf
  KCONFIG_OUTPUT=${HTML_DIR}/kconfig
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
add_dependencies(nrf-html nrf-doxy nrf-content)

add_custom_target(nrf)
add_dependencies(nrf nrf-html)

#
# Add mcuboot-related targets
#

set(MCUBOOT_SPHINXOPTS -d ${MCUBOOT_BINARY_DIR}/doctrees -c ${NRF_BASE}/doc/mcuboot ${MCUBOOT_SPHINXOPTS})

set(MCUBOOT_RST_OUT ${MCUBOOT_BINARY_DIR}/rst)
set(MCUBOOT_SPHINX_LOG ${MCUBOOT_BINARY_DIR}/sphinx.log)

set(MCUBOOT_DOC_DIR ${MCUBOOT_BASE}/docs)

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
  ${PYTHON_EXECUTABLE} ${NRF_BASE}/doc/scripts/fix_markdown.py
    ${MCUBOOT_RST_OUT}/doc/mcuboot

  WORKING_DIRECTORY ${MCUBOOT_DOC_DIR}
)

add_custom_target(
  mcuboot-html
  COMMAND ${CMAKE_COMMAND} -E env
  NRF_BASE=${NRF_BASE}
  ZEPHYR_OUTPUT=${HTML_DIR}/zephyr
  NRF_OUTPUT=${HTML_DIR}/nrf
  NRF_RST_SRC=${NRF_RST_OUT}/doc/nrf
  MCUBOOT_OUTPUT=${HTML_DIR}/mcuboot
  MCUBOOT_RST_SRC=${MCUBOOT_RST_OUT}
  KCONFIG_OUTPUT=${HTML_DIR}/kconfig
  ${SPHINXBUILD} -w ${MCUBOOT_SPHINX_LOG} -N -b html ${MCUBOOT_SPHINXOPTS} ${MCUBOOT_RST_OUT}/doc/mcuboot ${HTML_DIR}/mcuboot

  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
add_dependencies(mcuboot-html mcuboot-content)

add_custom_target(mcuboot)
add_dependencies(mcuboot mcuboot-html)

#
# Add nrfxlib-related targets (not to be confused with nrfx)
#

# Create an mbedtls configuration file with all settings on
file(STRINGS ${NRFXLIB_BASE}/nrf_security/configs/nrf-config.h.template MBEDTLS_TEMPLATE_CONFIG)
string(REGEX REPLACE
       "#cmakedefine ([-_A-Z0-9]*)"
       "#define \\1\n#define CONFIG_GLUE_\\1\n#define CONFIG_CC310_\\1\n#define CONFIG_VANILLA_\\1"
       MBEDTLS_CONFIG "${MBEDTLS_TEMPLATE_CONFIG}"
)
string(REGEX REPLACE ";" "\n" MBEDTLS_CONFIG "${MBEDTLS_CONFIG}")
file(WRITE ${NRFXLIB_BINARY_DIR}/mbedtls_doxygen_config.h ${MBEDTLS_CONFIG})


set(NRFXLIB_SPHINXOPTS -d ${NRFXLIB_BINARY_DIR}/doctrees -c ${NRF_BASE}/doc/nrfxlib ${NRFXLIB_SPHINXOPTS})


set(NRFXLIB_DOXYFILE_IN ${NRF_BASE}/doc/nrfxlib/nrfxlib.doxyfile.in)
set(NRFXLIB_DOXYFILE_OUT ${NRFXLIB_BINARY_DIR}/nrfxlib.doxyfile)
set(NRFXLIB_RST_OUT ${NRFXLIB_BINARY_DIR}/rst)
set(NRFXLIB_DOXY_LOG ${NRFXLIB_BINARY_DIR}/doxy.log)
set(NRFXLIB_SPHINX_LOG ${NRFXLIB_BINARY_DIR}/sphinx.log)


configure_file(${NRFXLIB_DOXYFILE_IN} ${NRFXLIB_DOXYFILE_OUT} @ONLY)

add_custom_target(
  nrfxlib-doxy
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS="${NRFXLIB_DOXYFILE_OUT}"
    -DOUTPUT_FILE=${NRFXLIB_DOXY_LOG}
    -DERROR_FILE=${NRFXLIB_DOXY_LOG}
    -DWORKING_DIRECTORY=${NRFXLIB_BASE}
    -P ${ZEPHYR_BASE}/cmake/util/execute_process.cmake
)


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
  NRFXLIB_OUTPUT=${HTML_DIR}/nrfxlib
  NRFXLIB_RST_SRC=${NRFXLIB_RST_OUT}
  KCONFIG_OUTPUT=${HTML_DIR}/kconfig
  ${SPHINXBUILD} -w ${NRFXLIB_SPHINX_LOG} -N -b html ${NRFXLIB_SPHINXOPTS} ${NRFXLIB_RST_OUT} ${HTML_DIR}/nrfxlib

  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
add_dependencies(nrfxlib-html nrfxlib-content nrfxlib-doxy)

add_custom_target(nrfxlib)
add_dependencies(nrfxlib nrfxlib-html)

#
# Add nrfx-related targets (not to be confusd with nrfxlib)
#

set(NRFX_DOXY_LOG ${NRFX_BINARY_DIR}/doxy.log)

add_custom_target(
  nrfx-doxy
  COMMAND ${CMAKE_COMMAND}
    -DCOMMAND=${DOXYGEN_EXECUTABLE}
    -DARGS=nrfx.doxyfile
    -DOUTPUT_FILE=${NRFX_DOXY_LOG}
    -DERROR_FILE=${NRFX_DOXY_LOG}
    -DWORKING_DIRECTORY=${NRFX_BASE}/doc
    -P ${ZEPHYR_BASE}/cmake/util/execute_process.cmake
)

#
# Add targets for building the shared Kconfig documentation
#

# The Kconfig documentation is a separate documentation set that's shared
# between all modules (nRF, Zephyr, etc.). This makes it possible to link to
# Kconfig symbols regardless of where they are defined.
#
# We rely on the Zephyr Kconfig files pulling in the other Kconfig files, and
# use the --modules option to genrest.py to split the documentation into a
# separate page for each module.

set(KCONFIG_RST_OUT ${KCONFIG_BINARY_DIR}/rst)

# The 'kconfig-content' target uses genrest.py to generate .rst files for all
# Kconfig symbols, as well as index pages that point to them
add_custom_target(
  kconfig-content
  COMMAND ${CMAKE_COMMAND} -E make_directory ${KCONFIG_RST_OUT}
  COMMAND ${CMAKE_COMMAND} -E env
    PYTHONPATH=${ZEPHYR_BASE}/scripts/kconfig${SEP}$ENV{PYTHONPATH}
    ZEPHYR_BASE=${ZEPHYR_BASE}
    srctree=${ZEPHYR_BASE}
    BOARD_DIR=boards/*/*/
    ARCH=*
    ARCH_DIR=arch/
    SOC_DIR=soc/
    CMAKE_BINARY_DIR=${CMAKE_BINARY_DIR}
    KCONFIG_WARN_UNDEF=y
    KCONFIG_TURBO_MODE=${KCONFIG_TURBO_MODE}
    KCONFIG_DOC_MODE=1
      ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/doc/scripts/genrest.py ${KCONFIG_RST_OUT}
        --separate-all-index
        --modules nRF,nrf,${NRF_BASE}
                  nrfxlib,nrfxlib,${NRFXLIB_BASE}

  VERBATIM
)

# No 'kconfig' target exists, because it clashes with the imported 'kconfig'
# target from the Zephyr repository

add_custom_target(
  kconfig-html
  COMMAND ${CMAKE_COMMAND} -E env
    ZEPHYR_BASE=${ZEPHYR_BASE}
    ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
      ${SPHINXBUILD}
        -b html
        -c ${NRF_BASE}/doc/kconfig
        -d ${KCONFIG_BINARY_DIR}/doctrees
        -N
        -w ${KCONFIG_BINARY_DIR}/sphinx.log
        # The Kconfig reference build doesn't use Breathe, so we can safely
        # parallelize it
        -j auto
        ${KCONFIG_RST_OUT}
        ${HTML_DIR}/kconfig
)
add_dependencies(kconfig-html kconfig-content)
