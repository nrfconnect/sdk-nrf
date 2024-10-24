# Copyright (c) 2024 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause

# Apply snippet to both images. Assumption is that SDP image is called 'sdp'.
function(sdp_apply_snippets snippet)
    sysbuild_cache_set(VAR ${DEFAULT_IMAGE}_SNIPPET APPEND REMOVE_DUPLICATES ${snippet})
    sysbuild_cache_set(VAR sdp_SNIPPET APPEND REMOVE_DUPLICATES ${snippet})
endfunction()

if(SB_CONFIG_SDP)
  if(SB_CONFIG_SDP_GPIO)
    if(SB_CONFIG_SDP_GPIO_BACKEND_MBOX)
      set(snip "sdp-gpio-mbox")
    elseif(SB_CONFIG_SDP_GPIO_BACKEND_ICMSG)
      set(snip "sdp-gpio-icmsg")
    elseif(SB_CONFIG_SDP_GPIO_BACKEND_ICBMSG)
      set(snip "sdp-gpio-icbmsg")
    endif()
  endif()

  sdp_apply_snippets(${snip})
endif()
