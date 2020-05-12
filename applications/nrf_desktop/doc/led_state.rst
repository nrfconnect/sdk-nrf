.. _nrf_desktop_led_state:

LED state module
################

The ``led_state`` module is used to set LED effects based on the Bluetooth peer state and the system state.

Controlled LEDs
***************

The module controls LEDs defined by enumerators in :cpp:enum:`led_id`:

* :cpp:enumerator:`LED_ID_SYSTEM_STATE` - shows system state.
  The system can be in one of the following states:

  * :cpp:enumerator:`LED_SYSTEM_STATE_IDLE` - The device is not being charged.
  * :cpp:enumerator:`LED_SYSTEM_STATE_CHARGING` - The device is being charged.
  * :cpp:enumerator:`LED_SYSTEM_STATE_ERROR` - Fatal application error occurred (a module reported error state or battery state error was reported).

* :cpp:enumerator:`LED_ID_PEER_STATE` - shows Bluetooth peer state.
  The Bluetooth peer can be in one of the following states:

  * :cpp:enumerator:`LED_PEER_STATE_DISCONNECTED` - The Bluetooth peer is disconnected.
  * :cpp:enumerator:`LED_PEER_STATE_CONNECTED` - The Bluetooth peer is connected.
  * :cpp:enumerator:`LED_PEER_STATE_PEER_SEARCH` - The device is looking for a peer, either by scanning or advertising.
  * :cpp:enumerator:`LED_PEER_STATE_CONFIRM_SELECT` - The Bluetooth peer is being selected (the device is waiting for confirmation).
  * :cpp:enumerator:`LED_PEER_STATE_CONFIRM_ERASE` - The device is waiting for user confirmation to erase peers (for Bluetooth central) or start erase advertising (for Bluetooth peripheral).
  * :cpp:enumerator:`LED_PEER_STATE_ERASE_ADV` - The device is advertising for peer erase.

For the complete description of peer management, see :ref:`nrf_desktop_ble_bond`.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_led_state_start
    :end-before: table_led_state_end

See the :ref:`nrf_desktop_architecture` for more information about the event-based communication in the nRF Desktop application and about how to read this table.

Configuration
*************

The module is enabled when you set the ``CONFIG_DESKTOP_LED_ENABLE`` option.
You must also configure :ref:`nrf_desktop_leds`, which is used as sink module for ``led_state``.

For every board that has this option enabled, you must define the module configuration.
Do this in the ``led_state_def.h`` file located in the board-specific directory in the application configuration folder.

The configuration consists of the following elements:

* ``led_map`` - maps the :cpp:enum:`led_id` values to IDs used by :ref:`nrf_desktop_leds`. If no physical LED is assigned to a :cpp:enum:`led_id` value, assign :c:macro:`LED_UNAVAILABLE` as ID used by :ref:`nrf_desktop_leds`.
* ``led_system_state_effect`` - defines the LED effects used to show the system states. The effect must be defined for every system state.
* ``led_peer_state_effect`` - defines the LED effects used to show the Bluetooth peer states. The effect must be defined for every state of every peer.

The LED effects are defined in the ``led_effect.h`` file in the common configuration folder.

LED effect API
**************

.. doxygengroup:: led_effect_DESK
   :project: nrf
   :members:
