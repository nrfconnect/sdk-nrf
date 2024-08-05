.. _wifi_sr_coex_sample:
.. _wifi_ble_coex_sample:

Wi-Fi: Bluetooth LE coexistence
###############################

.. contents::
   :local:
   :depth: 2

The Bluetooth LE coexistence sample demonstrates coexistence between Wi-Fi® and Bluetooth LE radios in 2.4 GHz frequency.
The sample documentation includes details of test setup used, build procedure, test procedure and the results obtained when the sample is run on the nRF7002 DK.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how the coexistence mechanism is implemented and enabled and disabled between Wi-Fi and Bluetooth® LE radios in 2.4 GHz band using Wi-Fi client’s throughput and Bluetooth LE central’s throughput.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_coex.svg
     :alt: Wi-Fi Coex test setup

     Wi-Fi coexistence reference test and evaluation setup

The reference test setup shows the connections between the following devices:

* :term:`Device Under Test (DUT)` (nRF7002 DK on which the coexistence sample runs)
* Wi-Fi peer device (access point with test PC that runs **iperf**)
* Bluetooth LE peer device (nRF5340 DK on which Bluetooth LE throughput sample runs)

This setup is kept in a shielded test enclosure box (for example, a Ramsey box).
The following table provides more details on the sample or application that runs on DUT and peer devices:

+--------------+----------------+------------------------------------------------------------------------------------+
| Device       | Application    |                             Details                                                |
+==============+================+====================================================================================+
| nRF7002 DK   | Bluetooth LE   | The sample runs Wi-Fi throughputs, Bluetooth LE throughputs                        |
|              | coex sample    | or a combination of both based on configuration selections in the                  |
|              |                | :file:`prj.conf` file.                                                             |
+--------------+----------------+------------------------------------------------------------------------------------+
| Test PC      | **iperf**      | Wi-Fi **iperf** UDP server is run on the test PC, and this acts as a peer device to|
|              | application    | Wi-Fi UDP client that runs on the nRF7002 DK.                                      |
+--------------+----------------+------------------------------------------------------------------------------------+
| nRF5340 DK   | Bluetooth LE   | Bluetooth LE throughput sample is run in peripheral mode on the nRF5340 DK, and    |
|              | throughput     | this acts as a peer device to Bluetooth LE central that runs on the nRF7002 DK.    |
|              | sample         |                                                                                    |
+--------------+----------------+------------------------------------------------------------------------------------+

To trigger concurrent transmissions at RF level on both Wi-Fi and Bluetooth LE, the sample runs traffic on separate threads, one for each.
The sample uses standard Zephyr threads.
The threads are configured with non-negative priority (pre-emptible thread).
For details on threads and scheduling, refer to `Threads`_.

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

* Antenna configuration: Use the :kconfig:option:`CONFIG_COEX_SEP_ANTENNAS` Kconfig option to select the antenna configuration.
  Set it to ``y`` to enable separate antennas and ``n`` to enable shared antenna.
* Test modes: Use the following Kconfig options to select the required test case:

  * :kconfig:option:`CONFIG_TEST_TYPE_WLAN_ONLY` for Wi-Fi only test
  * :kconfig:option:`CONFIG_TEST_TYPE_BLE_ONLY` for Bluetooth LE only test
  * :kconfig:option:`CONFIG_TEST_TYPE_WLAN_BLE` for concurrent Wi-Fi and Bluetooth LE test.

  Based on the required test, set only one of these to ``y``.
* Test duration: Use the :kconfig:option:`CONFIG_WIFI_TEST_DURATION` Kconfig option to set the duration of the Wi-Fi test and :kconfig:option:`CONFIG_BLE_TEST_DURATION` for the Bluetooth LE test.
  The units are in milliseconds.
  For example, to set the tests for 20 seconds, set the respective values to ``20000``.
  For the concurrent Wi-Fi and Bluetooth LE test, make sure that both are set to the same duration to ensure maximum overlap.
* Bluetooth LE configuration: Set the Bluetooth LE connection interval limits using the :kconfig:option:`CONFIG_INTERVAL_MIN` and :kconfig:option:`CONFIG_INTERVAL_MAX` Kconfig options.
  The units are 1.25 milliseconds.
  For example, ``CONFIG_INTERVAL_MIN=80`` corresponds to an interval of 100 ms (80 x 1.25).
* Wi-Fi connection: Use the :kconfig:option:`CONFIG_NET_CONFIG_PEER_IPV4_ADDR` Kconfig option to establish a connection to a peer host.

You must configure the following Wi-Fi credentials in the :file:`prj.conf` file:

.. include:: /includes/wifi_credentials_static.txt

.. note::
   You can also use ``menuconfig`` to configure ``Wi-Fi credentials``.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

Set up the following configuration parameters in the :file:`prj_nrf5340dk_nrf5340_cpuapp.conf` file:

* File or time-based throughput: Use :kconfig:option:`CONFIG_BT_THROUGHPUT_FILE` to select file or time-based throughput test.
  Set it to ``n`` to enable time-based throughput test only when running Bluetooth LE throughput in central role.
* Test duration: Use :kconfig:option:`CONFIG_BT_THROUGHPUT_DURATION` to set the duration of the Bluetooth LE throughput test only when running Bluetooth LE throughput in central role.
  The units are in milliseconds.

