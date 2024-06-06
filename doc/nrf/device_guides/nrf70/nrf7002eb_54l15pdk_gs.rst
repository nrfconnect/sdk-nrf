:orphan:

.. _ug_nrf7002eb_nrf54l15pdk_gs:

Getting started with nRF7002 EB for nRF54L15 PDK
################################################

.. contents::
   :local:
   :depth: 4

This section builds on :ref:`nRF7002 EB <ug_nrf7002eb_gs>` documentation, only changes specific to :ref:`zephyr:nrf54l15pdk_nrf54l15` are mentioned here.

Overview
********

The nRF7002 EB is a shield that can be used with the nRF54L15 PDK to add support for Wi-Fi.

Limitations
===========

The Wi-Fi support is experimental and has the following limitations:

* It only supports STA mode
* It only allows Wi-Fi open security
* It is only suitable for low throughput applications

Pin mapping for nRF54L15 PDK
############################

The pin mapping for the nRF54L15 PDK is as follows:

+-----------------------------------+-------------------+-----------------------------------------------+
| nRF70 Series pin name (EB name)   | nRF54L15 PDK pins | Function                                      |
+===================================+===================+===============================================+
| CLK (CLK)                         | P1.11             | SPI Clock                                     |
+-----------------------------------+-------------------+-----------------------------------------------+
| CS (CS)                           | P1.08             | SPI Chip Select                               |
+-----------------------------------+-------------------+-----------------------------------------------+
| MOSI (D0)                         | P1.10             | SPI MOSI                                      |
+-----------------------------------+-------------------+-----------------------------------------------+
| MISO (D1)                         | P1.09             | SPI MISO                                      |
+-----------------------------------+-------------------+-----------------------------------------------+
| BUCKEN (EN)                       | P1.13             | Enable Buck regulator                         |
+-----------------------------------+-------------------+-----------------------------------------------+
| IOVDDEN (Not used)                | P1.13             | Enable IOVDD regulator                        |
+-----------------------------------+-------------------+-----------------------------------------------+
| IRQ (IRQ)                         | P1.14             | Interrupt from nRF7002                        |
+-----------------------------------+-------------------+-----------------------------------------------+
| GRANT (GRT)                       | P1.12             | Coexistence grant from nRF7002                |
+-----------------------------------+-------------------+-----------------------------------------------+
| REQ (REQ)                         | P1.06             | Coexistence request to nRF7002                |
+-----------------------------------+-------------------+-----------------------------------------------+
| STATUS (ST0)                      | P1.07             | Coexistence status from nRF7002               |
+-----------------------------------+-------------------+-----------------------------------------------+

.. note::
   Connect ``VIO`` to 1.8 V and ``VBAT`` to 3.6 V and ``GND``.

Building and programming
************************

To build for the nRF7002 EB with the nRF54L15 PDK, use the ``nrf54l15pdk/nrf54l15/cpuapp`` board target with the CMake ``SHIELD`` variable set to ``nrf700x_nrf54l15pdk``.
For example, you can use the following command when building on the command line:

.. code-block:: console

   west build -b nrf54l15pdk/nrf54l15/cpuapp -- -DSHIELD=nrf700x_nrf54l15pdk

To build for a custom target, set ``-DSHIELD=nrf700x_nrf54l15pdk`` when you invoke ``west build`` or ``cmake`` in your |NCS| application.

Alternatively, add the shield in the project's :file:`CMakeLists.txt` file by using the following command:

.. code-block:: console

   set(SHIELD nrf700x_nrf54l15pdk)

To build with the |nRFVSC|, specify ``-DSHIELD=nrf700x_nrf54l15pdk`` in the **Extra Cmake arguments** field.
See :ref:`cmake_options` for instructions on how to provide CMake options.
