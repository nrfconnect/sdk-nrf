.. _nrf_desktop_keys_state:

Keys state utility
##################

.. contents::
   :local:
   :depth: 2

An application module uses keys state utility to track state of active keys.
The utility collects information about key state changes (press or release) and maintains state of all active keys.
The state of all active keys can be retrieved by the application module.

Configuration
*************

Use the :option:`CONFIG_DESKTOP_KEYS_STATE` Kconfig option to enable the utility.
You can change the maximum number of tracked keys that can be simultaneously active using the :option:`CONFIG_DESKTOP_KEYS_STATE_KEY_CNT_MAX` Kconfig option.

See Kconfig help for more details.

Using keys state
****************

An application module that handles keypresses can use this utility.

Initialization
==============

Initialize a utility instance before tracking active keys, using the :c:func:`keys_state_init` function.
The maximum number of active keys specified through the function must be lower than or equal to the limit specified through :option:`CONFIG_DESKTOP_KEYS_STATE_KEY_CNT_MAX` Kconfig option.

Updating keys state
===================

You can use the :c:func:`keys_state_key_update` function to notify keys state about a key press or release.
A key ID is used to identify the button related to the keypress.
The ID could be, for example, an application-specific identifier of a hardware button or HID usage ID.

You can use the :c:func:`keys_state_clear` function to clear keys state.
Clearing keys state drops all of the tracked active key presses.

Getting keys state
==================

You can use the :c:func:`keys_state_keys_get` function to get keys state.

API documentation
*****************

Application modules can use the following API of the keys state utility:

| Header file: :file:`applications/nrf_desktop/src/util/keys_state.h`
| Source file: :file:`applications/nrf_desktop/src/util/keys_state.c`

.. doxygengroup:: keys_state
