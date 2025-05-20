.. _bt_scanning_while_connecting:

Bluetooth: Scanning while connecting
####################################

.. contents::
   :local:
   :depth: 2

The sample demonstrates how to reduce the time to establish connections to many devices, typically done when provisioning devices to a network.
The total connection establishment time is reduced by scanning while connecting and by using the filter accept list.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

The sample also requires at least one other development kit.
Out of the box, this sample can be used together with the :ref:`bt_peripheral_with_multiple_identities`.
This sample filters out devices with a different name than a device running the :ref:`bt_peripheral_with_multiple_identities` sample.
The :ref:`bt_peripheral_with_multiple_identities` sample acts as multiple peripheral devices.

Overview
********

You can use this sample as a starting point to implement an application designed to provision a network of Bluetooth peripherals.
The approaches demonstrated in this sample will reduce the total time to connect to many devices.
A typical use-case is a gateway in a network of devices using Periodic Advertising with Responses.

To illustrate how connection establishment speed can be improved, it measures the time needed to connect to 16 devices with three different modes:

* sequential scanning and connection establishment
* concurrent scanning while connecting
* concurrent scanning while connecting with the filter accept list.

Sequential scanning and connection establishment
================================================

This is the slowest and simplest approach.
Sequential scanning and connection establishment are recommended when the application only needs to connect a handful of devices.

After a device is discovered, the application stops scanning and attempts to connect to the device.
Once the connection is established, the application starts the scanner again to discover other connectable devices.
The following message sequence chart illustrates the sequence of events.

.. msc::
    hscale = "1.3";
    App, Stack, Peers;
    App=>Stack [label="scan_start()"];
    Peers=>Stack [label="ADV_IND(A)"];
    Stack=>App [label="scan_recv(A)"];
    App=>Stack [label="scan_stop()"];
    App=>Stack [label="bt_conn_le_create(A)"];
    Peers=>Stack [label="ADV_IND(A)"];
    Stack=>Peers [label="CONNECT_IND(A)"];
    Stack=>App [label="connected_cb(A)"];
    App=>Stack [label="scan_start()"];
    Peers=>Stack [label="ADV_IND(B)"];
    Stack=>App [label="scan_recv(B)"];
    App=>Stack [label="scan_stop()"];
    App=>Stack [label="bt_conn_le_create(B)"];
    Peers=>Stack [label="ADV_IND(B)"];
    Stack=>Peers [label="CONNECT_IND(B)"];
    Stack=>App [label="connected_cb(B)"];

Concurrent scanning while connecting
====================================

This mode requires the application to enable the :kconfig:option:`CONFIG_BT_SCAN_AND_INITIATE_IN_PARALLEL` Kconfig option.
In this mode, the scanner is not stopped when the application creates connections.
During a connection establishment procedure to a device, the application caches other devices it also wants to connect to.
Once the connection establishment procedure is complete, it can immediately initiate a new connection establishment procedure to the cached device.
When connecting to a cached device, the connection establishment procedure takes one less advertising interval compared to the sequential scanning and connection establishment mode.

.. msc::
    hscale = "1.3";
    App, Stack, Peers;
    App=>Stack [label="scan_start()"];
    Peers=>Stack [label="ADV_IND(A)"];
    Stack=>App [label="scan_recv(A)"];
    App=>Stack [label="bt_conn_le_create(A)"];
    Peers=>Stack [label="ADV_IND(B)"];
    App rbox App [label="Cache address B"];
    Peers=>Stack [label="ADV_IND(A)"];
    Stack=>Peers [label="CONNECT_IND(A)"];
    Stack=>App [label="connected_cb(A)"];
    App=>Stack [label="bt_conn_le_create(B)"];
    Peers=>Stack [label="ADV_IND(B)"];
    Stack=>Peers [label="CONNECT_IND(B)"];
    Stack=>App [label="connected_cb(B)"];

Concurrent scanning while connecting with the filter accept list
================================================================

