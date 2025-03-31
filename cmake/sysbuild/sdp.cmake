# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Apply snippet to both images. Assumption is that SDP image is called 'sdp'.
function(sdp_apply_snippets snippet)
  sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES ${snippet})
  sysbuild_cache_set(VAR sdp_SNIPPET APPEND REMOVE_DUPLICATES ${snippet})
endfunction()

function(sdp_apply_flpr_fault_timer_params timeout)
  set_config_bool(sdp CONFIG_SDP_MSPI_FAULT_TIMER y)
  set_config_bool(${DEFAULT_IMAGE} CONFIG_MSPI_NRFE_FAULT_TIMER y)
  set_config_int(${DEFAULT_IMAGE} CONFIG_MSPI_NRFE_FAULT_TIMEOUT ${timeout})
endfunction()

if(SB_CONFIG_SDP)
  if(SB_CONFIG_SDP_GPIO)
    if(SB_CONFIG_SDP_GPIO_BACKEND_MBOX)
      set(snippet_name "sdp-gpio-mbox")
    elseif(SB_CONFIG_SDP_GPIO_BACKEND_ICMSG)
      set(snippet_name "sdp-gpio-icmsg")
    elseif(SB_CONFIG_SDP_GPIO_BACKEND_ICBMSG)
      set(snippet_name "sdp-gpio-icbmsg")
    endif()
  elseif(SB_CONFIG_SDP_MSPI)
    set(snippet_name "sdp-mspi")
    if(SB_CONFIG_SDP_FLPR_APP_FAULT_TIMER)
      sdp_apply_flpr_fault_timer_params(${SB_CONFIG_SDP_FLPR_APP_FAULT_TIMEOUT})
    endif()
  endif()
  if(SB_CONFIG_SDP_DEVELOPER_MODE)
    set_config_bool(sdp CONFIG_SDP_DEVELOPER_MODE y)
  endif()

  sdp_apply_snippets(${snippet_name})
  set(snippet_name)
endif()
