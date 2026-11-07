.. _wifi_coexistence_shell_sample:

.. ncs-sample::
   :title: Wi-Fi: Coexistence shell

   The Coexistence shell sample demonstrates coexistence between the Wi-Fi® and Bluetooth® LE radios of the nRF7120 DK in the 2.4 GHz band.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

The test setup also requires the following hardware and software:

* An access point to which the nRF7120 DK can connect.
* A test PC that is connected to the same network as the access point and runs the Wi-Fi **iPerf** application.
  Use iPerf version 2.0.5.
* An nRF5340 DK that runs the :ref:`ble_throughput` sample as the Bluetooth LE peer device.

Overview
********

The sample runs on the nRF7120 DK and uses the nRF71 Wi-Fi and Short-Range (SR) coexistence driver to arbitrate access to the 2.4 GHz band and to the shared antenna between the Wi-Fi and Bluetooth LE radios.
Enable the coexistence driver using the :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER` Kconfig option.

The sample provides shell commands that measure the following test cases:

* Wi-Fi UDP upload throughput only
* Bluetooth LE throughput only
* Wi-Fi and Bluetooth LE throughput simultaneously

Running both throughput tests at the same time with and without coexistence, shows how coexistence shares airtime between the two radios.

The sample enables Wi-Fi at boot, but it does not enable Bluetooth LE until you run the ``bt_cfg_tput`` command for the first time.
When Bluetooth LE is enabled, the sample informs the coexistence driver that the SR radio is powered up.

The sample uses the central Bluetooth LE role during the test.
It scans for the throughput service, discovers it, and exchanges the MTU.
It then configures the PHY, connection parameters, and data length, and writes data to the peer.
Only the central can start the throughput test.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_coex.svg
     :alt: Wi-Fi and Bluetooth LE coexistence test setup

     Wi-Fi and Bluetooth LE coexistence reference test and evaluation setup

The reference test setup shows the connections between the following devices:

* :term:`Device Under Test (DUT)` (an nRF7120 DK on which the coexistence shell sample runs)
* Wi-Fi peer device (an access point with a test PC that runs iPerf)
* Bluetooth LE peer device (an nRF5340 DK on which the Bluetooth LE throughput sample runs)

The following table provides details about the sample or application that runs on the DUT and peer devices:

+-----------------+----------------+---------------------------------------------------------------------------------+
| Device          | Application    | Details                                                                         |
+=================+================+=================================================================================+
| nRF7120 DK      | Coexistence    | The sample runs Wi-Fi throughput only, Bluetooth LE throughput only,            |
| (DUT)           | shell sample   | or both simultaneously.                                                         |
+-----------------+----------------+---------------------------------------------------------------------------------+
| Test PC         | **iPerf**      | The Wi-Fi iPerf UDP server runs on the test PC and acts as a peer device to     |
|                 | application    | the Wi-Fi UDP client that runs on the nRF7120 DK.                               |
+-----------------+----------------+---------------------------------------------------------------------------------+
| nRF5340 DK      | Bluetooth LE   | The Bluetooth LE throughput sample runs in the peripheral role on the nRF5340   |
| (peer)          | throughput     | DK and acts as a peer device to the Bluetooth LE central that runs on the       |
|                 | sample         | nRF7120 DK.                                                                     |
+-----------------+----------------+---------------------------------------------------------------------------------+

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/coex_shell/Kconfig`):

.. options-from-kconfig::
   :show-type:

Additional configuration
========================

The following additional configuration applies to this sample:

* Connection interval: Use the :kconfig:option:`CONFIG_COEX_SHELL_BT_INTERVAL_MIN` and :kconfig:option:`CONFIG_COEX_SHELL_BT_INTERVAL_MAX` Kconfig options to set the Bluetooth LE connection interval for the throughput test.
  The units are 1.25 milliseconds.
  The default value is ``80`` (100 ms).
  For example, ``100`` corresponds to an interval of 125 ms (100 x 1.25).
