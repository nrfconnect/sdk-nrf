.. _nordic-flpr:

Nordic FLPR snippet with execution from SRAM (nordic-flpr)
##########################################################

.. contents::
   :local:
   :depth: 2


Overview
********

This snippet allows you to build Zephyr with the capability to boot Nordic FLPR (Fast Lightweight Peripheral Processor) from the application core.
The FLPR code must be executed from SRAM, so the FLPR image must be built without the ``xip`` board variant, or with the :kconfig:option:`CONFIG_XIP` Kconfig option disabled.
