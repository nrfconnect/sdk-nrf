#
# Copyright (c) 2025 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if("thread-platform" IN_LIST template_SNIPPET)
  # If the thread-platform snippet is used, we need to have
  # the "diagnostic-logs" and "matter-debug" snippets as well.
  set(${DEFAULT_IMAGE}_SNIPPET ${${DEFAULT_IMAGE}_SNIPPET} diagnostic-logs  CACHE STRING "" FORCE)
  set(${DEFAULT_IMAGE}_SNIPPET ${${DEFAULT_IMAGE}_SNIPPET} matter-debug  CACHE STRING "" FORCE)
endif()
