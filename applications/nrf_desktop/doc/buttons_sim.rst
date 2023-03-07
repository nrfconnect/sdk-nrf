.. _nrf_desktop_buttons_sim:

Button simulator module
#######################

.. contents::
   :local:
   :depth: 2

Use the |button_sim| to generate the sequence of simulated key presses.
The time between subsequent key presses is defined as a module configuration option.
Generating keys can be started and stopped by pressing the predefined button.

Module events
*************

.. include:: event_propagation.rst
    :start-after: table_buttons_sim_start
    :end-before: table_buttons_sim_end

.. note::
    |nrf_desktop_module_event_note|

Configuration
*************

To configure the |button_sim|:

1. Enable and configure the :ref:`caf_buttons`.
   :c:struct:`button_event` is used to trigger the simulated button sequence.
#. Enable the ``buttons_sim`` module by setting the :ref:`CONFIG_DESKTOP_BUTTONS_SIM_ENABLE <config_desktop_app_options>` Kconfig option.
#. Define the output key ID sequence in the :file:`buttons_sim_def.h` file located in the board-specific directory in the :file:`configuration` directory.
   The mapping from the defined key ID to the HID report ID and usage ID is defined in :file:`hid_keymap_def.h` (this might be different for different boards).
#. Define the interval between subsequent simulated button presses (:ref:`CONFIG_DESKTOP_BUTTONS_SIM_INTERVAL <config_desktop_app_options>`).
   One second is used by default.

If you want the sequence to automatically restart after it ends, set :ref:`CONFIG_DESKTOP_BUTTONS_SIM_LOOP_FOREVER <config_desktop_app_options>`.
By default, the sequence is generated only once.

Implementation details
**********************

The |button_sim| generates button sequence using :c:struct:`k_work_delayable`, which resubmits itself.
The work handler submits the press and the release of a single button from the sequence.

Receiving :c:struct:`button_event` with the key ID set to :ref:`CONFIG_DESKTOP_BUTTONS_SIM_TRIGGER_KEY_ID <config_desktop_app_options>` either stops generating the sequence (in case it is already being generated) or starts generating it.
