.. _ug_radio_fem_nrf2220ek:

Developing with the nRF2220 EK
##############################

.. contents::
   :local:
   :depth: 2

The nRF2220 :term:`Evaluation Kit (EK)` is an RF :term:`Front-End Module (FEM)` for Bluetooth® Low Energy, Bluetooth Mesh, 2.4 GHz proprietary, Thread, and Zigbee range extension.
When combined with an nRF52, nRF53 or nRF54L Series SoC, the nRF2220 RF FEM's output power is up to +14 dBm with ability to transmit with lower output powers through built-in bypass circuit.

.. _ug_radio_fem_nrf2220ek_dk_preparation:

Preparation of a development kit to work with the nRF2220EK
***********************************************************

On Arduino-compatible development kits like the :zephyr:board:`nrf52840dk` or :zephyr:board:`nrf5340dk`, plug the *Nordic Interposer Board A* (PCA64172) into the development kit.
Plug nRF2220 EK board into ``SLOT 2`` of the *Nordic Interposer Board A*.

On the :zephyr:board:`nrf54l15dk` development kit, plug the nRF2220 EK board into the ``PORT P0`` expansion slot.

.. caution::

   On the :zephyr:board:`nrf54l15dk` development kit pins **P0.00** ... **P0.03** of the nRF54L15 SoC are connected to the debugger chip and by default connect ``UART0`` of the debugger chip to the nRF54L15 SoC.
   Disable the UART0 function (VCOM0) of the debugger chip to allow the pins to be used as FEM control signals and FEM I2C interface.
   You can use the `Board Configurator app`_ , which is part of the `nRF Connect for Desktop`_, for this purpose.
   The pin **P0.04** of the nRF54L15 SoC is connected also to **Button 3** of the development kit.
   Do not press this button while the firmware containing the code supporting the nRF2220 EK shield is running.

.. _ug_radio_fem_nrf2220ek_programming:

Building and programming with nRF2220 EK
****************************************

To build for the nRF2220 EK, build for the compatible :ref:`nRF52, nRF53 or nRF54L board target <app_boards_names>` with the CMake ``SHIELD`` option set to ``nrf2220ek``.
See :ref:`cmake_options` for instructions on how to provide CMake options.

For example, if you build for nRF52840 DK on the command line, you can use the following command:

.. code-block:: console

   west build -b nrf52840dk/nrf52840 -- -DSHIELD=nrf2220ek

If you use |nRFVSC|, specify ``-DSHIELD=nrf2220ek`` in the *Extra Cmake arguments* field when `setting up a build configuration <How to work with build configurations_>`_.

Alternatively, add the shield in the project's :file:`CMakeLists.txt` file:

.. code-block:: none

   set(SHIELD nrf2220ek)

Building for a multicore board
==============================

When building for a board with an additional network core, like the nRF5340, add the ``-DSHIELD`` parameter to the command line:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf2220ek

In this case, the sysbuild will pass the ``SHIELD=nrf2220ek`` variable to all images that are built by the command.
The build system will pick automatically appropriate overlay and configuration files for images for each core.
The files are different for each of the cores.
For the application core, the overlay containing forwarding the FEM pins to the network core will be used.
For the network core, the overlay enabling nRF2220 FEM on the network core will be used.
In case the application contains additional images for which the ``SHIELD`` variable should not be passed, you must pass manually the ``SHIELD`` variable to each relevant image build separately.

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -D<app_name_image>_SHIELD=nrf2220ek -Dipc_radio_SHIELD=nrf2220ek

In this case the ``SHIELD=nrf2220ek`` will be passed to the build of the *app_image_name* image for the application core.
The build system will pick automatically an overlay file containing forwarding the FEM pins to the network core.
The ``SHIELD=nrf2220ek`` variable will be passed to the build of the ``ipc_radio`` image for the network core.
The build system will pick automatically an overlay file enabling nRF2220 FEM on the network core.

In this command, the ``ipc_radio`` image is used as default and builds the network core image with support for the combination of 802.15.4 and Bluetooth.
The ``ipc_radio`` has been used since the build system migration to sysbuild.
See :ref:`Migrating to sysbuild <child_parent_to_sysbuild_migration>` page.
Setting the correct sysbuild option enables support for 802.15.4 and Bluetooth :ref:`ipc_radio`.

``ipc_radio`` represents all applications with support for the combination of both 802.15.4 and Bluetooth.
You can configure your application using the following sysbuild Kconfig options:

* :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO` for applications having support for 802.15.4, but not for Bluetooth.
* :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC` for application having support for Bluetooth, but not for 802.15.4.
* :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO` and :kconfig:option:`SB_CONFIG_NETCORE_IPC_RADIO_BT_HCI_IPC` for multiprotocol applications having support for both 802.15.4 and Bluetooth.


.. note::
   On nRF53 devices, ``TWIM0`` and ``UARTE0`` are mutually exclusive AHB bus masters on the network core as described in the `Product Specification <nRF5340 Product Specification_>`_, Section 6.4.3.1, Table 22.
   As a result, they cannot be used simultaneously.
   For the I2C part of the nRF2220 interface to be functional, you must disable the ``UARTE0`` node in the network core's devicetree file.

   .. code-block:: devicetree

      &uart0 {
         status = "disabled";
      };
