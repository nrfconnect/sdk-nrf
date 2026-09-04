.. _vpr_vio_toggle_sample:

VPR VIO toggle
##############

.. contents::
   :local:
   :depth: 2

This sample runs on a VPR coprocessor and drives a single GPIO pin directly through the VPR VIO (VPR I/O) interface (not through GPIO).
Each VIO write is a single-cycle CSR write, so the pin can be toggled with cycle-accurate, deterministic timing.
The sample generates a continuous block wave whose HIGH and LOW widths are exact core-cycle counts.

Requirements
************

The sample supports the following development kits:

.. table-from-sample-yaml::

Overview
********

VIO (VPR IO) is a basic GPIO controller for the VPR cores, with support for up to 16 GPIO pins.
The pins can be accessed in a single clock cycle making it possible to implement cycle-accurate IO operations.
The pins are routed to the VPR-core by configuring them in devicetree overlay for the application running on the APP-core.
When building for a VPR target, sysbuild will automatically build a vpr_launcher application for the APP-core.

Configuration
*************

|config|

The mapping of the GPIO pins to VIO pins is depends on the device and can not be changed.
You can find which VIO pin of which VPR core corresponds to which GPIO pin in the GPIO port mapping of the device.
To configure a specific GPIO pin you need to assign it to the VPR core inside `sysbuild/vpr_launcher/boards/<board>.overlay`.
To allow for fast pin toggling, you can set the drive strength to `high`` or `extra high`, depending on the port.
You need to set the corresponding VIO pin in :kconfig:option:`CONFIG_APP_VIO_PIN`.


Building and running
********************

This sample is built for the VPR core and uses sysbuild:

.. code-block:: console

   west build -b nrf9251dk/nrf9251/cpuflpr
   west flash

Testing
*******

After programming, observe the selected pin on a logic analyzer or oscilloscope. The pin emits a continuous block wave, driven entirely by the VPR through VIO. The VPR console prints the board target and VIO index at startup.
