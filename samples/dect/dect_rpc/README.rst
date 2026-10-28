.. _dect_rpc_sample:

.. ncs-sample::
   :title: DECT NR+ RPC

   This sample shows how an external MCU without a DECT NR+ modem can use a normal Zephyr DECT NR+ network interface while an nRF91 Series device runs the modem and stack over UART nRF RPC.
   The default boards are nRF54L15 DK (client) and nRF9151 DK (server).
   It uses the :ref:`dect_rpc` library.

.. note::
   DECT NR+ RPC and this sample are :ref:`experimental <software_maturity>`.
   Enable the :kconfig:option:`CONFIG_DECT_NR_RPC` Kconfig option and expect API and protocol changes.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

You need two of the development kits (default nRF54L15 DK as RPC client and nRF9151 DK as RPC server), jumper wires for the RPC UART (see :ref:`dect_rpc_client_uart_wiring_nrf54l15`), and (for over-the-air DECT) DECT NR+ modem firmware and antennas on the server kit.

When you build for the nRF9151 DK (``*/ns`` target), the application uses Trusted Firmware-M:

.. include:: /includes/tfm.txt

Overview
********

One sample tree builds the following two firmware images:

* An RPC client on the external MCU (:file:`prj.conf`).
* An RPC server on the nRF91 Series device (:file:`server.conf`).

The client exposes a tunneled ``dect0`` ``net_if``.
The server runs the DECT NR+ modem, L2, and a local ``dect0`` interface and mirrors the state and IPv6 to the client over UART RPC at 1 Mbps with RTS/CTS.

RPC is initialized in the :c:func:`main` function of the application on each side (see :ref:`dect_rpc_client_rpc_init`).
Start the client before the server.

For architecture and protocol details, see :ref:`dect_rpc`.

.. _dect_rpc_client_uart_wiring:

Wiring
******

The baud rate is 1000000 (1 Mbps) with hardware flow control (RTS/CTS).
Use four signals plus GND.
Connect by signal (TX to RX), not by pin label alone when wiring the kits.

The default setup is nRF54L15 DK (client) to nRF9151 DK (server).
An optional setup with two nRF9151 DKs uses UART1 on both (same pinmux as :ref:`dect_shell_application` Serial Modem overlay).

.. _dect_rpc_client_uart_wiring_nrf54l15:

nRF54L15 DK client to nRF9151 DK server
=======================================

.. table:: Wire an nRF54L15 DK application core (RPC client) to an nRF9151 DK (RPC server).

   +---------------------------+---------------------------+
   | RPC client (nRF54L15 DK)  | RPC server (nRF9151 DK)   |
   +===========================+===========================+
   | P1.08 (TX)                | P0.10 (RX)                |
   +---------------------------+---------------------------+
   | P1.09 (RX)                | P0.11 (TX)                |
   +---------------------------+---------------------------+
   | P1.11 (RTS)               | P0.12 (CTS)               |
   +---------------------------+---------------------------+
   | P1.12 (CTS)               | P0.13 (RTS)               |
   +---------------------------+---------------------------+
   | GND                       | GND                       |
   +---------------------------+---------------------------+

Two nRF9151 DKs (alternate client and server)
=============================================

.. table:: Optional: wire two nRF9151 DKs as RPC client and RPC server.

   +---------------------------+---------------------------+
   | RPC client (nRF9151 DK)   | RPC server (nRF9151 DK)   |
   +===========================+===========================+
   | P0.11 (TX)                | P0.10 (RX)                |
   +---------------------------+---------------------------+
   | P0.10 (RX)                | P0.11 (TX)                |
   +---------------------------+---------------------------+
   | P0.13 (RTS)               | P0.12 (CTS)               |
   +---------------------------+---------------------------+
   | P0.12 (CTS)               | P0.13 (RTS)               |
   +---------------------------+---------------------------+
   | GND                       | GND                       |
   +---------------------------+---------------------------+

