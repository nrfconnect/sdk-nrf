.. _dect_shell_application:

nRF91x1: DECT NR+ Shell
#######################

.. contents::
   :local:
   :depth: 2

The DECT NR+ Shell (DeSh) sample application demonstrates how to set up a DECT NR+ application on top of the DECT NR+ networking stack and enables you to test various stack and modem features.

Requirements
************

The sample supports the following development kit and requires at least two kits:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

DeSh enables testing of the DECT NR+ networking stack in the |NCS| with DECT NR+ modem firmware v2.x.

The subsections list the DeSh features, show shell command examples, and describe their usage.

The following abbreviations from the DECT NR+ MAC specification (`ETSI TS 103 636-4`_) are used in the examples:

* FT: Fixed Termination point
* PT: Portable Termination point
* BR: Border Router that connects the DECT NR network to the Internet

Main command structure:

  .. code-block:: console

     at
       at_cmd_mode
     auto_connect
       enable
       disable
       sett_read
     cloud
       connect
       disconnect
       raw_data_tx (with CoAP)
       alert_tx (with CoAP)
     dect
       activate
       deactivate
       sett
       rssi_scan
       status
       scan
       associate
       dissociate
       cluster_start
       cluster_info
       neighbor_list
       neighbor_info
       nw_beacon_start
       nw_beacon_stop
       nw_create
       nw_remove
       nw_join
       nw_unjoin
       connect
       disconnect
       rx
       tx
     hostname
       read
       write
     ping
     print
       timestamps
       cloud (with MQTT)
     version

Quick start tutorial
====================

**Step 1: Basic Two-Device Setup**

Device 1 (FT - Network Creator)::

   dect sett --dev_type FT
   dect activate
   dect connect
   # Wait for "Network status: created" message

Device 2 (PT - Network Joiner)::

   dect sett --dev_type PT
   dect activate
   dect connect
   # Wait for "Network status: joined" message

**Step 2: Verify Connection**

Both devices::

   dect status                          # Shows associations with addressing information
   ping -d <neighbor_ipv6_address>      # Test connectivity

Write and read hostname
=======================

DeSh command ``hostname``.

To set and read the hostname of the DECT NR+ device, use the following commands:

  .. code-block:: console

     desh:~$ hostname write dect-ft-device
     desh:~$ hostname read

Application settings
====================

DeSh command ``dect sett``.

You can store some of the main DeSh command parameters into settings that are persistent between sessions.
The settings are stored in the persistent storage and loaded when the application starts.

Examples
--------

* See the usage and read the current settings:

  .. code-block:: console

     desh:~$ dect sett -h
     desh:~$ dect sett -r

* Reset the settings to their default values:

  .. code-block:: console

     desh:~$ dect sett --reset

* Change the default TX power for the cluster beacon:

  .. code-block:: console

     desh:~$ dect sett --cluster_max_beacon_tx_pwr 4

* Change the default band to ``2``:

  .. code-block:: console

     desh:~$ dect sett -b 2

Activate DECT NR+ stack
========================

DeSh command ``dect activate``.

* Activate DECT NR+ stack:

  .. code-block:: console

     desh:~$ dect activate

RSSI measurement
================

DeSh command ``dect rssi_scan``.

Execute RSSI measurement/scan.

* Execute shorter (100 frames on each channel) RSSI measurements on all channels on band #1:

  .. code-block:: console

     desh:~$ dect rssi_scan -b 1 --frames 100

FT: Start a cluster manually
============================

DeSh command ``dect cluster_start``.

The ``dect cluster_start`` command starts a DECT NR+ cluster based on settings.
The command is available only for FT devices.

Examples
--------

* Set device as an FT device and set the transmission ID:

  .. code-block:: console

     desh:~$ dect sett --dev_type FT -t 1

* Activate DECT NR+ stack (if not already activated by auto_activate setting):

  .. code-block:: console

     desh:~$ dect activate

* Start a DECT NR+ cluster as set in settings:

  .. code-block:: console

     desh:~$ dect cluster_start
     Cluster start initiated.
     NET_EVENT_DECT_RSSI_SCAN_RESULT
     RSSI scan result:
     Channel:                             1657
     All subslots free:                   yes
     Busy percentage:                     0%
     NET_EVENT_DECT_RSSI_SCAN_DONE: scan done
     NET_EVENT_DECT_CLUSTER_CREATED_RESULT
     Cluster started/reconfigured at channel 1657.

FT: Start advertising the created cluster by starting a periodic network beacon
===============================================================================

DeSh command ``dect nw_beacon_start``.

The ``dect nw_beacon_start`` command starts sending of a DECT NR+ network beacon.
The command is available only for FT devices.

