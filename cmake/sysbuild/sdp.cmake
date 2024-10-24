# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

function(sdp_apply_overlays)
  if(SB_CONFIG_EGPIO_BACKEND_MBOX)
    sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES "sdp-gpio-mbox")
  elseif(SB_CONFIG_EGPIO_BACKEND_ICMSG)
    sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES "sdp-gpio-icmsg")
    sysbuild_cache_set(VAR sdp_SNIPPET APPEND REMOVE_DUPLICATES "sdp-gpio-icmsg")
  elseif(SB_CONFIG_EGPIO_BACKEND_ICBMSG)
    sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES "sdp-gpio-icbmsg")
    sysbuild_cache_set(VAR sdp_SNIPPET APPEND REMOVE_DUPLICATES "sdp-gpio-icbmsg")
  endif()
endfunction()

if(SB_CONFIG_SDP)
  if(SB_CONFIG_SDP_GPIO)
    sdp_apply_overlays()
  endif()
endif()
