.. _ug_nrf70_features:

Features of nRF70 Series
########################

.. contents::
    :local:
    :depth: 2

The nRF70 Series of wireless companion ICs add Wi-Fi® capabilities to another System-on-Chip (SoC), Microprocessor Unit (MPU), or Microcontroller (MCU) host device.
It implements the Physical (PHY) and Medium Access Control (MAC) layers of the IEEE 802.11 protocol stack, while the higher layers of the networking stack run on the host device.
The nRF70 Series wireless companion IC includes the RF front end and power management function.

The nRF70 Series devices are designed for Internet of Things (IoT) applications and are ideal for adding modern Wi-Fi 6 capabilities to existing Bluetooth® Low Energy, Thread, or Zigbee systems, as well as adding Wi-Fi Access Point scanning capabilities to LTE/GPS systems.

For additional information, see the following documentation:

* `nRF70 Series`_ for the technical documentation on the nRF70 Series devices.
* :ref:`nrf7002dk_nrf5340`.
* :ref:`ug_nrf70_developing` documentation for topics related to developing Wi-Fi applications on nRF52, nRF53, and nRF91 Series, using the nRF70 companion IC devices.
* :ref:`installation` and :ref:`configuration_and_build` documentation to install the |NCS| and learn more about its development environment.

The nRF70 Series devices are Wi-Fi companion ICs and do not serve as standalone platforms for developing applications.
Wi-Fi applications may be developed on nRF52, nRF53, and nRF91 Series SoCs that support the nRF70 Series companion ICs.

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
       | :ref:`Getting started <nrf7002dk_nrf5340>`
       | `User Guide <nRF7002 DK Hardware_>`_
   * - nRF7001 emulation on nRF7002 DK
     - Not applicable
     - PCA10143
     - ``nrf7002dk_nrf7001_nrf5340_cpuapp``
     - | `Product Specification <nRF7001 Product Specification_>`_
   * - :ref:`zephyr:nrf5340dk_nrf5340`
     - nRF7002 EK
     - PCA10095
     - ``nrf5340dk_nrf5340_cpuapp``
     - | `Product Specification <nRF5340 Product Specification_>`_
       | :ref:`Getting started <ug_nrf5340_gs>`
       | `User Guide <nRF5340 DK User Guide_>`_
   * - :ref:`zephyr:nrf52840dk_nrf52840`
     - nRF7002 EK
     - PCA10056
     - ``nrf52840dk_nrf52840``
     - | `Product Specification <nRF52840 Product Specification_>`_
       | :ref:`Getting started <ug_nrf52_gs>`
       | `User Guide <nRF52840 DK User Guide_>`_
   * - :ref:`zephyr:nrf9160dk_nrf9160`
     - nRF7002 EK
     - PCA10090
     - ``nrf9160dk_nrf9160_ns``
     - | `Product Specification <nRF9160 Product Specification_>`_
       | :ref:`Getting started <ug_nrf9160_gs>`
       | `User Guide <nRF9160 DK Hardware_>`_
   * - :ref:`zephyr:nrf9161dk_nrf9161`
     - nRF7002 EK
     - PCA10153
     - ``nrf9161dk_nrf9161_ns``
     - | `Product Specification <nRF9161 Product Specification_>`_
       | `User Guide <nRF9161 DK Hardware_>`_
   * - :ref:`zephyr:thingy53_nrf5340`
     - nRF7002 EB
     - PCA20053
     - ``thingy53_nrf5340_cpuapp``
     - | :ref:`Getting started <ug_thingy53_gs>`
       | `User Guide <Nordic Thingy:53 Hardware_>`_

nRF70 Series shields
====================

The following nRF70 Series shields are available and defined in the :file:`nrf/boards/shields` folder.

.. list-table::
   :header-rows: 1

   * - Hardware platform
     - PCA number
     - Build target
     - Documentation
   * - nRF7002 :term:`Evaluation Kit (EK)`
     - PCA63556
     - ``nrf7002ek``
     - | :ref:`Getting started <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 EK with emulated support for the nRF7001 IC
     - PCA63556
     - ``nrf7002ek_nrf7001``
     - | :ref:`Getting started <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 EK with emulated support for the nRF7000 IC
     - PCA63556
     - ``nrf7002ek_nrf7000``
     - | :ref:`Getting started <ug_nrf7002ek_gs>`
       | `User Guide <nRF7002 EK User Guide_>`_
   * - nRF7002 :term:`Expansion Board (EB)`
     - PCA63561
     - ``nrf7002eb``
     - | :ref:`Getting started <ug_nrf7002eb_gs>`
       | `User Guide <nRF7002 EB User Guide_>`_

Supported protocols
*******************

The nRF70 Series devices support the Wi-Fi protocol.
Wi-Fi is a half-duplex packet-based protocol operating on a fixed channel, using CSMA/CA for channel access.

See the :ref:`ug_wifi` documentation for information related to the Wi-Fi protocol.

Supported Wi-Fi standards and modes
===================================

The nRF70 Series wireless companion ICs add Wi-Fi 6 support to a host device that includes IP-based networking support.
Wi-Fi 6 aligns with IEEE 802.11ax and all earlier versions of the IEEE 802.11 suite of wireless LAN standards.

Currently, the nRF70 Series devices support the following modes:

* :ref:`Station (STA) <wifi_station_sample>`: Operates as a wireless client device.
* :ref:`Software-enabled Access Point (SoftAP or SAP) <nRF70_soft_ap_mode>`: Operates as a virtual access point device.
* :ref:`Scan <ug_nrf70_developing_scan_operation>`: Operates as a scan-only device.
* :ref:`Radio test <wifi_radio_test>`: For PHY (Baseband and Radio) characterizations and calibrations.

The nRF70  Series devices also support the functionality of :ref:`ug_nrf70_developing_raw_ieee_80211_packet_transmission`.
This allows the injection of raw 802.11 frames in any of the above modes.

Peer-to-peer support in the form of Wi-Fi Direct® will be available in the future.

See the :ref:`ug_wifi` documentation for more information related to Wi-Fi modes of operation.
