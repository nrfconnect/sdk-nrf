.. _buttons_sim:

Button simulator module
########################

Use the ``buttons_sim`` module to generate the sequence of simulated key presses.
The time between subsequent key presses is defined as a module configuration option.
Generating keys can be started and stopped by pressing the predefined button.

Module Events
*************

+------------------+------------------+----------------+------------------+--------------------------------------+
| Source Module    | Input Event      | This Module    | Output Event     | Sink Module                          |
+==================+==================+================+==================+======================================+
| :ref:`buttons`   | ``button_event`` | ``butons_sim`` |                  |                                      |
+------------------+                  |                |                  |                                      |
| :ref:`fn_keys`   |                  |                |                  |                                      |
+------------------+                  |                |                  |                                      |
| ``buttons_sim``  |                  |                |                  |                                      |
+------------------+------------------+                +------------------+--------------------------------------+
|                  |                  |                | ``button_event`` | :ref:`fn_keys`                       |
|                  |                  |                |                  +--------------------------------------+
|                  |                  |                |                  | :ref:`motion` (``motion_buttons``)   |
|                  |                  |                |                  +--------------------------------------+
|                  |                  |                |                  | :ref:`click_detector`                |
|                  |                  |                |                  +--------------------------------------+
|                  |                  |                |                  | :ref:`passkey` (``passkey_buttons``) |
|                  |                  |                |                  +--------------------------------------+
|                  |                  |                |                  | ``buttons_sim``                      |
|                  |                  |                |                  +--------------------------------------+
|                  |                  |                |                  | :ref:`hid_state`                     |
+------------------+------------------+----------------+------------------+--------------------------------------+

Configuration
*************

To configure the ``buttons_sim`` module:

1. Enable and configure the :ref:`buttons` module. ``button_event`` is used to trigger the simulated button sequence.
#. Enable the ``buttons_sim``  module by setting the ``CONFIG_DESKTOP_BUTTONS_SIM_ENABLE`` Kconfig option.
#. Define the output key ID sequence in the :file:`buttons_sim_def.h` file located in the board-specific directory in the :file:`configuration` folder. The mapping from the defined key ID to the HID report ID and usage ID is defined in :file:`hid_keymap_def.h` (this might be different for different boards).
#. Define the interval between subsequent simulated button presses - ``CONFIG_DESKTOP_BUTTONS_SIM_INTERVAL``. One second is used by default.

If you want the sequence to automatically restart after it ends, set ``CONFIG_DESKTOP_BUTTONS_SIM_LOOP_FOREVER``.
By default, the sequence is generated only once.

Implementation details
**********************

The ``buttons_sim`` module generates button sequence using :c:type:`struct k_delayed_work`, that resubmits itself.
The work handler submits the press and the release of a single button from the sequence.

Receiving ``button_event`` with the key ID set to ``CONFIG_DESKTOP_BUTTONS_SIM_TRIGGER_KEY_ID`` either stops generating the sequence (in case it is already being generated) or starts generating it.
