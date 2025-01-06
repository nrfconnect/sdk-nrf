.. _programming_custom_board:

Connecting custom boards for programming
########################################

.. contents::
   :local:
   :depth: 2

After you :ref:`created custom board files <defining_custom_board>`, you need to connect your custom board to a debug probe for programming.

Hardware requirements
*********************

To connect a custom board for programming, you need the following hardware:

* A debug probe supporting Serial Wire Debug (SWD), such as:

  * A Development Kit (DK) from Nordic Semiconductor (all include an onboard debug probe)
  * Dedicated debug adapter (like SEGGER J-Link)

* Connecting wires
* USB cable for the DK or the debug adapter

Required connections
********************

You have to connect the following pins between your custom board and the debug probe:

* Essential connections:

  * SWDCLK (Clock)
  * SWDIO (Data)
  * GND (Ground)
  * VTref (Target Reference Voltage)

* Optional connections:

  * VDD - For powering the board from debug probe.
    If the custom board is not powered from VDD, it must be powered externally.
  * RESET - For resetting the board.
    If the custom board does not have a reset pin, you can use the reset pin on the debug probe.

Connecting the debug probe
**************************

The following steps describe how to connect your custom board to a debug probe.

.. tabs::

   .. group-tab:: Using a DK as debug probe

      Development Kits from Nordic Semiconductor include an onboard debug probe that supports the J-Link interface.

      To connect your custom board to this onboard debug probe, complete the following steps:

      1. Locate the SWD debug output header pins on your DK.
         Check the DK user guide on `Nordic Semiconductor TechDocs`_ for the exact location of these pins.
         For example, for the nRF52840 DK, read the `Debug output <nRF52840 DK Debug output_>`_ page.
      #. Connect the required pins to your custom board.
      #. Connect the DK to your PC using the USB cable.
      #. Install required J-Link drivers if not already present.

   .. group-tab:: Using a dedicated debug adapter

      If you do not have a Development Kit, use a dedicated debug adapter:

      1. Connect a compatible debug probe (like SEGGER J-Link) to your custom board.
         See the debug probe documentation for the exact pinout.
      #. Install the appropriate debug interface drivers.
      #. Connect the debug probe to your PC using the USB cable.

.. note::
      The |NCS| supports various debug interfaces like J-Link and CMSIS-DAP, provided you have the proper drivers installed.

Programming custom boards
*************************

After you connected the custom board, you can program your application to the board using either the :ref:`standard programming instructions <programming>` or the `Programmer app`_ from `nRF Connect for Desktop`_.
