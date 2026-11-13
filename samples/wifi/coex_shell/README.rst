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
  The test PC runs the iPerf server for the UDP upload test and the iPerf client for the UDP download test.
  Use iPerf version 2.0.5.
* An nRF5340 DK that runs the :ref:`ble_throughput` sample as the Bluetooth LE peer device.

Overview
********

The sample runs on the nRF7120 DK and uses the nRF71 Wi-Fi and Short-Range (SR) coexistence driver to arbitrate access to the 2.4 GHz band and to the shared antenna between the Wi-Fi and Bluetooth LE radios.
Enable the coexistence driver using the :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER` Kconfig option.

The sample provides shell commands that measure the following test cases:

* Wi-Fi UDP throughput only, in the upload or download direction
* Bluetooth LE throughput only, with the nRF7120 DK in the central or peripheral role
* Wi-Fi and Bluetooth LE throughput simultaneously

Running both throughput tests at the same time with and without coexistence, shows how coexistence shares airtime between the two radios.

The Wi-Fi throughput test supports the following directions:

* UDP upload - The nRF7120 DK runs the zperf UDP client and sends data to the iPerf server on the test PC.
* UDP download - The nRF7120 DK runs the zperf UDP server and receives data from the iPerf client on the test PC.
  The download direction requires the :kconfig:option:`CONFIG_NET_ZPERF_SERVER` Kconfig option, which is enabled by default in the :ncs-file:`/samples/wifi/coex_shell/prj.conf` file.

.. note::
   Wi-Fi TCP throughput is not supported in either direction.
   The ``wifi_run_tput`` command rejects the ``tcp`` protocol, even though the command help still lists it as a supported option.

The sample enables Wi-Fi at boot, but it does not enable Bluetooth LE until you run the ``bt_cfg_tput`` command for the first time.
When Bluetooth LE is enabled, the sample informs the coexistence driver that the SR radio is powered up.

The sample supports both the central and the peripheral Bluetooth LE roles.
Select the role using the ``bt_cfg_tput`` command:

* Central (``bt_cfg_tput 1``) - The nRF7120 DK scans for the throughput service, discovers it, and exchanges the MTU.
  It then configures the PHY, connection parameters, and data length.
  Start the test using the ``bt_run_tput`` command on the nRF7120 DK, which writes data to the peer.
* Peripheral (``bt_cfg_tput 0``) - The nRF7120 DK advertises the throughput service and waits for the peer central to connect.
  It then configures the PHY, connection parameters, and data length.
  Start the test from the peer central, which writes data to the nRF7120 DK.

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
| Test PC         | **iPerf**      | For UDP upload, the iPerf UDP server runs on the test PC and acts as a peer     |
|                 | application    | device to the Wi-Fi UDP client that runs on the nRF7120 DK.                     |
|                 |                | For UDP download, the iPerf UDP client runs on the test PC and acts as a peer   |
|                 |                | device to the Wi-Fi UDP server that runs on the nRF7120 DK.                     |
|                 |                | TCP is not supported.                                                           |
+-----------------+----------------+---------------------------------------------------------------------------------+
| nRF5340 DK      | Bluetooth LE   | The Bluetooth LE throughput sample runs on the nRF5340 DK in the role opposite  |
| (peer)          | throughput     | to the nRF7120 DK.                                                              |
|                 | sample         | It runs in the peripheral role when the nRF7120 DK is the central, and in the   |
|                 |                | central role when the nRF7120 DK is the peripheral.                             |
+-----------------+----------------+---------------------------------------------------------------------------------+

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/coex_shell/Kconfig`):

.. options-from-kconfig::
   :show-type:

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
       | Only the ``udp`` protocol is supported. The ``tcp`` protocol is rejected.
       | Example: ``wifi_run_tput udp upload 192.168.1.253 5001 40 1K 5M``
   * - ``wifi_run_tput udp download [<port>]``
     - | Starts the zperf UDP server on the nRF7120 DK for the Wi-Fi UDP download throughput test.
       | The command prints the iPerf client command to run on the test PC.
       | The result is printed when the iPerf client ends the session.
       | The server keeps running for further sessions until you stop it with ``wifi_stop_tput udp``.
       | ``<port>`` is the local port of the zperf server. The default value is ``5001``.
       | Only the ``udp`` protocol is supported. The ``tcp`` protocol is rejected.
       | Example: ``wifi_run_tput udp download 5001``
   * - ``wifi_stop_tput udp``
     - Stops the zperf UDP server started for the Wi-Fi UDP download throughput test.
   * - ``bt_cfg_tput <bt_role>``
     - | Enables Bluetooth LE on the first run, connects to the peer in the selected role, and configures the connection for the throughput test.
       | ``<bt_role>`` is ``1`` for central and ``0`` for peripheral.
       | In the central role, the nRF7120 DK scans for the peer. In the peripheral role, it advertises and waits for the peer central to connect.
       | The command times out if a connection is not established within 20 seconds.
       | Example: ``bt_cfg_tput 1`` (central) or ``bt_cfg_tput 0`` (peripheral)
   * - ``bt_run_tput <duration>``
     - | Runs the Bluetooth LE throughput test.
       | Only the central can run this command.
       | When the nRF7120 DK is the peripheral, start the test from the peer central instead.
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
The option is enabled by default in the :ncs-file:`/samples/wifi/coex_shell/prj.conf` file.

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

   .. nrfutil-command:: console

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

