.. _nordic-bt-rpc:

Nordic Bluetooth Low Energy Remote Procedure Call snippet (nordic-bt-rpc)
#########################################################################

Overview
********

This snippet allows users to build Bluetooth applications with Bluetooth stack running on a core dedicated for radio communication.
For example in the nRF54H20 SOC it is radio core.
The communication between an application and the Bluetooth stack is handled by the Bluetooth RPC subsystem.

Supported SoCs
**************

Currently the following SoCs from Nordic Semiconductor are supported for use with the snippet:

* :ref:`nRF5340 <programming_board_names>`
* :ref:`nRF54H20 <programming_board_names>`

.. note::
   For the nRF54H20 SOC the snippet modifies memory map to make room for settings storage for radio core.
   The memory for settings storage is obtained by shrinking the application core ``cpuapp_slot0_partition`` partition.
