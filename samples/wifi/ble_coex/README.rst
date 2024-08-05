.. _wifi_sr_coex_sample:
.. _wifi_ble_coex_sample:

Wi-Fi: Bluetooth LE coexistence
###############################

.. contents::
   :local:
   :depth: 2

The Bluetooth LE coexistence sample demonstrates coexistence between Wi-Fi® and Bluetooth® LE radios in 2.4 GHz frequency.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Running test cases for this sample requires additional software, such as the Wi-Fi **iPerf** application.
Use iPerf version 2.0.5.
For more details, see `Network Traffic Generator`_.

Overview
********

The sample demonstrates how the coexistence mechanism is implemented, and how it can be enabled and disabled between Wi-Fi and Bluetooth LE radios in the 2.4 GHz band.
This is done by using the throughput of the Wi-Fi client and the Bluetooth LE central.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_coex.svg
     :alt: Wi-Fi and Bluetooth LE Coex test setup

     Wi-Fi and Bluetooth LE coexistence reference test and evaluation setup

The reference test setup shows the connections between the following devices:

* :term:`Device Under Test (DUT)` (nRF7002 DK on which the coexistence sample runs)
* Wi-Fi peer device (access point with test PC that runs iPerf)
* Bluetooth LE peer device (nRF5340 DK on which Bluetooth LE throughput sample runs)

The following table provides more details on the sample or application that runs on DUT and peer devices:

+--------------+----------------+------------------------------------------------------------------------------------+
| Device       | Application    |                             Details                                                |
+==============+================+====================================================================================+
| nRF7002 DK   | Bluetooth LE   | The sample runs Wi-Fi throughput only, Bluetooth LE throughput only,               |
| (DUT)        | coexistence    | or a combination of both based on configuration selections in the                  |
|              | sample         | :file:`prj.conf`.                                                                  |
+--------------+----------------+------------------------------------------------------------------------------------+
| Test PC      | iPerf          | Wi-Fi iPerf UDP server is run on the test PC, and this acts as a peer device to    |
|              | application    | the Wi-Fi UDP client that runs on the nRF7002 DK.                                  |
+--------------+----------------+------------------------------------------------------------------------------------+
| nRF5340 DK   | Bluetooth LE   | Bluetooth LE throughput sample is run in peripheral mode on the nRF5340 DK, and    |
| (peer)       | throughput     | this acts as a peer device to Bluetooth LE central that runs on the nRF7002 DK.    |
|              | sample         |                                                                                    |
+--------------+----------------+------------------------------------------------------------------------------------+

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/ble_coex/Kconfig`):

.. options-from-kconfig::
   :show-type:

Additional configuration
========================

To enable different test modes, set up the following configuration parameters in the :file:`prj.conf` file:

* Test modes: Use the following Kconfig options to select the required test case:

  * :kconfig:option:`CONFIG_TEST_TYPE_WLAN_ONLY` for Wi-Fi-only test.
  * :kconfig:option:`CONFIG_TEST_TYPE_BLE_ONLY` for Bluetooth LE-only test.
  * :kconfig:option:`CONFIG_TEST_TYPE_WLAN_BLE` for concurrent Wi-Fi and Bluetooth LE test.

  Based on the required test, set only one of these to ``y``.
* Test duration: Use the :kconfig:option:`CONFIG_WIFI_TEST_DURATION` Kconfig option to set the duration of the Wi-Fi test and :kconfig:option:`CONFIG_BLE_TEST_DURATION` for the Bluetooth LE test.
  The units are in milliseconds.
  For example, to set the test for 20 seconds, set these values to ``20000``.
  For the concurrent Wi-Fi and Bluetooth LE test, make sure that both are set to the same duration to ensure maximum overlap.
* Bluetooth LE configuration: Set the Bluetooth LE connection interval limits using the :kconfig:option:`CONFIG_INTERVAL_MIN` and :kconfig:option:`CONFIG_INTERVAL_MAX` Kconfig options.
  The units are 1.25 milliseconds.
  For example, ``CONFIG_INTERVAL_MIN=80`` corresponds to an interval of 100 ms (80 x 1.25).

* Wi-Fi connection: Configure the following Wi-Fi credentials in the :file:`prj.conf`: appropriately as per the credentials of the access point used for this testing:


.. include:: /includes/wifi_credentials_static.txt

.. note::
   You can also use ``menuconfig`` to configure ``Wi-Fi credentials``.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

* Wi-Fi throughput test: Set the :kconfig:option:`CONFIG_NET_CONFIG_PEER_IPV4_ADDR` Kconfig option appropriately as per the Wi-Fi interface IP address of the test PC on which iPerf is run.

Set up the test duration configuration parameters in the :file:`Kconfig.conf` file of the Bluetooth throughput sample from the :file:`nrf/samples/bluetooth/throughput` folder.
Use :kconfig:option:`CONFIG_BT_THROUGHPUT_DURATION` to set the duration of the Bluetooth LE throughput test only when running Bluetooth LE throughput in central role.
The units are in milliseconds.

.. note::
   Use the same test duration value for :kconfig:option:`CONFIG_WIFI_TEST_DURATION`, :kconfig:option:`CONFIG_BLE_TEST_DURATION`, and :kconfig:option:`CONFIG_BT_THROUGHPUT_DURATION`.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/ble_coex`

