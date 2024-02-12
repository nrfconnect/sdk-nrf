.. _wifi_monitor_sample:

Wi-Fi: Monitor
##############

.. contents::
   :local:
   :depth: 2

The Monitor sample demonstrates how to set monitor mode, analyzing incoming Wi-Fi packets, and printing packet statistics.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how to configure nrf70 Series device in Monitor mode.
It analyzes the Wi-Fi incoming packets on a raw socket and prints the packet statistics at fixed interal.

* :kconfig:option:`CONFIG_STATS_PRINT_TIMEOUT`: Wait duration for printing Wi-Fi packet statistics.

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/monitor/Kconfig`):

.. options-from-kconfig::

You must configure the :kconfig:option:`CONFIG_MONITOR_MODE_CHANNEL` Kconfig option in the :file:`prj.conf` file.

This specifies the Wi-Fi channel to be used for communication on the wireless network.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/monitor`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following are examples of the CLI commands:

  .. code-block:: console

     west build -b nrf7002dk_nrf5340_cpuapp

Change the build target as given below for the nRF7002 EK.

.. code-block:: console

   nrf5340dk_nrf5340_cpuapp -- -DSHIELD=nrf7002ek

Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

      *** Booting nRF Connect SDK v3.4.99-ncs1-4796-g703d0a7d3d11 ***
      [00:00:00.318,359] <inf> net_config: Initializing network
      [00:00:00.318,389] <inf> net_config: Waiting interface 1 (0x20001434) to be up...
      [00:00:00.318,511] <inf> monitor: Waiting for packets ...
      [00:00:00.319,702] <inf> monitor: Mode set to Monitor
      [00:00:00.320,312] <dbg> net_if: update_operational_state: (0x20002e10): iface 0x20001434, oper state UP admin UP carrier ON dormant OFF
      [00:00:00.320,312] <dbg> net_if: net_if_start_dad: (0x20002e10): Starting DAD for iface 0x20001434
      [00:00:00.320,404] <dbg> net_if: net_if_ipv6_addr_add: (0x20002e10): [0] interface 0x20001434 address fe80::f6ce:36ff:fe00:16 type AUTO added
      [00:00:00.320,465] <dbg> net_if: net_if_ipv6_maddr_add: (0x20002e10): [0] interface 0x20001434 address ff02::1 added
      [00:00:00.320,739] <dbg> net_if: net_if_ipv6_maddr_add: (0x20002e10): [1] interface 0x20001434 address ff02::1:ff00:16 added
      [00:00:00.321,014] <dbg> net_if: net_if_ipv6_start_dad: (0x20002e10): Interface 0x20001434 ll addr F4:CE:36:00:00:16 tentative IPv6 addr fe80::f6ce:36ff:fe00:16
      [00:00:00.321,166] <dbg> net_if: net_if_start_rs: (0x20002e10): Starting ND/RS for iface 0x20001434
      [00:00:00.321,380] <dbg> net_if: net_if_tx: (0x20002508): Processing (pkt 0x20058290, prio 1) network packet iface 0x20001434/1
      [00:00:00.321,411] <dbg> net_if: net_if_tx: (0x20002508): Processing (pkt 0x2005824c, prio 1) network packet iface 0x20001434/1
      [00:00:00.321,472] <dbg> net_if: net_if_tx: (0x20002508): Processing (pkt 0x20058208, prio 1) network packet iface 0x20001434/1
      [00:00:00.321,502] <dbg> net_if: net_if_tx: (0x20002508): Processing (pkt 0x200581c4, prio 1) network packet iface 0x20001434/1
      [00:00:00.321,624] <err> wifi_nrf: hal_rpu_eventq_process: Interrupt callback failed
      [00:00:00.321,685] <err> wifi_nrf: event_tasklet_fn: Event queue processing failed
      [00:00:00.322,662] <inf> monitor: Wi-Fi channel set to 1
      [00:00:00.421,264] <dbg> net_if: dad_timeout: (0x20002e10): DAD succeeded for fe80::f6ce:36ff:fe00:16
      [00:00:00.421,356] <inf> net_config: IPv6 address: fe80::f6ce:36ff:fe00:16
      [00:00:01.321,350] <dbg> net_if: rs_timeout: (0x20002e10): RS no respond iface 0x20001434 count 1
      [00:00:01.321,380] <dbg> net_if: net_if_start_rs: (0x20002e10): Starting ND/RS for iface 0x20001434
      [00:00:01.321,533] <dbg> net_if: net_if_tx: (0x20002508): Processing (pkt 0x200581c4, prio 1) network packet iface 0x20001434/1
      [00:00:02.321,594] <dbg> net_if: rs_timeout: (0x20002e10): RS no respond iface 0x20001434 count 2
      [00:00:02.321,624] <dbg> net_if: net_if_start_rs: (0x20002e10): Starting ND/RS for iface 0x20001434
      [00:00:02.321,777] <dbg> net_if: net_if_tx: (0x20002508): Processing (pkt 0x200581c4, prio 1) network packet iface 0x20001434/1
      [00:00:03.321,838] <dbg> net_if: rs_timeout: (0x20002e10): RS no respond iface 0x20001434 count 3
      [00:00:05.319,519] <inf> monitor: Management Frames:
      [00:00:05.319,549] <inf> monitor:       Beacon Count: 478
      [00:00:05.319,580] <inf> monitor:       Probe Request Count: 52
      [00:00:05.319,580] <inf> monitor:       Probe Response Count: 378
      [00:00:05.319,580] <inf> monitor: Control Frames:
      [00:00:05.319,580] <inf> monitor:        Ack Count 107
      [00:00:05.319,580] <inf> monitor:        RTS Count 2
      [00:00:05.319,610] <inf> monitor:        CTS Count 6
      [00:00:05.319,610] <inf> monitor:        Block Ack Count 0
      [00:00:05.319,610] <inf> monitor:        Block Ack Req Count 0
      [00:00:05.319,641] <inf> monitor: Data Frames:
      [00:00:05.319,641] <inf> monitor:       Data Count: 51
      [00:00:05.319,641] <inf> monitor:       QoS Data Count: 0
      [00:00:05.319,641] <inf> monitor:       Null Count: 0
      [00:00:05.319,641] <inf> monitor:       QoS Null Count: 0

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
