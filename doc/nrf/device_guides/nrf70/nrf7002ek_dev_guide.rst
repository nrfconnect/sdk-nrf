.. _ug_nrf7002ek_gs:

Developing with nRF7002 EK
##########################

.. contents::
   :local:
   :depth: 2

The nRF7002 :term:`Evaluation Kit (EK)` (PCA63556), part of the `nRF70 Series Family <nRF70 Series product page_>`_, is an Arduino shield that features the nRF7002 companion IC.
The kit is designed to provide Wi-Fi® connectivity and Wi-Fi (SSID) scanning capabilities through the nRF7002 companion IC to a compatible host development board.
In addition, the shield may be used to emulate the nRF7001 and nRF7000 companion IC variants.

.. figure:: images/nRF7002ek.png
   :alt: nRF7002 EK

   nRF7002 EK

The nRF7002 EK features an Arduino shield form factor and interface connector that allows it to be used with Arduino compatible boards, such as the `nRF52840 DK <nRF52840 DK product page_>`_, `nRF5340 DK <nRF5340 DK product page_>`_, `nRF9160 DK <nRF9160 DK product page_>`_, or `nRF9161 DK <Nordic nRF9161 DK_>`_.
This interface is used to connect the nRF7002 companion device to a host :term:`System on Chip (SoC)`, Microprocessor Unit (MPU), or :term:`Microcontroller Unit (MCU)`.
For the pin assignment of the interface connectors, see the `nRF7002 companion IC`_ page in the kit's `Hardware User Guide <nRF7002 EK User Guide_>`_.

.. note::
   ``UART1`` cannot be used and must be disabled on the nRF9160 DK, as ``UART1`` ``RXD`` and ``TXD`` conflict with pins **P0.00** and **P0.01** (Arduino pins **D0** and **D1**).
   These correspond to the IOVDD-CTRL-GPIOS and BUCKEN-GPIOS signals in the nRF7002 EK, respectively.

   For an nRF7002 EK connected to an nRF9160 DK, you can find example overlays at the following paths:

   * :file:`sdk-nrf/boards/shields/nrf7002ek/boards/nrf9160dk_nrf9160_ns.overlay`
   * :file:`sdk-nrf/samples/cellular/lwm2m_client/boards/nrf9160dk_with_nrf7002ek.overlay`

.. _nrf7002ek_gs_building_programming:

Building and programming with nRF7002 EK
****************************************

To build for the nRF7002 EB, build for the compatible :ref:`board target <app_boards_names>` with the CMake ``SHIELD`` option set to ``nrf7002eb``.

To add support for the nRF7002 EK on an application running on a compatible host development board, build for the compatible :ref:`board target <app_boards_names>` and specify the ``SHIELD`` CMake option:

* To add support for the nRF7002 EK and the nRF7002 IC, set ``SHIELD`` to ``nrf7002ek``.
* To emulate support for the nRF7001 or nRF7000 ICs, set ``SHIELD`` to ``nrf7002ek_nrf7001`` or ``nrf7002ek_nrf7000``, respectively.

See :ref:`cmake_options` for instructions on how to provide CMake options.

For example, if you build for nRF5340 DK on the command line, you can use the following command:

.. code-block:: console

   west build -b nrf5340dk/nrf5340/cpuapp -- -DSHIELD=nrf7002ek

If you use the |nRFVSC|, specify ``-DSHIELD=nrf7002ek`` in the **Extra Cmake arguments** field when `setting up a build configuration <How to work with build configurations_>`_.

Alternatively, add the shield in the project's :file:`CMakeLists.txt` file, specifying the below settings, depending on which IC is to be used:

.. code-block:: console

   set(SHIELD nrf7002ek)

.. code-block:: console

   set(SHIELD nrf7002ek_nrf7001)

.. code-block:: console

   set(SHIELD nrf7002ek_nrf7000)
