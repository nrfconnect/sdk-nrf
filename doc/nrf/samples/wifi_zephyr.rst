.. _wifi_zephyr_samples:

Wi-Fi: Zephyr networking samples
################################

.. contents::
   :local:
   :depth: 2

In addition to |NCS| samples, it is possible to run selected networking samples with Wi-Fi®, provided and maintained as part of the upstream Zephyr project.
The following list specifies samples that are currently supported on :zephyr:board:`nrf7002dk` and :zephyr:board:`nrf7120dk`:

* :ref:`nrf_mqtt_sn_publisher_sample`
* :ref:`nrf_coap_server_sample`

.. toctree::
   :maxdepth: 1
   :hidden:

   ../../../samples/zephyr/net/mqtt_sn_publisher/README
   ../../../samples/zephyr/net/sockets/coap_server/README

Building and running
********************

To build the sample with |VSC|, follow the steps listed on the `How to build an application`_ page in the |nRFVSC| documentation.
See :ref:`building` for other building scenarios, :ref:`programming` for programming steps, and :ref:`testing` for general information about testing and debugging in the |NCS|.

A :ref:`Wi-Fi snippet <zephyr:snippet-wifi-ipv4>` configuration is provided to all Zephyr samples, which configures the sample to run with the Wi-Fi driver.

To build Zephyr samples, use the ``nrf7002dk/nrf5340/cpuapp`` or ``nrf7120dk/nrf7120/cpuapp`` board target, and ``wifi-ipv4`` snippet. 
See more details in the `Building and running`_ section of the respective sample.
