.. _wifi_ble_coex_sample:

Wi-Fi: Bluetooth LE coexistence
###############################

.. contents::
   :local:
   :depth: 2

This sample demonstrates coexistence between Wi-Fi® and Bluetooth® LE radios in 2.4 GHz frequency.
The sample documentation includes details of test setup used, build procedure, test procedure and the results obtained when the sample is run on the nRF7002 DK.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Overview
********

The sample demonstrates how the coexistence mechanism is implemented, enabled and disabled between Wi-Fi and Bluetooth LE radios in 2.4 GHz band using Wi-Fi and Bluetooth LE throughput for different Wi-Fi and BLE roles.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_coex.svg
     :alt: Wi-Fi Coex test setup

     Wi-Fi coexistence reference test and evaluation setup

The reference test setup shows the connections between the following devices:

* :term:`Device Under Test (DUT)` - nRF7002 DK on which the coexistence sample runs
* Wi-Fi peer device - access point with test PC that runs **iperf**
* Bluetooth LE peer device - nRF5340 DK on which Bluetooth LE throughput sample runs

This setup is kept in a shielded test enclosure box (for example, a Ramsey box).
The following table provides more details on the sample or application that runs on DUT and peer devices:

+--------------+----------------+-----------------------------------------------------------------------------+
| Device       | Application    |                             Details                                         |
+==============+================+=============================================================================+
| nRF7002 DK   | Bluetooth LE   | The sample runs Wi-Fi throughputs, Bluetooth LE throughputs or a combination|
|              | coex sample    | of both based on configuration selections in the :file:`prj.conf` file,     |
|              |                | test case selection in the :file:`test/test_cases.conf` file and            |
|              |                | configuration overlay file selection from :file:`test` folder.              |
+--------------+----------------+-----------------------------------------------------------------------------+
| Test PC      | **iperf**      | This test PC acts as a peer device to DUT. If Wi-Fi **iperf** UDP client/   |
|              | application    | server is run on DUT then Wi-Fi **iperf** UDP server/client  is run in this |
|              |                | peer. Similarly for Wi-Fi **iperf** TCP.                                    |
+--------------+----------------+-----------------------------------------------------------------------------+
| nRF5340 DK   | Bluetooth LE   | When Bluetooth LE central/peripheral is run on the nRF7002 DK then Bluetooth|
|              | throughput     | LE throughput sample is run in peripheral/central mode on the nRF5340 DK.   |
|              | sample         |                                                                             |
+--------------+----------------+-----------------------------------------------------------------------------+

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

.. _CONFIG_COEX_SEP_ANTENNAS:

CONFIG_COEX_SEP_ANTENNAS
   This option specifies whether the antennas are shared or separate for Bluetooth and WLAN.

.. _CONFIG_TEST_TYPE_WLAN_ONLY:

CONFIG_TEST_TYPE_WLAN_ONLY
   This option enables the WLAN test type.

.. _CONFIG_TEST_TYPE_BLE_ONLY:

CONFIG_TEST_TYPE_BLE_ONLY
   This option enables the Bluetooth LE test type.

.. _CONFIG_TEST_TYPE_WLAN_BLE:

CONFIG_TEST_TYPE_WLAN_BLE
   This option enables concurrent WLAN and Bluetooth LE tests.

.. _CONFIG_COEX_TEST_DURATION:

CONFIG_COEX_TEST_DURATION
   This option sets the test duration in milliseconds.

.. _CONFIG_BT_INTERVAL_MIN:

CONFIG_BT_INTERVAL_MIN
   This option sets the Bluetooth minimum connection interval (each unit is 1.25 milliseconds).

.. _CONFIG_BT_INTERVAL_MAX:

CONFIG_BT_INTERVAL_MAX
   This option sets the Bluetooth maximum connection interval (each unit is 1.25 milliseconds).

.. _CONFIG_STA_SSID:

CONFIG_STA_SSID
   This option specifies the SSID to connect.

.. _CONFIG_STA_PASSWORD:

CONFIG_STA_PASSWORD
   This option specifies the passphrase (WPA2) or password WPA3 to connect.

