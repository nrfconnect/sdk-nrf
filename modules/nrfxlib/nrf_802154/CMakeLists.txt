#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)
  if(CONFIG_NRF_802154_SL)
    if(CONFIG_SOC_COMPATIBLE_NRF52X OR CONFIG_SOC_COMPATIBLE_NRF53X)
      target_sources(nrf-802154-platform
        PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}/sl/platform/nrf_802154_hp_timer.c
          ${CMAKE_CURRENT_SOURCE_DIR}/sl/platform/nrf_802154_platform_sl_lptimer_zephyr.c
      )
    elseif(CONFIG_SOC_SERIES_NRF54H OR CONFIG_SOC_SERIES_NRF54L)
      target_sources(nrf-802154-platform
        PRIVATE
          ${CMAKE_CURRENT_SOURCE_DIR}/sl/platform/nrf_802154_platform_timestamper.c
          ${CMAKE_CURRENT_SOURCE_DIR}/sl/platform/nrf_802154_platform_sl_lptimer_grtc.c
          ${CMAKE_CURRENT_SOURCE_DIR}/sl/platform/nrf_802154_platform_sl_lptimer_grtc_hw_task.c
      )
    endif()
  endif()

  if(CONFIG_MPSL)
    target_compile_definitions(nrf-802154-platform PUBLIC NRF_802154_VERIFY_PERIPHS_ALLOC_AGAINST_MPSL=1)
  endif()

  set(NRF53_SERIES  ${CONFIG_SOC_SERIES_NRF53})
  set(SER_HOST      ${CONFIG_NRF_802154_SER_HOST})
  set(SL_OPENSOURCE ${CONFIG_NRF_802154_SL_OPENSOURCE})

  if(CONFIG_NRF_802154_SOURCE_NRFXLIB)
    add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/nrf_802154 nrf_802154)

    target_link_libraries(nrf-802154-driver-interface INTERFACE zephyr-802154-interface)
    target_link_libraries(nrf-802154-serialization-interface INTERFACE zephyr-802154-interface)
  endif(CONFIG_NRF_802154_SOURCE_NRFXLIB)

  target_compile_definitions(zephyr-802154-interface
    INTERFACE
      NRF_802154_ECB_PRIORITY=-1
      NRF_802154_SWI_PRIORITY=1
      NRF_802154_CCAIDLE_TO_TXEN_EXTRA_TIME_US=${CONFIG_NRF_802154_CCAIDLE_TO_TXEN_EXTRA_TIME_US}
      NRF_802154_CSMA_CA_MIN_BE_DEFAULT=${CONFIG_NRF_802154_CSMA_CA_MIN_BE_DEFAULT}
      NRF_802154_CSMA_CA_MAX_BE_DEFAULT=${CONFIG_NRF_802154_CSMA_CA_MAX_BE_DEFAULT}
      NRF_802154_CSMA_CA_MAX_CSMA_BACKOFFS_DEFAULT=${CONFIG_NRF_802154_CSMA_CA_MAX_CSMA_BACKOFFS_DEFAULT}
  )

  if(CONFIG_NRF_802154_TX_DIAGNOSTIC_MODE)
    target_compile_definitions(zephyr-802154-interface
      INTERFACE
        NRF_802154_TX_DIAGNOSTIC_MODE=1)
  endif()

  if(CONFIG_NRF_802154_ACK_TIMEOUT_CUSTOM_US)
    target_compile_definitions(zephyr-802154-interface
      INTERFACE
        NRF_802154_PRECISE_ACK_TIMEOUT_DEFAULT_TIMEOUT=${CONFIG_NRF_802154_ACK_TIMEOUT_CUSTOM_US})
  endif()
endif(CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)
