.. _wifi_thread_coex_sample:

Wi-Fi: Thread coexistence
#########################

.. contents::
   :local:
   :depth: 2

The Thread coexistence sample demonstrates coexistence between Wi-Fi® and OpenThread device radios in 2.4 GHz frequency.

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

Running coexistence sample test cases require additional software such as the Wi-Fi **iperf** application.
Use **iperf** version 2.0.5.
For more details, see `Network Traffic Generator`_.

Overview
********

The sample demonstrates how the coexistence mechanism is implemented, and how it can be enabled and disabled between Wi-Fi and Thread radios in the 2.4 GHz band.
This is done by using the throughput of the Wi-Fi client and the Thread client, as well as the throughput of the Wi-Fi client and the Thread server.

Test setup
==========

The following figure shows a reference test setup.

.. figure:: /images/wifi_thread_coex.svg
     :alt: Wi-Fi Thread Coex test setup

     Wi-Fi Thread coexistence reference test and evaluation setup

The reference test setup shows the connections between the following devices:

* :term:`Device Under Test (DUT)` (nRF7002 DK on which the coexistence sample runs)
* Wi-Fi peer device (access point with test PC that runs **iperf**)
* Thread peer device (nRF7002 DK on which Thread throughput runs)

The following table provides more details on the sample or application that runs on DUT and peer devices:

+--------------+-------------------+-------------------------------------------------------------------------------------+
| Device       | Application       |                             Details                                                 |
+==============+===================+=====================================================================================+
| nRF7002 DK   | Thread            | The sample runs Wi-Fi throughputs, Thread throughputs, or a combination of both     |
| (DUT)        | coexistence sample| based on configuration selections in the :file:`prj.conf` file.                     |
+--------------+-------------------+-------------------------------------------------------------------------------------+
| Test PC      | **iperf**         | Wi-Fi **iperf** UDP server is run on the test PC, and this acts as a peer device to |
|              | application       | the Wi-Fi UDP client.                                                               |
+--------------+-------------------+-------------------------------------------------------------------------------------+
| nRF7002 DK   | Thread            | Case 1: Thread UDP throughput is run in server mode on the peer nRF7002 DK device,  |
| (peer)       | throughput        | and this acts as a peer device to the Thread client that runs on the DUT nRF7002 DK.|
|              |                   | Case 2: Thread UDP throughput is run in client mode on the peer nRF7002 DK device,  |
|              |                   | and this acts as a peer device to the Thread server that runs on the DUT nRF7002 DK.|
+--------------+-------------------+-------------------------------------------------------------------------------------+

Configuration
*************

|config|

Configuration options
=====================

The following sample-specific Kconfig options are used in this sample (located in :file:`samples/wifi/thread_coex/Kconfig`):


.. _CONFIG_TEST_TYPE_WLAN_ONLY:

CONFIG_TEST_TYPE_WLAN_ONLY
   This option enables the Wi-Fi test type.

.. _CONFIG_TEST_TYPE_OT_ONLY:

CONFIG_TEST_TYPE_OT_ONLY
   This option enables the Thread test type.

.. _CONFIG_TEST_TYPE_WLAN_OT:

CONFIG_TEST_TYPE_WLAN_OT
   This option enables concurrent Wi-Fi and Thread tests.

.. _CONFIG_COEX_TEST_DURATION:

CONFIG_COEX_TEST_DURATION
   This option sets the Wi-Fi, Thread, or both test duration in milliseconds.

.. _CONFIG_STA_SSID:

CONFIG_STA_SSID
   This option specifies the SSID of the Wi-Fi access point to connect.

.. _CONFIG_STA_PASSWORD:

CONFIG_STA_PASSWORD
   This option specifies the Wi-Fi passphrase (WPA2™) or password (WPA3™) to connect.

.. _CONFIG_STA_KEY_MGMT_*:

CONFIG_STA_KEY_MGMT_*
   These options specify the Wi-Fi key security option.

Additional configuration
========================

To enable different test modes, set up the following configuration parameters in the :file:`prj.conf` file:

* Test modes - Use the following Kconfig options to select the required test case:

  * :ref:`CONFIG_TEST_TYPE_WLAN_ONLY <CONFIG_TEST_TYPE_WLAN_ONLY>` for Wi-Fi-only test.
  * :ref:`CONFIG_TEST_TYPE_OT_ONLY <CONFIG_TEST_TYPE_OT_ONLY>` for Thread-only test.
  * :ref:`CONFIG_TEST_TYPE_WLAN_OT <CONFIG_TEST_TYPE_WLAN_OT>` for concurrent Wi-Fi and Thread test.

* Test duration - Use the :ref:`CONFIG_COEX_TEST_DURATION <CONFIG_COEX_TEST_DURATION>` Kconfig option to set the duration of the Wi-Fi-only test or Thread-only test or both.
  The units are in milliseconds.
  For example, to set the test for 20 seconds, set this value to ``20000``.

