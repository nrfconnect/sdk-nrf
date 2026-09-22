.. _nrf_coap_server_sample:

.. ncs-sample::
   :title: CoAP Server

   The CoAP Server sample demonstrates how to register CoAP resources and respond to CoAP requests from a client, using Zephyr's CoAP server subsystem.
   It runs on an nRF71 Series or nRF70 Series device connected over Wi-Fi®.

Requirements
************

The sample supports the following development kits:

.. table-from-rows:: /includes/sample_board_rows.txt
   :header: heading
   :rows: nrf7002dk_nrf5340_cpuapp, nrf7120dk_nrf7120_cpuapp

Overview
********

This is Zephyr's :zephyr:code-sample:`coap-server` sample, built to run on the development kits listed above.
The sample registers CoAP resources to a main CoAP service and responds to client requests for them over UDP.
All services and resources must be available at compile time, as they are placed into dedicated linker sections.

The sample listens for requests on the default CoAP UDP port (5683, or 5684 for secure CoAP), over either IPv4 or IPv6 depending on the Wi-Fi snippet used to build it (see `Wi-Fi`_).
Over IPv6, the sample joins the site-local CoAP all-nodes multicast address.
The server replies from the same local address a request arrived on, so a client sees a matching reply even when the device has more than one active address.

The sample serves plain CoAP by default.
Building with the :file:`overlay-dtls.conf` extra configuration file (see `Configuration files`_) switches it to secure CoAP (CoAPS) over DTLS, using the PSK identity and key defined in :file:`src/dummy_psk.h`.

The DTLS service can also authenticate itself with a server certificate and private key (see :file:`src/certificate.h`) instead of PSK, using a certificate-based cipher suite.
By default this is a self-signed demo certificate.
Enabling :option:`CONFIG_NET_SAMPLE_CERTS_WITH_SC` switches the sample to a CA-signed certificate and its CA certificate instead, for testing proper certificate chain validation.
It only builds in when an RSA or ECDSA ECDHE cipher suite is enabled, which :file:`overlay-dtls.conf` does not select by default.
To use it, add an extra configuration file on top of :file:`overlay-dtls.conf` that selects a matching cipher suite, such as :kconfig:option:`CONFIG_MBEDTLS_CIPHERSUITE_TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256` for the sample's RSA demo certificate.

The sample exports the following resources:

.. code-block:: none

   /test
   /seg1/seg2/seg3
   /query
   /separate
   /large
   /location-query
   /large-update

These resources allow a good part of the ETSI test cases to be run against the sample.

The block-wise :file:`/large*` endpoints track each client's transfer separately, from a small fixed-size pool (see `Configuration options`_).
A new client is rejected with ``5.03 Service Unavailable`` if the pool is full and every slot is still active.

The sample itself has no output on success beyond the logging described in `Sample output`_.
Verify its functionality using an external CoAP client, or a tool such as tcpdump or Wireshark to inspect the traffic.

Configuration
*************

|config|

Configuration options
======================

The following sample-specific Kconfig options are used in this sample (located in :file:`zephyr/samples/net/sockets/coap_server/Kconfig`):

* :option:`CONFIG_NET_SAMPLE_COAPS_SERVICE` - Enables the CoAP secure service (CoAPS) over DTLS.
* :option:`CONFIG_NET_SAMPLE_COAP_SERVER_SERVICE_PORT` - Port number for the CoAP service. Defaults to 5684 if the secure service is enabled, or 5683 otherwise.
* :option:`CONFIG_NET_SAMPLE_COAP_MAX_LARGE_TRANSFERS` - Maximum number of clients that can have a block-wise transfer in progress on the same :file:`/large*` endpoint at once.
* :option:`CONFIG_NET_SAMPLE_COAP_LARGE_TRANSFER_TIMEOUT_SEC` - Idle time before a stalled client's slot on a :file:`/large*` endpoint can be reused by another client.
* :option:`CONFIG_NET_SAMPLE_PSK_HEADER_FILE` - Header file containing the pre-shared key used by the secure service.
* :option:`CONFIG_NET_SAMPLE_CERTS_WITH_SC` - Runs the secure service with signed certificates instead of a pre-shared key.

Configuration files
====================

The sample provides predefined configuration files, located in :file:`zephyr/samples/net/sockets/coap_server`:

* :file:`prj.conf` - Default configuration file, used for plain (non-secure) CoAP.
* :file:`overlay-dtls.conf` - Additional configuration for secure CoAP (CoAPS) over DTLS.

To add a specific extra configuration file to the build, add the ``-DEXTRA_CONF_FILE=<extra_conf_file>`` flag to your west build command.

Wi-Fi
=====

Use the :ref:`Wi-Fi snippet <zephyr:snippet-wifi-ipv4>` to build and run the sample over IPv4, or the :ref:`Wi-Fi snippet <zephyr:snippet-wifi-ipv6>` to run it over IPv6 instead, on the development kits listed above.

