.. _migration-nrf54h20-290-300:

Snippet allowing migration of applications built for nRF54H20 from SDK v2.9.0-H20-1 to v3.0.0 using DFU
#######################################################################################################

.. contents::
   :local:
   :depth: 2

Overview
********

The default memory map has changed between SDK v2.9.0-H20-1 and v3.0.0.
Apart from that, a communication between radio and secure core has ben enablet to provide a true entropy source for network stacks.
This snippet allows you to build a DFU package for your application built and deployed using SDK v2.9.0-H20-1 release.

.. warning::
   Do not apply this snippet if your application defines its own RAM memory map and/or already enabled the communication in the previous version of the SDK.
   In such cases, a proper investigation between output device tree files must be performed.
   The device is upgradeable only if the UCIR binaries between the two builds matches.

Supported SoCs
**************

Currently, the following SoCs from Nordic Semiconductor are supported for use with the snippet:

* :ref:`zephyr:nrf54h20dk_nrf54h20`
