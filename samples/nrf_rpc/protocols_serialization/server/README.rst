.. _nrf_rpc_protocols_serialization_server:

nRF RPC: Protocols serialization server
#######################################

.. contents::
   :local:
   :depth: 2

The Protocols serialization server sample runs full Bluetooth® LE, OpenThread or NFC stacks, and it exposes selected functions from these stacks over UART.

Requirements
************

The sample supports the following development kits for testing the network status:

.. table-from-sample-yaml::

To test the sample, you also need another device running the :ref:`Protocols serialization client <nrf_rpc_protocols_serialization_client>` sample.

For testing the Bluetooth LE API serialization, you need to have the `nRF Connect for Mobile`_ app installed on your smartphone or tablet.

For testing the NFC API serialization, you also need a smartphone or tablet that can read NFC tags.

Overview
********

The Protocols serialization server sample implements the server part of the serialization of OpenThread, Bluetooth LE API and NFC calls between two devices that communicate with each other using the UART interface.
The sample uses the :ref:`nrfxlib:nrf_rpc` and CBOR encoding to serialize function calls.

Configuration
*************

|config|

Snippets
========

.. |snippet| replace:: :makevar:`server_SNIPPET`

The following snippets are available:

* ``ble`` - Enables the server part of the :ref:`Bluetooth LE RPC <ble_rpc>`.
  Also enables the :ref:`Bluetooth LE Nordic UART Service <nus_service_readme>`.
* ``coex`` - Enables the :ref:`MPSL software coexistence <nrfxlib:mpsl_cx>` implementation on the server device.
* ``debug`` - Enables debugging the sample by enabling :c:func:`__ASSERT()` statements globally and verbose logging.
* ``log_rpc`` - Enables logging over RPC.
* ``openthread`` - Enables the server part of the OpenThread RPC.
* ``nfc`` - Enables the server part of the :ref:`NFC RPC <nfc_rpc>`.

User interface
**************

.. tabs::
   .. group-tab:: nRF52840 DK

      Button 4:
         * When the ``log_rpc`` snippet is enabled: triggers a fatal error.
            This is used for testing the crash log feature.
         * Otherwise: not available.

   .. group-tab:: nRF54L15 DK
      Button 3:
         * When the ``log_rpc`` snippet is enabled: triggers a fatal error.
           This is used for testing the crash log feature.
         * Otherwise: not available.

Building and running
********************

.. |sample path| replace:: :file:`samples/nrf_rpc/protocols_serialization/server`

.. include:: /includes/build_and_run.txt

You can modify the list of enabled features, which by default includes Bluetooth LE support and debug logs.

.. _protocols_serialization_server_sample_testing:

Testing
*******

After building the Protocols serialization server sample and programming it to your development kit, connect it to a second device running the :ref:`Protocol serialization client <nrf_rpc_protocols_serialization_client>` sample to test either the Bluetooth LE, OpenThread or NFC functionality.

.. _protocols_serialization_server_app_connection:

Connecting the client and server samples
========================================

The client and server devices are connected using two UART peripherals.
In the protocols serialization samples, one peripheral is used for shell and logging purposes, similarly to other applications and samples, while the other peripheral is used for sending OpenThread, Bluetooth LE and NFC remote procedure calls (RPCs).