1. Wi-Fi-only UDP upload throughput:

   a. On the test PC, run the following command to start the iPerf server:

      .. code-block:: console

         iperf -s -i 1 -u

   #. On the nRF7120 DK, connect to the access point and wait for the ``Connected`` message:

      .. code-block:: console

         wifi connect -s <SSID> -k 1 -p <passphrase>

   #. On the nRF7120 DK, start the Wi-Fi UDP upload:

      .. code-block:: console

         wifi_run_tput udp upload <test PC IP address> 5001 40 1K 5M

      For example, if the IP address of the test PC is ``192.168.1.253``:

      .. code-block:: console

         wifi_run_tput udp upload 192.168.1.253 5001 40 1K 5M

#. Wi-Fi-only UDP download throughput:

   a. On the nRF7120 DK, connect to the access point as described in the previous test case.
   #. On the nRF7120 DK, start the zperf UDP server:

      .. code-block:: console

         wifi_run_tput udp download 5001

   #. On the test PC, start the iPerf client towards the nRF7120 DK:

      .. code-block:: console

         iperf -c <nRF7120 DK IP address> -i 1 -t 40 -u -b 5M -p 5001

      For example, if the IP address of the nRF7120 DK is ``192.168.1.88``:

      .. code-block:: console

         iperf -c 192.168.1.88 -i 1 -t 40 -u -b 5M -p 5001

      The nRF7120 DK also prints an example iPerf client command with its IP address when the zperf UDP server starts.

   #. When you have completed the download tests, stop the zperf UDP server on the nRF7120 DK:

      .. code-block:: console

         wifi_stop_tput udp

#. Bluetooth LE-only throughput with the nRF7120 DK as the central:

   a. On the nRF5340 DK, run the Bluetooth LE throughput sample in the peripheral role.
      See the :ref:`ble_throughput` sample for how to select the role.
   #. On the nRF7120 DK, connect in the central role:

      .. code-block:: console

         bt_cfg_tput 1

   #. On the nRF7120 DK, start the Bluetooth LE throughput test:

      .. code-block:: console

         bt_run_tput 20

#. Bluetooth LE-only throughput with the nRF7120 DK as the peripheral:

   a. On the nRF7120 DK, start advertising in the peripheral role:

      .. code-block:: console

         bt_cfg_tput 0

   #. On the nRF5340 DK, run the Bluetooth LE throughput sample in the central role.
      See the :ref:`ble_throughput` sample for how to select the role.
   #. On the nRF5340 DK, start the Bluetooth LE throughput test using the ``run`` command.

#. Combined Wi-Fi and Bluetooth LE throughput:

   a. Prepare the Wi-Fi test in the selected direction and connect the nRF7120 DK to the access point as described in the previous test cases.
      For UDP upload, start the iPerf server on the test PC.
      For UDP download, start the zperf UDP server on the nRF7120 DK with ``wifi_run_tput udp download``.
   #. Connect the nRF7120 DK to the Bluetooth LE peer in the selected role, with ``bt_cfg_tput 1`` for central or ``bt_cfg_tput 0`` for peripheral.
      Run the Bluetooth LE throughput sample on the nRF5340 DK in the opposite role.
   #. On the nRF7120 DK, run ``coex_disable_wifi_sr`` to measure with coexistence disabled.
   #. Start the Wi-Fi test.
      For UDP upload, run the ``wifi_run_tput udp upload`` command on the nRF7120 DK.
      The command returns immediately, and the Wi-Fi test runs in the background.
      For UDP download, run the iPerf client on the test PC.
   #. Start the Bluetooth LE throughput test from the central, using the same duration, and wait for the results.
      Use the ``bt_run_tput`` command on the nRF7120 DK when it is the central, or the ``run`` command on the nRF5340 DK when the nRF7120 DK is the peripheral.
   #. On the nRF7120 DK, run ``coex_enable_wifi_sr`` to measure with coexistence enabled, and repeat steps d and e.

The Wi-Fi throughput result is printed on the nRF7120 DK terminal when the Wi-Fi test is complete, and on the test PC terminal that runs iPerf.
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

This sample uses the following |NCS| library:

* :ref:`Wi-Fi credentials <zephyr:lib_wifi_credentials>`
