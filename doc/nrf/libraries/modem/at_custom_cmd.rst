.. _at_custom_cmd_readme:

Custom AT commands
##################

.. contents::
   :local:
   :depth: 2

The custom AT commands library allows any part of the application to register custom AT commands that are handled in the application.
The list of custom AT commands are passed to the Modem library. This allows the application to add custom AT commands and to override modem AT calls with application callbacks while using the standard AT API of the Modem library.

Configuration
*************

You can enable the custom AT commands library by setting the :kconfig:option:`CONFIG_AT_CUSTOM_CMD` Kconfig option.

Usage
*****

To add a custom AT command or overwrite an existing command, you must add the AT commands filter and callback with the :c:macro:`AT_CUSTOM_CMD` macro in the :file:`at_custom_cmd.h` file.
If the filter matches the first characters of the AT command that is sent to the :c:func:`nrf_modem_at_cmd` function, the AT command is passed to the filter callback instead of to the modem.
When returning from the callback, the content of the provided buffer is treated as the modem response and is returned to the ``nrf_modem_at_`` API that sent the command.

.. note::
   When the custom AT commands library is enabled, the application must not call the :c:func:`nrf_modem_at_cmd_filter_set` function because it overrides the handler set by the library.
   Instead, the application must add the AT filters using the :c:macro:`AT_CUSTOM_CMD` macro.

Adding a custom command
=======================

The application can define an custom command to receive a callback using the :c:macro:`AT_CUSTOM_CMD` macro.
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

   /* AT filter callback for +MYCOMMAND calls */
   AT_CUSTOM_CMD(my_command_filter, "AT+MYCOMMAND", my_command_callback);

	int my_command_callback(char *buf, size_t len, char *at_cmd);
	{
		printf("Callback for %s", at_cmd);
		return at_custom_cmd_respond(buf, len, "OK\r\n");
	}

AT command responses for custom commands
========================================

When returning from the callback, the content of the provided :c:var:`buf` buffer is treated as the modem response by the Modem library.
Hence, the following response format must match that of the modem:

* The successful responses end with ``OK\r\n``.
* For error response, use ``ERROR\r\n``, ``+CME ERROR: <errorcode>``, or ``+CMS ERROR: <errorcode>`` depending on the error.

To simplify filling the response buffer, you can use the :c:func:`at_custom_cmd_respond` function.
This allows formatting arguments and ensures that the response does not overflow the response buffer.

The following code snippet shows how responses can be added to the ``+MYCOMMAND`` AT command.

.. code-block:: c

	/* AT filter callback for +MYCOMMAND calls */
	AT_CUSTOM_CMD(my_command_filter, "AT+MYCOMMAND", my_command_callback);

	int my_command_callback(char *buf, size_t len, char *at_cmd);
	{
		/* test */
		if(strncmp("AT+MYCOMMAND=?", at_cmd, strlen("AT+MYCOMMAND=?")) == 0) {
			return at_custom_cmd_respond(buf, len, "+MYCOMMAND: (%d, %d)\r\nOK\r\n", 0, 1);
		}
		/* set */
		if(strncmp("AT+MYCOMMAND=", at_cmd, strlen("AT+MYCOMMAND=")) == 0) {
			return at_custom_cmd_respond(buf, len, "OK\r\n");
		}
		/* read */
		if(strncmp("AT+MYCOMMAND?", at_cmd, strlen("AT+MYCOMMAND?")) == 0) {
			return at_custom_cmd_respond(buf, len, "+CME ERROR: %d\r\n", 1);
		}
	}


Pausing and resuming
====================

A custom AT command is active by default.
A custom AT command can be paused and resumed with the :c:func:`at_custom_cmd_pause` and :c:func:`at_custom_cmd_resume` functions, respectively.
You can pause a custom command at declaration by appending :c:macro:`AT_CUSTOM_CMD_PAUSED` to the filter definition.

The following code snippet shows how to resume a custom command that is paused by default:

.. code-block:: c

	/* AT filter callback for +MYCOMMAND calls */
	AT_CUSTOM_CMD(my_command_filter, "AT+MYCOMMAND", my_command_callback, AT_CUSTOM_CMD_PAUSED);

	int resume_my_command_filter(void)
	{
		/* resume the filter */
		at_custom_cmd_resume(&my_command_filter);
	}

	int my_command_callback(char *buf, size_t len, char *at_cmd);
	{
		return at_custom_cmd_respond(buf, len "OK\r\n");
	}


API documentation
*****************

| Header file: :file:`include/modem/at_custom_cmd.h`
| Source file: :file:`lib/at_custom_cmd/src/at_custom_cmd.c`

.. doxygengroup:: at_custom_cmd
   :project: nrf
   :members:
