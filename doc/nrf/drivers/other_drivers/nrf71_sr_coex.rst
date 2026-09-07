.. _nrf71_sr_coex:

nRF71 short-range coexistence driver
####################################

.. contents::
   :local:
   :depth: 2

The nRF71 short-range (SR) coexistence driver coordinates the concurrent operation of the Wi-Fi® and short-range (Bluetooth® LE) radios on an nRF71 Series device, which integrates both radios on a single die.

Coexistence is arbitrated on-chip by the Coexistence Controller (COEXC) hardware together with a Coexistence Manager (CM) that runs in the Wi-Fi processor firmware.
The Coexistence Driver (CD) is the host-side coordination layer between the Wi-Fi driver, the short-range driver, and the Coexistence Manager.
This differs from the nRF70 Series, where a separate Wi-Fi companion arbitrates with an external short-range SoC over a wired Packet Traffic Arbitration (PTA) interface.

Overview
********

The coexistence driver performs the following functions:

* Exposes APIs (``coex_cd_*``) that the short-range and Wi-Fi drivers call to request short-range priority windows, report short-range activity, and notify radio power state changes.
* Invokes APIs (``coex_sr_*``) on the short-range driver to enable coexistence and to configure the short-range priorities.
* Forwards coexistence commands to, and processes events from, the Coexistence Manager over the Wi-Fi host-to-firmware control path.

The users of the driver are the nRF71 *Wi-Fi driver* and the *short-range (Bluetooth LE) driver*.
Applications do not call it directly.
The driver depends on the nRF71 Wi-Fi driver, because the command and event path to the Coexistence Manager runs over it, and it is only meaningful when the Wi-Fi radio is present.

Architecture
============

The following diagram shows the components and the interfaces between them:

.. uml::

   @startuml
   skinparam componentStyle rectangle
   skinparam shadowing false
   skinparam defaultTextAlignment center

   package "Host (Cortex-M33)" {
     [Wi-Fi driver] as WIFID
     [Coexistence Driver\n(CD)] as CD
     [short-range (BLE) driver] as SRD
   }

   package "Wi-Fi core (RPU)" {
     [Wi-Fi MAC\nHW clients 0-3] as MAC
     [Coexistence Manager\n(CM)] as CM
   }

   [short-range radio\nHW clients 4-5] as SRR
   [Coexistence Controller (COEXC)] as COEXC

   WIFID --> CD : power notify\n(coex_cd_wifi_power_notify)
   SRD --> CD : coex_cd_*\n(SR request / activity / power)
   CD --> SRD : coex_sr_*\n(enable / set priority)
   CD --> WIFID : send CD2CM\n(nrf71_wifi_coex_cmd_send)
   WIFID --> CD : deliver CM2CD\n(nrf71_wifi_coex_on_event)
   WIFID <--> MAC : FMAC / IPC\n(host <-> RPU)
   CD ..> CM : CD2CM commands /\nCM2CD events (via FMAC)
   CM --> COEXC : configure / CCCONF
   MAC --> COEXC : HW requests (0-3)
   SRR --> COEXC : HW requests (4-5)
   @enduml

The main interfaces are:

* *Wi-Fi driver ↔ CD* - Radio power state notifications (``coex_cd_wifi_power_notify()``), and the transport the CD uses to reach the Coexistence Manager (``nrf71_wifi_coex_cmd_send()`` and ``nrf71_wifi_coex_on_event()``).
* *short-range driver ↔ CD* - The short-range driver requests priority windows, reports activity, and notifies power state changes through ``coex_cd_*``.
  The CD configures the short-range driver through ``coex_sr_*``.
* *CD ↔ Coexistence Manager* - ``CD2CM`` commands and ``CM2CD`` events, carried over the Wi-Fi control path.
  The Coexistence Manager programs the COEXC hardware and handles hardware requests from the radios.

Relationship to MPSL coexistence
--------------------------------

The Multiprotocol Service Layer (MPSL) coexistence feature (``mpsl_cx``) implements a wired-PTA request/grant interface for arbitrating a Nordic short-range radio against an *external* Wi-Fi companion.
The nRF71 coexistence driver targets a different topology, supporting on-die arbitration between co-located radios through the COEXC hardware and the firmware Coexistence Manager.
The two mechanisms are therefore independent, and the coexistence driver is a standalone driver rather than an MPSL coexistence backend.

Feature support
***************

The following coexistence features are implemented in the current release:

* Startup coexistence configuration (Wi-Fi and short-range priority ranges and user parameters).
* Runtime Wi-Fi coexistence enable and disable.
* Radio power-down and power-up handling for the Wi-Fi and short-range drivers.

The following features are planned for a later phase, and are not available in the current release:

* Short-range Single Priority Window requests (a request returns ``-ENOTSUP``).
* Periodic Priority Window generation, without and with alignment (a request returns ``-ENOTSUP``).
* Thread activity Periodic Priority Windows.
* Scheduling service and runtime adaptive priority optimization.
* Wi-Fi channel or band change handling and the associated Bluetooth LE bad-channel mapping.

Configuration
*************

To enable the driver, use the :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER` Kconfig option.
As coexistence involves both radios, the driver depends on the nRF71 Wi-Fi driver (:kconfig:option:`CONFIG_WIFI_NRF71`) and Bluetooth (:kconfig:option:`CONFIG_BT`).

The following Kconfig options configure the driver:

* :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER_INIT_PRIORITY` - Sets the initialization priority within the ``APPLICATION`` initialization level, kept after the Wi-Fi driver.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL` - Sets the log level for the driver.

API documentation
*****************

The coexistence API contract (the ``coex_cd_*`` and ``coex_sr_*`` functions and the ``CD2CM``/``CM2CD`` message types) is defined by the coexistence firmware interface headers, which match the interface of the ROMed nRF71 firmware.

| CD-to-CM interface header: :file:`drivers/wifi/nrf71/inc/common/fw_if/nrf71_coex_if.h`
| CD-to-short-range interface header: :file:`drivers/wifi/nrf71/inc/common/fw_if/nrf71_cd_sr_if.h`
| Wi-Fi transport header: :file:`include/drivers/wifi/nrf71/nrf71_wifi_coex.h`
| Source files: :file:`drivers/nrf71_sr_coex/src/`

The following APIs are exposed by the coexistence driver to the short-range and Wi-Fi drivers:

* :c:func:`coex_cd_sr_software_client_request` - Requests or releases a short-range Single Priority Window (Phase 2; currently returns ``-ENOTSUP``).
* :c:func:`coex_cd_update_short_range_activity_info` - Reports short-range activity for Periodic Priority Window generation (Phase 2; currently returns ``-ENOTSUP``).
* :c:func:`coex_cd_sr_power_notify` - Notifies the coexistence driver of short-range power-down and power-up events.
* :c:func:`coex_cd_wifi_power_notify` - Notifies the coexistence driver of Wi-Fi power-down and power-up events.

The following APIs are exposed by the short-range driver and invoked by the coexistence driver:

* :c:func:`coex_sr_enable` - Enables or disables short-range coexistence.
* :c:func:`coex_sr_set_client_priority` - Configures the short-range client priority ranges.