* Start a DECT NR+ network beacon at channel 1659 with additional channels:

  .. code-block:: console

     desh:~$ dect nw_beacon_start -c 1659 --add_channels 1661,1663,1665
     ..
     NW beacon started.

FT: Creating a DECT NR+ network
===============================

DeSh command ``dect nw_create``.

The ``dect nw_create`` command creates a DECT NR+ network in a set band.
This higher level command combines the ``rssi_scan`` and ``cluster_start`` commands and creates a DECT NR+ network.
Optionally, it also starts a network beacon on a set channel (preset in settings using ``dect sett --nw_beacon_channel <channel>``), which means it includes functionalities of ``dect nw_beacon_start``.
The command is available only for FT devices.

* Create a DECT NR+ network in a set band (FT device is activated but no cluster running):

  .. code-block:: console

     desh:~$ dect sett --nw_beacon_channel 1659
     Settings updated.
     desh:~$ dect nw_create
     Network creation initiated.
     NET_EVENT_DECT_RSSI_SCAN_RESULT
     RSSI scan result:
     Channel:                             1657
     All subslots free:                   yes
     Busy percentage:                     0%
     NET_EVENT_DECT_RSSI_SCAN_DONE: scan done
     NET_EVENT_DECT_CLUSTER_CREATED_RESULT
     Cluster started/reconfigured at channel 1657.
     NET_EVENT_DECT_NW_BEACON_START_RESULT
     NW beacon started.
     FT: network created
     NET_EVENT_DECT_NETWORK_STATUS:
     Network status: created

PT: Manually scan for a DECT NR+ cluster
========================================

DeSh command ``dect scan``.

The ``dect scan`` command scans for DECT NR+ clusters and network beacons.

* Start a DECT NR+ scan on band #1 (PT device is activated but not associated with any cluster):

  .. code-block:: console

     desh:~$ dect scan -b 1
     Scan initiated.
     NET_EVENT_DECT_SCAN_RESULT
     Scan result:
      Beacon type:             Cluster
      Reception channel:       1657
      Long RD ID:              1 (0x00000001)
      NW ID:                   2271560481 (0x87654321)
      RX RSSI-2:               -32dBm
      RX SNR:                  26dB
      RX MCS index:            4
      RX Transmit power:       10 (10 dBm)
     NET_EVENT_DECT_SCAN_RESULT
     ...
     NET_EVENT_DECT_SCAN_RESULT
      Scan result:
      Beacon type:             NW
      Reception channel:       1659
      Long RD ID:              1 (0x00000001)
      NW ID:                   2271560481 (0x87654321)
      RX RSSI-2:               -31dBm
      RX SNR:                  26dB
      RX MCS index:            4
      RX Transmit power:       10 (10 dBm)
      Current cluster channel: not available
      Next cluster channel:    1657
      NET_EVENT_DECT_SCAN_RESULT
      Scan result:
      Beacon type:             NW
      Reception channel:       1659
      Long RD ID:              1 (0x00000001)
      NW ID:                   2271560481 (0x87654321)
      RX RSSI-2:               -31dBm
      RX SNR:                  26dB
      RX MCS index:            4
      RX Transmit power:       10 (10 dBm)
      Current cluster channel: not available
      Next cluster channel:    1657
     NET_EVENT_DECT_SCAN_DONE
     Scan request done

PT: Associate with an FT device
===============================

DeSh command ``dect associate``.

The ``dect associate`` command associates a PT device with an FT device.

* Associate with a scanned FT device:

  .. code-block:: console

     desh:~$ dect associate -t 1
     NET_EVENT_DECT_ASSOCIATION_CHANGED
      DECT_ASSOCIATION_CREATED:
       Association created with long RD ID:                 1
       Neighbor role:                                       Parent
     PT: Joined a network
     NET_EVENT_DECT_NETWORK_STATUS:
      Network status: joined

* See the DECT NR+ status:

  .. code-block:: console

     desh:~$ dect status
     DECT NR+ status:
      Modem FW version:             mfw-nr+_nrf91x1_2.0.0
      Modem activated:              yes
      Cluster running:              no
      Network beacon running:       no
      Associations:
         Parent long RD ID:              1 (0x00000001)
            Local IPv6 address:           fe80::1:0:1

* See the networking status of the DECT NR+ interface:

  .. code-block:: console

     desh:~$ net iface

PT: Joining a DECT NR+ network
===============================

DeSh command ``dect nw_join``.

This higher level command combines  the ``scan`` and ``associate`` commands and joins the found network in a set band.

