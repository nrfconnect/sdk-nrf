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
      message(WARNING "SoC ${soc} is pre-release. Not suitable for production.")
    elseif(status STREQUAL "maintenance")
      yaml_get(release NAME device_support KEY devices ${i} release)
      message(STATUS
        "SoC ${soc} is in maintenance mode. Consider using latest tag on v${release}-series instead."
      )
    elseif(status STREQUAL "community")
      message(WARNING "SoC ${soc} is maintained by the Zephyr community and not tested by this release.")
    elseif(status STREQUAL "unsupported")
      message(WARNING "SoC ${soc} is not supported by this release.")
    endif()
    break()
  endif()
endforeach()

if(NOT soc)
  message(WARNING "SoC ${SOC_NAME} is not supported by this release.")
endif()
