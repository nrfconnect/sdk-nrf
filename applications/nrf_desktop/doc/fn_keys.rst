.. _nrf_desktop_fn_keys:

Function key module
####################

.. contents::
   :local:
   :depth: 2

The Function key module applies the Fn key modifier to activate special functions assigned to dual-purpose keys.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_fn_keys_start
    :end-before: table_fn_keys_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

The module uses ``button_event`` sent by :ref:`nrf_desktop_buttons`.
Make sure mentioned hardware interface is defined.

The module is enabled with :option:`CONFIG_DESKTOP_FN_KEYS_ENABLE` option.

You must configure the following options:

* :option:`CONFIG_DESKTOP_FN_KEYS_SWITCH` - Fn key button.
* :option:`CONFIG_DESKTOP_FN_KEYS_LOCK` - Fn lock button.
* :option:`CONFIG_DESKTOP_STORE_FN_LOCK` - Option for defining if the device should store the Fn lock state after reboot (set by default to storing the state).
* :option:`CONFIG_DESKTOP_FN_KEYS_MAX_ACTIVE` - Maximum number of dual-purpose keys pressed at the same time (8 by default).
  The module remembers the pressed keys to send proper key releases.

In the file :file:`fn_keys_def.h``, define all the dual-purpose keys.
The ``fn_keys`` array must be sorted by key ID (the module uses binary search).

Implementation details
**********************

The module replaces the key ID for the dual-purpose keys when the Fn modifier is active (that is, when the Fn key is pressed or the Fn lock is active, but not both).
The original :c:struct:`button_event` is consumed and the new event of the same type is submitted, but with remapped :c:member:`button_event.key_id`.
