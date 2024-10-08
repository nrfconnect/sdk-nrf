.. _wifi_offloaded_raw_tx_packet_sample:

Wi-Fi: Offloaded raw TX
#######################

.. contents::
   :local:
   :depth: 2

The Offloaded raw TX sample demonstrates how to use the offloaded raw transmit APIs provided by the nRF Wi-Fi driver for transmitting raw packets.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

The sample generates and broadcasts 802.11 beacon frames as raw TX packets.
As a result, the nRF70 Series device can be identified as a Wi-Fi® beaconing device.

For more information on the offloaded raw transmit operation, see :ref:`ug_nrf70_developing_offloaded_raw_tx`.

Configuration
*************

|config|

Configuration options
=====================

.. options-from-kconfig::

Building and running
********************

.. |sample path| replace:: :file:`samples/wifi/offloaded_raw_tx`

.. include:: /includes/build_and_run_ns.txt

To build for the nRF7002 DK, use the ``nrf7002dk/nrf5340/cpuapp`` board target.
The following is an example of the CLI command:

  .. code-block:: console

     west build -p -b nrf7002dk/nrf5340/cpuapp

To generate and transmit beacons, use the following commands:

.. tabs::

   .. group-tab:: Generate beacons

      To generate beacons with random source MAC address and BSSID, run the following command:

      .. code-block:: console

	 west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_GENERATE_MAC_ADDRESS=y -DCONFIG_ENTROPY_GENERATOR=y

   .. group-tab:: Transmit beacons

      To transmit beacons at a specified interval, run the following command:

      .. code-block:: console

	 west build -p -b nrf7002dk/nrf5340/cpuapp -- -DCONFIG_BEACON_INTERVAL=200

Change the board target as given below for the nRF7002 EK.

.. code-block:: console

   west build -p -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek

Testing
=======

|test_sample|

1. |connect_kit|
#. |connect_terminal|

   The sample shows the following output:

   .. code-block:: console

	*** Booting nRF Connect SDK v2.7.99-cb26b7c84971 ***
	*** Using Zephyr OS v3.7.99-9056bece3e70 ***
	----- Initializing nRF70 -----
	----- Starting to transmit beacons with the following configuration -----
	                SSID: nRF70_off_raw_tx_1
	                Period: 200000
	                TX Power: 15
	                Channel: 1
	                Short Preamble: 0
	                Number of Retries: 10
	                Throughput Mode: Legacy
	                Rate: 54M
	                HE GI: 1
	                HE LTF: 1
	-----  Statistics -----
	                Packet sent: 150
	----- Updating configuration to -----
	                SSID: nRF70_off_raw_tx_2
	                Period: 200000
	                TX Power: 11
	                Channel: 36
	                Short Preamble: 0
	                Number of Retries: 10
	                Throughput Mode: Legacy
	                Rate: 12M
	                HE GI: 1
	                HE LTF: 1
	-----  Statistics -----
	                Packet sent: 299
	----- Stopping transmission -----
	----- Deinitializing nRF70 -----

#. Observe the packets that are sent out, in a sniffer capture, by filtering the packets based on their transmit MAC address or SSID.
