.. _ug_nrf70_features:

Features of nRF70 Series
########################

.. contents::
    :local:
    :depth: 2

The nRF70 Series of wireless companion ICs add Wi-Fi® capabilities to another System-on-Chip (SoC), Microprocessor Unit (MPU), or Microcontroller (MCU) host device.
It implements the Physical (PHY) and Medium Access Controller (MAC) layers of the IEEE 802.11 protocol stack, while the higher layers of the networking stack run on the host device.
The nRF70 series wireless companion IC includes the RF front end and power management function.

The nRF70 series devices are designed for Internet of Things (IoT) applications and are ideal for adding modern Wi-Fi 6 capabilities to existing Bluetooth® Low Energy, Thread, or Zigbee systems, as well as adding Wi-Fi Access Point scanning capabilities to LTE/GPS systems.

For additional information, see the following documentation:

* :ref:`nrf7002dk_nrf5340`
* The :ref:`introductory documentation <getting_started>` if you are not familiar with the |NCS| and the development environment.

The following figure illustrates the partitioning of the Wi-Fi stack between the nRF70 series device and the host device:

.. figure:: images/nrf70_ug_overview.svg
   :alt: Overview of nRF70 application architecture

   Overview of nRF70 application architecture

Supported boards
****************

Devices in the nRF70 Series are supported by the following boards in the `Zephyr`_ open source project and in |NCS|.

.. list-table::
   :header-rows: 1

   * - DK or Prototype platform
     - Companion module
     - PCA number
     - Build target
     - Documentation
   * - nRF7002 DK
     - Not applicable
     - PCA10143
     - ``nrf7002dk_nrf5340_cpuapp``
     - | `Product Specification <Product specification for nRF70 Series devices_>`_
       | `User Guide <nRF7002 DK Hardware_>`_
   * - nRF7001 emulation on nRF7002 DK
     - Not applicable
     - PCA10143
     - ``nrf7002dk_nrf7001_nrf5340_cpuapp``
     - | `nRF70 Series Specification <nRF70 Series_>`_
   * - nRF5340 DK
     - nRF7002 EK
     - PCA10095
     - ``nrf5340dk_nrf5340_cpuapp``
     - | `Product Specification <nRF5340 Product Specification_>`_
       | :ref:`Getting started <ug_nrf5340_gs>`
       | `User Guide <nRF5340 DK User Guide_>`_
   * - nRF52840 DK
     - nRF7002 EK
     - PCA10056
     - ``nrf52840dk_nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | :ref:`Getting started <ug_nrf52_gs>`
       | `User Guide <nRF52840 DK User Guide_>`_
   * - nRF9160 DK
     - nRF7002 EK
     - PCA10090
     - ``nrf9160dk_nrf9160_ns``
     - | `Product Specification <nRF9160 Product Specification_>`_
       | :ref:`Getting started <ug_nrf9160_gs>`
       | `User Guide <nRF9160 DK Hardware_>`_
   * - Thingy:53
     - nRF7002 EB
     - PCA20053
     - ``thingy53_nrf5340_cpuapp``
     - | :ref:`Getting started <ug_thingy53_gs>`
       | `User Guide <Nordic Thingy:53 Hardware_>`_

Supported Wi-Fi standards and modes
***********************************

The nRF70 series wireless companion ICs add Wi-Fi 6 support to a host device that includes IP-based networking support.
Wi-Fi 6 aligns with IEEE 802.11ax and all earlier versions of the IEEE 802.11 suite of wireless LAN standards.

Currently only Station (STA) support is included.
Access Point (AP) and peer-to-peer support in the form of Wi-Fi Direct will be available in the future.
