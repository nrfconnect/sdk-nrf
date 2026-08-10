.. _ug_nrf71_features:

Features of the nRF71 Series
############################

.. contents::
   :local:
   :depth: 2

.. TODO: Introduce the nRF71 Series as a standalone Wi-Fi 6 + Bluetooth Low Energy combo SoC.
   Contrast the single-SoC integration with the nRF70 Series companion IC architecture.
   State the target applications (low-power IoT, Wi-Fi 6, dual-band 2.4/5 GHz).

Architecture
************

.. TODO: Describe the on-chip architecture:
   * Application core (Arm Cortex-M33) running the host Wi-Fi driver and the application.
   * VPR/FLPR coprocessor and its role.
   * Wi-Fi radio (RPU) running the IEEE 802.11 MAC firmware (FullMAC).
   * Bluetooth Low Energy support.

Supported protocols
********************

.. TODO: List supported protocols and cross-link to :ref:`ug_wifi` and the Bluetooth LE documentation.

Supported Wi-Fi standards and modes
===================================

.. TODO: Wi-Fi 6 (IEEE 802.11ax), dual-band. List supported modes and roles
   (Station, SoftAP, Scan, Monitor, Raw TX) with cross-references.

hostap
******

.. TODO: Describe the use of the WPA supplicant (Zephyr hostap fork) and supported security modes.
   Reuse and adapt the description from :ref:`ug_nrf70_features_hostap` as applicable to the nRF71 Series.
