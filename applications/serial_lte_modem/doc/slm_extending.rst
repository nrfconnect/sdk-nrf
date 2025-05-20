.. _slm_extending:

Extending the application
#########################

.. contents::
   :local:
   :depth: 2

The AT commands in the Serial LTE modem are implemented with the :ref:`at_cmd_custom_readme` library.
You can extend the Serial LTE modem application by adding your own AT commands.

Adding AT commands
******************

If you want to implement a custom AT command in SLM, you will need to follow the instructions in the :ref:`at_cmd_custom_readme` library documentation.
Setting the AT command filter and callback with the :c:macro:`AT_CMD_CUSTOM` macro is enough for a custom AT command that does not require setup or teardown.

As a preferred alternative to the :c:macro:`AT_CMD_CUSTOM` macro, SLM defines the :c:macro:`SLM_AT_CMD_CUSTOM` macro, which is a wrapper around the :c:macro:`AT_CMD_CUSTOM` macro.
The :c:macro:`SLM_AT_CMD_CUSTOM` macro pre-processes the AT command parameters for the AT command callback and sets the default ``OK`` response if the callback returns successfully.

If your custom AT command requires setup or teardown, you will need to perform the following steps:

1. Create a header file in :file:`applications/serial_lte_modem/src/`.
   The file must expose the following function declarations:

   * ``*_init()`` - Your setup function for the custom AT command.
   * ``*_uninit()`` - Your teardown function for the custom AT command.

#. Create a corresponding :file:`.c` file in :file:`applications/serial_lte_modem/src/`.
   The file must implement the following functions:

   * Your AT command callback, which is declared with the :c:macro:`AT_CMD_CUSTOM` macro.
   * ``*_init()`` - Your setup function for the custom AT command.
   * ``*_uninit()`` - Your teardown function for the custom AT command.
     Make sure that the teardown function exits successfully.
     It will be called before entering power saving states or shutting down.

#. Edit :file:`applications/serial_lte_modem/src/slm_at_commands.c` and make the following changes:

   a. Include your header file.
   #. In the ``slm_at_init()`` function, add a call to your setup function.
   #. In the ``slm_at_uninit()`` function, add a call to your teardown function.