.. _CONFIG_STA_KEY_MGMT_*:

CONFIG_STA_KEY_MGMT_*
   These options specify the key security option.

.. _CONFIG_BT_ROLE_CENTRAL

CONFIG_BT_ROLE_CENTRAL
   This option sets the BT role to central.

The following Kconfig options are used in generating hex file for peer BLE device (located in :file:`samples/bluetooth/throughput/Kconfig`):

.. _CONFIG_BT_THROUGHPUT_FILE:

CONFIG_BT_THROUGHPUT_FILE
   This option selects the type of the throughput test.

.. _CONFIG_BT_THROUGHPUT_DURATION:

CONFIG_BT_THROUGHPUT_DURATION
   This option sets the Bluetooth throughput test duration in milliseconds.

Configuration files
===================

To enable different test modes, set up the following configuration parameters in the :file:`prj.conf` file:

* Antenna configuration: Use the :ref:`CONFIG_COEX_SEP_ANTENNAS <CONFIG_COEX_SEP_ANTENNAS>` Kconfig option to select the antenna configuration.
  Set it to ``y`` to enable separate antennas and ``n`` to enable shared antenna.
* Test modes: Use the following Kconfig options to select the required test case:

  * :ref:`CONFIG_TEST_TYPE_WLAN_ONLY <CONFIG_TEST_TYPE_WLAN_ONLY>` for Wi-Fi only test
  * :ref:`CONFIG_TEST_TYPE_BLE_ONLY <CONFIG_TEST_TYPE_BLE_ONLY>` for Bluetooth LE only test
  * :ref:`CONFIG_TEST_TYPE_WLAN_BLE <CONFIG_TEST_TYPE_WLAN_BLE>` for concurrent Wi-Fi and Bluetooth LE test.

  Based on the required test, set only one of these to ``y``.
* Test duration: Use the :ref:`CONFIG_COEX_TEST_DURATION <CONFIG_COEX_TEST_DURATION>` Kconfig option to set the duration of the test.
  The units are in milliseconds.
  For example, to set the tests for 20 seconds, set the respective values to ``20000``.

* Bluetooth LE configuration: Set the Bluetooth LE connection interval limits using the :ref:`CONFIG_BT_INTERVAL_MIN <CONFIG_BT_INTERVAL_MIN>` and :ref:`CONFIG_BT_INTERVAL_MAX <CONFIG_BT_INTERVAL_MAX>` Kconfig options.
  The units are 1.25 milliseconds.
  For example, ``CONFIG_BT_INTERVAL_MIN=80`` corresponds to an interval of 100 ms (80 x 1.25).
  * Set the Bluetooth LE role using :ref:`CONFIG_BT_ROLE_CENTRAL <CONFIG_BT_ROLE_CENTRAL>`. If this is set to ``y`` then Bluetooth LE role becomes central. If this is set to ``n`` then Bluetooth LE role becomes peripheral.

* Wi-Fi connection: Set the following options appropriately as per the credentials of the access point used for this testing:

  * :ref:`CONFIG_STA_SSID <CONFIG_STA_SSID>`
  * :ref:`CONFIG_STA_PASSWORD <CONFIG_STA_PASSWORD>`
  * :ref:`CONFIG_STA_KEY_MGMT_* <CONFIG_STA_KEY_MGMT_*>`
  * :kconfig:option:`CONFIG_NET_CONFIG_PEER_IPV4_ADDR`

.. note::
   ``menuconfig`` can also be used to enable the ``Key management`` option.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

Set up the following configuration parameters in the :file:`samples/bluetooth/throughput/boards/prj_nrf5340dk_nrf5340_cpuapp.conf` file:

* File or time-based throughput: Use :ref:`CONFIG_BT_THROUGHPUT_FILE <CONFIG_BT_THROUGHPUT_FILE>` to select file or time-based throughput test.
  Set it to ``n`` to enable time-based throughput test.
* Test duration: Use :ref:`CONFIG_BT_THROUGHPUT_DURATION <CONFIG_BT_THROUGHPUT_DURATION>` to set the duration of the Bluetooth LE throughput test.
  The units are in milliseconds.

