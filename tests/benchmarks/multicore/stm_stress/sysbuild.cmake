if(SB_CONFIG_NETCORE_TESTAPP)
  set_config_bool(stm_stress CONFIG_SOC_NRF54H20_CPURAD_ENABLE y)
endif()

if(SB_CONFIG_PPRCORE_TESTAPP)
  add_overlay_dts(stm_stress enable_ppr.overlay)
endif()

if(SB_CONFIG_FLPRCORE_TESTAPP)
  add_overlay_dts(stm_stress enable_flpr.overlay)
endif()