* Join a DECT NR+ network (PT device is activated but not associated with any network):

  .. code-block:: console

     desh:~$ dect nw_join
     ...
     NET_EVENT_DECT_NETWORK_STATUS:
     Network status: joined

PT: ICMPv6 ping an FT device
============================

DeSh command ``ping``.

The ``ping`` command sends ICMPv6 echo request to an FT device using the AF_INET6/SOCK_RAW/IPPROTO_IP sockets.

* PT device: Using global IPv6 address of the FT device (the global address is only available if the FT device is connected to the Internet):

  .. code-block:: console

     desh:~$ ping -d 2001:14bb:119:35b5:0:1:0:1
     Initiating ping to: 2001:14bb:119:35b5:0:1:0:1
     Source IP addr: 2001:14bb:119:35b5:0:1:0:29a
     Destination IP addr: 2001:14bb:119:35b5:0:1:0:1
     Pinging 2001:14bb:119:35b5:0:1:0:1 results: time=1.218secs, payload sent: 0, payload received 0
     ...
     Packets: Sent = 4, Received = 4, Lost = 0 (0% loss)
     Approximate round trip times in milli-seconds:
       Minimum = 992ms, Maximum = 1218ms, Average = 1048ms
     Pinging DONE

* PT device: Using local ipv6 address by first using mDNS to query the address by name:

  .. code-block:: console

     desh:~$ net dns dect-ft-device.local AAAA
     Query for 'dect-ft-device.local' sent.
     dns: fe80::1:0:1
     dns: All results received

     desh:~$ ping -d fe80::1:0:1

* PT device: Using a hostname of the FT device directly:

  .. code-block:: console

     desh:~$ ping -d dect-ft-device.local
     Initiating ping to: dect-ft-device.local
     Source IP addr: fe80::1:0:29a
     Destination IP addr: fe80::1:0:1
     Pinging dect-ft-device.local results: time=1.995secs, payload sent: 0, payload received 0
     ...
     Ping statistics for dect-ft-device.local:
        Packets: Sent = 4, Received = 4, Lost = 0 (0% loss)
     Approximate round trip times in milli-seconds:
        Minimum = 991ms, Maximum = 1995ms, Average = 1242ms
     Pinging DONE

* PT device: Zephyr IP stack ``net ping`` command is also available:

  .. code-block:: console

     desh:~$ net ping fe80::1:0:1

FT: Start RX for receiving raw data
===================================

* Start RX:

  .. code-block:: console

     desh:~$ dect rx start

PT: Send raw data to FT device
==============================

* Send data to a FT device:

  .. code-block:: console

     desh:~$ dect tx -t 1 -d "Hello FT device"

FT: Observe that data is received
=================================

* Observe received data:

  .. code-block:: console

     desh:~$
     Received data (len 16, src long RD ID 4257231875):
       Hello FT device

PT: Release association
=======================

DeSh command ``dect dissociate``.

The ``dect dissociate`` command releases the association between a PT device and an FT device.
You can also use the ``dect nw_unjoin`` command  to release the association and leave the DECT NR+ network.
This does not need the long RD ID of the FT device to be added as a parameter.

* Release the association with an FT device:

  .. code-block:: console

     desh:~$ dect dissociate -t 1

Using DECT NR+ Connection Manager
=================================

DeSh command ``dect connect``.

The ``dect connect`` command connects a DECT NR+ device to the DECT NR+ network.
This higher level command uses the DECT NR+ Connection Manager.
Depending on the configured device type, it can connect to a DECT NR+ cluster (PT device) or create a DECT NR+ network (FT device).

* FT device with Internet connection:

  .. code-block:: console

     desh:~$ dect sett -t 1 --dev_type FT
     desh:~$ dect connect
     connect initiated.
     NET_EVENT_DECT_RSSI_SCAN_RESULT
      RSSI scan result:
      Channel:                             1657
      All subslots free:                   yes
      Busy percentage:                     0%
     NET_EVENT_DECT_RSSI_SCAN_DONE: scan done
     NET_EVENT_DECT_CLUSTER_CREATED_RESULT
      Cluster started/reconfigured at channel 1657.
     NET_EVENT_DECT_NW_BEACON_START_RESULT
      NW beacon started.
     FT: network created
     NET_EVENT_DECT_NETWORK_STATUS:
     Network status: created

* PT device:

  .. code-block:: console

     desh:~$ dect sett -t 2 --dev_type PT
     desh:~$ dect connect
     NET_EVENT_DECT_ASSOCIATION_REQ_RESULT
     Association created with a parent with long RD ID 1
     PT: Joined a network
     NET_EVENT_DECT_NETWORK_STATUS:
     Network status: joined
     NET_EVENT_L4_CONNECTED: Network connectivity established and global IPv6 address assigned, iface 0x20010840
     NET_EVENT_L4_IPV6_CONNECTED: IPv6 connectivity established, iface 0x20010840

  .. note::
     This example output shows that the device is getting an Internet connection over the DECT NR+ network.