Building and running
********************

.. |sample path| replace:: :file:`samples/zephyr/net/sockets/coap_server`

.. include:: /includes/build_and_run.txt

Use the ``nrf7002dk/nrf5340/cpuapp`` or ``nrf7120dk/nrf7120/cpuapp`` board target together with the ``wifi-ipv4`` or ``wifi-ipv6`` snippet.
For example:

.. code-block:: console

   # nRF7002 DK, IPv4
   west build -b nrf7002dk/nrf5340/cpuapp -S wifi-ipv4

   # nRF7120 DK, IPv4
   west build -b nrf7120dk/nrf7120/cpuapp -S wifi-ipv4

   # nRF7002 DK, IPv6
   west build -b nrf7002dk/nrf5340/cpuapp -S wifi-ipv6

Alternatively, build against the predefined test case in :file:`sample.yaml`:

.. code-block:: console

   west build -b nrf7120dk/nrf7120/cpuapp -T nrf.extended.sample.net.sockets.coap_server.wifi.nrf71dk

To build the sample with secure CoAP resources instead, add the :file:`overlay-dtls.conf` extra configuration file on top of the base configuration.

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|
#. Wait for the sample to connect to the configured Wi-Fi network, and note the IP address logged on the terminal.
#. From a host on the same network, install `libcoap`_ (for example, ``sudo apt install libcoap3-bin`` on Ubuntu) and send a CoAP request to one of the resources listed in `Overview`_:

   .. code-block:: console

      coap-client-notls -m get coap://<dut_ip_address>/test

   Replace ``<dut_ip_address>`` with the IP address of the development kit.
   Use ``-m put``, ``-m post``, or ``-m delete`` together with ``-e <data>`` to exercise the other CoAP methods, for example:

   .. code-block:: console

      coap-client-notls -m put -e "test data" coap://<dut_ip_address>/large-update

   For an IPv6 build, wrap the address in brackets instead: ``coap://[<dut_ipv6_address>]/test``.

#. Observe the response printed by ``coap-client``, and the matching request logged by the sample on the terminal.
#. Optionally, inspect the exchanged traffic with a tool such as Wireshark, or see the :ref:`wifi_monitor_sample` sample documentation to capture Wi-Fi traffic directly from the development kit.

To test the secure CoAP (CoAPS) variant, build with the :file:`overlay-dtls.conf` extra configuration file (see `Configuration files`_), then connect with a DTLS-PSK capable build of the client, such as ``coap-client-gnutls`` or ``coap-client-openssl``, using the identity and key from :file:`src/dummy_psk.h`:

.. code-block:: console

   coap-client-gnutls -m get -u PSK_identity -k $'\x01\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f' coaps://<dut_ip_address>/test

The ``$'...'`` quoting passes the key's raw bytes through the shell. Check ``coap-client --help`` if your build expects the key encoded differently.

Sample output
==============

This is a typical log output of the sample::

   [00:00:26.518,841] <inf> net_samples_common: Network connectivity established and IP address assigned
   [00:00:27.568,380] <inf> net_coap_service_sample: *******
   [00:00:27.568,391] <inf> net_coap_service_sample: type: 0 code 1 id 44042
   [00:00:27.568,393] <inf> net_coap_service_sample: *******
   [...]
   [00:00:50.363,259] <inf> net_coap_service_sample: *******
   [00:00:50.363,266] <inf> net_coap_service_sample: type: 0 code 1 id 29028
   [00:00:50.363,273] <inf> net_coap_service_sample: num queries: 2
   [00:00:50.363,285] <inf> net_coap_service_sample: query[1]: first=1
   [00:00:50.363,298] <inf> net_coap_service_sample: query[2]: second=2
   [00:00:50.363,303] <inf> net_coap_service_sample: *******
   [...]
   [00:00:50.411,302] <inf> net_coap_service_sample: CoAP observer added
   [...]
   [00:01:12.429,818] <inf> net_coap_service_sample: CoAP observer removed

The ``*******`` lines bracket each request's type, code, and ID. Some resources, such as :file:`/query`, log extra detail in between.

Troubleshooting
================

If the sample does not connect to the Wi-Fi network, see the :ref:`wifi_monitor_sample` sample documentation to learn how to capture and analyze Wi-Fi traffic to debug connectivity issues.
Also verify that the Wi-Fi credentials configured for your device match your access point.

References
**********

* `RFC 7252 - The Constrained Application Protocol`_
* `libcoap`_

.. _`libcoap`: https://github.com/obgm/libcoap

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`net_if_interface`
* :ref:`net_mgmt_interface`
* CoAP and the CoAP server subsystem (:file:`include/zephyr/net/coap.h`, :file:`include/zephyr/net/coap_service.h`)
