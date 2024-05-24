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
* :ref:`Monitor <ug_nrf70_developing_raw_ieee_80211_packet_reception>`: Operates as an IEEE 802.11 wireless packet sniffer.

The nRF70 Series devices also support the following functionalities:

* :ref:`ug_nrf70_developing_raw_ieee_80211_packet_transmission`: Allows the injection of raw IEEE 802.11 frames in Station and Monitor modes.
* :ref:`Promiscuous reception <ug_nrf70_developing_promiscuous_packet_reception>`: Allows the reception of IEEE 802.11 packets from a connected BSSID when operating in Station mode.

Peer-to-peer support in the form of Wi-Fi Direct® will be available in the future.

See the :ref:`ug_wifi` documentation for more information related to Wi-Fi modes of operation.
