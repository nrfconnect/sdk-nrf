#
# Copyright (c) 2026 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

foreach(scenario SYNC RTIO_QUEUED RTIO_CALLBACK RTIO_HOLD_ON_CS)
  if(SB_CONFIG_TEST_SCENARIO_${scenario})
    set(test_scenario ${scenario})
  endif()
endforeach()

set(test_images ${DEFAULT_IMAGE})

if(SB_CONFIG_FLPRCORE_TESTAPP)
  list(APPEND test_images ${SB_CONFIG_FLPRCORE_IMAGE_NAME})
endif()

foreach(image ${test_images})
  set_config_int(${image} CONFIG_TEST_SPI_MODE ${SB_CONFIG_TEST_SPI_MODE})
  set_config_bool(${image} CONFIG_TEST_SCENARIO_${test_scenario} y)
endforeach()
