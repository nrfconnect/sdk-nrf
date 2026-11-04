# nRF54H does not support wifi-enterprise
if(NOT SB_CONFIG_SOC_SERIES_NRF54H AND NOT "wifi-enterprise" IN_LIST ${DEFAULT_IMAGE}_SNIPPET)
  list(APPEND ${DEFAULT_IMAGE}_SNIPPET ${SNIPPET})
  list(APPEND ${DEFAULT_IMAGE}_SNIPPET wifi-enterprise)
  set(${DEFAULT_IMAGE}_SNIPPET ${${DEFAULT_IMAGE}_SNIPPET} CACHE STRING "" FORCE)
endif()

# The enterprise credential copies that Zephyr reserves in the system heap do
# not fit in the nRF5340 RAM with the data heap of the wifi-enterprise snippet.
if(SB_CONFIG_SOC_SERIES_NRF53)
  set_config_int(${DEFAULT_IMAGE} CONFIG_NRF_WIFI_DATA_HEAP_SIZE 88000)
endif()
