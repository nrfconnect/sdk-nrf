#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

#
# Add nordic-flpr snippet for the radio_test
# application, so it starts up the VPR core
# which runs the modem_bypass
#

if(SB_CONFIG_APPCORE_RADIO_TEST)
if(NOT "nordic-flpr" IN_LIST radio_test_SNIPPET)
  list(APPEND radio_test_SNIPPET nordic-flpr)
  set(radio_test_SNIPPET ${radio_test_SNIPPET} CACHE STRING "" FORCE)
endif()
endif()