* Connection latency: Use the :kconfig:option:`CONFIG_COEX_SHELL_BT_CONN_LATENCY` Kconfig option to set the number of connection events that the peripheral can skip.
  The default value is ``0``.
* Supervision timeout: Use the :kconfig:option:`CONFIG_COEX_SHELL_BT_SUPERVISION_TIMEOUT` Kconfig option to set the Bluetooth LE supervision timeout.
  The units are 10 milliseconds.
  The default value is ``3200`` (32 s, the maximum allowed by the Bluetooth specification).
  The long supervision timeout keeps the link alive while Wi-Fi holds the radio during the coexistence test.
  If the peer is reset or switched off, the sample detects the lost link only after the supervision timeout expires.
* Wi-Fi ROM patch: The sample enables the nRF7120 Wi-Fi ROM patch with the :kconfig:option:`CONFIG_WIFI_NRF71_PATCH` Kconfig option.
  The patch regions are defined in the :ncs-file:`socs/nrf7120_cpuapp.overlay` devicetree overlay.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/coex_shell`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7120 DK, use the ``nrf7120dk/nrf7120/cpuapp`` board target, set the ``SB_CONFIG_WIFI_NRF70`` sysbuild option to ``n``.

The following are examples of the build commands for the test cases:

* Build (default):

  .. code-block:: console

     west build -p -b nrf7120dk/nrf7120/cpuapp

* Build with a different Bluetooth LE connection interval (in units of 1.25 ms):

  .. code-block:: console

     west build -p -b nrf7120dk/nrf7120/cpuapp -- -DCONFIG_COEX_SHELL_BT_INTERVAL_MIN=100 -DCONFIG_COEX_SHELL_BT_INTERVAL_MAX=100

Use the Bluetooth LE throughput sample from the :file:`nrf/samples/bluetooth/throughput` folder on the peer nRF5340 DK.

Build for the nRF5340 DK:

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp -- -DCONFIG_BT_THROUGHPUT_DURATION=20000

Supported CLI commands
======================

The sample supports the following shell commands.
Use the Tab key to autocomplete commands and enter a command without parameters to view its usage.

.. list-table:: Coexistence shell commands
   :header-rows: 1

   * - Command
     - Description
   * - ``wifi connect -s <SSID> -k <key_mgmt> -p <passphrase>``
     - | Connects to a Wi-Fi access point.
       | ``<key_mgmt>`` is ``0`` for no security and ``1`` for WPA2-PSK.
       | Example: ``wifi connect -s wifi_sr_coex_5 -k 1 -p 12345678``
   * - ``wifi_run_tput udp upload <peerIP> <destPort> <duration> <pktSize> <baudrate>``
     - | Runs the Wi-Fi UDP upload throughput test towards the iPerf server.
       | The command returns immediately and prints the result when the test is complete.
       | ``<peerIP>`` is the IPv4 address of the test PC.
       | ``<destPort>`` is the port of the iPerf server.
       | ``<duration>`` is the test duration in seconds.
       | ``<pktSize>`` is the UDP payload size in bytes, or in kilobytes with the ``K`` suffix.
       | The maximum size is set by :kconfig:option:`CONFIG_NET_ZPERF_MAX_PACKET_SIZE`.
       | ``<baudrate>`` is the target rate in bit/s, or in kbit/s or Mbit/s with the ``K`` or ``M`` suffix.
       | Only the ``udp`` protocol and the ``upload`` direction are supported.
       | Example: ``wifi_run_tput udp upload 192.168.1.253 5001 20 1K 10M``
   * - ``bt_cfg_tput <bt_role>``
     - | Enables Bluetooth LE on the first run, connects to the peer in the selected role, and configures the connection for the throughput test.
       | ``<bt_role>`` is ``1`` for central and ``0`` for peripheral.
       | The command times out if a connection is not established within 20 seconds.
       | Example: ``bt_cfg_tput 1``
   * - ``bt_run_tput <duration>``
     - | Runs the Bluetooth LE throughput test.
       | Only the central can run this command.
       | ``<duration>`` is the test duration in seconds.
       | Example: ``bt_run_tput 20``
   * - ``bt_print_cfg``
     - Prints the Bluetooth LE configuration used for the throughput test.
   * - ``bt_disconnect``
     - Disconnects the Bluetooth LE connection.

   * - ``net iface down <index>``
     - | Zephyr network shell command that brings the Wi-Fi interface down, disabling Wi-Fi RX and Wi-Fi TX.
       | Use ``net iface`` to find the index of the Wi-Fi interface.
       | There is no ``wifi if_down`` command in the Wi-Fi shell.
       | Use ``net iface up <index>`` to bring the interface back up.
   * - ``coex_enable_wifi_sr``
     - Enables both Wi-Fi and SR coexistence by calling ``coex_cm_enable(true)`` and ``cd_sr_enable(true)``.
   * - ``coex_disable_wifi_sr``
     - Disables both Wi-Fi and SR coexistence by calling ``coex_cm_enable(false)`` and ``cd_sr_enable(false)``.

The ``coex_enable_wifi_sr`` and ``coex_disable_wifi_sr`` commands are available only when the coexistence driver is enabled with the :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER` Kconfig option.
The option is enabled by default in the :ncs-file:`prj.conf` file.