Use all signal rows plus GND for reliable flow control.

.. _dect_rpc_client_rpc_init:

RPC bring-up order
******************

RPC is initialized in the :c:func:`main` function of the application (no shell command).
The client blocks in the :c:func:`nrf_rpc_init` function until the server sends the group init after modem init.

The following order is required in the bring-up:

1. Start the client and wait for the log line that says the client is waiting for the server.
#. Start the server.
   After modem initialization, it sends the group init and the client unblocks.

If the server starts first, it fails after about 15 seconds.
Reset both kits and repeat with the client first.

User interface
**************

The client exposes tunneled L2 DECT NR+ shell and RPC shell commands.
The server runs the local DECT NR+ stack and modem shell on ``dect0``.

.. _dect_rpc_client_role:

RPC client
==========

The client image uses the default :file:`prj.conf` file (or the same options on the nRF9151 device without :file:`server.conf`).
It does not run the DECT NR+ modem.
It mirrors the server's DECT NR+ interface state and forwards IPv6 over RPC, so the sockets and the IP stack run locally on the external MCU.

Shell and logs use the kit serial port at 115200 8N1.
RPC uses a separate UART: UART21 on the nRF54L15 DK (P1.08, P1.09, P1.11, P1.12) or UART1 on nRF9151 client (P0.10 through P0.13).

Use the following ``dect`` shell commands on the client:

* ``dect rpc ping`` - RPC UART loopback test to the server.
* ``dect sync`` - Pull IPv6 addresses from the server and bring tunneled ``dect0`` up.
* ``dect connect`` or ``dect disconnect`` - Start or stop DECT NR+ association on the server (:kconfig:option:`CONFIG_DECT_NR_RPC_CONN_MGR`, default enabled).
* ``dect status`` - DECT NR+ status from the server (dedicated RPC path).
* ``dect run <subcmd> [args...]`` - Run a DECT L2 shell command on the server (for example ``dect run nw_join``).
  Use ``dect status`` instead of ``dect run status`` for status.
  This command does not run ICMP.
* ``ping -d <destination>`` - ICMPv6 on ``dect0`` (requires the :file:`ping.conf` file, add :file:`dns.conf` for hostnames).

Connection Manager on the client raises ``NET_EVENT_L4_*`` when the mirrored interface has a global IPv6 address.
The DECT association policy (``CONFIG_NET_L2_DECT_CONN_MGR_*`` on the server) is configured in the :file:`server.conf` file only.

.. _dect_rpc_server_role:

RPC server
==========

The server image runs on nRF9151 DK only.
Use the :file:`server.conf` file with :file:`nrf9151-server-modem.overlay`.
The overlay reserves modem IPC shared memory on the nRF9151 DK.
Flash partitions stay as in the default board overlay.

Shell and logs use the kit serial port at 115200 8N1.
RPC uses UART1 on P0.10 through P0.13 (same as the nRF9151 client role).

The server runs the local DECT NR+ L2 stack and modem on ``dect0``, with the local DECT NR+ shell (as in :ref:`dect_shell_application`).
Use ``dect help`` on the server terminal for the full list.

The server does not run client-only RPC shell commands such as ``dect sync`` and does not expose a tunneled ``net_if``.
IPv6 on the air side uses server ``dect0``.
The client mirrors addresses on its own ``dect0``.

Configuration
*************

|config|

Configuration options
=====================

Check and configure the following Kconfig options:

.. options-from-kconfig::
   :show-type:

Configuration files
===================

The sample provides predefined configuration files for typical use cases.
You can find them in the sample directory.

