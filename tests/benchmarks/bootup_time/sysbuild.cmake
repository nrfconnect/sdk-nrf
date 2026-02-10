if(SB_CONFIG_NETCORE_TESTAPP)
  set_config_bool(bootup_time CONFIG_SOC_NRF54H20_CPURAD_ENABLE y)
endif()

if(SB_CONFIG_PPRCORE_TESTAPP)
  add_overlay_dts(bootup_time enable_ppr.overlay)
endif()

if(SB_CONFIG_FLPRCORE_TESTAPP)
  add_overlay_dts(bootup_time enable_flpr.overlay)
endif()
