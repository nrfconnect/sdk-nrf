#
# Copyright (c) 2021 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT fota/settings.c)

zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_FIRMWARE_UPDATE_OBJ_SUPPORT lwm2m/lwm2m_firmware.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_CONN_MON_OBJ_SUPPORT lwm2m/lwm2m_connmon.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_DEVICE_OBJ_SUPPORT lwm2m/lwm2m_device.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_SECURITY_OBJ_SUPPORT lwm2m/lwm2m_security.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_SIGNAL_MEAS_INFO_OBJ_SUPPORT
			     lwm2m/ecid_signal_meas_info.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_NEIGHBOUR_CELL_LISTENER
			     lwm2m/neighbour_cell_info.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_OBJ_SUPPORT
			     lwm2m/location_assistance_obj.c)
zephyr_library_sources_ifdef(CONFIG_LWM2M_CLIENT_UTILS_LOCATION_ASSIST_EVENTS
			     location/location_events.c
			     location/location_event_handler.c)
