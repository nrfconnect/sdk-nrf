.. _ug_zigbee_architectures:

Zigbee architectures
####################

.. contents::
   :local:
   :depth: 2

This page describes the platform designs that are possible with the Zigbee stack on Nordic Semiconductor devices.

The designs are described from the least to the most complex, that is from simple applications that consist of a single chip running single or multiple protocols to scenarios in which the nRF SoC acts as a network co-processor when the application is running on a much more powerful host processor.

.. _ug_zigbee_platform_design_soc:

System-on-Chip designs
**********************

This single-chip solution has the combined RFIC (the IEEE 802.15.4 in case of Zigbee) and processor.
The Zigbee stack and the application layer run on the local processor.

This design has the following advantages:

* Lowest cost
* Lowest power consumption
* Lowest complexity

It also has the following disadvantages:

* For some uses, the nRF SoC's MCU can be to slow to handle both the application and the network stack load.
* The application and the network stack share flash and RAM space.
  This leaves less resources for the application.
* Dual-bank DFU or an external flash is needed to update the firmware.

Single-chip, single protocol (SoC)
==================================

In this design, the application layer and the stack run on the same processor.
The application uses the :ref:`nrfxlib:zboss` APIs directly.

This is the design most commonly used for End Devices and Routers.

.. figure:: /images/zigbee_platform_design_soc.svg
   :alt: Zigbee-only architecture

   Zigbee-only architecture

Single-chip, multiprotocol (SoC)
================================

With the nRF devices supporting multiple wireless technologies, including IEEE 802.15.4 and Bluetooth Low Energy (Bluetooth LE), the application layer and the Zigbee and BLE stack run on the same processor.

This design has the following advantages:

* It allows to run Zigbee and Bluetooth LE simultaneously on a single chip, which reduces the overall BOM cost.

It also has the following disadvantages:

* Bluetooth LE activity can degradate the connectivity on Zigbee if not implemented with efficiency in mind.

.. figure:: /images/zigbee_platform_design_multi.svg
   :alt: Multiprotocol Zigbee and Bluetooth LE architecture

   Multiprotocol Zigbee and Bluetooth LE architecture

For more information, see :ref:`ug_multiprotocol_support` and :ref:`zigbee_light_switch_sample_nus`.

.. _ug_zigbee_platform_design_ncp:

Co-processor designs
********************

The co-processor designs can be used when a more powerful processor (host) that does not have a suitable radio interface.
In these designs, the host interacts with the Zigbee network through Network Co-Processor (NCP).

Split-stack co-processor design
===============================

In this design, the host processor runs only the Zigbee application layer (ZCL).
The NCP runs the Zigbee MAC and the Zigbee Pro network layer as well as provides commands to execute BDB commissioning primitives.
The host processor communicates with the NCP through a serial interface (USB or UART).

.. figure:: /images/zigbee_platform_design_ncp.svg
   :alt: Split Zigbee architecture

   Split Zigbee architecture

The NCP firmware source is available to `ZBOSS Open Initiative (ZOI)`_ members.

This design has the following advantages:

* Cost-optimized solution - uses the resources on the more powerful processor.
* The NCP device does not require the support for the dual-bank DFU.
  It can be upgraded by the host processor.

It also has the following disadvantages:

* The host part of the stack must be built and run for every individual host processor in use.
