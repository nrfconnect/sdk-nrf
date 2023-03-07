.. _nrf_desktop_led_state:

LED state module
################

.. contents::
   :local:
   :depth: 2

The |led_state| is used to set LED effects based on the Bluetooth® peer state and the system state.

Controlled LEDs
***************

The module controls LEDs defined by enumerators in :c:enum:`led_id`:

* :c:enumerator:`LED_ID_SYSTEM_STATE` - Shows the system state.
  The system can be in one of the following states:

  * :c:enumerator:`LED_SYSTEM_STATE_IDLE` - Device is not being charged.
  * :c:enumerator:`LED_SYSTEM_STATE_CHARGING` - Device is being charged.
  * :c:enumerator:`LED_SYSTEM_STATE_ERROR` - Fatal application error occurred (a module reported error state or battery state error was reported).

* :c:enumerator:`LED_ID_PEER_STATE` - Shows the Bluetooth peer state.
  The Bluetooth peer can be in one of the following states:

  * :c:enumerator:`LED_PEER_STATE_DISCONNECTED` - Bluetooth peer is disconnected.
  * :c:enumerator:`LED_PEER_STATE_CONNECTED` - Bluetooth peer is connected.
  * :c:enumerator:`LED_PEER_STATE_PEER_SEARCH` - Device is looking for a peer, either by scanning or advertising.
  * :c:enumerator:`LED_PEER_STATE_CONFIRM_SELECT` - Bluetooth peer is being selected (the device is waiting for confirmation).
  * :c:enumerator:`LED_PEER_STATE_CONFIRM_ERASE` - Device is waiting for user confirmation to erase peers (for Bluetooth® Central) or start erase advertising (for Bluetooth® Peripheral).
  * :c:enumerator:`LED_PEER_STATE_ERASE_ADV` - Device is advertising for peer erase.

For the complete description of peer management, see :ref:`nrf_desktop_ble_bond`.

.. note::
   If a configuration does not support Bluetooth (:kconfig:option:`CONFIG_CAF_BLE_COMMON_EVENTS` is disabled), the Bluetooth peer state will not be notified.
   In that case, the Bluetooth peer state LED is set only once during the boot.
   The set LED effect represents :c:enumerator:`LED_PEER_STATE_DISCONNECTED` state for the default peer.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_led_state_start
    :end-before: table_led_state_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The |led_state| is enabled when you set the :kconfig:option:`CONFIG_CAF_LEDS` option.
You must also configure :ref:`caf_leds`, which is used as a sink module for ``led_state``.

For every board that has this option enabled, you must define the module configuration.
The configuration must be defined in the file named :ref:`CONFIG_DESKTOP_LED_STATE_DEF_PATH <config_desktop_app_options>` located in the board-specific directory in the application configuration directory.
By default, the file is named as :file:`led_state_def.h`.

The configuration consists of the following elements:

* ``led_map`` - Maps the :c:enum:`led_id` values to IDs used by :ref:`caf_leds`.
  If no physical LED is assigned to a :c:enum:`led_id` value, assign :c:macro:`LED_UNAVAILABLE` as the ID used by :ref:`caf_leds`.
* ``led_system_state_effect`` - Defines the LED effects used to show the system states.
  The effect must be defined for every system state.
* ``led_peer_state_effect`` - Defines the LED effects used to show the Bluetooth peer states.
  The effect must be defined for every state of every peer.

The LED effects are defined in the :file:`caf/led_effect.h` file in the common configuration directory.

LED effect API
**************

.. doxygengroup:: led_effect_CAF
   :project: nrf
   :members:
