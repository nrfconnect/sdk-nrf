# SPDX-License-Identifier: Apache-2.0

# Purpose of this CMake file is to install a NcsConfig package reference in:
# Unix/Linux/MacOS: ~/.cmake/packages/Ncs
# Windows         : HKEY_CURRENT_USER
#
# Having NcsConfig package allows for find_package(NCS) for out of tree projects.
#
# Create the reference by running `cmake -P ncs_export.cmake` in this directory.

set(MD5_INFILE "current_path.txt")

# We write CMAKE_CURRENT_LIST_DIR into MD5_INFILE, as the content of that file will be used for MD5 calculation.
# This means we effectively get the MD5 of CMAKE_CURRENT_LIST_DIR which must be used for CMake user package registry.
file(WRITE ${CMAKE_CURRENT_LIST_DIR}/${MD5_INFILE} ${CMAKE_CURRENT_LIST_DIR})
execute_process(COMMAND ${CMAKE_COMMAND} -E md5sum ${CMAKE_CURRENT_LIST_DIR}/${MD5_INFILE}
                OUTPUT_VARIABLE MD5_SUM
)
string(SUBSTRING ${MD5_SUM} 0 32 MD5_SUM)
if(WIN32)
  execute_process(COMMAND ${CMAKE_COMMAND}
                  -E  write_regv
                 "HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Packages\\Ncs\;${MD5_SUM}" "${CMAKE_CURRENT_LIST_DIR}"
)
else()
  file(WRITE $ENV{HOME}/.cmake/packages/Ncs/${MD5_SUM} ${CMAKE_CURRENT_LIST_DIR})
endif()

message("nRF Connect SDK (NCS) (${CMAKE_CURRENT_LIST_DIR})")
message("has been added to the user package registry in:")
if(WIN32)
  message("HKEY_CURRENT_USER\\Software\\Kitware\\CMake\\Packages\\Ncs\n")
else()
  message("~/.cmake/packages/Ncs\n")
endif()

file(REMOVE ${CMAKE_CURRENT_LIST_DIR}/${MD5_INFILE})