* :file:`prj.conf` - Default RPC client (nRF54L15 or nRF9151 without modem).
* :file:`server.conf` - RPC server on the nRF9151 DK (DECT NR+ modem, conn_mgr on DECT NR+).
* :file:`nrf9151-server-modem.overlay` - Required with :file:`server.conf` (modem SHMEM).
* :file:`dns.conf` and :file:`ping.conf` - Optional client networking (see :ref:`dect_rpc_build_client`).
* :file:`debug-net.conf` - Verbose net stack logging (client or server).
* :file:`rpc-large-rx.conf` - Larger RPC UART RX ring on the client.
* :file:`iperf3-common.conf`, :file:`iperf3-tx.conf`, and :file:`iperf3-rx.conf` - iperf3 on the client.

Additional configuration
========================

Check and configure the following Kconfig options that are used by the sample:

* :kconfig:option:`CONFIG_DECT_NR_RPC` - Enables DECT NR+ RPC (experimental).
* :kconfig:option:`CONFIG_DECT_NR_RPC_CLIENT` and :kconfig:option:`CONFIG_DECT_NR_RPC_SERVER` - Client or server role.
* :kconfig:option:`CONFIG_DECT_NR_RPC_NET_IF` - Network interface and packet bridge (must match on both sides).
* :kconfig:option:`CONFIG_NRF_RPC` and :kconfig:option:`CONFIG_NRF_RPC_UART_TRANSPORT` - UART transport between kits.

RPC UART 1 Mbps with RTS/CTS is set in the :file:`boards/nrf54l15dk_nrf54l15_cpuapp.overlay` and :file:`boards/nrf9151dk_nrf9151_ns.overlay` files.
See :file:`subsys/net/dect/rpc/Kconfig` for tunneled packet size, event pacing, and shell options.

Building and running
********************

.. |sample path| replace:: :file:`samples/dect/dect_rpc`

.. include:: /includes/build_and_run.txt

.. note::
   In your |NCS| workspace, change the directory to this sample tree before you run ``west build``, as described in :ref:`building`.
   Do not pass a separate source path on the ``west build`` command line.

See :ref:`cmake_options` for instructions on how to provide CMake options, for example ``-DEXTRA_CONF_FILE`` and ``-DEXTRA_DTC_OVERLAY_FILE``.

.. _dect_rpc_build_client:

Building the RPC client
=======================

When you build the client on an nRF9151 DK, files under :file:`boards/` select the client devicetree (RPC UART, modem disabled in the :file:`nrf9151dk_nrf9151_ns.conf` and :file:`nrf9151dk_nrf9151_ns.overlay` files).

nRF54L15 DK (default)
---------------------

To build and flash the RPC client on the nRF54L15 DK, use the following command:

.. code-block:: console

   west build -p -d build/client -b nrf54l15dk/nrf54l15/cpuapp && west flash -d build/client

nRF9151 DK (alternate client)
-------------------------------

To build and flash the RPC client on the nRF9151 DK, use the following command:

.. code-block:: console

   west build -p -d build/client -b nrf9151dk/nrf9151/ns && west flash -d build/client

.. _dect_rpc_client_extra_ping_dns:

ICMP ping with DNS (client, optional)
-------------------------------------

The :file:`ping.conf` file enables the ``ping`` shell command.
Use it with the :file:`dns.conf` file for hostnames.

To build and flash the client with ping and DNS on the nRF54L15 DK, use the following command:

.. code-block:: console

   west build -p -d build/client -b nrf54l15dk/nrf54l15/cpuapp -- -DEXTRA_CONF_FILE="ping.conf;dns.conf" && west flash -d build/client

.. _dect_rpc_client_extra_iperf3:

iperf3 (client, optional)
-------------------------

See the iperf3 support section in :ref:`dect_shell_application` for general iperf3 usage on DECT NR+.
In this sample, run ``iperf3`` on the client ``dect0`` after ``dect connect``.
To build and flash the client with iperf3 (TX) on the nRF54L15 DK, use the following command:

.. code-block:: console

   west build -p -d build/client -b nrf54l15dk/nrf54l15/cpuapp -- -DEXTRA_CONF_FILE="iperf3-common.conf;iperf3-tx.conf" && west flash -d build/client