.. note::
   Use the same duration for the Wi-Fi throughput test and for the Bluetooth LE throughput test when you run both at the same time.

.. note::
   The coexistence driver configures the Coexistence Manager and enables Wi-Fi coexistence each time the Wi-Fi interface is brought up.
   Therefore, the ``coex_disable_wifi_sr`` command lasts only until the next time the Wi-Fi interface is brought up.
   Run ``coex_enable_wifi_sr`` and ``coex_disable_wifi_sr`` after the ``nRF71 SR coexistence configured`` message is printed.
   While the Coexistence Manager is being configured, the commands fail with a busy error and do not change either Wi-Fi or SR coexistence.
   If this happens, run the commands again later.
   While the Wi-Fi interface is down, the commands change only SR coexistence.

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Run the following command to list the connected devices and their serial ports:

   .. code-block:: console

      nrfutil device list

   Use the serial port that corresponds to VCOM0 on the nRF7120 DK and VCOM1 on the nRF5340 DK.
   Connect to each kit using a terminal emulator at 115200 baud.

Programming DKs
===============

Complete the following steps to program the DKs:

.. tabs::

   .. group-tab:: nRF5340 DK

      1. |open_terminal_window_with_environment|
      #. Navigate to the :file:`<ncs code>/nrf/samples/bluetooth/throughput/` folder.
      #. Run the following command to program the nRF5340 DK with the Bluetooth LE throughput sample:

         .. code-block:: console

            west flash --dev-id <device-id>

   .. group-tab:: nRF7120 DK

      1. |open_terminal_window_with_environment|
      #. Navigate to the :file:`<ncs code>/nrf/samples/wifi/coex_shell/` folder.
      #. Run the following command to program the nRF7120 DK with this sample:

         .. code-block:: console

            west flash --dev-id <device-id>

Test procedure
==============

Run the test cases in the following order.
Enable and disable coexistence at runtime using shell commands, so you do not need to rebuild the sample between measurements.

1. Wi-Fi-only throughput:

   a. On the test PC, run the following command to start the iPerf server:

      .. code-block:: console

         iperf -s -i 1 -u

   #. On the nRF7120 DK, connect to the access point and wait for the ``Connected`` message:

      .. code-block:: console

         wifi connect -s <SSID> -k 1 -p <passphrase>

   #. On the nRF7120 DK, start the Wi-Fi UDP upload:

      .. code-block:: console

         wifi_run_tput udp upload <test PC IP address> 5001 20 1K 10M