This mode requires the application to enable the :kconfig:option:`CONFIG_BT_FILTER_ACCEPT_LIST` Kconfig option in addition to :kconfig:option:`CONFIG_BT_SCAN_AND_INITIATE_IN_PARALLEL`.
When the application starts the connection establishment procedure with the filter accept list, it can connect to any of the previously cached devices.
This reduces the total connection setup time even more, because connection establishment is not relying on the on-air presence of only one of the cached devices.

.. msc::
    hscale = "1.3";
    App, Stack, Peers;
    App=>Stack [label="scan_start()"];
    Peers=>Stack [label="ADV_IND(A)"];
    Stack=>App [label="scan_recv(A)"];
    App=>Stack [label="bt_conn_le_create(A)"];
    Peers=>Stack [label="ADV_IND(B)"];
    Peers=>Stack [label="ADV_IND(C)"];
    Peers=>Stack [label="ADV_IND(D)"];
    App rbox App [label="Cache addresses B, C, D"];
    Peers=>Stack [label="ADV_IND(A)"];
    Stack=>Peers [label="CONNECT_IND(A)"];
    Stack=>App [label="connected_cb(A)"];
    App rbox App [label="Set filter accept list to\nB, C, D"];
    App=>Stack [label="bt_conn_le_create_auto()"];
    Stack rbox Stack [label="The stack will connect to the first present ADV_IND of\nthe peers B,C,D"];
    Peers=>Stack [label="ADV_IND(C)"];
    Stack=>Peers [label="CONNECT_IND(C)"];
    Stack=>App [label="connected_cb(C)"];

.. note::
   This sample application assumes it will never have to cache more devices than the maximum number of addresses that can be stored in the filter accept list.
   For applications that cannot adhere to this simplification, the function :c:func:`cache_peer_address` can be changed to not store more than defined by the :kconfig:option:`CONFIG_BT_CTLR_FAL_SIZE` Kconfig option.
   Another simplification done in the sample application is storing duplicate devices in the filter accept list.

Configuration
*************

|config|

Building and running
********************

.. |sample path| replace:: :file:`samples/bluetooth/scanning_while_connecting`

.. include:: /includes/build_and_run.txt

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Observe that the sample connects and prints out how much time it takes to connect to all peripherals.

Sample output
=============

