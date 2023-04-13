.. _nrf_desktop_ble_discovery:

Bluetooth LE discovery module
#############################

.. contents::
   :local:
   :depth: 2

Use the Bluetooth® LE discovery module to discover GATT Services and read GATT Attribute Values from an nRF Desktop peripheral.
The module is mandatory for the nRF Desktop central.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_ble_discovery_start
    :end-before: table_ble_discovery_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

Complete the following steps to configure the module:

1. Complete the basic Bluetooth configuration, as described in :ref:`nrf_desktop_bluetooth_guide`.
   Make sure that both :ref:`CONFIG_DESKTOP_ROLE_HID_DONGLE <config_desktop_app_options>` and :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` options are enabled.
   The |ble_discovery| is enabled by the :ref:`CONFIG_DESKTOP_BLE_DISCOVERY_ENABLE <config_desktop_app_options>` Kconfig option which is implied by the :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` Kconfig option together with other application modules.
   These modules are required for HID dongle that forwards the data from HID peripherals connected over Bluetooth.
#. Make sure that the :kconfig:option:`CONFIG_BT_GATT_CLIENT` Kconfig option is enabled to support the GATT Client role.
   The GATT Client role is enabled by default by the :ref:`CONFIG_DESKTOP_BT_CENTRAL <config_desktop_app_options>` Kconfig option.
#. The |ble_discovery| module uses the :ref:`gatt_dm_readme` and selects the :kconfig:option:`CONFIG_BT_GATT_DM` Kconfig option to enable the library.
#. Define the module configuration in the :file:`ble_discovery_def.h` file, located in the board-specific directory in the application configuration directory.
   You must define the following parameters for every nRF Desktop peripheral that connects with the given nRF Desktop central:

   * Parameters common for all the peripherals:

     * Vendor ID (VID)

   * Parameters defined separately for every peripheral:

     * Product ID (PID)
     * Peer type (:c:enumerator:`PEER_TYPE_MOUSE` or :c:enumerator:`PEER_TYPE_KEYBOARD`)

   For an example of the module configuration, see :file:`configuration/nrf52840dongle_nrf52840/ble_discovery_def.h`.

   .. note::
        The module configuration example uses ``0x1915`` as Nordic Semiconductor's VID.
        Make sure to change this value to the VID of your company.

        The assigned PIDs should be unique for devices with the same VID.

Implementation details
**********************

The |ble_discovery| implementation is tasked with peripheral discovery and verification.

Peripheral discovery
====================

The module starts the peripheral device discovery when it receives :c:struct:`ble_peer_event` with :c:member:`ble_peer_event.state` set to :c:enumerator:`PEER_STATE_CONNECTED`.

The peripheral discovery consists of the following steps:

1. Reading device description.
   The central checks if the connected peripheral supports the LLPM (Low Latency Packet Mode).
   The device description is a GATT Characteristic that is specific for nRF Desktop peripherals and is implemented by :ref:`nrf_desktop_dev_descr`.
#. Reading Hardware ID (HW ID).
   The HW ID is unique per device and it is used to identify the device while forwarding the :ref:`nrf_desktop_config_channel` data.
   The HW ID is a read from a GATT Characteristic that is specific for nRF Desktop peripherals and is implemented by :ref:`nrf_desktop_dev_descr`.
#. Reading DIS (Device Information Service).
   The central reads VID and PID of the connected peripheral.
#. HIDS (Human Interface Device Service) discovery.
   The central discovers HIDS and forwards the information to other application modules using ``ble_discovery_complete`` event.
   The :ref:`nrf_desktop_hid_forward` uses the event to register a new subscriber.

.. note::
   Only one peripheral can be discovered at a time.
   The nRF Desktop central will not scan for new peripherals if a peripheral discovery is in progress.

Peripheral verification
=======================

If the connected peripheral does not provide one of the required GATT Characteristics, the central disconnects.
The same actions are taken if the peripheral's VID and PID value combination is unknown to the central.

The nRF Desktop central works only with predefined subset of peripherals.
The mentioned peripherals must be described in the :file:`ble_discovery_def.h` file.
