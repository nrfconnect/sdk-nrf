.. _nordic-ppr:

Nordic PPR snippet with execution from SRAM (nordic-ppr)
##########################################################

.. contents::
   :local:
   :depth: 2


Overview
********

This snippet allows you to build Zephyr with the capability to boot Nordic PPR (Peripheral Processor) from the application core.
The PPR code must be executed from SRAM, so the PPR image must be built without the ``xip`` board variant, or with the :kconfig:option:`CONFIG_XIP` Kconfig option disabled.