.. include:: /includes/build_and_run_ns.txt

You can build the coexistence sample for the following configurations:

* Wi-Fi throughput only
* Bluetooth LE throughput only
* Concurrent Wi-Fi and Bluetooth LE throughput (with coexistence enabled and disabled modes)

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following are examples of the CLI commands:

* Build with coexistence disabled:

  .. code-block:: console

     west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_MPSL_CX=n -Dipc_radio_CONFIG_MPSL_CX=n

Use this command for Wi-Fi throughput only, Bluetooth LE throughput only, or concurrent Wi-Fi and Bluetooth LE throughput with coexistence disabled tests.

* Build with coexistence enabled:

  .. code-block:: console

     west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_MPSL_CX=y -Dipc_radio_CONFIG_MPSL_CX=y

Use this command for concurrent Wi-Fi and Bluetooth LE throughput with coexistence enabled test.

Change the board target as given below for the nRF7001 DK, nRF7002 EK, and nRF7001 EK.

* Board target for nRF7001 DK:

  .. code-block:: console

     nrf7002dk/nrf5340/cpuapp/nrf7001

* Board target for nRF7002 EK and nRF7001 EK:

  .. code-block:: console

     nrf5340dk/nrf5340/cpuapp

Add the following SHIELD options for the nRF7002 EK and nRF7001 EK.

* For nRF7002 EK:

  .. code-block:: console

     -DSHIELD=nrf7002ek

* For nRF7001 EK:

  .. code-block:: console

     -DSHIELD=nrf7002ek_nrf7001

The generated HEX file to be used is :file:`ble_coex/build/merged.hex`.

Use the Bluetooth throughput sample from the :file:`nrf/samples/bluetooth/throughput` folder on the peer nRF5340 DK device.

Build for the nRF5340 DK:

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp

The generated HEX file to be used is :file:`throughput/build/merged.hex`.

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|

#. Run the following command to check the available devices:

   .. code-block:: console

      nrfjprog --com

   This command returned the following output in the setup used to run the coexistence tests.

   .. code-block:: console

      1050043161         /dev/ttyACM0    VCOM0
      1050043161         /dev/ttyACM1    VCOM1
      1050724225         /dev/ttyACM2    VCOM0
      1050724225         /dev/ttyACM3    VCOM1


   In this example, ``1050043161`` is the serial number of the nRF5340 DK and ``1050724225`` is the serial number of the nRF7002 DK.

   While connecting to a particular device, use the ``/dev/ttyACM`` corresponding to VCOM1.
   In the example, use ``/dev/ttyACM1`` to connect to the device with the serial number ``1050043161``.
   Similarly, use ``/dev/ttyACM3`` to connect to the device with the serial number ``1050724225``.

#. Run the following commands to connect to the desired devices:

   .. code-block:: console

      minicom -D /dev/ttyACM1 -b 115200
      minicom -D /dev/ttyACM3 -b 115200

Programming DKs
===============

Complete the following steps to program the DKs:

.. tabs::

   .. group-tab:: nRF5340 DK

      1. |open_terminal_window_with_environment|
      #. Navigate to the :file:`<ncs code>/nrf/samples/bluetooth/throughput/` folder.
      #. Run the following command:

         .. code-block:: console

            west flash --dev-id <device-id> --hex-file build/merged.hex

   .. group-tab:: nRF7002 DK

      1. |open_terminal_window_with_environment|
      #. Navigate to the :file:`<ncs code>/nrf/samples/wifi/ble_coex/` folder.
      #. Run the following command:

         .. code-block:: console

            west flash --dev-id <device-id> --hex-file build/merged.hex

Test procedure
==============

The following table provides the procedure to run Wi-Fi only, Bluetooth LE-only, and combined throughput.