.. tabs::

    .. group-tab:: nRF52840 DK

        By default, the nRF52840 DK uses the ``uart0`` peripheral for shell and logging purposes, and the ``uart1`` peripheral for sending OpenThread and Bluetooth remote procedure calls (RPCs).

        The ``uart1`` peripheral is configured to use the following pins:

        .. list-table::
           :header-rows: 1

           * - Server
             - Client
             - Function on server
           * - **P1.1**
             - **P1.2**
             - RX
           * - **P1.2**
             - **P1.1**
             - TX
           * - **P1.3**
             - **P1.4**
             - RTS (hardware flow control)
           * - **P1.4**
             - **P1.3**
             - CTS (hardware flow control)
           * - **GND**
             - **GND**
             - Ground

        To enable the communication between the client and the server devices, connect the pins on the two nRF52840 DKs using jumper wires.
        The following illustration demonstrates the pin connections:

        .. figure:: /images/ps_nrf52_connections.png
            :alt: nRF52840 DK server and client pin connections

    .. group-tab:: nRF54L15 DK

        By default, the nRF54L15 DK uses the ``uart20`` peripheral for shell and logging purposes, and the ``uart21`` peripheral for sending OpenThread and Bluetooth remote procedure calls (RPCs).

        The ``uart21`` peripheral is configured to use the following pins:

        .. list-table::
           :header-rows: 1

           * - Server
             - Client
             - Function on server
           * - **P1.10**
             - **P1.11**
             - RX
           * - **P1.11**
             - **P1.10**
             - TX
           * - **P1.12**
             - **P1.13**
             - RTS (hardware flow control)
           * - **P1.13**
             - **P1.12**
             - CTS (hardware flow control)
           * - **GND**
             - **GND**
             - Ground

        To enable the communication between the client and the server devices, connect the pins on the two nRF54L15 DKs using jumper wires.
        The following illustration demonstrates the pin connections:

        .. figure:: /images/ps_nrf54l_connections.png
            :alt: nRF54L15 DK server and client pin connections

Testing Bluetooth LE API serialization
======================================

Complete the following steps to test Bluetooth LE API serialization:

#. |connect_terminal_both|

#. Reboot both devices at the same time by pressing the **RESET** button on each DK.

#. Wait a few seconds until you see a message similar to the following on both terminal emulators:

   .. code-block:: console

      [00:00:00.842,862] <inf> nrf_rpc_host: RPC client ready


   This indicates that the communication between the devices has been initialized properly.

#. Run the following command on the client's terminal emulator to start Bluetooth LE advertising:

   .. code-block:: console

      uart:~$ bt init
      uart:~$ bt advertise on


#. Start the `nRF Connect for Mobile`_ app on your smartphone or tablet.

#. Connect to the client device from the nRF Connect app.

   The device is advertising as **Nordic_UART_Service**.

#. Observe a message similar to the following on the client's terminal emulator:

   .. code-block:: console

      LE conn param updated: int 0x0027 lat 0 to 42

#. Send data over the RX characteristic of the UART service using the nRF Connect app.

#. Observe a message similar to the following on the client's terminal emulator:

   .. code-block:: console

      bt_nus: on_receive: Received data, handle 0, conn 0x200023c4

Testing OpenThread API serialization
====================================

Complete the following steps to test OpenThread API serialization:

#. |connect_terminal_both|

#. Reboot both devices at the same time using the **RESET** button on each DK.

#. Wait a few seconds until you see a message similar to the following on both terminal emulators:

   .. code-block:: console

      uart:~$ [00:00:03.392,517] <dbg> NRF_RPC: nrf_rpc_init: Done initializing nRF RPC module


   This indicates that the communication between the devices has been initialized properly.

#. Run the following command on the client's terminal emulator to bring up the OpenThread interface on the server device:

   .. code-block:: console

      uart:~$ ot ifconfig up
      Done
      [00:02:28.980,041] <dbg> NRF_RPC: cmd_ctx_alloc: Command context 0 allocated
      [00:02:28.980,102] <dbg> NRF_RPC: nrf_rpc_cmd_common: Sending command 0x00 from group 0x01
      [00:02:28.980,133] <dbg> nrf_rpc_uart: send: Sending frame
                                             80 00 ff 01 01 f6
      ...


#. Run the following command on the client's terminal emulator to bring up the corresponding Zephyr network interface on the client device:

   .. code-block:: console

      uart:~$ net iface up 1
      Interface 1 is up

