#
# Copyright (c) 2026 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#
if(NOT EXISTS ${NRF_DIR}/release.yaml)
  return()
endif()

yaml_load(FILE ${NRF_DIR}/release.yaml NAME device_support)

yaml_length(device_length NAME device_support KEY devices)
foreach(i RANGE ${device_length})
  yaml_get(soc NAME device_support KEY devices ${i} soc)
  if(soc STREQUAL SOC_NAME)
    yaml_get(status NAME device_support KEY devices ${i} status)
    if(status STREQUAL "pre-release")
    elseif(status STREQUAL "maintenance")
    elseif(status STREQUAL "community")
    elseif(status STREQUAL "unsupported")
    endif()
    break()
  endif()
endforeach()

if(NOT soc)
  message(WARNING "SoC ${SOC_NAME} is not supported by this release.")
endif()
