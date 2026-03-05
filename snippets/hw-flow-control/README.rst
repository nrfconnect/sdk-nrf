.. _hw-flow-control:

Hardware flow control snippet (hw-flow-control)
###############################################

.. contents::
   :local:
   :depth: 2


Overview
********

This snippet enables UART hardware flow control (RTS/CTS) on the console UART used for debugging and serial communication on supported development kits.
When applied, the snippet adds the ``hw-flow-control`` devicetree property to the relevant UART node so that the host and the board can use RTS (Request to Send) and CTS (Clear to Send) signals to pace data and avoid buffer overruns.

Use this snippet when you need more reliable serial communication at higher baud rates or when you observe data loss or instability over the debug UART.
The host (PC or bridge) must also support and use hardware flow control, otherwise, enable it only on the board side or use the default (no hardware flow control).

Supported boards
****************

The snippet provides board-specific overlays for:

* :zephyr:board:`nrf5340dk`
* :zephyr:board:`nrf54h20dk`
* :zephyr:board:`nrf54l15dk`

Usage
*****

Apply the snippet when building, for example:

.. code-block:: bash

   west build -b nrf5340dk/nrf5340/cpuapp -- -DSNIPPET=hw-flow-control

The snippet appends the appropriate devicetree overlay that sets ``hw-flow-control`` on the console UART instance for the selected board.
