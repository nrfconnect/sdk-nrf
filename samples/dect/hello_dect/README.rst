.. _hello_dect:

nRF91x1: Hello DECT
###################

.. contents::
   :local:
   :depth: 2

This sample demonstrates how to transmit and receive IPv6 data over DECT NR+ between two devices.
It shows the basic usage of the DECT NR+ stack through the DECT NR+ connection manager, with automatic peer discovery using mDNS.
You can compile the sample to operate as either a Fixed Terminal (FT) or Portable Terminal (PT).

Requirements
************

The sample supports the following development kit:

.. table-from-sample-yaml::

.. include:: /includes/tfm.txt

Overview
********

This sample shows how to perform the following operations:

* Configure DECT NR+ settings using Kconfig options.
* Use the connection manager to handle DECT NR+ connectivity.
* Respond to network interface up/down events (``NET_EVENT_IF_UP`` or ``NET_EVENT_IF_DOWN``).
* Use mDNS for peer device discovery.
* Send periodic demo messages over IPv6 UDP to discovered peers.
* Receive and log incoming UDP messages on port 12345.
* Use system work queue for periodic message transmission.
* Demonstrate bidirectional DECT NR+ networking.
* Provide visual feedback using LEDs for connection status and transmission activity.

The sample supports two operational modes through Kconfig, with the connection manager handling all network operations:

* Fixed Terminal (FT) mode - The connection manager creates networks and clusters, advertises as ``dect-nr+-ft-device``.
* Portable Terminal (PT) mode - The connection manager scans for and joins existing networks, advertises as ``dect-nr+-pt-device``.


.. important::

   This sample uses default DECT NR+ MAC security credentials (integrity and cipher keys) that are stored insecurely in Zephyr settings.
   The default credentials are intended for development and demonstration purposes only.

User Interface
**************

Buttons
=======

Button 1:
   Initiates DECT connection through the connection manager (``conn_mgr_if_connect()``).

Button 2:
   Disconnects from DECT network through the connection manager (``conn_mgr_if_disconnect()``).

LEDs
====

LED 1 - Connection Status:
   Indicates the DECT NR+ network interface connection state:

   * ON - DECT NR+ interface is up and connected.
   * OFF - DECT NR+ interface is down and disconnected.

LED 2 - Transmission Activity:
   Indicates UDP message transmission activity:

   * ON - Actively transmitting a message (briefly turns on during each send).
   * OFF - Not transmitting (idle state).

   The LED turns on before sending each periodic demo message and turns off after the transmission completes.

Configuration
*************

|config|

Configuration options
=====================

The sample uses the DECT NR+ driver's Kconfig options for configuration.
You can customize the following key options:

.. options-from-kconfig::
   :show-type:

* :kconfig:option:`CONFIG_DECT_DEFAULT_NW_ID`

  * The sample overrides the default network ID to ``0x12345678``.

* :kconfig:option:`CONFIG_DECT_DEFAULT_DEV_TYPE_FT`

  * The sample configures the device as FT only.
    The device will create a cluster.
    Sets hostname to ``dect-nr+-ft-device`` and resolves ``dect-nr+-pt-device.local`` as peer.

* :kconfig:option:`CONFIG_DECT_DEFAULT_DEV_TYPE_PT`

  * The sample configures the device as PT only.
    The device will join existing cluster.
    Sets hostname to ``dect-nr+-pt-device`` and resolves ``dect-nr+-ft-device.local`` as peer.

* :kconfig:option:`CONFIG_NET_HOSTNAME`

  * The sample sets the hostname to ``dect-nr+-ft-device`` for FT mode and ``dect-nr+-pt-device`` for PT mode.
    This hostname is advertised using mDNS and used for peer discovery.

Building and running
********************

.. |sample path| replace:: :file:`samples/dect/hello_dect`

.. include:: /includes/build_and_run_ns.txt

Building for FT mode
====================

To build the sample for FT operation, use the extra configuration file:

.. code-block:: console

   west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE=ft.conf

Building for PT mode
====================

To build the sample for PT operation, use the extra configuration file:

.. code-block:: console

   west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE=pt.conf

Building with DECT NR+ L2 Shell
===============================

To enable the DECT MAC L2 shell for debugging and configuration, use the extra configuration file:

.. code-block:: console

   west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE=shell.conf

You can also have multiple extra configuration files.
For example, to build an FT device with shell enabled, use the following command:

.. code-block:: console

   west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE="ft.conf;shell.conf"

The DECT NR+ L2 shell provides commands for:

* Activating/deactivating the DECT stack
* Scanning for networks and RSSI
* Configuring DECT settings
* Managing associations
* Viewing status information

Use ``dect help`` in the shell to see all available commands.

.. note::
   Certain DECT NR+ settings changed at runtime using the DECT NR+ L2 shell dect sett command must be set before the connection is established to take effect on the modem.
   This includes changes to identifiers like the network ID or the device type, as well as resetting settings to default values.

Testing
*******

|test_sample|

1. |connect_kit|
#. |connect_terminal|
#. Reset the kit.
#. Observe the sample output in the terminal.

Testing with two devices
========================

For full mDNS peer discovery and bidirectional communication testing, complete the following steps:

.. note::
   Programming with ``--erase`` or ``--recover`` (or similar parameters) will erase the device settings area and restore the default settings.

1. Build and flash one device as FT:

   .. code-block:: console

      west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE=ft.conf
      west flash --recover

#. Build and flash another device as PT:

   .. code-block:: console

      west build -p -b nrf9151dk/nrf9151/ns -- -DEXTRA_CONF_FILE=pt.conf
      west flash --recover

#. The FT device performs the following operations:

   * Creates a DECT network.
   * Advertises mDNS hostname as ``dect-nr+-ft-device.local``.
   * Listens for UDP messages on port 12345.
   * Attempts to resolve ``dect-nr+-pt-device.local`` using mDNS.
   * Sends periodic messages to the PT device once resolved.

#. The PT device performs the following operations:

   * Join the DECT network created by FT
   * Advertise mDNS hostname as ``dect-nr+-pt-device.local``
   * Listen for UDP messages on port 12345
   * Attempt to resolve ``dect-nr+-ft-device.local`` using mDNS
   * Send periodic messages to the FT device once resolved

#. Both devices log the received messages showing the sender address and content.
#. Press **Button 1** to manually connect or **Button 2** to disconnect either device.

Dependencies
************

This sample uses the following |NCS| libraries:

* :ref:`nrfxlib:nrf_modem`
* :ref:`dk_buttons_and_leds_readme`
* :ref:`nrf_modem_lib_readme`

It uses the following `sdk-zephyr`_ libraries:

* :ref:`zephyr:networking_api`
* :ref:`zephyr:bsd_sockets_interface`
* :ref:`zephyr:conn_mgr_overview`
* :ref:`zephyr:logging_api`
