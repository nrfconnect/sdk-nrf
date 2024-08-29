#
# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

# Usage:
#   ot_report_git_version(<output> <directory>)
#
# This function uses the git describe command to read the current revision
# of the <directory> directory and saves the result in the <output> variable
#
#
# directory: Directory that contains checked out the git repository of which
#            you want to read the revision.
# output: Output variable that will contain the read revision.
#
function(ot_report_git_version output directory)
    execute_process(
        COMMAND git describe --dirty --always
        WORKING_DIRECTORY ${directory}
        OUTPUT_VARIABLE GIT_REV OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    set(${output} "${GIT_REV}" PARENT_SCOPE)
endfunction()

# Usage:
#   ot_report_git_head_sha(<output> <directory>)
#
# This function uses the git rev-parse command to read the current revision SHA
# of the <directory> directory and saves the result in the <output> variable
#
#
# directory: Directory that contains checked out the git repository of which
#            you want to read the revision SHA.
# output: Output variable that will contain the read revision SHA.
#
function(ot_report_git_head_sha output directory)
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${directory}
        OUTPUT_VARIABLE GIT_REV OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    set(${output} "${GIT_REV}" PARENT_SCOPE)
endfunction()

# Usage:
#   ot_report_add_message(<message> <to_stdout>)
#
# This function writes the <message> text to the file which is defined by
# the CONFIG_OPENTHREAD_REPORT_BUILD_ARTEFACT_NAME kconfig value located in the
# application build directory.
#
#
# If the <to_stdout> argument is set to TRUE then the message will be printed
# to the stdout console.
#
#
# message: Text message to be written
# to_stdout: Boolean to decide whether the text message should be shown in the
#            std console.
#
function(ot_report_add_message message to_stdout)
    if(NOT ${message} STREQUAL "")
        set(ARTEFACT ${CMAKE_BINARY_DIR}/${CONFIG_OPENTHREAD_REPORT_BUILD_ARTEFACT_NAME})
        file(APPEND ${ARTEFACT} ${message}\n)
        if(${to_stdout})
            message(${message})
        endif()
    endif()
endfunction()

# Usage:
#   ot_report_git_diff(<output> <base_revision> <base_directory> <subdirectory>)
#
# This function generates git difference between the <base_revision> and the current
# revision that is checked out within the <base_directory>.
# You can provide the <subdirectory> argument to narrow down the scope of the diff
# function.
#
# For example to compare the current revision of the nrfxlib to the state from
# NCS 2.7.0 release and take into account only the openthread directory use the following
# invocation:
#
# ot_report_git_diff(MY_VARIABLE v2.7.0 ${ZEPHYR_NRFXLIB_MODULE_DIR} "openthread")
#
# OUTPUT: Output variable that contains the git diff.
# base_revision: The revision to be compared with the current one.
# base_directory: Directory that contains the git repository of which
#                 you want to read the revision.
# subdirectory: A subdirectory within the base_directory to be checked by the git
#               diff command.
#
function(ot_report_git_diff output base_revision base_directory subdirectory)
    execute_process(
        COMMAND git diff ${base_revision} -- ":${subdirectory}/*.a" ":${subdirectory}/*.h"
        WORKING_DIRECTORY ${base_directory}
        OUTPUT_VARIABLE GIT_REV OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    set(${output} "${GIT_REV}" PARENT_SCOPE)
endfunction()
