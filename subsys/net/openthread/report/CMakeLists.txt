#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

include(report_generation_utils.cmake)
include(${ZEPHYR_NRFXLIB_MODULE_DIR}/openthread/cmake/extensions.cmake)

# Remove the existing file before the next report generation
set(ARTEFACT ${CMAKE_BINARY_DIR}/${CONFIG_OPENTHREAD_REPORT_BUILD_ARTEFACT_NAME})
if(EXISTS ${ARTEFACT})
    file(REMOVE ${ARTEFACT})
endif()

ot_report_add_message("################### OPENTHREAD REPORT ###################" TRUE)

# Add basic information
ot_report_add_message("+ Target device: ${CONFIG_SOC}" TRUE)
ot_report_add_message("+ Thread version: v${CONFIG_OPENTHREAD_THREAD_VERSION}" TRUE)

# Add feature set
if(DEFINED CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER)
    set(FEATURE_SET "Master")
elseif(DEFINED CONFIG_OPENTHREAD_NORDIC_LIBRARY_FTD)
    set(FEATURE_SET "Full Thread Device (FTD)")
elseif(DEFINED CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD)
    set(FEATURE_SET "Minimal Thread Device (MTD)")
elseif(DEFINED CONFIG_OPENTHREAD_NORDIC_LIBRARY_RCP)
    set(FEATURE_SET "Radio Coprocessor (RCP)")
else()
    set(FEATURE_SET "Custom")
endif()
ot_report_add_message("+ OpenThread library feature set: ${FEATURE_SET}" TRUE)

# Check whether OpenThread is built from library or from sources
if(CONFIG_OPENTHREAD_SOURCES)
    ot_report_add_message("+ OpenThread library has been built from sources" TRUE)
else()
    openthread_calculate_lib_path("v${CONFIG_OPENTHREAD_THREAD_VERSION}" LIB_PATH)
    cmake_path(RELATIVE_PATH LIB_PATH BASE_DIRECTORY ${ZEPHYR_NRFXLIB_MODULE_DIR} OUTPUT_VARIABLE LIB_PATH_REL)
    ot_report_add_message("+ OpenThread Library: ${LIB_PATH_REL}" TRUE)
endif()

# Get OpenThread revision
set(OPENTHREAD_VERSION "" CACHE STRING "OpenThread revision")
if(OPENTHREAD_VERSION STREQUAL "")
    ot_report_git_version(OPENTHREAD_VERSION ${ZEPHYR_OPENTHREAD_MODULE_DIR})
    ot_report_add_message("+ OpenThread NCS revision: ${OPENTHREAD_VERSION}" TRUE)
endif()
set(OPENTHREAD_SHA "" CACHE STRING "OpenThread revision")
if(OPENTHREAD_SHA STREQUAL "")
    ot_report_git_head_sha(OPENTHREAD_SHA ${ZEPHYR_OPENTHREAD_MODULE_DIR})
    ot_report_add_message("+ OpenThread NCS SHA: ${OPENTHREAD_SHA}" TRUE)
endif()

# Get NCS revision
set(NCS_VERSION "" CACHE STRING "NCS revision")
if(NCS_VERSION STREQUAL "")
    ot_report_git_version(NCS_VERSION ${ZEPHYR_NRF_MODULE_DIR})
    ot_report_add_message("+ NCS revision: ${NCS_VERSION}" TRUE)
endif()
set(NCS_SHA "" CACHE STRING "NCS revision")
if(NCS_SHA STREQUAL "")
    ot_report_git_head_sha(NCS_SHA ${ZEPHYR_NRF_MODULE_DIR})
    ot_report_add_message("+ NCS SHA: ${NCS_SHA}" TRUE)
endif()

# Diff of nrfxlib directory between the current and the latest NCS release revisions.
if(NOT DEFINED CONFIG_OPENTHREAD_SOURCES)
    # Get NCS version from openthread lib configuration file.
    file(READ "${LIB_PATH}/openthread_lib_configuration.txt" INSTANCE_FILE_CONTENT)
    string(REGEX MATCH "NRFXLIB_RELEASE_TAG=([^ \n]+)" MACRO_DEF "${INSTANCE_FILE_CONTENT}")
    if(NOT ${CMAKE_MATCH_1} STREQUAL "")
        set(LATEST_NCS_RELEASE ${CMAKE_MATCH_1})
    else()
        # Read NRFXLIB commit version if NCS_VERSION is not available - backward compatibility for NCS 2.7.0
        string(REGEX MATCH "NRFXLIB_commit=([^ -]+)" MACRO_DEF "${INSTANCE_FILE_CONTENT}")
        set(LATEST_NCS_RELEASE ${CMAKE_MATCH_1})
    endif()

    set(NRFXLIB_DIFF "" CACHE STRING "nrfxlib diff to the recent revision")
    ot_report_git_diff(NRFXLIB_DIFF ${LATEST_NCS_RELEASE} ${ZEPHYR_NRFXLIB_MODULE_DIR} "openthread")

    if(NOT NRFXLIB_DIFF STREQUAL "")
        ot_report_add_message("+ Found differences in the nrfxlib repository in comparison to the NCS ${LATEST_NCS_RELEASE} release. See the ${CONFIG_OPENTHREAD_REPORT_BUILD_ARTEFACT_NAME} report file to learn more." TRUE)
        ot_report_add_message(${NRFXLIB_DIFF} FALSE)
    else()
        ot_report_add_message("+ No differences in the used Thread library in comparison to the NCS ${LATEST_NCS_RELEASE} release." TRUE)
    endif()
endif()

ot_report_add_message("###################        END        ###################" TRUE)
