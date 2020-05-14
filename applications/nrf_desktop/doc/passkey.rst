.. _nrf_desktop_passkey:

Passkey module
##############

Use the ``passkey`` module to handle numeric passkey input.
The input handling is based on button presses.

Module Events
*************

.. include:: event_propagation.rst
    :start-after: table_passkey_start
    :end-before: table_passkey_end

See the :ref:`nrf_desktop_architecture` for more information about the event-based communication in the nRF Desktop application and about how to read this table.

Configuration
*************

The passkey input handling is based on ``button_event``.

To configure the ``passkey`` module, complete the following steps:

1. Enable and configure the :ref:`nrf_desktop_buttons` module.
#. Enable the ``passkey`` module by using the ``CONFIG_DESKTOP_PASSKEY_BUTTONS`` Kconfig option.
#. Define the maximum number of digits in the passkey by using ``CONFIG_DESKTOP_PASSKEY_MAX_LEN`` option.
#. Define the IDs of the keys used by the ``passkey`` module in the :file:`passkey_buttons_def.h` file located in the board-specific directory in the :file:`configuration` folder.
   You must define the IDs of the following keys:

    * Keys used to input the digits.
    * Keys used to confirm the input.
    * Keys used to remove the last digit (short press removes only the last digit, but if the key is held for longer than 2 seconds, the whole input is cleared).

   You can define multiple sets of keys used to input the digits, but every set should contain keys used to input every digit.
   This is to ensure that the user will be able to input the passkey.
   The index of key ID in the input configuration array represents the digit.

The example configuration of the module can be found in the :file:`configuration/nrf52kbd_nrf52832/passkey_buttons_def.h` file.

Implementation details
**********************

The ``passkey`` module reacts to ``passkey_req_event`` by switching between the idle and the active state.

The user input is handled only in the active state.
Key presses are remembered in an array.
When the input is confirmed, the ``passkey_input_event`` that contains the user input is submitted and the module switches to the idle state.