MQTT: Remote control using nRF Cloud
====================================

Once you have established an MQTT connection to nRF Cloud using the ``cloud`` command, you can use the **Terminal** window in the nRF Cloud portal to execute DeSh commands to the device.
This feature enables remote control of the DeSh application running on a device that is connected to cloud.
DeSh output, such as responses to commands and other notifications can be echoed to the ``messages`` endpoint and the **Terminal** window of the nRF Cloud portal.
Use the ``print cloud enable`` command to enable this behavior.
The data format of the input data in the **Terminal** window must be JSON.

Examples
--------

* PT device: Enabling printing also to cloud and establish the connection to nRF Cloud:

  .. code-block:: console

     desh:~$ print cloud enable

     desh:~$ cloud connect

* nRF Cloud: To request the DECT NR+ neighbor list for the PT device, enter the following command in the **Terminal** window of the nRF Cloud portal:

  .. code-block:: console

     {"appId":"DECT_SHELL", "data":"dect neighbor_list"}

  The response appears in the **d2c** terminal.

* nRF Cloud: To request icmpv6 ping towards Internet from a PT device, enter the following command in the **Terminal** window of the nRF Cloud portal:

  .. code-block:: console

     {"appId":"DECT_SHELL", "data":"ping -d nordicsemi.com"}

  The response appears in the **d2c** terminal.

Iperf3
======

DeSh command ``iperf``.

The ``iperf3`` command starts the iperf3 tool that is used for measuring data transfer performance both in uplink and downlink direction.

.. note::
   Run the iperf3 server on the FT device and the iperf3 client on the PT device (direct connection between the devices).
   Some features, for example, file operations and TCP option tuning, are not supported.

Examples
--------

* FT device: Create a network and see local IPv6 address for the DECT NR+ networking interface:

  .. code-block:: console

     desh:~$ dect sett -t 1 --dev_type FT

     desh:~$ dect connect
     ...
     NET_EVENT_DECT_CLUSTER_CREATED_RESULT
     Cluster started/reconfigured at channel 1659.
     FT: network created
     NET_EVENT_DECT_NETWORK_STATUS:
     Network status: created
     ...
     NET_EVENT_DECT_ASSOCIATION_CHANGED
     DECT_ASSOCIATION_CREATED:
      Association created with long RD ID:                 3872119375
      Neighbor role:                                       Child
     ...
     desh:~$ net iface 1
     Hostname: dect-ft-device
     Default interface: 1

     Interface dect0 (0x20017070) (<unknown type>) [1]
     =========================================
     Interface is down.
     Link addr : 00:00:00:01:00:00:00:01
     MTU       : 1280
     Flags     : AUTO_START,IPv6,NO_ND
     Device    : dect0 (0x57b2c)
     Status    : oper=DORMANT, admin=UP, carrier=ON
     IPv6 unicast addresses (max 2):
           fe80::1:0:1 autoconf preferred infinite
     IPv6 multicast addresses (max 3):
           ff02::fb  <not joined>
     IPv6 prefixes (max 2):
           <none>
     IPv6 hop limit           : 64
     IPv6 base reachable time : 30000
     IPv6 reachable time      : 23839
     IPv6 retransmit timer    : 0

* FT device: Start iperf3 server on the DECT NR+ networking interface on port 5555:

  .. code-block:: console

     desh:~$ iperf3 -s -B fe80::1:0:1 -p 5555 -1 -V -6

* PT device: Connect iperf3 client to the FT device iperf3 server on port 5555, using UDP protocol, with a payload size of 1220 bytes, for 50 seconds, and with a bandwidth of 2 Mbps:

  .. code-block:: console

     desh:~$ iperf3 -c fe80::1:0:1 -p 5555 -V -6 -u -l 1220 -t 50 -O 6 -b 1500k

User interface
==============

The buttons have the following functions:

Button 1:
   Raises a kill or abort signal.
   A long press of the button kills or aborts all supported running commands.
   You can abort commands ``iperf3`` and ``ping``.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following Kconfig options:

.. options-from-kconfig::
   :show-type:

Building
********

.. |sample path| replace:: :file:`samples/dect/dect_shell`

.. include:: /includes/build_and_run_ns.txt

See :ref:`cmake_options` for instructions on how to provide CMake options, for example to use a configuration overlay.

