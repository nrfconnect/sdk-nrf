.. _wifi_zephyr_samples:

Wi-Fi: Zephyr networking samples
################################

.. contents::
   :local:
   :depth: 2

In addition to |NCS| samples, it is possible to run selected networking samples with Wi-Fi®, provided and maintained as part of the upstream Zephyr project.
The following list specifies samples that are currently supported with the Wi-Fi driver:

* :zephyr:code-sample:`dhcpv4-client`
* :zephyr:code-sample:`dns-resolve`
* :zephyr:code-sample:`ipv4-autoconf`
* :zephyr:code-sample:`mdns-responder`
* :zephyr:code-sample:`mqtt-publisher`
* :zephyr:code-sample:`mqtt-sn-publisher`
* :zephyr:code-sample:`coap-client`
* :zephyr:code-sample:`coap-server`
* :zephyr:code-sample:`sockets-echo`
* :zephyr:code-sample:`async-sockets-echo`
* :zephyr:code-sample:`sockets-echo-client`
* :zephyr:code-sample:`sockets-echo-server`
* :zephyr:code-sample:`sockets-http-get`
* :zephyr:code-sample:`sntp-client`
* :zephyr:code-sample:`syslog-net`
* :zephyr:code-sample:`telnet-console`

Configuration
*************

|config|

Before you build a sample, you must configure the following Wi-Fi credentials in the :file:`overlay-nrf700x.conf` file:

* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_SSID` - Network name (SSID)
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_PASSWORD` - Password
* :kconfig:option:`CONFIG_WIFI_CREDENTIALS_STATIC_TYPE` - Security type (Optional)

.. note::
   You can also use ``menuconfig`` to configure Wi-Fi credentials.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

Building and running
********************

See :ref:`building` for the available building scenarios, :ref:`programming` for programming steps, and :ref:`testing` for general information about testing and debugging in the |NCS|.

To build any of the listed samples with the Wi-Fi driver, use the following variables for the chosen hardware platform:

.. list-table::
  :header-rows: 1

  * - Hardware platform
    - Build target
    - Files to use
    - Build variables
  * - nRF7002 DK
    - ``nrf7002dk_nrf5340_cpuapp``
    - :file:`overlay-nrf700x.conf`
    - ``-DEXTRA_CONF_FILE=overlay-nrf700x.conf``
  * - nRF7002 EK with nRF5340 DK
    - ``nrf5340dk_nrf5340_cpuapp``
    - :file:`overlay-nrf700x.conf`
    - * ``-DEXTRA_CONF_FILE=overlay-nrf700x.conf``
      * ``-DSHIELD=nrf7002ek``

.. |variable_feature| replace:: the first of the provided variants
.. |makevar| replace:: EXTRA_CONF_FILE
.. |cmake_file_name| replace:: overlay-nrf700x.conf
.. |board_name| replace:: nrf7002dk_nrf5340_cpuapp

.. include:: /includes/apply_cmake_variable.txt

For additional details about running a sample, refer to the respective sample in Zephyr's Samples and Demos documentation.
