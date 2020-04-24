.. _nrf_desktop_ble_discovery:

Bluetooth LE discovery module
#############################

Use the Bluetooth LE discovery module to discover GATT Services and read GATT Attribute Values from an nRF Desktop peripheral.
The module is mandatory for the nRF Desktop central.

Module Events
*************

+------------------------------+----------------------------------+-------------------+----------------------------------+--------------------------------+
| Source Module                | Input Event                      | This Module       | Output Event                     | Sink Module                    |
+==============================+==================================+===================+==================================+================================+
| :ref:`nrf_desktop_ble_state` | ``ble_peer_event``               | ``ble_discovery`` |                                  |                                |
+------------------------------+----------------------------------+                   |                                  |                                |
| ``ble_discovery``            | ``ble_discovery_complete_event`` |                   |                                  |                                |
+------------------------------+----------------------------------+                   +----------------------------------+--------------------------------+
|                              |                                  |                   | ``ble_discovery_complete_event`` | :ref:`nrf_desktop_hid_forward` |
|                              |                                  |                   |                                  +--------------------------------+
|                              |                                  |                   |                                  | :ref:`nrf_desktop_ble_scan`    |
|                              |                                  |                   |                                  +--------------------------------+
|                              |                                  |                   |                                  | ``ble_discovery``              |
+------------------------------+----------------------------------+-------------------+----------------------------------+--------------------------------+

Configuration
*************

Complete the following steps to configure the module:

1. Complete the basic Bluetooth configuration, as described in the Bluetooth guide.
#. Set the :option:`CONFIG_BT_GATT_CLIENT` Kconfig option to enable support for the GATT Client role.
#. Set the :option:`CONFIG_BT_GATT_DM` Kconfig option to enable the :ref:`gatt_dm_readme`.
   The :ref:`gatt_dm_readme` is used by the ``ble_discovery`` application module.
#. Define the module configuration in the ``ble_discovery_def.h`` file, located in the board-specific directory in the application configuration folder.
   You must define the following parameters for every nRF Desktop peripheral that connects with the given nRF Desktop central:

      * Parameters common for all the peripherals:

            * Vendor ID (VID)

      * Parameters defined separately for every peripheral:

            * Product ID (PID)
            * Peer type (:cpp:enum:`PEER_TYPE_MOUSE` or :cpp:enum:`PEER_TYPE_KEYBOARD`)

   For an example of the module configuration, see :file:`configuration/nrf52840dongle_nrf52840/ble_discovery_def.h`.

   .. note::
        The module configuration example uses ``0x1915`` as Nordic Semiconductor's VID.
        Make sure to change this value to the VID of your company.

        The assigned PIDs should be unique for devices with the same VID.

#. Set the ``CONFIG_DESKTOP_BLE_DISCOVERY_ENABLE`` Kconfig option to enable the ``ble_discovery`` application module.

Implementation details
**********************

Peripheral discovery
   The module starts the peripheral device discovery when it receives ``ble_peer_event`` with :cpp:member:`state` set to :cpp:enum:`PEER_STATE_SECURED`.

   The peripheral discovery consists of the following steps:

   * Reading device description.
      The central checks if the connected peripheral supports the LLPM (Low Latency Packet Mode).
      The device description is a GATT Service that is specific for nRF Desktop peripherals and is implemented by :ref:`nrf_desktop_dev_descr`.
   * Reading DIS (Device Information Service).
      The central reads VID and PID of the connected peripheral.
   * HIDS (Human Interface Device Service) discovery.
      The central discovers HIDS and forwards the information to other application modules using ``ble_discovery_complete`` event.
      The :ref:`nrf_desktop_hid_forward` uses the event to register a new subsriber.

.. note::
   Only one peripheral can be discovered at a time.
   The nRF Desktop central will not scan for new peripherals if a peripheral discovery is in progress.

Peripheral verification
   If the connected peripheral does not provide one of the required GATT Characteristics, the central unpairs the peripheral and disconnects.
   The same actions are taken if the peripheral's VID and PID value combination is unknown to the central.

   The nRF Desktop central works only with predefined subset of peripherals.
   The mentioned peripherals must be described in the :file:`ble_discovery_def.h` file.