Expected throughput (UDP over RPC)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The RPC link uses a 1 Mbps UART, but UDP throughput on client ``dect0`` is lower than 1 Mbit/s because each datagram waits for one blocking ``DECT_RPC_CMD_IF_SEND`` RPC round-trip before the next ``send()`` can finish.
With typical iperf payloads (for example ``-l 1220``), the client throughput is on the order of half a megabit in second to a parent FT.
The :ref:`dect_shell_application` sample without RPC reaches roughly twice that for the same iperf settings.

.. _dect_rpc_build_server:

Building the RPC server
=======================

To build and flash the RPC server on the nRF9151 DK, use the following command:

.. code-block:: console

   west build -p -d build/server -b nrf9151dk/nrf9151/ns -- \
     -DEXTRA_CONF_FILE=server.conf \
     -DEXTRA_DTC_OVERLAY_FILE=nrf9151-server-modem.overlay && west flash -d build/server

Testing
*******

|test_sample|

#. Wire the kits as in :ref:`dect_rpc_client_uart_wiring`.
#. Flash the server image (:ref:`dect_rpc_build_server`) on the nRF9151 DK and the client image (:ref:`dect_rpc_build_client`) on the other kit.
#. Connect both kits to the computer with USB cables.
#. Open a serial terminal on each kit at 115200 8N1 (for example the Serial Terminal app).
#. Reset the client first, then the server (:ref:`dect_rpc_client_rpc_init`).
   Both terminals should log that RPC is initialized.
#. On the client, run ``dect rpc ping``.
   Expect ``DECT NR+ RPC UART test: OK``.
#. Wait for auto address sync or run ``dect sync`` on the client.
   ``dect status`` on the client should show addresses aligned with the server.
#. On the client, run ``dect connect``.
   After association, ``dect status`` should show a connected DECT state on the server side.
#. Optional: with the :file:`ping.conf` file, run ``ping -d <IPv6>`` on the client ``dect0``.
#. On the server, use local ``dect`` and ``net`` commands as needed for your DECT scenario.

Troubleshooting
***************

If the UART RPC link fails on the client or server, check the following messages and the bring-up order in :ref:`dect_rpc_client_rpc_init`.

* ``nrf_rpc_uart: Ack timeout``
* ``RPC send failed``
* ``nrf_rpc_init failed: -35`` or ``Failed to send group init packet ... err: -71`` on the server

The most common cause is that the server was started before the client.

* Check the initialization order: client waiting log first, then server until both log ``RPC initialized``.
* Check the wiring: TX to RX, RTS to CTS, and GND (see :ref:`dect_rpc_client_uart_wiring_nrf54l15`).
* Check the baud rate and UART: 1 Mbps; UART21 on nRF54L15 DK client or UART1 on nRF9151.
* For RPC UART debug, set :kconfig:option:`CONFIG_NRF_RPC_TR_LOG_LEVEL_DBG` and :kconfig:option:`CONFIG_NRF_RPC_LOG_LEVEL_DBG` to ``y`` in :file:`prj.conf` or :file:`server.conf`.
  For net stack tracing, append :file:`debug-net.conf` to ``EXTRA_CONF_FILE`` on the client or use ``server.conf;debug-net.conf`` on the server.

Dependencies
************

This sample uses the following |NCS| libraries and subsystems:

* :ref:`dect_rpc` - Split-MCU DECT NR+ RPC (tunneled client ``dect0``; server modem and L2 on ``dect0``).
* :ref:`nrf_rpc` - UART transport and CBOR RPC between kits.
* :ref:`ug_dect` - DECT NR+ stack and modem on the server image.

It uses the following Zephyr library:

* :ref:`zephyr:networking_api`: IPv6 on the client ``dect0`` interface.

It uses the following modem library on the server:

* :ref:`nrf_modem_lib_readme`

In addition, it uses the following secure firmware component:

* :ref:`Trusted Firmware-M <ug_tfm>` (nRF9151 ``*/ns`` builds)
