.. _fn_keys:

Fn keys module
##############

The ``fn_keys`` module applies Fn key modifier to activate special functions assigned to dual-purpose keys.

Module Events
*************

+----------------+------------------+--------------+------------------+------------------------------------+
| Source Module  | Input Event      | This Module  | Output Event     | Sink Module                        |
+================+==================+==============+==================+====================================+
| :ref:`buttons` | ``button_event`` | ``fn_keys``  | ``button_event`` | :ref:`click_detector`              |
|                |                  |              |                  +------------------------------------+
|                |                  |              |                  | :ref:`motion` (``motion_buttons``) |
|                |                  |              |                  +------------------------------------+
|                |                  |              |                  | :ref:`hid_state`                   |
+----------------+------------------+--------------+------------------+------------------------------------+

Configuration
*************

The module uses ``button_event`` sent by :ref:`buttons`.
Make sure mentioned hardware interface is defined.

The module is enabled with ``CONFIG_DESKTOP_FN_KEYS_ENABLE`` option.

You must configure the following options:

* ``CONFIG_DESKTOP_FN_KEYS_SWITCH`` - Fn key button.
* ``CONFIG_DESKTOP_FN_KEYS_LOCK`` - Fn lock button.
* ``CONFIG_DESKTOP_STORE_FN_LOCK`` - option for defining if the device should store the Fn lock state after reboot (set by default to storing the state).
* ``CONFIG_DESKTOP_FN_KEYS_MAX_ACTIVE`` - maximum number of dual-purpose keys pressed at the same time (8 by default). The module remembers the pressed keys to send proper key releases.

In the file ``fn_keys_def.h``, define all the dual-purpose keys.
The ``fn_keys`` array must be sorted by key ID (the module uses binary search).

Implementation details
**********************

The module replaces the key ID for the dual-purpose keys when the Fn modifier is active (that is, when the Fn key is pressed or the Fn lock is active, but not both).
The original ``button_event`` is consumed and the new event of the same type is submitted, but with remapped :cpp:member:`key_id`.
