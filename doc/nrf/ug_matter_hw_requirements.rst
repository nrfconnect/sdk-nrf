.. _ug_matter_hw_requirements:

Matter hardware and memory requirements
#######################################

.. contents::
   :local:
   :depth: 2

Hardware that runs Matter protocol applications must meet specification requirements, including providing the right amount of flash memory and being able to run Bluetooth LE and Thread concurrently.

Supported SoCs
**************

Currently the following SoCs from Nordic Semiconductor are supported for use with the Matter protocol:

* :ref:`nRF5340 <gs_programming_board_names>` (Matter over Thread and Matter over Wi-Fi through the ``nrf7002_ek`` shield)
* :ref:`nRF5340 + nRF7002 <gs_programming_board_names>` (Matter over Thread and Matter over Wi-Fi)
* :ref:`nRF52840 <gs_programming_board_names>` (Matter over Thread)

Front-End Modules
=================

SoCs from Nordic Semiconductor that can run the Matter protocol over Thread can also work with external Front-End Modules.
For more information about the FEM support in the |NCS|, see :ref:`ug_radio_fem` and :ref:`nRF21540 DK <gs_programming_board_names>`.

External flash
**************

For the currently supported SoCs, you must use an external memory with at least 1 MB of flash for nRF52840 and 1.5MB for nRF5340.
This is required to perform the DFU operation.

The development kits for the supported SoCs from Nordic Semiconductor are supplied with the MX25R64 type of external flash that meets these memory requirements.
However, it is possible to configure the SoCs with different QSPI or SPI memory if it is supported by Zephyr.
For this purpose, check the reference design for Nordic DKs for information about how to connect the external memory with SoC, specifically whether the pins are designed for the QSPI or the high-speed SPIM operations.
