.. _wifi_zephyr_samples:

Wi-Fi: Zephyr networking samples
################################

.. contents::
   :local:
   :depth: 2

In addition to |NCS| samples, it is possible to run selected networking samples with Wi-Fi®, provided and maintained as part of the upstream Zephyr project.
The following list specifies samples that are currently supported with the Wi-Fi driver:

* :zephyr:code-sample:`dns-resolve`
* :zephyr:code-sample:`ipv4-autoconf`
* :zephyr:code-sample:`mdns-responder`
* :zephyr:code-sample:`mqtt-publisher`
* :zephyr:code-sample:`mqtt-sn-publisher`
* :zephyr:code-sample:`coap-server`
* :zephyr:code-sample:`async-sockets-echo`
* :zephyr:code-sample:`sockets-echo-client`
* :zephyr:code-sample:`sockets-echo-server`
* :zephyr:code-sample:`sockets-http-get`
* :zephyr:code-sample:`sntp-client`
* :zephyr:code-sample:`syslog-net`
* :zephyr:code-sample:`telnet-console`

Building and running
********************

To build the sample with |VSC|, follow the steps listed on the `How to build an application`_ page in the |nRFVSC| documentation.
See :ref:`building` for other building scenarios, :ref:`programming` for programming steps, and :ref:`testing` for general information about testing and debugging in the |NCS|.

A :ref:`Wi-Fi snippet <zephyr:snippet-wifi-ipv4>` configuration is provided to all Zephyr samples, which configures the sample to run with the Wi-Fi driver.

To build Zephyr samples for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf7002dk/nrf5340/cpuapp -S wifi-ipv4

To build for the nRF7002 EK with nRF5340 DK, use the ``nrf5340dk/nrf5340/cpuapp`` board target with the ``SHIELD`` CMake option set to ``nrf7002ek``.
The following is an example of the CLI command:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -S wifi-ipv4 -- -DSHIELD=nrf7002ek -DSB_CONFIG_WIFI_NRF70=y

For additional details about running a sample, refer to the respective sample in Zephyr’s :ref:`Samples and Demos documentation <zephyr:samples-and-demos>`.
