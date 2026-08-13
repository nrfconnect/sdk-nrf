.. _ug_nrf93m1_building:

Configuring and building with nRF93M1 DK
########################################

.. contents::
   :local:
   :depth: 2

When you build for an nRF93M1 DK, you are building host firmware.
The module runs Nordic-maintained modem firmware that you do not build and that you update separately.
See :ref:`ug_nrf93m1_updating_modem_fw`.

This page covers what is specific to nRF93 Series host builds.
For general instructions, see :ref:`configuration_and_build`.

.. _ug_nrf93m1_building_target:

Board target
************

On the nRF93M1 DK, the host is an nRF54L15 SoC with an Arm Cortex-M33 application core and a RISC-V FLPR core.
Three board targets are available:

.. list-table::
   :header-rows: 1

   * - Board target
     - Use for
   * - ``nrf93m1dk/nrf54l15/cpuapp``
     - Host application development. Both nRF93M1 DK samples build for this target.
   * - ``nrf93m1dk/nrf54l15/cpuapp/ns``
     - Host application development with TF-M and ARM TrustZone.
   * - ``nrf93m1dk/nrf54l15/cpuflpr``
     - Code on the RISC-V FLPR core.

To build for the application core, run the following commands:

.. code-block:: console

   west build -b nrf93m1dk/nrf54l15/cpuapp

.. important::
   The modem node is declared on the ``cpuapp`` side, in the revision-specific board file.
   Build host applications that talk to the module for ``cpuapp`` or ``cpuapp/ns``, not ``cpuflpr``.

Because the host is an nRF54L15 SoC, the general :ref:`ug_nrf54l` guidance applies to the host side of your application, including partitioning, TF-M, and cryptography.

.. note::
   The board target names the host SoC, not the module.
   The same board target is used for both module variants described in :ref:`ug_nrf93m1_variants` because the variants are software compatible.

.. _ug_nrf93m1_building_models:

Configuration per integration model
***********************************

Your Kconfig set depends on the integration model you chose in :ref:`ug_nrf93m1_architecture`.

AT command model
================

The host needs a UART and an AT command handler.
It does not need a networking stack.

.. code-block:: cfg

   CONFIG_MODEM_MODULES=y
   CONFIG_MODEM_CHAT=y
   CONFIG_MODEM_BACKEND_UART=y

This is the smallest configuration.
Nothing from the Zephyr networking stack is pulled in, because IP terminates in the module.

PPP over CMUX model
===================

The host runs the IP stack, so networking is required.

.. code-block:: cfg

   CONFIG_MODEM=y
   CONFIG_MODEM_CELLULAR=y
   CONFIG_NETWORKING=y
   CONFIG_NET_L2_PPP=y
   CONFIG_NET_IPV4=y
   CONFIG_NET_SOCKETS=y
   CONFIG_NET_MGMT=y
   CONFIG_NET_MGMT_EVENT=y

Add ``CONFIG_NET_IPV6=y`` if your deployment requires IPv6 support.
Use TLS on the host only when you are not using the module's TLS stack, and be aware of the flash and RAM cost on the host.

Enable :kconfig:option:`CONFIG_MODEM_AT_SHELL` to access the AT channel from the shell while PPP is running.
This is the approach used by the :ref:`nrf93m1dk_ppp_shell` sample.
Because the shell backend shares the driver's CMUX channel rather than competing for the UART, it avoids the desynchronization problem described in :ref:`ug_nrf93m1_at_commands_host`,

Instead of building this configuration yourself, start from the :ref:`nrf93m1dk_ppp_shell` sample and remove any components your application does not require.

.. note::
   Choosing the PPP model roughly doubles the host footprint compared to the AT command model, and it duplicates functionality the module already provides.
   Use PPP when your application requires Zephyr socket, not by default.

.. _ug_nrf93m1_building_devicetree:

Devicetree
**********

For the PPP model, the module is described as a child of the host UART node using the ``nordic,nrf93m1`` compatible.
The DK board files already contain this node.
For a custom board, see :ref:`ug_nrf93m1_custom_board`.

Two host-side settings need attention:

* Enable hardware flow control on the UART.
  Without it, high baud rates lose data.
* Match ``current-speed`` to the module's configured baud rate.
  If you lower the module baud rate with ``AT+IPR`` for power reasons, the host must follow.

.. _ug_nrf93m1_building_programming:

Programming
***********

The nRF93M1 DK includes an on-board SEGGER J-Link, so you program the host over the same USB connection you use for serial output:

.. code-block:: console

   west flash

This command programs the nRF54L15 host only.
It does not touch module firmware.

.. note::
   If ``west flash`` succeeds but the module does not respond to AT commands, the host firmware is running and the module is either unpowered or held in reset.
   Check the ``POWERKEY`` and ``nRESET`` handling in your application, and confirm the module powered on.

.. _ug_nrf93m1_building_fota:

Host firmware updates
*********************

Host application updates and module firmware updates are separately versioned, but they can share the same delivery channel.

* Host firmware images can be pulled through the module from nRF Cloud using ``AT%NRFCLOUDFOTA``.
  See :ref:`ug_nrf93m1_updating_modem_fw_host`.
  This means you do not need a second connectivity path for host updates.
* Applying the image on the host still uses the standard |NCS| mechanisms for the host SoC.
  For the nRF54L15 SoC, see :ref:`ug_nrf54l_developing_ble_fota`.
* Module firmware updates are built and signed by Nordic.
  See :ref:`ug_nrf93m1_updating_modem_fw`.

.. note::
   A field device needs a way to update host firmware and module firmware, and the two have independent version numbers.
   Record both versions in your device telemetry so you can correlate field issues with a specific combination.
