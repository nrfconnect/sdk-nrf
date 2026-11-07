.. _nrf71_sr_coex:

nRF71 short-range coexistence driver
####################################

.. contents::
   :local:
   :depth: 2

The nRF71 short-range (SR) coexistence driver coordinates the concurrent operation of the Wi-Fi® and short-range (Bluetooth® LE) radios on an nRF71 Series device, which integrates both radios on a single die.

Coexistence is arbitrated on-chip by the Coexistence Controller (COEXC) hardware together with a Coexistence Manager (CM) that runs in the Wi-Fi processor firmware.
The Coexistence Driver (CD) is the host-side coordination layer between the application, the Wi-Fi driver, the short-range driver, and the Coexistence Manager.
This differs from the nRF70 Series, where a separate Wi-Fi companion arbitrates with an external short-range SoC over a wired Packet Traffic Arbitration (PTA) interface.

Overview
********

The coexistence driver performs the following functions:

* Programs the COEXC client allow masks, priority mapping, and turnaround times for the shared or separate antenna setup.
* Follows the Wi-Fi interface state through a notifier in the Wi-Fi driver, and configures the Coexistence Manager every time Wi-Fi comes up.
* Stores the coexistence configuration that the application passes once, and applies it on every Wi-Fi bring-up.
* Enables coexistence on the short-range driver and sets its priorities through callbacks that the short-range driver registers.
* Optionally keeps Bluetooth LE off the data channels used by the connected Wi-Fi station.

The driver starts at the ``APPLICATION`` initialization level and works with built-in defaults, so an application does not have to call it.
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
     [Application] as APP
     [Wi-Fi driver] as WIFID
     [Coexistence Driver\n(CD)] as CD
     [short-range (BLE) driver] as SRD
     [Bluetooth host] as BTH
   }

   package "Wi-Fi core (RPU)" {
     [Wi-Fi MAC\nHW clients 0-3] as MAC
     [Coexistence Manager\n(CM)] as CM
   }

   [short-range radio\nHW clients 4-5] as SRR
   [Coexistence Controller (COEXC)] as COEXC

   APP --> CD : one-time configuration\n(nrf71_sr_coex_init)
   WIFID --> CD : state notifier\n(admin up / down,\nconnected / disconnected)
   SRD --> CD : register callbacks,\npower notify
   CD --> SRD : enable / set priority\n(registered callbacks)
   CD --> WIFID : send CD2CM\n(nrf71_wifi_coex_cmd_send)
   WIFID --> CD : deliver CM2CD\n(event callback)
   CD --> BTH : channel map\n(optional)
   CD --> COEXC : CCMALLOW / CCCONF /\nTURNAROUND
   WIFID <--> MAC : FMAC / IPC\n(host <-> RPU)
   CD ..> CM : CD2CM commands /\nCM2CD events (via FMAC)
   CM --> COEXC : band mode
   MAC --> COEXC : HW requests (0-3)
   SRR --> COEXC : HW requests (4-5)
   @enduml

The main interfaces are:

* *Application ↔ CD* - The application can pass its antenna setup, priorities, user parameters, and an optional coexistence parameter blob once, using the :c:func:`nrf71_sr_coex_init` function.
* *Wi-Fi driver ↔ CD* - The Wi-Fi driver reports its state through a single-consumer notifier, and carries ``CD2CM`` commands and ``CM2CD`` events between the CD and the Coexistence Manager.
  The interface is declared in :ncs-file:`/include/drivers/wifi/nrf71/nrf71_wifi_coex.h`.
* *short-range driver ↔ CD* - The short-range driver registers its enable and set-priority callbacks with the :c:func:`nrf71_sr_coex_sr_register` function, and reports power-down and power-up with the :c:func:`nrf71_sr_coex_sr_power_notify` function.
* *CD ↔ Coexistence Manager* - ``CD2CM`` commands and ``CM2CD`` events, carried over the Wi-Fi control path.
  Each command waits for its matching event before the next one is sent.

Wi-Fi state notifications
=========================

The Wi-Fi driver reports the following events to the coexistence driver:

* Admin up - The first Wi-Fi interface is up and the firmware accepts commands.
* Admin down - The last Wi-Fi interface is going down, reported before the interface is torn down.
  This is also reported when the Wi-Fi device is removed, for example during recovery.
* Connected - The station has associated, with the center frequency and the bandwidth of the operating channel.
* Disconnected - The station has left the channel, reported before admin down when the interface goes down while connected.

The Wi-Fi driver calls the notifier from its own context, so the coexistence driver only records the event and schedules work.
When the coexistence driver registers, the Wi-Fi driver replays the current state, so the order of initialization does not matter.

Coexistence Manager configuration
=================================

On every admin up event, the coexistence driver configures the Coexistence Manager from its own work queue.
It sends the priority ranges, passes the short-range ranges to the short-range driver, and sends the user parameters, the coexistence parameters, and the enable command, in this order.
The short-range driver enables its side of coexistence when it reports power-up with the :c:func:`nrf71_sr_coex_sr_power_notify` function, and disables it before power-down.
The configuration is retried if the Coexistence Manager reports a failure, and it is stopped if Wi-Fi goes down in the middle of it.
If the first attempt times out, the driver does not retry, as the firmware is not expected to answer.

The driver keeps the configuration across Wi-Fi restarts.
If the application calls :c:func:`nrf71_sr_coex_init` while Wi-Fi is up, the new configuration is applied right away.

Relationship to MPSL coexistence
================================