#. Verify that the Zephyr network interface has been automatically configured with OpenThread's link-local address:

   .. code-block:: console

      uart:~$ ot ipaddr
      fe80:0:0:0:6c26:956a:813:1e33
      Done
      ...

      uart:~$ net iface  show

      Interface net0 (0x200012c8) (<unknown type>) [1]
      =========================================
      MTU       : 1280
      Flags     : NO_AUTO_START,IPv6,NO_ND,NO_MLD
      Device    : ot_rpc (0x2b748)
      IPv6 unicast addresses (max 5):
            fe80::6c26:956a:813:1e33 autoconf preferred infinite

#. Start Thread and become a leader:

   .. code-block:: console

      uart:~$ ot thread start
      Done
      ...

      uart:~$ ot state leader
      Done

#. Verify that the Zephyr network interface has automatically received OpenThread's mesh-local addresses:

   .. code-block:: console

      uart:~$ net iface  show

      Interface net0 (0x200012c8) (<unknown type>) [1]
      =========================================
      MTU       : 1280
      Flags     : NO_AUTO_START,IPv6,NO_ND,NO_MLD
      Device    : ot_rpc (0x2b748)
      IPv6 unicast addresses (max 5):
            fe80::6c26:956a:813:1e33 autoconf preferred infinite
            fdde:ad00:beef:0:e503:abfd:1c8d:2664 autoconf preferred infinite meshlocal
            fdde:ad00:beef::ff:fe00:fc00 autoconf preferred infinite meshlocal


   This happens because the client registers a notification callback for OpenThread state changes at the server device and it continuously refreshes the client's IPv6 address list when that changes on the server side.

#. Retrieve the operational dataset of the Thread network:

    .. code-block:: console

      uart:~$ ot dataset active -x
      0e080000000000000000000300000b35060004001fffe00208dead00beef00cafe0708fddead00beef00000510f7893f15a55d8adeacad288c38bf32cc030a4f70656e546872656164010240120410d48d777a474f80e61aa5680de764bd6d0c0402a0f7f8
      Done

#. To send a UDP packet to a peer device and UDP port 5555, run the following command:

   .. code-block:: console

      uart:~$ net udp send fe80:0:0:0:6c26:956a:813:1e34 5555 AAAA
      Message sent

#. To open the UDP port 5555 and listen for incoming UDP datagrams from peer devices, run the following command:

   .. code-block:: console

      uart:~$ net udp bind fe80:0:0:0:6c26:956a:813:1e33 5555

Note that all IPv6 addresses shown here are just examples and will be replaced with the actual IPv6 addresses of the peer devices.

Testing NFC API serialization
====================================

#. |connect_terminal_both|

#. Reboot both devices at the same time by pressing the **RESET** button on each DK.

#. Wait a few seconds until you see a message similar to the following on both terminal emulators:

   .. code-block:: console

      [00:00:00.842,862] <inf> nrf_rpc_host: RPC client ready


   This indicates that the communication between the devices has been initialized properly.

#. Run the following commands on the client's terminal emulator to bring up NFC interface on the server device:

   .. code-block:: console

      uart:~$ nfc init
      uart:~$ nfc start

#. Start any application on smartphone or tablet that is able to read NFC tags.

#. Touch the NFC antenna with the smartphone or tablet and observe it displays the encoded text "Hello world!".

#. Run the following commands on the client's terminal emulator to set and encode a custom message on the server device:

   .. code-block:: console

      uart:~$ nfc stop
      uart:~$ nfc set_new_msg "https://www.nordicsemi.com/Products/Development-software/nRF-Connect-SDK"
      uart:~$ nfc start

#. Touch the NFC antenna with the smartphone or tablet and observe it displays the custom message as the encoded text.

#. Run the following commands on the client's terminal emulator to stop and to release NFC frontend on the server device:

   .. code-block:: console

      uart:~$ nfc stop
      uart:~$ nfc release

Dependencies
************

This sample uses the following `sdk-nrfxlib`_ library:

* :ref:`nrfxlib:nrf_rpc`
* :ref:`nrfxlib:nfc_api_type2`
