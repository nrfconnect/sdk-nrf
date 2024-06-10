:orphan:

.. _ug_nrf7002eb_nrf54h20dk_gs:

Getting started with nRF7002 EB for nRF54H20 DK
###############################################

.. contents::
   :local:
   :depth: 4

This section provides information about how to use the nRF7002 :term:`Expansion Board (EB)` (PCA63561) to provide Wi-Fi® connectivity to the :ref:`zephyr:nrf54h20dk_nrf54h20`.
The changes mentioned are specific to the nRF54H20 DK.
See the :ref:`nRF7002 EB <ug_nrf7002eb_gs>` documentation for more details on building and programming with the nRF7002 EB.

Overview
********

The nRF7002 EB is a shield that can be used with the nRF54H20 DK to add support for Wi-Fi.

Limitations
===========

The Wi-Fi support is experimental and has the following limitations:

* It only supports STA mode.
* It is only suitable for low-throughput applications.

Pin mapping for nRF54H20 DK
===========================

The pin mapping for the nRF54H20 DK is as follows:

+-----------------------------------+-------------------+-----------------------------------------------+
| nRF70 Series pin name (EB name)   | nRF54H20 DK pins  | Function                                      |
+===================================+===================+===============================================+
| CLK (CLK)                         | P1.01             | SPI Clock                                     |
+-----------------------------------+-------------------+-----------------------------------------------+
| CS (CS)                           | P1.04             | SPI Chip Select                               |
+-----------------------------------+-------------------+-----------------------------------------------+
| MOSI (D0)                         | P1.05             | SPI MOSI                                      |
+-----------------------------------+-------------------+-----------------------------------------------+
| MISO (D1)                         | P1.06             | SPI MISO                                      |
+-----------------------------------+-------------------+-----------------------------------------------+
| BUCKEN (EN)                       | P1.00             | Enable Buck regulator                         |
+-----------------------------------+-------------------+-----------------------------------------------+
| IOVDDEN (Not used)                | P1.00             | Enable IOVDD regulator                        |
+-----------------------------------+-------------------+-----------------------------------------------+
| IRQ (IRQ)                         | P1.02             | Interrupt from nRF7002                        |
+-----------------------------------+-------------------+-----------------------------------------------+
| GRANT (GRT)                       | P1.03             | Coexistence grant from nRF7002                |
+-----------------------------------+-------------------+-----------------------------------------------+
| REQ (REQ)                         | P1.08             | Coexistence request to nRF7002                |
+-----------------------------------+-------------------+-----------------------------------------------+
| STATUS (ST0)                      | P1.07             | Coexistence status from nRF7002               |
+-----------------------------------+-------------------+-----------------------------------------------+

.. note::
   Connect ``VIO`` to 1.8 V and ``VBAT`` to 3.6 V and ``GND``.

Building and programming
************************

To build for the nRF7002 EB with nRF54H20DK, use the ``nrf54h20dk/nrf54h20/cpuapp`` board target with the CMake ``SHIELD`` variable set to ``nrf700x_nrf54h20dk``.
For example, you can use the following command when building on the command line:

.. code-block:: console

   west build -b nrf54h20dk/nrf54h20/cpuapp -- -DSHIELD=nrf700x_nrf54h20dk

To build for a custom target, set ``-DSHIELD=nrf700x_nrf54h20dk`` when you invoke ``west build`` or ``cmake`` in your |NCS| application.

Alternatively, add the shield in the project's :file:`CMakeLists.txt` file by using the following command:

.. code-block:: console

   set(SHIELD nrf700x_nrf54h20dk)

To build with the |nRFVSC|, specify ``-DSHIELD=nrf700x_nrf54h20dk`` in the **Extra Cmake arguments** field.
See :ref:`cmake_options` for instructions on how to provide CMake options.
