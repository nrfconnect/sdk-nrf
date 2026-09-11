.. _vpr_vio_shift_output:

VPR VIO shift output
####################

.. contents::
   :local:
   :depth: 2

The sample emulates a UART transmitter by driving a single GPIO pin from a VPR coprocessor (FLPR or PPR).
It uses output shifting to automatically output a whole (8N1) frame while the processor can prepare the next frame.
This sample is not intended to be a full UART implementation, it is a showcase of how to implement similar protocols.

Application overview
********************

The sample combines the VIO output shift register with a VTIM counter:

* **VTIM counter 0** runs as the bit-clock (baud-rate) generator.
  It counts down to zero, when it hits zero it generates an event and reloads the bit period.

* **VIO output shift register** When shift mode is enabled, instead of copying the whole buffered output register (OUTB) to the output register (OUT) only a specific number of bit is copied.
  The left over bit are moved to the right with the same number of bits.
  In this sample we pack the byte with a start and stop bit into the buffered output register (OUTB).
  When all bits are shifted out the next packed byte is put into OUTB.

To get a consistent result, independent on the compiler configuration, the loop packing the bytes is provided as inline assembly.
It takes a maximum of 10 clock cycles between writes to OUTB.
As the packed bytes are 10 bits (start bit, 8 data bits and a stop bit), we can output at the full clock speed of the VPR core.

The application drives the VIO bit selected with :kconfig:option:`CONFIG_APP_VIO_PIN`.
* On the **nRF9xx**, the pin is assigned to the VPR through the UICR, generated from the ``vpr_launcher`` devicetree overlay.
* On the **nRF54L series**, the FLPR routes the pin to the VPR at runtime, using :kconfig:option:`CONFIG_APP_TX_GPIO_PORT` and :kconfig:option:`CONFIG_APP_TX_GPIO_PIN`.


Requirements
************

The sample supports the following development kits and cores:

.. table-from-sample-yaml::

Pin mapping
===========

The TX pin depends on the target.
The defaults, set in the board ``.conf`` files and/or routed in the matching ``vpr_launcher`` overlay, are:

.. list-table::
   :header-rows: 1

   * - Target
     - VIO index
     - GPIO pin
   * - nRF9251 DK, ``cpuflpr``
     - 8
     - P2.05 (TXD1)
   * - nRF9251 DK, ``cpuppr``
     - 0
     - P1.06 (TXD0)
   * - nRF54L15 DK, ``cpuflpr``
     - 9
     - P2.09 (LED0)

Configuration
*************

To use a different pin:
* On the nRF9xx, update :kconfig:option:`CONFIG_APP_VIO_PIN` and (:file:`sysbuild/vpr_launcher/nrf9251_flpr.overlay` or :file:`_ppr.overlay`).
* On the nRF54L series, update :kconfig:option:`CONFIG_APP_VIO_PIN`, :kconfig:option:`CONFIG_APP_TX_GPIO_PORT` and :kconfig:option:`CONFIG_APP_TX_GPIO_PIN`.

Building and running
********************

.. |sample path| replace:: :file:`samples/vpr/vio_shift_output`

Build for a VPR core.
Sysbuild automatically adds the VPR launcher image for the application core:

.. code-block:: console

   west build -b nrf9251dk/nrf9251/cpuflpr
   west flash

Testing
*******

* On the nRF9251dk the signals are routed to the debugger so you can test without external hardware.
* On the nRF54L15dk you need to attach a logic analyzer or oscilloscope to the TX pin.
