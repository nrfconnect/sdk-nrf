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

* The `Quick Start app`_ to start development with the nRF7002 DK.
* `nRF70 Series`_ for the technical documentation on the nRF70 Series devices.
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

* :ref:`Wi-Fi mode <ug_wifi>`: For IEEE 802.11 protocol stack functionality.
* :ref:`Radio test <wifi_radio_test>`: For PHY (Baseband and Radio) characterizations and calibrations.
* :ref:`Offloaded raw transmission <ug_nrf70_developing_offloaded_raw_tx>`: Allows the offloading of raw IEEE 802.11 frame transmission to the nRF Wi-Fi driver.

The nRF70 Series devices support the following functionalities in the Wi-Fi mode:

* :ref:`Station (STA) <wifi_station_sample>`: Operates as a wireless client device.
* :ref:`Software-enabled Access Point (SoftAP or SAP) <nRF70_soft_ap_mode>`: Operates as a virtual access point device.
* :ref:`Scan <ug_nrf70_developing_scan_operation>`: Operates as a scan-only device.
* :ref:`Wi-Fi advanced security <ug_nrf70_wifi_advanced_security_modes>`: Allows the use of advanced security, certificate-based Wi-Fi security, and the Platform Security Architecture (PSA) security framework.
* :ref:`Wi-Fi Direct® (P2P) mode <ug_wifi_direct>`: Allows the establishment of direct device-to-device connections without requiring a traditional access point.
* :ref:`ug_nrf70_developing_raw_ieee_80211_packet_transmission`: Allows the injection of raw IEEE 802.11 frames in Station and Monitor modes.
* :ref:`Monitor <ug_nrf70_developing_raw_ieee_80211_packet_reception>`: Operates as an IEEE 802.11 wireless packet sniffer.
* :ref:`Promiscuous reception <ug_nrf70_developing_promiscuous_packet_reception>`: Allows the reception of IEEE 802.11 packets from a connected BSSID when operating in Station mode.

See the :ref:`ug_nrf70_features_hostap` section on the :ref:`ug_wifi_overview` page for information about the WPA supplicant used by the nRF70 Series devices.
