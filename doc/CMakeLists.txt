cmake_minimum_required(VERSION 3.8.2)
project(nrf-connect-sdk-doc VERSION ${PROJECT_VERSION} LANGUAGES)

set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})
get_filename_component(NRF_BASE ${CMAKE_CURRENT_LIST_DIR}../ DIRECTORY)

message(STATUS "ZEPHYR_BASE: ${ZEPHYR_BASE}")
message(STATUS "NRF_BASE: ${NRF_BASE}")

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
set(NRF_DOC_DIR ${NRF_BASE}/doc/nrf)

# Build Zephyr documentation
set(SPHINXOPTS -c ${NRF_BASE}/doc/zephyr)
add_subdirectory(${ZEPHYR_BASE}/doc ${ZEPHYR_BINARY_DIR})

add_custom_target(
  zephyr
  DEPENDS html
)

# Everything below this line is nrf-specific

set(NRF_DOXYFILE ${NRF_DOC_DIR}/nrf.doxyfile)
set(NRF_BUILD_DOXYFILE ${NRF_BINARY_DIR}/build.doxyfile)
set(NRF_CONF_PY_IN ${NRF_DOC_DIR}/conf.py.in)
set(NRF_CONF_PY_OUT ${NRF_DOC_DIR}/conf.py)
set(NRF_DOC_LOG doc.log)
set(NRF_DOXY_LOG doxy.log)
set(NRF_SPHINX_LOG sphinx.log)
set(NRF_DOC_WARN doc.warnings)

add_custom_target(
  nrf
  DEPENDS nrf-html
)

add_custom_target(
  nrf-pristine
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${NRF_DOC_DIR}/_doxygen
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${NRF_DOC_DIR}/_samples
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${NRF_DOC_DIR}/_boards
  COMMAND ${CMAKE_COMMAND} -E remove ${NRF_DOC_DIR}/reference/kconfig/*.rst
  COMMAND ${CMAKE_COMMAND} -E remove ${NRF_CONF_PY_OUT}
  DEPENDS pristine
)

configure_file(${NRF_DOXYFILE} ${NRF_BUILD_DOXYFILE} COPYONLY)
file(APPEND ${NRF_BUILD_DOXYFILE} "STRIP_FROM_PATH=${NRF_BASE}\n")

set(ARGS ${NRF_BUILD_DOXYFILE})

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

set(EXTRACT_CONTENT ${ZEPHYR_BASE}/doc/scripts/extract_content.py)

add_custom_target(
  nrf-content
  ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${NRF_BASE}
  ${PYTHON_EXECUTABLE} ${EXTRACT_CONTENT}
  WORKING_DIRECTORY ${NRF_DOC_DIR}
  # extract_content.py places folders in doc/, move them
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${NRF_BASE}/doc/samples ${NRF_DOC_DIR}/_samples
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${NRF_BASE}/doc/samples
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${NRF_BASE}/doc/boards ${NRF_DOC_DIR}/_boards
  COMMAND ${CMAKE_COMMAND} -E remove_directory ${NRF_BASE}/doc/boards
)

if(WIN32)
  set(SEP ;)
else()
  set(SEP :)
endif()

add_custom_target(
  nrf-kconfig
  ${CMAKE_COMMAND} -E env
  PYTHONPATH="${ZEPHYR_BASE}/scripts/kconfig${SEP}$ENV{PYTHONPATH}"
  srctree=${NRF_BASE}
  ENV_VAR_BOARD_DIR=boards/*/*/
  ENV_VAR_ARCH=*
  KERNELVERSION=${PROJECT_VERSION}
  SRCARCH=x86
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/doc/scripts/genrest.py ${NRF_BASE}/Kconfig.nrf ${NRF_DOC_DIR}/reference/kconfig/
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

set(ZEPHYR_HTML_DIR ${ZEPHYR_BINARY_DIR}/html)
configure_file(${NRF_CONF_PY_IN} ${NRF_CONF_PY_OUT} @ONLY)

add_custom_target(
  nrf-html
  COMMAND ${SPHINXBUILD} -w ${NRF_SPHINX_LOG} -N -b html ${NRF_DOC_DIR} ${NRF_BINARY_DIR}/html
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${NRF_DOC_LOG} ${NRF_DOXY_LOG} ${NRF_SPHINX_LOG}
  #COMMAND ${PYTHON_EXECUTABLE} ${KI_SCRIPT} --config-dir ${CONFIG_DIR} --errors ${DOC_WARN} --warnings ${DOC_WARN} ${DOC_LOG}
  DEPENDS html nrf-doxy nrf-content nrf-kconfig
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
