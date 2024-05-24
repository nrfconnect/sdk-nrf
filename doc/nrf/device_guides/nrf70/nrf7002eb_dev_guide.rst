.. _ug_nrf7002eb_gs:

Developing with nRF7002 EB
##########################

.. contents::
   :local:
   :depth: 2

The nRF7002 :term:`Expansion Board (EB)` (PCA63561), part of the `nRF70 Series Family <nRF70 Series product page_>`_, can be used to provide Wi-Fi® connectivity to compatible development or evaluation boards through the nRF7002 Wi-Fi 6 companion IC.
For example, you can use it with the :ref:`Nordic Thingy:53 <ug_thingy53>`, an IoT prototyping platform from Nordic Semiconductor.

.. figure:: images/nRF7002eb.png
   :alt: nRF7002 EB

   nRF7002 EB

The nRF7002 EB features a :term:`Printed Circuit Board (PCB)` edge connector and castellated holes to provide Wi-Fi connectivity through the nRF7002 companion IC.

In case of the Nordic Thingy:53, the PCB edge connector connects nRF7002 to nRF5340, which acts as a host.
For the pinout of the PCB edge connector, see `nRF7002 EB PCB edge connector`_ in the board's `Hardware User Guide <nRF7002 EB User Guide_>`_.
For information about how to connect to the Nordic Thingy:53, see `Using nRF7002 EB with the Nordic Thingy:53`_, also in the hardware guide.

The castellated holes on the side of the board allow the EB to be used as a breakout board that can be soldered to other PCB assemblies.
This way, you can provide Wi-Fi capabilities to develop Wi-Fi applications with another System on Chip (SoC), MPU, or MCU host.
For the pinout of the castellated holes, see `nRF7002 EB castellated edge holes`_ in the board's `Hardware User Guide <nRF7002 EB User Guide_>`_.

.. _nrf7002eb_building_programming:

Building and programming with nRF7002 EB
****************************************

To build for the nRF7002 EB, build for the compatible :ref:`board target <app_boards_names>` with the CMake ``SHIELD`` option set to ``nrf7002eb``.
See :ref:`cmake_options` for instructions on how to provide CMake options.

For example, if you build for Thingy:53 on the command line, you can use the following command:

.. code-block:: console

   west build -b thingy53/nrf5340/cpuapp -- -DSHIELD=nrf7002eb

If you use the |nRFVSC|, specify ``-DSHIELD=nrf7002eb`` in the **Extra Cmake arguments** field when `setting up a build configuration <How to work with build configurations_>`_.

Alternatively, add the shield in the project's :file:`CMakeLists.txt` file by using the following command:

.. code-block:: console

   set(SHIELD nrf7002eb)
