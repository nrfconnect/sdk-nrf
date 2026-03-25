#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
# Rewrite the hex_file: line in runners.yaml to point at merged.hex.
# Invoked at build time from cmake/keys.cmake.

if(NOT EXISTS "${RUNNERS_YAML}")
  return()
endif()

file(READ "${RUNNERS_YAML}" content)
string(REGEX REPLACE
  "(\n[ \t]*hex_file:)[ \t]*[^\n]*"
  "\\1 ${MERGED_HEX}"
  content_new "${content}")
file(WRITE "${RUNNERS_YAML}" "${content_new}")
