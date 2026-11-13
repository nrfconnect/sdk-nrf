.. _snippet-nordic-ppr:

Nordic PPR snippet with execution from RAM (nordic-ppr)
#######################################################

.. contents::
   :local:
   :depth: 2

Overview
********

This snippet allows you to build Zephyr with the capability to boot the Nordic PPR (Peripheral Processor) from the application core.
The primary purpose of the snippet is to enable the transfer of the PPR code to the designated memory region (if required) and to initiate the PPR core.

The PPR code must be executed from RAM, so the PPR image must be built without the ``xip`` board variant, or with the :kconfig:option:`CONFIG_XIP` Kconfig option disabled.
Use the :ref:`snippet-nordic-ppr-xip` snippet instead when the PPR image runs in place.

On the nRF9251 application core, the snippet also reserves the **UART132** instance for the PPR core and routes its interrupts to the PPR interrupt controller.

Usage
*****

Apply the snippet to the application core image when building, for example:

.. code-block:: console

   west build -p -b nrf54h20dk/nrf54h20/cpuapp -S nordic-ppr --no-sysbuild

See also
********

* :ref:`ug_nrf54h20_ppr` for the full build and programming procedure on the nRF54H20 DK.
