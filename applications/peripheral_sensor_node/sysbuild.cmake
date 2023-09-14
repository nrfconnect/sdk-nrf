#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

list(LENGTH SHIELD SHIELD_NUM)
if(SHIELD_NUM EQUAL 0)
  message(FATAL_ERROR "Missing configuration for the ${NORMALIZED_BOARD_TARGET}. It requires additional shield or snippet")
endif()
