#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(serial_lte_modem)

# NORDIC SDK APP START
target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/slm_util.c)
target_sources(app PRIVATE src/slm_settings.c)
target_sources(app PRIVATE src/slm_at_host.c)
target_sources(app PRIVATE src/slm_at_commands.c)
target_sources(app PRIVATE src/slm_at_socket.c)
target_sources(app PRIVATE src/slm_at_tcp_proxy.c)
target_sources(app PRIVATE src/slm_at_udp_proxy.c)
target_sources(app PRIVATE src/slm_at_icmp.c)
target_sources(app PRIVATE src/slm_at_fota.c)
# NORDIC SDK APP END
target_sources_ifdef(CONFIG_SLM_SMS app PRIVATE src/slm_at_sms.c)
target_sources_ifdef(CONFIG_SLM_NATIVE_TLS app PRIVATE src/slm_native_tls.c)
target_sources_ifdef(CONFIG_SLM_NATIVE_TLS app PRIVATE src/slm_at_cmng.c)

add_subdirectory_ifdef(CONFIG_SLM_GNSS src/gnss)
add_subdirectory_ifdef(CONFIG_SLM_NRF_CLOUD src/nrfcloud)
add_subdirectory_ifdef(CONFIG_SLM_FTPC src/ftp_c)
add_subdirectory_ifdef(CONFIG_SLM_MQTTC src/mqtt_c)
add_subdirectory_ifdef(CONFIG_SLM_HTTPC src/http_c)
add_subdirectory_ifdef(CONFIG_SLM_TWI src/twi)
add_subdirectory_ifdef(CONFIG_SLM_GPIO src/gpio)
add_subdirectory_ifdef(CONFIG_SLM_CARRIER src/lwm2m_carrier)

zephyr_include_directories(src)
