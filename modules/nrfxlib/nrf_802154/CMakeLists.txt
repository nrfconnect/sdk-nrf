#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if (CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)
  if (NOT CONFIG_NRF_802154_SL_OPENSOURCE AND NOT CONFIG_SOC_SERIES_NRF53X)
    target_sources(nrf-802154-platform
      PRIVATE
        ${ZEPHYR_CURRENT_MODULE_DIR}/nrf_802154/sl/platform/hp_timer/nrf_802154_hp_timer.c
        ${ZEPHYR_CURRENT_MODULE_DIR}/nrf_802154/sl/platform/lp_timer/nrf_802154_lp_timer.c
    )
  endif ()

  if (CONFIG_MPSL)
    target_compile_definitions(nrf-802154-platform PUBLIC NRF_802154_VERIFY_PERIPHS_ALLOC_AGAINST_MPSL=1)
  endif()

  set(NRF52_SERIES  ${CONFIG_SOC_SERIES_NRF52X})
  set(NRF53_SERIES  ${CONFIG_SOC_SERIES_NRF53X})
  set(SER_HOST      ${CONFIG_NRF_802154_SER_HOST})
  set(SL_OPENSOURCE ${CONFIG_NRF_802154_SL_OPENSOURCE})

  add_subdirectory(${ZEPHYR_CURRENT_MODULE_DIR}/nrf_802154 nrf_802154)
  target_link_libraries(nrf-802154-driver-interface INTERFACE zephyr-802154-interface)
  target_link_libraries(nrf-802154-serialization-interface INTERFACE zephyr-802154-interface)
endif (CONFIG_NRF_802154_RADIO_DRIVER OR CONFIG_NRF_802154_SERIALIZATION)