FT/Sink: Border Router
======================

This section describes how to build the DeSh sample to have Internet connection through Border Router, which is a cellular modem running on external 9151DK.

LTE with `Serial Modem <ncs-serial-modem_>`_ running on external nRF9151 DK
---------------------------------------------------------------------------

* `Serial Modem <ncs-serial-modem_>`_ :

  .. note::
     Change the current speed of uart2 to 1000000 (in the :file:`overlay-external-mcu.overlay` file) to reflect the DECT side speed where current speed of uart1 is 1000000.

  .. code-block:: console

     west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE="overlay-ppp.conf;overlay-cmux.conf" -DEXTRA_DTC_OVERLAY_FILE=overlay-external-mcu.overlay -Dapp_SNIPPET=nrf91-modem-trace-uart

* DeSh with FT/Sink configuration using Zephyr's cellular modem feature:

  .. code-block:: console

     nrf/samples/dect/dect_shell:
     west build -p -b nrf9151dk/nrf9151/ns -- -DFILE_SUFFIX=sm

* Wiring as in the DeSh :file:`boards/nrf9151dk_nrf9151_ns_sm.overlay` and `Serial Modem <ncs-serial-modem_>`_ :file:`ncs-serial-modem/app/overlay-external-mcu.overlay` files:

  .. table:: Wire the DKs together as shown.

     +------------------+-----------------+
     | Serial  Modem    | DECT NR+ Sink   |
     +------------------+-----------------+
     | nRF9151 DK       | nRF9151 DK      |
     |                  |                 |
     +==================+=================+
     | **P0.10** (TX)   | **P0.10** (RX)  |
     +------------------+-----------------+
     | **P0.11** (RX)   | **P0.11** (TX)  |
     +------------------+-----------------+
     | **P0.12** (RTS)  | **P0.12** (CTS) |
     +------------------+-----------------+
     | **P0.13** (CTS)  | **P0.13** (RTS) |
     +------------------+-----------------+
     | **P0.30** (RI)   | **P0.30** (RING)|
     +------------------+-----------------+
     | **P0.31** (DTR)  | **P0.31** (DTR) |
     +------------------+-----------------+
     | GND              | GND             |
     +------------------+-----------------+

.. note::
   Change VDD (nPM VOUT1) from 1.8V to 3.3V using the `Board Configurator app`_ in both DKs.

iperf3 support
==============

To build the DeSh sample with iperf3 support, for example:

PT (or FT without the sink) device: DeSh with Zephyr's network management-based shell commands:

.. code-block:: console

   nrf9151dk:
   west build -p -b nrf9151dk/nrf9151/ns

   With iperf3 support with TX optimized (usually acts as iperf3 client in PT device):
   west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE="iperf3-common.conf;iperf3-tx.conf"

   With iperf3 support with RX optimized (usually acts as iperf3 server in FT device):
   west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE="iperf3-common.conf;iperf3-rx.conf"

nRF Cloud
=========

nRF Cloud offers location services and allows devices to report data to the cloud for collection and analysis.
This section describes how to build the DeSh sample with nRF Cloud support using MQTT and CoAP protocols.

Certificates
------------

You can store certificates on the device using a custom AT%CMNG command (implemented by DeSh custom AT command) in the following two ways:

* Directly with the AT command (``desh:~$ at at%cmng=******``) or the ``at_cmd_mode`` command (``desh:~$ at_cmd_mode start``), or by a script.
* Using the `Cellular Monitor app`_ to store the certificates to the modem (default nRF Cloud security tag).

  .. code-block:: console

     desh:~$ dect deactivate
     desh:~$ at at_cmd_mode start

.. note::
   As a result of the custom %CMNG command in DeSh, the MQTT credentials are stored insecurely in settings, but with CoAP, they are stored more securely in the Protected Storage.

MQTT
----

.. code-block:: console

   nrf9151dk:
   $ west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE="nrf_cloud_mqtt.conf"

CoAP
----

.. code-block:: console

   nrf9151dk:
   $ west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE="nrf_cloud_coap.conf"

.. note::
   System time is retrieved by using NTP.
   For the CA certificate, only the nRF Cloud CoAP CA certificate needs to be stored on the device with CoAP.
   Do not store the Amazon root CA certificate on the device with CoAP due to crypto limitations for handling RSA certificates.

Dependencies
************

* DECT NR+ :ref:`Connection Manager <zephyr:conn_mgr_overview>` and related APIs:

  * .. doxygengroup:: dect_net_l2_mgmt
  * .. doxygengroup:: dect_net_l2

This sample uses the following |NCS| libraries:

* :ref:`dk_buttons_and_leds_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>`
