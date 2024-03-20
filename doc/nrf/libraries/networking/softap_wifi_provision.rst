.. _lib_softap_wifi_provision:

SoftAP Wi-Fi provision
######################

.. contents::
   :local:
   :depth: 2

The SoftAP Wi-Fi provision library provides means to provision a Wi-Fi® device over HTTPS using softAP mode.

Configuration
*************

To use the SoftAP Wi-Fi provision library, you must enable the :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION` Kconfig option.

The following Kconfig options are available:

* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATE_REGISTER`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SERVER_CERTIFICATES_FOLDER`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_RESPONSE_BUFFER_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_TCP_RECV_BUF_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SCAN_RESULT_BUFFER_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_THREAD_STACK_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_URL_MAX_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_BODY_MAX_SIZE`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SSID`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_SOCKET_CLOSE_ON_COMPLETION`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_CERTIFICATE_SEC_TAG`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_TCP_PORT`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_IPV4_ADDRESS`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_IPV4_NETMASK`
* :kconfig:option:`CONFIG_SOFTAP_WIFI_PROVISION_MESSAGE_QUEUE_ENTRIES`

API documentation
*****************

| Header file: :file:`include/net/softap_wifi_provision.h`
| Source files: :file:`subsys/net/lib/softap_wifi_provision`

.. doxygengroup:: softap_wifi_provision_library
   :project: nrf
   :members:
