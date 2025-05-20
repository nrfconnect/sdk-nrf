#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT lwm2m/lwm2m_firmware.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT lwm2m/lwm2m_connmon.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_DEVICE_OBJ_SUPPORT lwm2m/lwm2m_device.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_SECURITY_OBJ_SUPPORT lwm2m/lwm2m_security.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT
			     lwm2m/ecid_signal_meas_info.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_CELL_CONN_OBJ_SUPPORT
			     lwm2m/lwm2m_cellconn_obj.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_GROUND_FIX_OBJ_SUPPORT
			     lwm2m/ground_fix_obj.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_GNSS_ASSIST_OBJ_SUPPORT
			     lwm2m/gnss_assistance_obj.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSISTANCE
			     location/location_assistance.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSISTANCE
			     location/location_retry_timer.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_RAI
			     lwm2m/lwm2m_rai.c)
zephyr_library_sources(lwm2m/lwm2m_lte_notification.c)
zephyr_library_sources(lwm2m/lwm2m_utils_con_management.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_ADV_FIRMWARE_UPDATE_OBJ_SUPPORT lwm2m/lwm2m_adv_firmware.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_WIFI_AP_SCANNER location/location_wifi_ap_scanner.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_VISIBLE_WIFI_AP_OBJ_SUPPORT lwm2m/visible_wifi_ap.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_LTE_CONNEVAL lwm2m/lwm2m_conneval.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_LOCATION_OBJ_SUPPORT lwm2m_obj_location_optional.c)
zephyr_include_directories(lwm2m/include)

# For strnlen()
zephyr_library_compile_definitions(_POSIX_C_SOURCE=200809L)
