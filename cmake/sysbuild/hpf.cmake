# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Apply snippet to both images.
function(hpf_apply_snippets snippet)
  sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES ${snippet})
  sysbuild_cache_set(VAR ${SB_CONFIG_FLPRCORE_IMAGE_NAME}_SNIPPET APPEND REMOVE_DUPLICATES ${snippet})
endfunction()

function(hpf_apply_flpr_fault_timer_params timeout)
  set_config_bool(${SB_CONFIG_FLPRCORE_IMAGE_NAME} CONFIG_HPF_MSPI_FAULT_TIMER y)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_MSPI_HPF_FAULT_TIMER y)
  set_config_int(${DEFAULT_IMAGE} CONFIG_MSPI_HPF_FAULT_TIMEOUT ${timeout})
endfunction()

if(SB_CONFIG_HPF)
  if(SB_CONFIG_HPF_GPIO)
    if(SB_CONFIG_HPF_GPIO_BACKEND_MBOX)
      set(snippet_name "hpf-gpio-mbox")
    elseif(SB_CONFIG_HPF_GPIO_BACKEND_ICMSG)
      set(snippet_name "hpf-gpio-icmsg")
    elseif(SB_CONFIG_HPF_GPIO_BACKEND_ICBMSG)
      set(snippet_name "hpf-gpio-icbmsg")
    endif()
  elseif(SB_CONFIG_HPF_MSPI)
    set(snippet_name "hpf-mspi")
    if(SB_CONFIG_HPF_FLPR_APP_FAULT_TIMER)
      hpf_apply_flpr_fault_timer_params(${SB_CONFIG_HPF_FLPR_APP_FAULT_TIMEOUT})
    endif()
  endif()
  if(SB_CONFIG_HPF_DEVELOPER_MODE)
    set_config_bool(${SB_CONFIG_FLPRCORE_IMAGE_NAME} CONFIG_HPF_DEVELOPER_MODE y)
  endif()

  if(SB_CONFIG_HPF_APPLY_SNIPPET)
    hpf_apply_snippets(${snippet_name})
  endif()
  set(snippet_name)
endif()
