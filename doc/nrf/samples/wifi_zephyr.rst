.. _wifi_zephyr_samples:

Wi-Fi: Zephyr networking samples
################################

.. contents::
   :local:
   :depth: 2

In addition to |NCS| samples, it is possible to run selected networking samples with Wi-Fi®, provided and maintained as part of the upstream Zephyr project.
The following list specifies samples that are currently supported with the Wi-Fi driver:

* :ref:`dhcpv4-client-sample`
* :ref:`dns-resolve-sample`
* :ref:`ipv4-autoconf-sample`
* :ref:`mdns-responder-sample`
* :ref:`mqtt-publisher-sample`
* :ref:`mqtt-sn-publisher-sample`
* :ref:`coap-client-sample`
* :ref:`coap-server-sample`
* :ref:`sockets-echo-sample`
* :ref:`async-sockets-echo-sample`
* :ref:`sockets-echo-client-sample`
* :ref:`sockets-echo-server-sample`
* :ref:`sockets-http-get`
* :ref:`sntp-client-sample`
* :ref:`syslog-net-sample`
* :ref:`telnet-console-sample`

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

To build the sample with |VSC|, follow the steps listed on the `How to build an application`_ page in the |nRFVSC| documentation.
See :ref:`programming` for other building and programming scenarios and :ref:`testing` for general information about testing and debugging in the |NCS|.

An overlay file, ``overlay-nrf700x.conf`` is provided to all Zephyr samples, which configures the sample to run with the Wi-Fi driver.

To build Zephyr samples for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk_nrf5340_cpuapp -- -DOVERLAY_CONFIG=overlay-nrf700x.conf

To build for the nRF7002 EK with nRF5340 DK, use the ``nrf5340dk_nrf5340_cpuapp`` build target with the ``SHIELD`` CMake option set to ``nrf7002ek``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002ek -DOVERLAY_CONFIG=overlay-nrf700x.conf

For additional details about running a sample, refer to the respective sample in Zephyr’s Samples and Demos documentation.