* Wi-Fi connection - Set the following options appropriately as per the credentials of the access point used for this testing:

.. include:: /includes/wifi_credentials_static.txt

.. note::
   ``menuconfig`` can also be used to configure ``Wi-Fi credentials``

* Wi-Fi throughput test - Set the :kconfig:option:`CONFIG_NET_CONFIG_PEER_IPV4_ADDR` Kconfig option appropriately as per the IP address of the test PC on which **iperf** is run.

See :ref:`zephyr:menuconfig` in the Zephyr documentation for instructions on how to run ``menuconfig``.

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/thread_coex`

.. include:: /includes/build_and_run_ns.txt

You can build the sample for the following configurations:

* Wi-Fi throughput only
* Thread throughput only
* Concurrent Wi-Fi and Thread throughput (with coexistence enabled and disabled modes)

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following are examples of the CLI commands:

* Build with coexistence disabled:

  .. code-block:: console

	 west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_MPSL_CX=n -Dipc_radio_CONFIG_MPSL_CX=n


Use this command for Wi-Fi throughput only, Thread throughput only, or concurrent Wi-Fi and Thread throughput with coexistence disabled tests.

* Build with coexistence enabled:

  .. code-block:: console

	 west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_MPSL_CX=y -Dipc_radio_CONFIG_MPSL_CX=y

Use this command for concurrent Wi-Fi and Thread throughput with coexistence enabled test.

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

     -DSHIELD=nrf7002ek ipc_radio_SHIELD=nrf7002ek_coex

* For nRF7001 EK:

  .. code-block:: console

     -DSHIELD=nrf7002ek_nrf7001 ipc_radio_SHIELD=nrf7002ek_nrf7001_coex

* Overlay files

   * Use the :file:`overlay-ot.conf overlay-wifi-udp-client-thread-udp-client.conf` file to build for both Wi-Fi and Thread in client roles.
   * Use the :file:`overlay-ot.conf overlay-wifi-udp-client-thread-udp-server.conf` file to build for Wi-Fi in the client role and Thread in the server role.

The generated HEX file to be used is :file:`thread_coex/build/merged.hex`.


Testing
=======

|test_sample|

#. |connect_kit|
#. |connect_terminal|

#. Use the following commands to check the available devices:

   .. code-block:: console

      $ nrfjprog --com
      1050779496         /dev/ttyACM0    VCOM0
      1050779496         /dev/ttyACM1    VCOM1
      1050759502         /dev/ttyACM2    VCOM0
      1050759502         /dev/ttyACM3    VCOM1
      $

	In this example, ``1050779496`` is the device ID of the first nRF7002 DK and ``1050759502`` is the device ID of the second nRF7002 DK.

	While connecting to a particular board, use the ttyACMx corresponding to VCOM1.
	In the example, use ttyACM1 to connect to the board with device ID ``1050779496``.
	Similarly, use ttyACM3 to connect to the board with device ID ``1050759502``.

#. Use the following commands to connect to the desired devices:

	.. code-block:: console

	   $ minicom -D /dev/ttyACM1 -b 115200
	   $ minicom -D /dev/ttyACM3 -b 115200

#. Program the nRF7002 DK by completing the following steps:

	1. Open a new terminal in the test PC.
	#. Navigate to the :file:`<ncs code>/nrf/samples/wifi/thread_coex/` folder.
	#. Run the following command:

	   .. code-block:: console

		  $ west flash --dev-id <device-id> --hex-file build/zephyr/merged_domains.hex

	When the sample runs Wi-Fi UDP throughput in client mode, a peer device (test PC) runs UDP throughput
	in server mode using the following command:

	.. code-block:: console

	   $ iperf -s -i 1 -u

#. Complete the following steps to run the Wi-Fi client and Thread client:

	+---------------+-------------+--------------------------------------------------------------------------+
	| Test case     | Coexistence | Test procedure                                                           |
	+===============+=============+==========================================================================+
	| Wi-Fi-only    | NA          | Run Wi-Fi **iperf** in server mode on the test PC.                       |
	| throughput    |             | Build the coexistence sample for Wi-Fi-only throughput in the client     |
	|               |             | role and program the DUT nRF7002 DK.                                     |
	+---------------+-------------+--------------------------------------------------------------------------+
	| Thread-       | NA          | Build the coexistence sample for Thread-only throughput in the server    |
	| only          |             | role and program the peer nRF7002 DK.                                    |
	| throughput    |             | Build the coexistence sample for Thread-only throughput in the client    |
	|               |             | role and program the DUT nRF7002 DK after **Run thread application       |
	|               |             | on client** is seen on the peer nRF7002 DK's UART console window.        |
	+---------------+-------------+--------------------------------------------------------------------------+
	| Wi-Fi and     | Disabled/   | Build the coexistence sample for Thread-only throughput in the server    |
	| Thread        | Enabled     | role and program the peer nRF7002 DK.                                    |
	| combined      |             | Run Wi-Fi **iperf** in server mode on the test PC.                       |
	| throughput    |             | Build the coexistence sample for concurrent Wi-Fi and Thread             |
	|               |             | throughput with both Wi-Fi and Thread in client roles. Program the sample|
	|               |             | on the DUT nRF7002 DK after **Run thread application on client** is      |
	|               |             | seen on the peer nRF7002 DK's UART console window.                       |
	+---------------+-------------+--------------------------------------------------------------------------+

#. Complete the following steps to run the Wi-Fi client and Thread server:

	+---------------+-------------+-------------------------------------------------------------------------+
	| Test case     | Coexistence | Test procedure                                                          |
	+===============+=============+=========================================================================+
	| Wi-Fi-only    | NA          | Run Wi-Fi **iperf** in server mode on the test PC.                      |
	| throughput    |             | Build the coexistence sample for Wi-Fi-only throughput in the client    |
	|               |             | role and program the DUT nRF7002 DK.                                    |
	+---------------+-------------+-------------------------------------------------------------------------+
	| Thread-       | NA          | Build the coexistence sample for Thread-only throughput in the server   |
	| only          |             | role and program the DUT nRF7002 DK.                                    |
	| throughput    |             | Build the coexistence sample for Thread-only throughput in the client   |
	|               |             | role and program the peer nRF7002 DK after **Run thread application     |
	|               |             | on client** is seen on the DUT nRF7002 DK's UART console window.        |
	+---------------+-------------+-------------------------------------------------------------------------+
	| Wi-Fi and     | Disabled/   | Build the coexistence sample for concurrent Wi-Fi and Thread            |
	| Thread        | Enabled     | throughput with Wi-Fi in the client role and Thread in the server role, |
	| combined      |             | program this on the DUT nRF7002 DK.                                     |
	| throughput    |             | Run Wi-Fi **iperf** in server mode on the test PC.                      |
	|               |             | Build the coexistence sample for Thread-only throughput in the client   |
	|               |             | role and program the peer nRF7002 DK after **Run thread application     |
	|               |             | on client** is seen on the DUT nRF7002 DK's UART console window.        |
	+---------------+-------------+-------------------------------------------------------------------------+

#. Observe that the Wi-Fi throughput result appears on the test PC terminal on which **iperf** server is run.
   The Thread throughput result appears on the minicom terminal connected to the nRF7002 DK, on which Thread is run in the client role.

Results
=======

The following tables summarize the results obtained from coexistence tests conducted in a clean RF environment for different Wi-Fi operating bands, with Wi-Fi and Thread data rates set to 10 Mbps and 65 kbps, respectively.
These results are representative and might vary based on the RSSI and the level of external interference.

The Wi-Fi and Thread channels (both in the 2.4 GHz band) are selected to overlap, thereby maximizing interference.
Tests are run using separate antenna configurations only, as Thread does not support shared antenna configurations due to being in idle listening mode when not active.

Wi-Fi (802.11n mode) in 2.4 GHz
-------------------------------

Wi-Fi in client role (channel 11), Thread in client role (channel 22):

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only             | 9.6                | N.A                |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread-only            | N.A                | 63                 |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.5                | 14                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.3                | 59                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Wi-Fi in client role (channel 11), Thread in server role (channel 22):

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only             | 9.6                | N.A                |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread-only            | N.A                | 63                 |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.4                | 14                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 8.8                | 63                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+


Wi-Fi (802.11n mode) in 5 GHz
-----------------------------

Wi-Fi in client role (channel 48), Thread in client role (channel 22):

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only             | 9.6                | N.A                |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread-only            | N.A                | 63                 |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.2                | 63                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 9.1                | 63                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

Wi-Fi in client role (channel 48), Thread in server role (channel 22):

+------------------------+--------------------+--------------------+
| Test case              | Wi-Fi UDP TX       | Thread             |
|                        | throughput in Mbps | throughput in kbps |
+========================+====================+====================+
| Wi-Fi-only             | 9.6                | N.A                |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Thread-only            | N.A                | 63                 |
|                        |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 8.6                | 63                 |
| coexistence disabled   |                    |                    |
+------------------------+--------------------+--------------------+
| Wi-Fi and Thread,      | 8.5                | 63                 |
| coexistence enabled    |                    |                    |
+------------------------+--------------------+--------------------+

The results show that coexistence harmonizes airtime between Wi-Fi and Thread rather than resulting in a higher combined throughput.
This is consistent with the design intent.

Dependencies
************

This sample uses the following library:

* :ref:`nrf_security`