+---------------+--------------+----------------------------------------------------------------+
| Test case     | Coexistence  | Test procedure                                                 |
+===============+==============+================================================================+
| Wi-Fi-only    | N.A.         | Run Wi-Fi iPerf in server mode on the test PC.                 |
| throughput    |              | Build the coexistence sample for Wi-Fi-only throughput and     |
|               |              | program the application on the nRF7002 DK.                     |
+---------------+--------------+----------------------------------------------------------------+
| Bluetooth LE  | N.A.         | Program the Bluetooth LE throughput application on the nRF5340 |
| -only         |              | DK and select role as peripheral.                              |
| throughput    |              | Build the coexistence sample for Bluetooth LE-only and program |
|               |              | the application on the nRF7002 DK.                             |
+---------------+--------------+----------------------------------------------------------------+
| Wi-Fi and     | Disabled/    | Run Wi-Fi iPerf in server mode on the test PC.                 |
| Bluetooth LE  | Enabled      | Program the Bluetooth LE throughput application on the nRF5340 |
| combined      |              | DK and select role as peripheral.                              |
| throughput    |              | Build the coexistence sample for concurrent Wi-Fi and Bluetooth|
|               |              | LE throughput and program the application on the nRF7002 DK.   |
+---------------+--------------+----------------------------------------------------------------+

When the sample is executed, the Wi-Fi UDP throughput operates with the Device Under Test (DUT) in client mode.
To run the UDP throughput in server mode on the peer device (test PC), use the following command.

.. code-block:: console

   iperf -s -i 1 -u

Observe that the Wi-Fi throughput result appears on the test PC terminal on which iPerf server is run.
The Bluetooth LE throughput result appears on the minicom terminal connected to the nRF5340 DK.

Results
=======

The following tables summarize the results obtained from coexistence tests conducted in a clean RF environment for different Wi-Fi operating bands, antenna configurations, and Wi-Fi modes.
These results are representative and might vary based on the RSSI and the level of external interference.

Wi-Fi (802.11n mode) in 2.4 GHz
-------------------------------

Separate antennas:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only,            | 10.2               | N.A.               |
| client (UDP TX)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE-only,     | N.A.               | 1107               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 9.9                | 145                |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 8.3                | 478                |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Shared antenna:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only,            | 10.2               | N.A.               |
| client (UDP TX)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE-only,     | N.A.               | 1219               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 10.2               | 29                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 6.2                | 749                |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Wi-Fi (802.11b mode) in 2.4 GHz
-------------------------------

Separate antennas:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only,            | 3.5                | N.A.               |
| client (UDP TX)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE-only,     | N.A.               | 1042               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 3.3                | 110                |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 2.2                | 563                |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Shared antenna:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only,            | 3.5                | N.A.               |
| client (UDP TX)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE-only,     | N.A.               | 1190               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 3.4                | 59                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 2.2                | 508                |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Wi-Fi (802.11n mode) in 5 GHz
-----------------------------

Separate antennas:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only,            | 10.2               | N.A.               |
| client (UDP TX)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE-only,     | N.A.               | 1139               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 10.2               | 1188               |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 10.2               | 1208               |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Shared antenna:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only,            | 10.2               | N.A.               |
| client (UDP TX)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE-only,     | N.A.               | 1180               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 10.2               | 1177               |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 10.2               | 1191               |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Sample output
=============

The following screenshots show coexistence test results obtained for separate antenna configuration with Wi-Fi mode set to 802.11n.
These tests were run with WLAN connected to an AP in 2.4 GHz band.
In the images, the top image result shows Wi-Fi throughput that appears on a test PC terminal in which Wi-Fi iPerf server is run and the bottom image result shows Bluetooth LE throughput that appears on a minicom terminal in which the Bluetooth LE throughput sample is run.

.. figure:: /images/wifi_coex_wlan.png
     :width: 780px
     :align: center
     :alt: Wi-Fi-only throughput

     Wi-Fi-only throughput 10.2 Mbps

.. figure:: /images/wifi_coex_ble.png
     :width: 780px
     :align: center
     :alt: Bluetooth LE-only throughput

     Bluetooth LE-only throughput: 1107 kbps

.. figure:: /images/wifi_coex_wlan_ble_cd.png
     :width: 780px
     :align: center
     :alt: Wi-Fi and Bluetooth LE CD

     Wi-Fi and Bluetooth LE throughput, coexistence disabled: Wi-Fi 9.9 Mbps and Bluetooth LE 145 kbps

.. figure:: /images/wifi_coex_wlan_ble_ce.png
     :width: 780px
     :align: center
     :alt: Wi-Fi and Bluetooth LE CE

     Wi-Fi and Bluetooth LE throughput, coexistence enabled: Wi-Fi 8.3 Mbps and Bluetooth LE 478 kbps

The results show that coexistence harmonizes airtime between Wi-Fi and Bluetooth LE rather than resulting in a higher combined throughput.
This is consistent with the design intent.

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