.. note::
   Use the same test duration value for :kconfig:option:`CONFIG_WIFI_TEST_DURATION`, :kconfig:option:`CONFIG_BLE_TEST_DURATION`, and :kconfig:option:`CONFIG_BT_THROUGHPUT_DURATION`.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/ble_coex`

.. include:: /includes/build_and_run_ns.txt

The sample can be built for the following configurations:

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

Connecting to DKs
=================

After the DKs are connected to the test PC through USB connectors and powered on, open a suitable terminal, and run the following command:

.. code-block:: console

   $ nrfjprog --com
   1050043161         /dev/ttyACM0    VCOM0
   1050043161         /dev/ttyACM1    VCOM1
   1050724225         /dev/ttyACM2    VCOM0
   1050724225         /dev/ttyACM3    VCOM1
   $

In this example, ``1050043161`` is the device ID of the nRF5340 DK and ``1050724225`` is device ID of the nRF7002 DK.

While connecting to a particular board, use the ttyACMx corresponding to VCOM1.
In the example, use ttyACM1 to connect to the board with device ID ``1050043161``.
Similarly, use ttyACM3 to connect to the board with device ID ``1050724225``.

.. code-block:: console

   $ minicom -D /dev/ttyACM1 -b 115200
   $ minicom -D /dev/ttyACM3 -b 115200

Programming DKs
===============

To program the nRF5340 DK:

1. |open_terminal_window_with_environment|
#. Navigate to :file:`<ncs code>/nrf/samples/bluetooth/throughput/`.
#. Run the following command:

   .. code-block:: console

      $ west flash --dev-id <device-id> --hex-file build/merged.hex

To program the nRF7002 DK:

1. |open_terminal_window_with_environment|
#. Navigate to :file:`<ncs code>/nrf/samples/wifi/ble_coex/`.
#. Run the following command:

   .. code-block:: console

      $ west flash --dev-id <device-id> --hex-file build/merged.hex

Testing
=======

Running coexistence sample test cases require additional software such as the Wi-Fi **iperf** application.
When the sample runs Wi-Fi UDP throughput in client mode, a peer device runs UDP throughput in server mode using the following command:

.. code-block:: console

   $ iperf -s -i 1 -u

Use **iperf** version 2.0.5.
For more details, see `Network Traffic Generator`_.

+---------------+--------------+----------------------------------------------------------------+
| Test case     | Coexistence  | Test procedure                                                 |
|               |              |                                                                |
+===============+==============+================================================================+
| Wi-Fi only    | NA           | Run Wi-Fi **iperf** in server mode on the test PC.             |
| throughput    |              | Program the coexistence sample application on the nRF7002 DK.  |
+---------------+--------------+----------------------------------------------------------------+
| Bluetooth LE  | NA           | Program Bluetooth LE throughput application on the nRF5340     |
| only          |              | DK and select role as peripheral.                              |
| throughput    |              | Program the coexistence sample application on the nRF7002 DK.  |
+---------------+--------------+----------------------------------------------------------------+
| Wi-Fi and     | Disabled/    | Run Wi-Fi **iperf** in server mode on the test PC.             |
| Bluetooth LE  | Enabled      | Program Bluetooth LE throughput application on the nRF5340     |
| combined      |              | DK and select role as peripheral.                              |
| throughput    |              | Program the coexistence sample application on the nRF7002 DK.  |
+---------------+--------------+----------------------------------------------------------------+

The Wi-Fi throughput result appears on the test PC terminal on which **iperf** server is run.
The Bluetooth LE throughput result appears on the minicom terminal connected to the nRF5340 DK.

Results
=======

The following tables summarize the results obtained from coexistence tests conducted in a clean RF environment for different Wi-Fi operating bands, antenna configurations, and Wi-Fi modes.
These results are representative and might vary based on the RSSI and the level of external interference.

Wi-Fi in 2.4 GHz
----------------

Separate antennas, Wi-Fi in 802.11n mode:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 10.2               | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE only,     | N.A                | 1107               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 9.9                | 145                |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 8.3                | 478                |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Shared antenna, Wi-Fi in 802.11n mode:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 10.2               | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE only,     | N.A                | 1219               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 10.2               | 29                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 6.2                | 749                |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Separate antennas, Wi-Fi in 802.11b mode:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 3.5                | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE only,     | N.A                | 1042               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 3.3                | 110                |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 2.2                | 563                |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Shared antenna, Wi-Fi in 802.11b mode:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 3.5                | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE only,     | N.A                | 1190               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 3.4                | 59                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 2.2                | 508                |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Wi-Fi in 5 GHz
--------------

Separate antennas, Wi-Fi in 802.11n mode:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 10.2               | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE only,     | N.A                | 1139               |
| central                |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 10.2               | 1188               |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Bluetooth LE,| 10.2               | 1208               |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Shared antenna, Wi-Fi in 802.11n mode:

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP Tx       | Bluetooth LE       |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi only,            | 10.2               | N.A                |
| client (UDP Tx)        |                    |                    |
+------------------------+--------------------+--------------------+
| Bluetooth LE only,     | N.A                | 1180               |
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
In the images, the top image result shows Wi-Fi throughput that appears on a test PC terminal in which Wi-Fi **iperf** server is run and the bottom image result shows Bluetooth LE throughput that appears on a minicom terminal in which the Bluetooth LE throughput sample is run.

.. figure:: /images/wifi_coex_wlan.png
     :width: 780px
     :align: center
     :alt: Wi-Fi only throughput

     Wi-Fi only throughput 10.2 Mbps

.. figure:: /images/wifi_coex_ble.png
     :width: 780px
     :align: center
     :alt: Bluetooth LE only throughput

     Bluetooth LE only throughput: 1107 kbps

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

As is evident from the results of the sample execution, coexistence harmonizes air-time between Wi-Fi and Bluetooth LE rather than resulting in a higher combined throughput.
This is consistent with the design intent.

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