The result should look similar to the following output::

   *** Booting nRF Connect SDK v2.8.99-1c63490f0539 ***
   *** Using Zephyr OS v3.7.99-b9bc0846b926 ***
   I: SoftDevice Controller build revision:
   I: 49 40 e2 c0 6b e5 0d b3 |I@..k...
   I: ba a6 48 5e 49 a6 95 3d |..H^I..=
   I: 65 35 b6 7c             |e5.|
   I: HW Platform: Nordic Semiconductor (0x0002)
   I: HW Variant: nRF54Lx (0x0005)
   I: Firmware: Standard Bluetooth controller (0x00) Version 73.57920 Build 233139136
   I: Identity: F6:BF:24:7D:46:5D (random)
   I: HCI: version 6.0 (0x0e) revision 0x3030, manufacturer 0x0059
   I: LMP: version 6.0 (0x0e) subver 0x3030
   I: Bluetooth initialized

   I: SEQUENTIAL_SCAN_AND_CONNECT:
   I: starting sample benchmark
   I: Connected to FF:AB:68:0C:34:FD (random), number of connections 1
   I: Connected to E2:12:BF:D3:FB:D5 (random), number of connections 2
   I: Connected to EE:37:AA:62:A2:FB (random), number of connections 3
   I: Connected to C7:9B:42:1B:48:F8 (random), number of connections 4
   I: Connected to F0:27:5B:37:0F:4B (random), number of connections 5
   I: Connected to C8:BA:1D:6F:95:2B (random), number of connections 6
   I: Connected to DF:02:31:C3:0B:C2 (random), number of connections 7
   I: Connected to C7:E1:60:7A:F1:E0 (random), number of connections 8
   I: Connected to CA:89:50:33:AB:31 (random), number of connections 9
   I: Connected to E9:47:6B:FA:2F:DE (random), number of connections 10
   I: Connected to EF:92:DC:88:3B:B3 (random), number of connections 11
   I: Connected to F4:9C:8C:24:F9:44 (random), number of connections 12
   I: Connected to DD:84:44:64:5D:FB (random), number of connections 13
   I: Connected to FB:92:1D:8E:8C:D8 (random), number of connections 14
   I: Connected to D9:E5:51:E0:5E:24 (random), number of connections 15
   I: Connected to CF:2F:99:89:A3:4D (random), number of connections 16
   I: 8 seconds to create 16 connections
   I: Disconnecting connections...
   I: ---------------------------------------------------------------------
   I: ---------------------------------------------------------------------
   I: CONCURRENT_SCAN_AND_CONNECT:
   I: starting sample benchmark
   I: Connected to F0:27:5B:37:0F:4B (random), number of connections 1
   I: Connected to EE:37:AA:62:A2:FB (random), number of connections 2
   I: Connected to D1:3D:B1:AA:84:27 (random), number of connections 3
   I: Connected to CA:89:50:33:AB:31 (random), number of connections 4
   I: Connected to C0:38:F8:47:10:17 (random), number of connections 5
   I: Connected to E9:47:6B:FA:2F:DE (random), number of connections 6
   I: Connected to D9:E5:51:E0:5E:24 (random), number of connections 7
   I: Connected to FB:92:1D:8E:8C:D8 (random), number of connections 8
   I: Connected to C7:E1:60:7A:F1:E0 (random), number of connections 9
   I: Connected to E2:6D:21:28:C7:DB (random), number of connections 10
   I: Connected to DF:02:31:C3:0B:C2 (random), number of connections 11
   I: Connected to F0:F2:2A:C1:F7:72 (random), number of connections 12
   I: Connected to DD:84:44:64:5D:FB (random), number of connections 13
   I: Connected to F4:9C:8C:24:F9:44 (random), number of connections 14
   I: Connected to EF:92:DC:88:3B:B3 (random), number of connections 15
   I: Connected to C8:BA:1D:6F:95:2B (random), number of connections 16
   I: 7 seconds to create 16 connections
   I: Disconnecting connections...
   I: ---------------------------------------------------------------------
   I: ---------------------------------------------------------------------
   I: CONCURRENT_SCAN_AND_CONNECT_FILTER_ACCEPT_LIST:
   I: starting sample benchmark
   I: Connected to DD:84:44:64:5D:FB (random), number of connections 1
   I: Connected to C7:E1:60:7A:F1:E0 (random), number of connections 2
   I: Connected to C7:9B:42:1B:48:F8 (random), number of connections 3
   I: Connected to E9:47:6B:FA:2F:DE (random), number of connections 4
   I: Connected to E2:12:BF:D3:FB:D5 (random), number of connections 5
   I: Connected to FB:92:1D:8E:8C:D8 (random), number of connections 6
   I: Connected to F0:F2:2A:C1:F7:72 (random), number of connections 7
   I: Connected to CA:89:50:33:AB:31 (random), number of connections 8
   I: Connected to F4:9C:8C:24:F9:44 (random), number of connections 9
   I: Connected to E2:6D:21:28:C7:DB (random), number of connections 10
   I: Connected to FF:AB:68:0C:34:FD (random), number of connections 11
   I: Connected to EE:37:AA:62:A2:FB (random), number of connections 12
   I: Connected to D1:3D:B1:AA:84:27 (random), number of connections 13
   I: Connected to CF:2F:99:89:A3:4D (random), number of connections 14
   I: Connected to EF:92:DC:88:3B:B3 (random), number of connections 15
   I: Connected to D9:E5:51:E0:5E:24 (random), number of connections 16
   I: 2 seconds to create 16 connections
   I: Disconnecting connections...
   I: ---------------------------------------------------------------------
   I: ---------------------------------------------------------------------