.. note::
   Use the same test duration value for :ref:`CONFIG_COEX_TEST_DURATION <CONFIG_COEX_TEST_DURATION>` and :ref:`CONFIG_BT_THROUGHPUT_DURATION <CONFIG_BT_THROUGHPUT_DURATION>`.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/ble_coex`

.. include:: /includes/build_and_run_ns.txt

The sample can be built for the following configurations:

* Wi-Fi throughput only
* Bluetooth LE throughput only
* Concurrent Wi-Fi and Bluetooth LE throughput (with coexistence enabled and disabled mode)

To build for the nRF7002 DK, use the ``nrf7002dk_nrf5340_cpuapp`` build target.
The following are examples of the CLI commands:

* Build with coexistence disabled:

  .. code-block:: console

     west build -p -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_MPSL_CX=n -Dhci_rpmsg_CONFIG_MPSL_CX=n

Use this command for Wi-Fi throughput only, Bluetooth LE throughput only, or concurrent Wi-Fi and Bluetooth LE throughput with coexistence disabled tests.

* Build with coexistence enabled:

  .. code-block:: console

     west build -p -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_MPSL_CX=y -Dhci_rpmsg_CONFIG_MPSL_CX=y

Use this command for concurrent Wi-Fi and Bluetooth LE throughput with coexistence enabled test.

Change the build target as given below for the nRF7001 DK, nRF7002 EK and nRF7001 EK.

* Build target for nRF7001 DK:

  .. code-block:: console

     nrf7002dk_nrf7001_nrf5340_cpuapp

* Build target for nRF7002 EK and nRF7001 EK:

  .. code-block:: console

     nrf5340dk_nrf5340_cpuapp

Add the following SHIELD options for the nRF7002 EK and nRF7001 EK.

* For nRF7002 EK:

  .. code-block:: console

     -DSHIELD=nrf7002ek -Dhci_rpmsg_SHIELD=nrf7002ek_coex

* For nRF7001 EK:

  .. code-block:: console

     -DSHIELD=nrf7002ek_nrf7001 -Dhci_rpmsg_SHIELD=nrf7002ek_nrf7001_coex

* Test case and overlay configuration file:

List of Wi-Fi and Bluetooth LE throughput combinational test cases are available in the :file:`test/test_cases.conf` and the corresponding configuration overlay files are available in :file:`test` folder. Choose the test case and the configuration overlay file based on the Wi-Fi transport protocol, role and Bluetooth LE role chosen for the test.

Example: To run test for Wi-Fi UDP in client role and Bluetooth LE in central role, choose ``CONFIG_WIFI_TP_UDP_CLIENT_BLE_TP_CENTRAL`` as test case and ``overlay-wifi-udp-client-ble-central.conf`` as configuration overlay file. Given below is the build command to run simultaneous Wi-Fi UDP client and Bluetooth LE central throughput with coexistence enabled and separate antennas mode for 20seconds duration.

.. code-block:: console

   west build -p -b nrf7002dk_nrf5340_cpuapp -- -DCONFIG_MPSL_CX=y -Dhci_rpmsg_CONFIG_MPSL_CX=y -DCONFIG_COEX_SEP_ANTENNAS=y -DCONFIG_TEST_TYPE_WLAN_BLE=y -DCONFIG_WIFI_TP_UDP_CLIENT_BLE_TP_CENTRAL=y -DOVERLAY_CONFIG="test/overlay-wifi-udp-client-ble-central.conf" -DCONFIG_COEX_TEST_DURATION=20000

The generated HEX file to be used is :file:`ble_coex/build/zephyr/merged_domains.hex`.

Use the Bluetooth throughput sample from the :file:`nrf/samples/bluetooth/throughput` folder on the peer nRF5340 DK device.

* Build for the nRF5340 DK:

.. code-block:: console

   west build -p -b nrf5340dk_nrf5340_cpuapp -- -DCONFIG_BT_THROUGHPUT_FILE=n

The generated HEX file to be used is :file:`throughput/build/zephyr/merged_domains.hex`.

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

1. Open a new terminal in the test PC.
#. Navigate to :file:`<ncs code>/nrf/samples/bluetooth/throughput/`.
#. Run the following command:

   .. code-block:: console

      $ west flash --dev-id <device-id> --hex-file build/zephyr/merged_domains.hex

To program the nRF7002 DK:

1. Open a new terminal in the test PC.
#. Navigate to :file:`<ncs code>/nrf/samples/wifi/ble_coex/`.
#. Run the following command:

   .. code-block:: console

      $ west flash --dev-id <device-id> --hex-file build/zephyr/merged_domains.hex

Testing
=======

Running coexistence sample test cases require additional software such as the Wi-Fi **iperf** application.
Use **iperf** version 2.0.5.
For more details, see `Network Traffic Generator`_.

Test procedure depends on the test case chosen. Test procedure for different combinations of Wi-Fi and BLE roles is given below when sample is run on nRF7002 DK.

**Wi-Fi UDP/TCP client and BLE central**

+---------------+--------------+-----------------------------------------------------------------------------+
| Test case     | Coexistence  | Test procedure                                                              |
+===============+==============+=============================================================================+
| Wi-Fi only    | NA           | (1). Run Wi-Fi **iperf** in server mode on the test PC. (2). Program the    |
| throughput    |              | coexistence sample application on the nRF7002 DK.                           |
+---------------+--------------+-----------------------------------------------------------------------------+
| Bluetooth LE  | NA           | (1). Program Bluetooth LE throughput application on the nRF5340 DK and      |
| only          |              | selectrole as peripheral. (2). Program the coexistence sample application   |
| throughput    |              | on the nRF7002 DK.                                                          |
+---------------+--------------+-----------------------------------------------------------------------------+
| Wi-Fi and     | Disabled/    | (1). Run Wi-Fi **iperf** in server mode on the test PC. (2). Program        |
| Bluetooth LE  | Enabled      | Bluetooth LE throughput application on the nRF5340 DK and select role       |
| combined      |              | as peripheral. (3). Program the coexistence sample on the nRF7002 DK.       |
| throughput    |              |                                                                             |
+---------------+--------------+-----------------------------------------------------------------------------+

The Wi-Fi throughput result appears on the test PC terminal on which **iperf** server is run.
The Bluetooth LE throughput result appears on the minicom terminal connected to the nRF7002 DK.

**Wi-Fi UDP/TCP client and BLE peripheral**

+---------------+--------------+-----------------------------------------------------------------------------+
| Test case     | Coexistence  | Test procedure                                                              |
+===============+==============+=============================================================================+
| Wi-Fi only    | NA           | (1). Run Wi-Fi **iperf** in server mode on the test PC. (2). Program the    |
| throughput    |              | coexistence sample application on the nRF7002 DK.                           |
+---------------+--------------+-----------------------------------------------------------------------------+
| Bluetooth LE  | NA           | (1). Program Bluetooth LE throughput application on the nRF5340 DK and      |
| only          |              | select role as central. (2). Program the coexistence sample application on  |
| throughput    |              | the nRF7002 DK.                                                             |
+---------------+--------------+-----------------------------------------------------------------------------+
| Wi-Fi and     | Disabled/    | (1). Run Wi-Fi **iperf** in server mode on the test PC. (2). Program        |
| Bluetooth LE  | Enabled      | BluetoothLE throughput application on the nRF5340 DK and select role as     |
| combined      |              | central. (3). Program the coexistence sample application on the nRF7002 DK. |
| throughput    |              | (4). Wait until "Run  BLE central" is seen on terminal connected to nRF7002 |
|               |              | DK board and then issue "run" command on minicom terminal connected to the  |
|               |              | nRF5340 DK.                                                                 |
+---------------+--------------+-----------------------------------------------------------------------------+

The Wi-Fi throughput result appears on the test PC terminal on which **iperf** server is run.
The Bluetooth LE throughput result appears on the minicom terminal connected to the nRF7002 DK.

**Wi-Fi UDP/TCP server and BLE central**

+---------------+--------------+-----------------------------------------------------------------------------+
| Test case     | Coexistence  | Test procedure                                                              |
+===============+==============+=============================================================================+
| Wi-Fi only    | NA           | (1). Program the coexistence sample application on the nRF7002 DK. (2). Wait|
| throughput    |              | until "start Wi-Fi client" is printed on terminal connected to nRF7002 DK   |
|               |              | board. Run Wi-Fi **iperf** in client mode on the test PC.                   |
+---------------+--------------+-----------------------------------------------------------------------------+
| Bluetooth LE  | NA           | (1). Program Bluetooth LE throughput application on the nRF5340 DK and      |
| only          |              | select role as peripheral. (2). Program the coexistence sample application  |
| throughput    |              | on the nRF7002 DK.                                                          |
+---------------+--------------+-----------------------------------------------------------------------------+
| Wi-Fi and     | Disabled/    | (1). Program Bluetooth LE throughput application on the nRF5340 DK and      |
| Bluetooth LE  | Enabled      | select role as peripheral. (2). Program the coexistence sample application  |
| combined      |              | on the nRF7002 DK. (3). Wait until "start Wi-Fi client" is printed on       |
| throughput    |              | terminal connected to nRF7002 DK board. (4). Run Wi-Fi **iperf** in client  |
|               |              | mode on the test PC.                                                        |
+---------------+--------------+-----------------------------------------------------------------------------+

Both the Wi-Fi and Bluetooth LE throughput results appear on the minicom terminal connected to the nRF7002 DK.

**Wi-Fi UDP/TCP server and BLE peripheral**

+---------------+--------------+-----------------------------------------------------------------------------+
| Test case     | Coexistence  | Test procedure                                                              |
+===============+==============+=============================================================================+
| Wi-Fi only    | NA           | (1). Program the coexistence sample application on the nRF7002 DK. (2) Wait |
| throughput    |              | until "start Wi-Fi client" is printed on terminal connected to  nRF7002 DK. |
|               |              | board (3) Run Wi-Fi **iperf** in client mode on the test PC.                |
+---------------+--------------+-----------------------------------------------------------------------------+
| Bluetooth LE  | NA           | (1). Program Bluetooth LE throughput application on the nRF5340 DK and      |
| only          |              | select role as central. (2). Program the coexistence sample application on  |
| throughput    |              | the nRF7002 DK.                                                             |
+---------------+--------------+-----------------------------------------------------------------------------+
| Wi-Fi and     | Disabled/    | (1). Program Bluetooth LE throughput application on the nRF5340 DK and      |
| Bluetooth LE  | Enabled      | select role as central. (2). Program the coexistence sample application on  |
| combined      |              | the nRF7002 DK. (3). Wait until "start Wi-Fi client" is printed on terminal |
| throughput    |              | connected to nRF7002 DK board. (4). Run Wi-Fi **iperf** in client mode on   |
|               |              | the test PC. (5). Issue "run" command on minicom terminal connected to the  |
|               |              | nRF5340 DK.                                                                 |
+---------------+--------------+-----------------------------------------------------------------------------+

Both the Wi-Fi and Bluetooth LE throughput results appear on the minicom terminal connected to the nRF7002 DK.

Results
=======

The following tables collect a summary of results obtained when coexistence tests are run for different Wi-Fi operating bands, antenna configurations, and Wi-Fi modes.
The tests are run with the test setup inside an RF shield box.
Therefore, the results are representative and might change with adjustments in the test setup.

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

.. figure:: /images/wifi_ble_coex_wifi.png
     :width: 780px
     :align: center
     :alt: Wi-Fi only throughput

     Wi-Fi only throughput 10.2 Mbps

.. figure:: /images/wifi_ble_coex_ble.png
     :width: 780px
     :align: center
     :alt: Bluetooth LE only throughput

     Bluetooth LE only throughput: 1107 kbps

.. figure:: /images/wifi_ble_coex_cd.png
     :width: 780px
     :align: center
     :alt: Wi-Fi and Bluetooth LE CD

     Wi-Fi and Bluetooth LE throughput, coexistence disabled: Wi-Fi 9.9 Mbps and Bluetooth LE 145 kbps

.. figure:: /images/wifi_ble_coex_ce.png
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