#. Bluetooth LE-only throughput:

   a. On the nRF5340 DK, run the Bluetooth LE throughput sample in the peripheral role.
      See the :ref:`ble_throughput` sample for how to select the role.
   #. On the nRF7120 DK, connect in the central role:

      .. code-block:: console

         bt_cfg_tput 1

   #. On the nRF7120 DK, start the Bluetooth LE throughput test:

      .. code-block:: console

         bt_run_tput 20

#. Combined Wi-Fi and Bluetooth LE throughput:

   a. Start the iPerf server on the test PC, run the Bluetooth LE throughput sample in the peripheral role on the nRF5340 DK, and connect the nRF7120 DK to the access point as described in the previous test cases.
   #. On the nRF7120 DK, connect to the Bluetooth LE peer in the central role with ``bt_cfg_tput 1``.
   #. On the nRF7120 DK, run ``coex_disable_wifi_sr`` to measure with coexistence disabled.
   #. On the nRF7120 DK, start the Wi-Fi UDP upload with the ``wifi_run_tput`` command.
      The command returns immediately, and the Wi-Fi test runs in the background.
   #. On the nRF7120 DK, start the Bluetooth LE throughput test with the ``bt_run_tput`` command, using the same duration, and wait for the results.
   #. On the nRF7120 DK, run ``coex_enable_wifi_sr`` to measure with coexistence enabled, and repeat steps d and e.

The Wi-Fi throughput result is printed on the nRF7120 DK terminal when the Wi-Fi test is complete, and on the test PC terminal that runs the iPerf server.
The Bluetooth LE throughput result is printed on the nRF7120 DK terminal and the nRF5340 DK terminal.

Sample output
=============

The following example shows the output on the nRF7120 DK terminal after boot, when you scan for and connect to an access point.
The SSIDs, addresses, and signal levels depend on your environment.

.. code-block:: console

   I: Starting nrf7120dk with CPU frequency: 256 MHz
   I: Initializing network
   I: Waiting interface 1 to be up...
   I: IPv4 address: 192.168.1.99
   I: Running dhcpv4 client...
   I: wpa_supplicant initialized

   uart:~$ wifi scan
   Scan requested

   Num  | SSID                             (len) | Chan (Band)   | RSSI | Security             | BSSID
   1    | SSID                             4     | 6    (2.4GHz) | -35  | WPA2-PSK             | 10:C3:7B:CE:A0:60
   2    | SSID_5                           6     | 36   (5GHz  ) | -48  | WPA2-PSK             | 10:C3:7B:CE:A0:64
   Scan request done
   uart:~$ wifi connect -s SSID_5 -k 1 -p PASSWORD
   Connection requested
   I: wlan0: CTRL-EVENT-CONNECTED - Connection to 10:c3:7b:ce:a0:64 completed [id=0 id_str=]
   Connected
   I: Received: 192.168.1.88

After the Bluetooth LE throughput test, the sample prints the throughput measured locally and the throughput reported by the peer.
The values in ``< >`` depend on the test:

.. code-block:: console

   uart:~$ bt_cfg_tput 1
   Bluetooth LE tput config - SUCCESS
   uart:~$ bt_run_tput 20
   I: [local] sent <bytes> bytes (<KB> KB) in <ms> ms at <rate> kbps
   I: [peer] received <bytes> bytes (<KB> KB) in <count> GATT writes at <rate> bps
   Bluetooth LE throughput - SUCCESS

The measured Wi-Fi and Bluetooth LE throughput depend on the RSSI, the Wi-Fi band, the antenna configuration, and the level of interference.
When Wi-Fi and Bluetooth LE throughput tests are executed simultaneously with coexistence enabled, airtime is shared between the two radios rather than increasing the combined throughput.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`Wi-Fi credentials <zephyr:lib_wifi_credentials>`
