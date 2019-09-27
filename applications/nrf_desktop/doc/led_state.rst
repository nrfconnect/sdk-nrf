.. _led_state:

LED state module
################

The ``led_state`` module is used to set LED effects based on the Bluetooth peer state and the system state.

Controlled LEDs
***************

The module controls LEDs defined by enumerators in :cpp:enum:`led_id`:

* :cpp:enumerator:`LED_ID_SYSTEM_STATE` - shows system state. The system can be in one of the following states:

  * :cpp:enumerator:`LED_SYSTEM_STATE_IDLE` - the device is not being charged,
  * :cpp:enumerator:`LED_SYSTEM_STATE_CHARGING` - the device is being charged,
  * :cpp:enumerator:`LED_SYSTEM_STATE_ERROR` - fatal application error occurred (a module reported error state or battery state error was reported).

* :cpp:enumerator:`LED_ID_PEER_STATE` - shows Bluetooth peer state. The Bluetooth peer can be in one of the following states:

  * :cpp:enumerator:`LED_PEER_STATE_DISCONNECTED` - the Bluetooth peer is disconnected,
  * :cpp:enumerator:`LED_PEER_STATE_CONNECTED` - the Bluetooth peer is connected,
  * :cpp:enumerator:`LED_PEER_STATE_CONFIRM_SELECT` - the Bluetooth peer is being selected (the device is waiting for confirmation),
  * :cpp:enumerator:`LED_PEER_STATE_CONFIRM_ERASE` - the device is waiting for user confirmation to erase peers (for Bluetooth central) or start erase advertising (for Bluetooth peripheral),
  * :cpp:enumerator:`LED_PEER_STATE_ERASE_ADV` - the device is advertising for peer erase.

For the complete description of peer management, see :ref:`ble_bond`.

Module Events
*************

+------------------------+------------------------------+---------------+----------------+-------------+
| Source Module          | Input Event                  | This Module   | Output Event   | Sink Module |
+========================+==============================+===============+================+=============+
| :ref:`ble_bond`        | ``ble_peer_operation_event`` | ``led_state`` | ``led_event``  | :ref:`leds` |
+------------------------+------------------------------+               |                |             |
| :ref:`ble_state`       | ``ble_peer_event``           |               |                |             |
+------------------------+------------------------------+               |                |             |
| :ref:`battery_charger` | ``battery_state_event``      |               |                |             |
+------------------------+------------------------------+---------------+----------------+-------------+

Configuration
*************

The module is enabled when you set the ``CONFIG_DESKTOP_LED_ENABLE`` option.
You must also configure :ref:`leds`, which is used as sink module for ``led_state``.

For every board that has this option enabled, you must define the module configuration.
Do this in the ``led_state_def.h`` file located in the board-specific directory in the application configuration folder.

The configuration consists of the following elements:

* ``led_map`` - maps the :cpp:enum:`led_id` values to IDs used by :ref:`leds`. If no physical LED is assigned to a :cpp:enum:`led_id` value, assign :c:macro:`LED_UNAVAILABLE` as ID used by :ref:`leds`.
* ``led_system_state_effect`` - defines the LED effects used to show the system states. The effect must be defined for every system state.
* ``led_peer_state_effect`` - defines the LED effects used to show the Bluetooth peer states. The effect must be defined for every state of every peer.

The LED effects are defined in the ``led_effect.h`` file in the common configuration folder.

LED effect API
**************

.. doxygengroup:: led_effect_DESK
   :project: nrf
   :members:
