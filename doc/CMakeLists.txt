cmake_minimum_required(VERSION 3.8.2)
project(nrf-connect-sdk-doc VERSION ${PROJECT_VERSION} LANGUAGES)

set(ZEPHYR_BASE $ENV{ZEPHYR_BASE})
get_filename_component(NRF_BASE ${CMAKE_CURRENT_LIST_DIR}../ DIRECTORY)
set(ENV{NRF_BASE} ${NRF_BASE})

message(STATUS "ZEPHYR_BASE: ${ZEPHYR_BASE}")
message(STATUS "NRF_BASE: $ENV{NRF_BASE}")

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
)

add_dependencies(zephyr html)

# Everything below this line is nrf-specific

set(NRF_DOXYFILE_IN ${NRF_DOC_DIR}/nrf.doxyfile.in)
set(NRF_DOXYFILE_OUT ${NRF_BINARY_DIR}/nrf.doxyfile)
set(NRF_RST_OUT ${NRF_BINARY_DIR}/rst)
set(NRF_CONF_PY ${NRF_DOC_DIR}/conf.py)
set(NRF_DOC_LOG ${NRF_BINARY_DIR}/doc.log)
set(NRF_DOXY_LOG ${NRF_BINARY_DIR}/doxy.log)
set(NRF_SPHINX_LOG ${NRF_BINARY_DIR}/sphinx.log)
set(NRF_DOC_WARN ${NRF_BINARY_DIR}/doc.warnings)

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


set(EXTRACT_CONTENT ${ZEPHYR_BASE}/doc/scripts/extract_content.py)

add_custom_target(
  nrf-content

  # Copy the .rst files in doc/ to the rst folder
  COMMAND ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${NRF_BASE}
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${PYTHON_EXECUTABLE} ${EXTRACT_CONTENT} ${NRF_RST_OUT} doc
  # Copy the .rst files in samples/ and boards/ to the rst folder
  COMMAND ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${NRF_BASE}
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${PYTHON_EXECUTABLE} ${EXTRACT_CONTENT} ${NRF_RST_OUT} samples boards
  # Copy the .rst files in samples/ and boards/ to the doc/nrf folder inside rst
  COMMAND ${CMAKE_COMMAND} -E env
  ZEPHYR_BASE=${NRF_BASE}
  ZEPHYR_BUILD=${CMAKE_CURRENT_BINARY_DIR}
  ${PYTHON_EXECUTABLE} ${EXTRACT_CONTENT} ${NRF_RST_OUT}/doc/nrf samples boards

  WORKING_DIRECTORY ${NRF_DOC_DIR}
)

if(WIN32)
  set(SEP ;)
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
  ${PYTHON_EXECUTABLE} ${ZEPHYR_BASE}/doc/scripts/genrest.py ${NRF_BASE}/Kconfig.nrf ${NRF_RST_OUT}/doc/nrf/reference/kconfig/
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

configure_file(${NRF_DOC_DIR}/conf.py ${NRF_RST_OUT}/doc/nrf/conf.py COPYONLY)
configure_file(${NRF_DOC_DIR}/links.txt ${NRF_RST_OUT}/doc/nrf/links.txt COPYONLY)
configure_file(${NRF_DOC_DIR}/shortcuts.txt ${NRF_RST_OUT}/doc/nrf/shortcuts.txt COPYONLY)

add_custom_target(
  nrf-html
  COMMAND ${CMAKE_COMMAND} -E env
  ZEPHYR_BUILD=${ZEPHYR_BINARY_DIR}
  NRF_BASE=${NRF_BASE}
  NRF_BUILD=${NRF_BINARY_DIR}
  ${SPHINXBUILD} -w ${NRF_SPHINX_LOG} -N -b html ${ALLSPHINXOPTS} ${NRF_RST_OUT}/doc/nrf ${NRF_BINARY_DIR}/html
  # Merge the Doxygen and Sphinx logs into a single file
  COMMAND ${CMAKE_COMMAND} -P ${ZEPHYR_BASE}/cmake/util/fmerge.cmake ${NRF_DOC_LOG} ${NRF_DOXY_LOG} ${NRF_SPHINX_LOG}
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

# Ensure that Zephyr build happens first
add_dependencies(nrf-doxy html)
add_dependencies(nrf-content html)
add_dependencies(nrf-kconfig html)
add_dependencies(nrf-html html nrf-doxy nrf-content nrf-kconfig)
add_custom_target(
  nrf
)
add_dependencies(nrf nrf-html)