The Multiprotocol Service Layer (MPSL) coexistence feature (``mpsl_cx``) implements a wired-PTA request/grant interface for arbitrating a Nordic short-range radio against an *external* Wi-Fi companion.
The nRF71 coexistence driver targets a different topology, supporting on-die arbitration between co-located radios through the COEXC hardware and the firmware Coexistence Manager.
The two mechanisms are therefore independent, and the coexistence driver is a standalone driver rather than an MPSL coexistence backend.

Feature support
***************

The following coexistence features are implemented in the current release:

* COEXC programming for the shared and separate antenna setups.
* Coexistence Manager configuration on every Wi-Fi bring-up, with built-in defaults or the application configuration.
* Short-range driver power-down and power-up handling.
* Optional Bluetooth LE bad-channel mapping from the channel of the connected Wi-Fi station.
* Shell commands for the driver state and the Coexistence Manager statistics.

The following features are planned for a later phase, and are not available in the current release:

* Short-range Single Priority Window requests.
* Periodic Priority Window generation, without and with alignment.
* Thread activity Periodic Priority Windows.
* Scheduling service and runtime adaptive priority optimization.
* Following Wi-Fi channel switches after association, for the Bluetooth LE channel map.

Configuration
*************

To enable the driver, use the :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER` Kconfig option.
As coexistence involves both radios, the driver depends on the nRF71 Wi-Fi driver (:kconfig:option:`CONFIG_WIFI_NRF71`) and Bluetooth (:kconfig:option:`CONFIG_BT`).

The following Kconfig options configure the driver:

* :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER_INIT_PRIORITY` - Sets the initialization priority within the ``APPLICATION`` initialization level, kept after the Wi-Fi driver.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_CM_EVENT_WAIT_MS` - Sets how long the driver waits for the Coexistence Manager to answer a command.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_CM_RETRY_MAX` and :kconfig:option:`CONFIG_NRF71_SR_COEX_CM_RETRY_INTERVAL_MS` - Set how many times, and how often, a failed configuration is retried.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_WQ_STACK_SIZE` and :kconfig:option:`CONFIG_NRF71_SR_COEX_WQ_PRIORITY` - Set the stack size and the priority of the driver work queue.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_WAIT_FOR_APP_INIT` - Holds back the Coexistence Manager configuration until the application has called :c:func:`nrf71_sr_coex_init`, so the built-in defaults are never applied.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP` - Enables the Bluetooth LE bad-channel mapping, see :ref:`nrf71_sr_coex_ble_chan_map`.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_SHELL` - Enables the ``nrf71_coex`` shell commands.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_DRIVER_LOG_LEVEL` - Sets the log level for the driver.

Application configuration
=========================

To replace the built-in defaults, fill in a :c:struct:`nrf71_sr_coex_cfg` structure and pass it to the :c:func:`nrf71_sr_coex_init` function once, for example from ``main()``:

.. code-block:: c

   #include <drivers/nrf71_sr_coex/nrf71_sr_coex.h>

   static const struct nrf71_sr_coex_cfg coex_cfg = {
           .antenna = NRF71_SR_COEX_ANTENNA_SHARED,
           .prio = {
                   /* Wi-Fi and short-range priority ranges */
           },
           .user = {
                   .ant_alloc = NRF71_SR_COEX_ANT_ALLOC_DYNAMIC,
                   /* Protection times */
           },
   };

   int err = nrf71_sr_coex_init(&coex_cfg);

The driver copies the configuration, so the structure does not have to stay valid after the call.
A second call returns ``-EALREADY``.

.. _nrf71_sr_coex_ble_chan_map:

Bluetooth LE channel map
========================

When the :kconfig:option:`CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP` Kconfig option is enabled, the driver marks the Bluetooth LE data channels under the 2.4 GHz Wi-Fi channel as bad while the station is connected.
It applies the map with the ``bt_le_set_chan_map()`` function, which requires the :kconfig:option:`CONFIG_BT_CENTRAL` or :kconfig:option:`CONFIG_BT_BROADCASTER` Kconfig option.
The default map is restored when the station disconnects and when Wi-Fi goes down.

While this option is enabled, the driver owns the Bluetooth LE host channel classification and overwrites any map that the application sets.
Keep the option disabled if the application manages the channel map itself.

You can use the following Kconfig options to tune the channel map handling:

* :kconfig:option:`CONFIG_NRF71_SR_COEX_BLE_GUARD_BAND_MHZ` - Sets the guard band left on both sides of the Wi-Fi channel.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRIES` and :kconfig:option:`CONFIG_NRF71_SR_COEX_BLE_CHAN_MAP_RETRY_DELAY_MS` - Set how a channel map that the Bluetooth host rejected is retried.
* :kconfig:option:`CONFIG_NRF71_SR_COEX_BLE_WORKQ_STACK_SIZE` - Sets the stack size of the work queue that applies the channel map.

Shell commands
==============

When the :kconfig:option:`CONFIG_NRF71_SR_COEX_SHELL` Kconfig option is enabled, the following commands are available:

* ``nrf71_coex status`` - Shows the Coexistence Manager configuration state and, if enabled, the last Bluetooth LE channel map.
* ``nrf71_coex stats`` - Reads the statistics from the Coexistence Manager and prints them.

API documentation
*****************

| Header files: :ncs-file:`/include/drivers/nrf71_sr_coex/nrf71_sr_coex.h`, :ncs-file:`/include/drivers/nrf71_sr_coex/nrf71_sr_coex_sr.h`
| Source files: :file:`drivers/nrf71_sr_coex/src/`

Application interface
=====================

.. doxygengroup:: nrf71_sr_coex

Short-range driver interface
============================

.. doxygengroup:: nrf71_sr_coex_sr
