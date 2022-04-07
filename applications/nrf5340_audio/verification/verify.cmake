#
# Copyright (c) 2018 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

set(REQ_C_COMPILER_VERSION 10.3.1)
set(REQ_C_COMPILER arm-none-eabi-gcc)
set(DISCLAIMER_TXT "Using alternative tools may work but has not been tested. \n"
    "These checks can be ignored, but this may impact performance and stability")

if (NOT ${CMAKE_C_COMPILER} MATCHES ${REQ_C_COMPILER})
        message(WARNING "The compiler is ${CMAKE_C_COMPILER} \
         but should be of type: ${REQ_C_COMPILER}. ${DISCLAIMER_TXT}")
endif()

if(NOT CMAKE_C_COMPILER_VERSION)
    message(FATAL_ERROR "CMAKE_C_COMPILER_VERSION was not automatically detected. \n"
        "Is GNU Arm Embedded Toolchain installed?. ${DISCLAIMER_TXT}")
elseif(NOT (CMAKE_C_COMPILER_VERSION VERSION_EQUAL ${REQ_C_COMPILER_VERSION}))
    message(WARNING "The C compiler version is: ${CMAKE_C_COMPILER_VERSION}. \n"
        "It should be: ${REQ_C_COMPILER_VERSION}. ${DISCLAIMER_TXT}")
endif()

# gnuarmemb toochain can be found at
# https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
