.. _ug_wifi_direct:

Wi-Fi Direct (P2P mode)
#######################

.. contents::
   :local:
   :depth: 2

Wi-Fi Direct® (also known as Wi-Fi P2P or peer-to-peer mode) enables direct device-to-device connections without requiring a traditional access point.
The nRF70 Series devices support Wi-Fi Direct, allowing you to establish peer-to-peer connections with other Wi-Fi Direct-capable devices.

Building with Wi-Fi Direct support
**********************************

To build an application with Wi-Fi Direct support, use the :ref:`wifi_shell_sample` sample with the ``wifi-p2p`` snippet and external flash for firmware patches.

Build command
=============

To build the Wi-Fi shell sample with Wi-Fi Direct (P2P) support, run the following command:

.. code-block:: console

   west build --pristine --board nrf7002dk/nrf5340/cpuapp -S wifi-p2p -S nrf70-fw-patch-ext-flash
   west flash

Wi-Fi Direct commands
*********************

The following commands are available for Wi-Fi Direct operations.
Both Wi-Fi shell commands and ``wpa_cli`` commands are provided for each operation.

Finding peers
=============

To start discovering Wi-Fi Direct peers, use the following commands:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p find

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_find

This command initiates the P2P discovery process.
The device scans for other Wi-Fi Direct-capable devices in range.

Listing discovered peers
========================

To view the list of discovered peers, use the following commands:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p peers

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_peers

This command displays a table of discovered peers with the following information:

.. code-block:: console

   Num  | Device Name                      | MAC Address       | RSSI | Device Type          | Config Methods
   1    | Galaxy S22                       | D2:39:FA:43:23:C1 | -58  | 10-0050F204-5        | 0x188

The columns in the table represent the following attributes:

* ``Num`` - Sequential number of the peer in the list
* ``Device Name`` - Friendly name of the peer device
* ``MAC Address`` - MAC address of the peer device
* ``RSSI`` - Signal strength in dBm
* ``Device Type`` - WPS device type identifier
* ``Config Methods`` - Supported WPS configuration methods

Getting peer details
====================

To get detailed information about a specific peer, use the following commands:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p peer <mac_address>

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_peer <mac_address>

For example:

.. code-block:: console

   wpa_cli p2p_peer D2:39:FA:43:23:C1

This command displays detailed information about the specified peer device.

Connecting to a peer
====================

To establish a Wi-Fi Direct connection with a discovered peer:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p connect <mac_address> <pin|pbc> -g <go_intent>

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_connect <mac_address> <pin|pbc> go_intent=<go_intent>

Parameters:

* ``<mac_address>`` - MAC address of the peer device to connect to.
* ``<pbc|pin>`` - WPS provisioning method:

  * ``pin`` - Uses PIN-based WPS authentication.
    The command returns a PIN (for example, ``88282282``) that must be entered on the peer device.
  * ``pbc`` - Uses Push Button Configuration (PBC) for WPS authentication.

* ``go_intent`` - Group Owner (GO) intent value ``0-15``:

  * Higher values indicate a stronger preference to become the Group Owner.
  * A value of ``15`` forces the device to become the GO.
  * A value of ``0`` indicates the device prefers to be a client.

Example connection using the PIN method:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p connect D2:39:FA:43:23:C1 pin -g 0

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_connect D2:39:FA:43:23:C1 pin go_intent=0

The command outputs a PIN (for example, ``88282282``), which must be entered on the peer device to complete the connection.

To disconnect from a Wi-Fi Direct connection, use the following commands:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi disconnect

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli disconnect

Creating a P2P group (GO mode)
==============================

To create a P2P group and start as the GO, use the following commands:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p group_add

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_group_add

This command creates a P2P group with the device acting as the GO, allowing other devices to connect to it.

Inviting a peer to a P2P group
==============================

To invite a peer to join an existing P2P group, use the following commands:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p invite -g <Group interface name> -P <mac_address>

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_invite group=<Group interface name> peer=<mac_address>

Example:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p invite -g wlan0 -P D2:39:FA:43:23:C1

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_invite group=wlan0 peer=D2:39:FA:43:23:C1

Setting P2P power save mode
===========================

To enable or disable P2P power save mode, use the following commands:

.. tabs::

   .. group-tab:: Wi-Fi shell command

      .. code-block:: console

         wifi p2p power_save <on|off>

   .. group-tab:: ``wpa_cli`` command

      .. code-block:: console

         wpa_cli p2p_set ps <1|0>
