.. _nordic-flpr:

Nordic FLPR snippet with execution from SRAM (nordic-flpr)
##########################################################

.. contents::
   :local:
   :depth: 2


Overview
********

This snippet allows you to build Zephyr with the capability to boot Nordic FLPR (Fast Lightweight Peripheral Processor) from the application core.
The primary purpose of this snippet is to enable the transfer of the FLPR code to the designated memory region (if required) and to initiate the FLPR core.

The FLPR code must be executed from SRAM, so the FLPR image must be built without the ``xip`` board variant, or with the :kconfig:option:`CONFIG_XIP` Kconfig option disabled.
Use the :ref:`nordic-flpr-xip` snippet instead when the FLPR image runs in place.

Usage
*****

Apply the snippet to the application core image when building, for example:

.. code-block:: console

   west build -p -b nrf54h20dk/nrf54h20/cpuapp -S nordic-flpr --no-sysbuild

See also
********

* :ref:`ug_nrf54h20_flpr` for the full build and programming procedure on the nRF54H20 DK.
* :ref:`vpr_flpr_nrf54l` for the equivalent procedure on the nRF54L Series devices.
