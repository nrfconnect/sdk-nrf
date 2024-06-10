.. _at_cmd_custom_readme:

Custom AT commands
##################

.. contents::
   :local:
   :depth: 2

The custom AT commands library allows any part of the application to register custom AT commands that are handled in the application.
The list of custom AT commands are passed to the Modem library. This allows the application to add custom AT commands and to override modem AT calls with application callbacks while using the standard AT API of the Modem library.

Configuration
*************

You can enable the custom AT commands library by setting the :kconfig:option:`CONFIG_AT_CMD_CUSTOM` Kconfig option.

Usage
*****

To add a custom AT command or overwrite an existing command, you must add the AT commands filter and callback with the :c:macro:`AT_CMD_CUSTOM` macro in the :file:`at_cmd_custom.h` file.
If the filter matches the first characters of the AT command that is sent to the :c:func:`nrf_modem_at_cmd` function, the AT command is passed to the filter callback instead of to the modem.
When returning from the callback, the content of the provided buffer is treated as the modem response and is returned to the ``nrf_modem_at_`` API that sent the command.

.. note::
   When the custom AT commands library is enabled, the application must not call the :c:func:`nrf_modem_at_cmd_custom_set` function because it overrides the handler set by the library.
   Instead, the application must add the AT filters using the :c:macro:`AT_CMD_CUSTOM` macro.

.. note::
   The custom AT command filter is compared against the start of the AT command string sent to the Modem library.
   Therefore, you must make sure to remove any leading whitespace characters in the AT command string before calling the :c:func:`nrf_modem_at_cmd` function.

Adding a custom command
=======================

The application can define a custom command to receive a callback using the :c:macro:`AT_CMD_CUSTOM` macro.
A custom AT command has a name, a filter string, a callback function, and a state.
Multiple parts of the application can define their own custom commands.
However, there can be only one callback per filter match.
The filter uses the callback of the first match found in the list of custom commands.
Hence, make sure to keep the filters accurate to avoid conflicts with other filters.

.. note::
   The custom commands only trigger on calls to the :c:func:`nrf_modem_at_cmd` function.
   Calls to :c:func:`nrf_modem_at_printf`, :c:func:`nrf_modem_at_cmd_async`, and :c:func:`nrf_modem_at_scanf` are forwarded directly to the modem.

The following code snippet shows how to add a custom command that triggers on ``+MYCOMMAND`` calls to the :c:func:`nrf_modem_at_cmd` function:

.. code-block:: c

   /* Callback for +MYCOMMAND calls */
   AT_CMD_CUSTOM(my_command_filter, "AT+MYCOMMAND", my_command_callback);

	int my_command_callback(char *buf, size_t len, char *at_cmd);
	{
		printf("Callback for %s", at_cmd);
		return at_cmd_custom_respond(buf, len, "OK\r\n");
	}

AT command responses for custom commands
========================================

When returning from the callback, the content of the provided :c:var:`buf` buffer is treated as the modem response by the Modem library.
Hence, the following response format must match that of the modem:

* The successful responses end with ``OK\r\n``.
* For error response, use ``ERROR\r\n``, ``+CME ERROR: <errorcode>``, or ``+CMS ERROR: <errorcode>`` depending on the error.

To simplify filling the response buffer, you can use the :c:func:`at_cmd_custom_respond` function.
This allows formatting arguments and ensures that the response does not overflow the response buffer.

The following code snippet shows how responses can be added to the ``+MYCOMMAND`` AT command.

.. code-block:: c

	/* Callback for +MYCOMMAND calls */
	AT_CMD_CUSTOM(my_command_filter, "AT+MYCOMMAND", my_command_callback);

	int my_command_callback(char *buf, size_t len, char *at_cmd);
	{
		/* test */
		if(strncmp("AT+MYCOMMAND=?", at_cmd, strlen("AT+MYCOMMAND=?")) == 0) {
			return at_cmd_custom_respond(buf, len, "+MYCOMMAND: (%d, %d)\r\nOK\r\n", 0, 1);
		}
		/* set */
		if(strncmp("AT+MYCOMMAND=", at_cmd, strlen("AT+MYCOMMAND=")) == 0) {
			return at_cmd_custom_respond(buf, len, "OK\r\n");
		}
		/* read */
		if(strncmp("AT+MYCOMMAND?", at_cmd, strlen("AT+MYCOMMAND?")) == 0) {
			return at_cmd_custom_respond(buf, len, "+CME ERROR: %d\r\n", 1);
		}
	}

API documentation
*****************

| Header file: :file:`include/modem/at_cmd_custom.h`
| Source file: :file:`lib/at_cmd_custom/src/at_cmd_custom.c`

.. doxygengroup:: at_cmd_custom
   :project: nrf
   :members:
