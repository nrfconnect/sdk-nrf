.. _ot_cli_sample:

Thread: CLI
###########

.. contents::
   :local:
   :depth: 2

The :ref:`Thread <ug_thread>` CLI sample demonstrates how to send commands to a Thread device using the OpenThread Command Line Interface (CLI).
The CLI is integrated into the Zephyr shell.

Requirements
************

The sample supports the following development kits for testing the network status:

.. table-from-sample-yaml::

Optionally, you can use one or more compatible development kits programmed with this sample or another :ref:`Thread sample <openthread_samples>` for :ref:`testing communication or diagnostics <ot_cli_sample_testing_multiple>` and :ref:`thread_ot_commissioning_configuring_on-mesh`.

You need `nRF Sniffer for 802.15.4`_ to observe messages sent from the router to the leader kit when :ref:`ot_cli_sample_testing_multiple_v12`.

.. include:: /includes/tfm.txt

Overview
********

The sample demonstrates the usage of commands listed in `OpenThread CLI Reference`_.
OpenThread CLI is integrated into the system shell accessible over serial connection.
To indicate a Thread command, the ``ot`` keyword needs to precede the command.

The number of commands you can test depends on the application configuration.
The CLI sample comes with the :ref:`full set of OpenThread functionalities <thread_ug_feature_sets>` enabled (:kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER`).
Thread 1.2 version is selected as default.

If used alone, the sample allows you to test the network status.
It is recommended to use at least two development kits running the same sample for testing the communication.

.. _ot_cli_sample_thread_certification:

Certification tests with CLI sample
===================================

You can use the Thread CLI sample to run certification tests.
See :ref:`ug_thread_cert` for information on how to use this sample on Thread Certification Test Harness.

User interface
**************

All interactions with the application are handled using serial communication.
See `OpenThread CLI Reference`_ for the list of available serial commands.

.. _ot_cli_sample_diag_module:

Diagnostic module
=================

By default, the CLI sample comes with the :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER` :ref:`feature set <thread_ug_feature_sets>` enabled, which allows you to use Zephyr's diagnostic module with its ``diag`` commands.
Use these commands to manually check hardware-related functionalities without running a Thread network.
For example, to ensure radio communication is working when adding a new functionality or during the manufacturing process.
See `Testing diagnostic module`_ section for an example.

.. note::
    If you disable the :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER` feature set, you can enable the diagnostic module with the :kconfig:option:`CONFIG_OPENTHREAD_DIAG` Kconfig option.

Configuration
*************

|config|

.. _ot_cli_sample_activating_variants:

Configuration files
===================

The sample provides predefined configuration files for typical use cases, and to activate sample extensions.
You can find the configuration files in the root directory of the sample.

Specify the corresponding file names in the :makevar:`OVERLAY_CONFIG` option when building.
See :ref:`cmake_options` for instructions on how to add this option.
For more information about using configuration overlay files, see :ref:`zephyr:important-build-vars` in the Zephyr documentation.

The following configuration files are available:

* :file:`overlay-usb.conf` - Enables USB transport support.
  Additionally, you need to set :makevar:`DTC_OVERLAY_FILE` to :file:`usb.overlay`.
* :file:`overlay-logging.conf` - Sets more detailed logging.
* :file:`overlay-rtt.conf` - Redirects logs to RTT.
  For more information about RTT please refer to :ref:`RTT logging <ug_logging>`.
* :file:`overlay-debug.conf` - Enables debugging the Thread sample with GDB thread awareness.
* :file:`overlay-ci.conf` - Disables boot banner and shell prompt.
* :file:`overlay-multiprotocol.conf` - Enables Bluetooth LE support in this sample.
* :file:`overlay-tcp.conf` - Enables experimental TCP support in this sample.

FEM support
===========

.. include:: /includes/sample_fem_support.txt

.. _ot_cli_sample_minimal:

Memory optimization
===================

See :ref:`app_memory` for actions and configuration options you can use to optimize the memory footprint of the sample.

Serial transport
================

The Thread CLI sample supports UART and USB CDC ACM as serial transports.
By default, it uses UART transport.
To switch to USB transport, :ref:`activate the USB overlay extension <ot_cli_sample_activating_variants>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/openthread/cli`

|enable_thread_before_testing|

.. include:: /includes/build_and_run_ns.txt

To update the OpenThread libraries provided by ``nrfxlib``, invoke ``west build -b nrf52840dk_nrf52840 -t install_openthread_libraries``.

.. _ot_cli_sample_testing:

Testing
=======

After building the sample and programming it to your development kit, complete the following steps to test it:

#. Turn on the development kit.
#. |connect_terminal_ANSI|

   .. note::
        |thread_hwfc_enabled|

#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Invoke some of the OpenThread commands:

   a. Test the state of the Thread network with the ``ot state`` command.
      For example:

      .. code-block:: console

         uart:~$ ot state
         leader
         Done

   #. Get the Thread network name with the ``ot networkname`` command.
      For example:

      .. code-block:: console

         uart:~$ ot networkname
         OpenThread
         Done

   #. Get the IP addresses of the current Thread network with the ``ot ipaddr`` command.
      For example:

      .. code-block:: console

         uart:~$ ot ipaddr
         fdde:ad00:beef:0:0:ff:fe00:800
         fdde:ad00:beef:0:3102:d00b:5cbe:a61
         fe80:0:0:0:8467:5746:a29f:1196
         Done

.. _ot_cli_sample_testing_multiple:

Testing with multiple kits
--------------------------

If you are using more than one development kit for testing the CLI sample, you can also complete additional testing procedures.

.. note::
    The following testing procedures assume you are using two development kits.

Testing communication between kits
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To test communication between kits, complete the following steps:

#. Make sure both development kits are programmed with the CLI sample.
#. Turn on the developments kits.
#. |connect_terminal_both_ANSI|

   .. note::
        |thread_hwfc_enabled|

#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Test communication between the kits with the following command:

   .. parsed-literal::
      :class: highlight

      ot ping *ip_address_of_the_first_kit*

   For example:

   .. code-block:: console

      uart:~$ ot ping fdde:ad00:beef:0:3102:d00b:5cbe:a61
      16 bytes from fdde:ad00:beef:0:3102:d00b:5cbe:a61: icmp_seq=3 hlim=64 time=23ms
      1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/av
      Done

Testing diagnostic module
~~~~~~~~~~~~~~~~~~~~~~~~~

To test diagnostic commands, complete the following steps:

#. Make sure both development kits are programmed with the CLI sample.
#. Turn on the developments kits.
#. |connect_terminal_both_ANSI|

   .. note::
        |thread_hwfc_enabled|.

#. Make sure that the diagnostic module is enabled and configured with proper radio channel and transmission power.
   Run the following commands on both devices:

   .. code-block:: console

      uart:~$ ot diag start
      start diagnostics mode
      status 0x00
      Done
      uart:~$ ot diag channel 11
      set channel to 11
      status 0x00
      Done
      uart:~$ ot diag power 0
      set tx power to 0 dBm
      status 0x00
      Done

#. Transmit a fixed number of packets with the given length from one of the devices.
   For example, to transmit 20 packets that contain 100 B of random data, run the following command:

   .. code-block:: console

      uart:~$ ot diag send 20 100
      sending 0x14 packet(s), length 0x64
      status 0x00
      Done

#. To read the radio statistics on the other device, run the following command:

   .. code-block:: console

      uart:~$ ot diag stats
      received packets: 20
      sent packets: 0
      first received packet: rssi=-29, lqi=255
      last received packet: rssi=-30, lqi=255
      Done

.. _ot_cli_sample_testing_multiple_v12:

Testing Thread 1.2 and Thread 1.3 features
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

To test the Thread 1.2 and Thread 1.3 features, complete the following steps:

#. Enable the extra options :kconfig:option:`CONFIG_OPENTHREAD_BORDER_ROUTER`, :kconfig:option:`CONFIG_OPENTHREAD_BACKBONE_ROUTER` and :kconfig:option:`CONFIG_OPENTHREAD_SRP_SERVER` when building the CLI sample.
#. Make sure both development kits are programmed with the CLI sample.
#. Turn on the developments kits.
#. |connect_terminal_both_ANSI|
#. .. include:: /includes/thread_configure_network.txt
#. .. include:: /includes/thread_enable_network.txt
#. Test the state of the Thread network with the ``ot state`` command to see which kit is the leader:

   .. code-block:: console

      uart:~$ ot state
      leader
      Done

#. On the leader kit, enable the Backbone Router function:

   .. code-block:: console

      uart:~$ ot bbr enable
      Done

#. On the leader kit, configure the Domain prefix:

   .. code-block:: console

      uart:~$ ot prefix add fd00:7d03:7d03:7d03::/64 prosD med
      Done
      uart:~$ ot netdata register
      Done

#. On the router kit, display the autoconfigured Domain Unicast Address and set another one manually:

   .. code-block:: console

      uart:~$ ot ipaddr
      fd00:7d03:7d03:7d03:ee2d:eed:4b59:2736
      fdde:ad00:beef:0:0:ff:fe00:c400
      fdde:ad00:beef:0:e0fc:dc28:1d12:8c2
      fe80:0:0:0:acbd:53bf:1461:a861
      Done
      uart:~$ ot dua iid 0004000300020001
      Done
      uart:~$ ot ipaddr
      fd00:7d03:7d03:7d03:4:3:2:1
      fdde:ad00:beef:0:0:ff:fe00:c400
      fdde:ad00:beef:0:e0fc:dc28:1d12:8c2
      fe80:0:0:0:acbd:53bf:1461:a861
      Done

#. On the router kit, configure a multicast address with a scope greater than realm-local:

   .. code-block:: console

      uart:~$ ot ipmaddr add ff04::1
      Done
      uart:~$ ot ipmaddr
      ff04:0:0:0:0:0:0:1
      ff33:40:fdde:ad00:beef:0:0:1
      ff32:40:fdde:ad00:beef:0:0:1
      ff02:0:0:0:0:0:0:2
      ff03:0:0:0:0:0:0:2
      ff02:0:0:0:0:0:0:1
      ff03:0:0:0:0:0:0:1
      ff03:0:0:0:0:0:0:fc
      Done

   The router kit sends an ``MLR.req`` message and a ``DUA.req`` message to the leader kit (Backbone Router).
   Use the `nRF Sniffer for 802.15.4`_ to observe this.

#. On the leader kit, list the IPv6 addresses:

   .. code-block:: console

      uart:~$ ot ipaddr
      fd00:7d03:7d03:7d03:84c9:572d:be24:cbe
      fdde:ad00:beef:0:0:ff:fe00:fc10
      fdde:ad00:beef:0:0:ff:fe00:fc38
      fdde:ad00:beef:0:0:ff:fe00:fc00
      fdde:ad00:beef:0:0:ff:fe00:7000
      fdde:ad00:beef:0:a318:bf4f:b9c6:5f7d
      fe80:0:0:0:10b1:93ea:c0ee:eeb7

   Note down the link-local address.
   You must use this address when sending Link Metrics commands from the router kit to the leader kit.

   The following steps use the address ``fe80:0:0:0:10b1:93ea:c0ee:eeb7``.
   Replace it with the link-local address of your leader kit in all commands.

#. Run the following commands on the router kit:

   a. Reattach the router kit as SED with a polling period of three seconds:

      .. code-block:: console

         uart:~$ ot pollperiod 3000
         Done
         uart:~$ ot mode -
         Done

   #. Perform a Link Metrics query (Single Probe):

      .. code-block:: console

         uart:~$ ot linkmetrics query fe80:0:0:0:10b1:93ea:c0ee:eeb7 single qmr
         Done
         Received Link Metrics Report from: fe80:0:0:0:10b1:93ea:c0ee:eeb7
         - LQI: 220 (Exponential Moving Average)
         - Margin: 60 (dB) (Exponential Moving Average)
         - RSSI: -40 (dBm) (Exponential Moving Average)

   #. Send a Link Metrics Management Request to configure a Forward Tracking Series:

      .. code-block:: console

         uart:~$ ot linkmetrics mgmt fe80:0:0:0:10b1:93ea:c0ee:eeb7 forward 1 dra pqmr
         Done
         Received Link Metrics Management Response from: fe80:0:0:0:10b1:93ea:c0ee:eeb7
         Status: Success

   #. Send an MLE Link Probe message to the peer:

      .. code-block:: console

         uart:~$ ot linkmetrics probe fe80:0:0:0:10b1:93ea:c0ee:eeb7 1 10
         Done

   #. Perform a Link Metrics query (Forward Tracking Series):

      .. code-block:: console

         uart:~$ ot linkmetrics query fe80:0:0:0:10b1:93ea:c0ee:eeb7 forward 1
         Done
         Received Link Metrics Report from: fe80:0:0:0:10b1:93ea:c0ee:eeb7
         - PDU Counter: 13 (Count/Summation)
         - LQI: 212 (Exponential Moving Average)
         - Margin: 60 (dB) (Exponential Moving Average)
         - RSSI: -40 (dBm) (Exponential Moving Average)

   #. Send a Link Metrics Management Request to register an Enhanced ACK-based Probing:

      .. code-block:: console

         uart:~$ ot linkmetrics mgmt fe80:0:0:0:10b1:93ea:c0ee:eeb7 enhanced-ack register qm
         Done
         Received Link Metrics data in Enh Ack from neighbor, short address:0xa400 , extended address:12b193eac0eeeeb7
         - LQI: 255 (Exponential Moving Average)
         - Margin: 68 (dB) (Exponential Moving Average)

   #. Send a Link Metrics Management Request to clear an Enhanced ACK-based Probing:

      .. code-block:: console

         uart:~$ ot linkmetrics mgmt fe80:0:0:0:10b1:93ea:c0ee:eeb7 enhanced-ack clear
         Done
         Received Link Metrics Management Response from: fe80:0:0:0:10b1:93ea:c0ee:eeb7
         Status: Success

#. Verify the Coordinated Sampled Listening (CSL) functionality.

   The following steps use the address ``fe80:0:0:0:acbd:53bf:1461:a861``.
   Replace it with the link-local address of your router kit in all commands.

   a. Send an ICMPv6 Echo Request from the leader kit to link-local address of the router kit:

      .. code-block:: console

         uart:~$ ot ping fe80:0:0:0:acbd:53bf:1461:a861
         16 bytes from fe80:0:0:0:acbd:53bf:1461:a861: icmp_seq=2 hlim=64 time=2494ms
         1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/a
         Done

      Observe that there is a long latency, up to 3000 ms, on the reply.
      This is due to the indirect transmission mechanism based on data polling.

   #. Stop frequent polling on the router kit (now SED) by configuring a polling period of 240 seconds:

      .. code-block:: console

         uart:~$ ot pollperiod 240000
         Done

   #. Enable a CSL Receiver on the router kit (now SED) by configuring a CSL period of 0.5 seconds:

      .. code-block:: console

         uart:~$ ot csl period 3125
         Done

   #. Send an ICMPv6 Echo Request from the leader kit to the link-local address of the router kit:

      .. code-block:: console

         uart:~$ ot ping fe80:0:0:0:acbd:53bf:1461:a861
         16 bytes from fe80:0:0:0:acbd:53bf:1461:a861: icmp_seq=3 hlim=64 time=421ms
         1 packets transmitted, 1 packets received. Packet loss = 0.0%. Round-trip min/a
         Done

      Observe that the reply latency is reduced to a value below 500 ms.
      The reduction occurs because the transmission from the leader is performed using CSL, based on the CSL Information Elements sent by the CSL Receiver.

#. Verify the Service Registration Protocol (SRP) functionality.

   a. On the leader kit, enable the SRP Server function:

      .. code-block:: console

         uart:~$ ot srp server enable
         Done

   #. Register a `_ipps._tcp`` service on the router kit (now SED):

      .. code-block:: console

         uart:~$ ot srp client host name my-host
         Done
         uart:~$ ot srp client host address fdde:ad00:beef:0:e0fc:dc28:1d12:8c2
         Done
         uart:~$ ot srp client service add my-service _ipps._tcp 12345
         Done
         uart:~$ ot srp client autostart enable
         Done

   #. On the router kit (now SED), check that the host and service have been successfully registered:

      .. code-block:: console

         uart:~$ ot srp client host
         name:"my-host", state:Registered, addrs:[fdde:ad00:beef:0:e0fc:dc28:1d12:8c2]
         Done

   #. Check the host and service on the leader kit:

      .. code-block:: console

         uart:~$ ot srp server host
         my-host.default.service.arpa.
            deleted: false
            addresses: [fdde:ad00:beef:0:e0fc:dc28:1d12:8c2]
         Done
         uart:~$ ot srp server service
         my-service._ipps._tcp.default.service.arpa.
            deleted: false
            subtypes: (null)
            port: 12345
            priority: 0
            weight: 0
            ttl: 7200
            TXT: []
            host: my-host.default.service.arpa.
            addresses: [fdde:ad00:beef:0:e0fc:dc28:1d12:8c2]
         Done

Dependencies
************

This sample uses the following Zephyr libraries:

* :ref:`zephyr:kernel_api`:

  * ``include/kernel.h``

* :ref:`zephyr:thread_protocol_interface`

The following dependencies are added by the optional multiprotocol Bluetooth® LE extension:

* :ref:`nrfxlib:softdevice_controller`
* :ref:`nus_service_readme`
* Zephyr's :ref:`zephyr:bluetooth_api`:

  * ``include/bluetooth/bluetooth.h``
  * ``include/bluetooth/gatt.h``
  * ``include/bluetooth/hci.h``
  * ``include/bluetooth/uuid.h``

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
