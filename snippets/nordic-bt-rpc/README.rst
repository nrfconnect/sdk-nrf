.. _nordic-bt-rpc:

Nordic Bluetooth Low Energy Remote Procedure Call snippet (nordic-bt-rpc)
#########################################################################

.. contents::
   :local:
   :depth: 2

Overview
********

This snippet allows you to build Bluetooth® applications with Bluetooth stack running on a core dedicated for radio communication.
For example in the nRF54H20 SoC, it is the radio core.
The communication between an application and the Bluetooth stack is handled by the Bluetooth RPC library.

Supported SoCs
**************

Currently, the following SoCs from Nordic Semiconductor are supported for use with the snippet:

* :ref:`zephyr:nrf5340dk_nrf5340`
* :ref:`zephyr:nrf54h20dk_nrf54h20`

.. note::
   On the nRF54H20 SoC, the snippet modifies the memory map to make room for settings storage for the radio core.
   The memory for settings storage is obtained by shrinking the application core ``cpuapp_slot0_partition`` partition.
