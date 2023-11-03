.. _nrf_modem_at:

AT interface
############

.. contents::
   :local:
   :depth: 2

The Modem library supports sending AT commands to the modem, reading responses, and receiving AT notifications using the :ref:`nrf_modem_at_api` found in :file:`nrfxlib/nrf_modem/include/nrf_modem_at.h`.

AT commands
***********

AT commands are essentially a set of modem instructions that are used to configure the modem, establish a network connection (or in general, execute operations), and retrieve modem and connection status information.
For the full set of supported AT commands, see the `nRF91x1 AT Commands Reference Guide`_  or `nRF9160 AT Commands Reference Guide`_ depending on the SiP you are using.

Initialization
**************

The AT interface can be used once the application initializes the Modem library in :c:enum:`NORMAL_MODE` by calling :c:func:`nrf_modem_init`.

The API and the C library
*************************

The AT interface is modelled after the :c:func:`printf` and :c:func:`scanf`  C library functions, which are two powerful functions to print output and scan input according to a specified format.
Internally, the AT interface implementation uses the :c:func:`vsnprintf` and :c:func:`vsscanf` functions from the standard C library, and therefore, these functions affect the behavior of this API.

.. note::

   The Modem library requires a C library implementation that provides both :c:func:`vsnprintf` and :c:func:`vsscanf`.

Depending on the implementation of the C library that is built with the application, some format specifiers and length modifiers might not be supported or might not behave as expected when using the AT interface functions.
This might be the case for format specifiers or length modifiers introduced in newer version of the C standard, as is the case for the ``hh``, ``ll``, ``j``, ``z`` and ``t`` length modifiers introduced in C99.

Newlibc is a common C library for embedded systems that can be used in |NCS|.
It is available in two versions, nano and vanilla, the former having a smaller feature set and reduced ROM footprint.
Generally, the vanilla version will support more format and length modifiers than the nano version.
Keep in mind that the Newlibc C library implementation is provided by the compiler toolchain, and the choice of a toolchain can indirectly affect the functionality of the C library.
Between the nano and the vanilla Newlibc C library versions, only the latter is known to support C99 length specifiers when using Zephyr's sdk-ng toolchain or the third-party GNU Arm Embedded Toolchain gnuarmemb.

Sending AT commands
*******************

The application can use :c:func:`nrf_modem_at_printf` to send a formatted AT command to the modem when it is interested only in the result of the operation, for example, ``OK``, ``ERROR``, ``+CME ERROR`` or ``+CMS ERROR``.
This function works similarly to :c:func:`printf` from the standard C library, and it supports all the format specifiers supported by the :c:func:`printf` implementation of the selected C library.

The following snippet shows how to use :c:func:`nrf_modem_at_printf` to send a formatted AT command to the modem and check the result of the operation:

.. code-block:: c

	int cfun_control(int mode)
	{
		int err;

		err = nrf_modem_at_printf("AT+CFUN=%d", mode);
		if (err = 0) {
			/* OK, success */
		} else if (err < 0) {
			/* Failed to send command, err is an nrf_errno */
		} else if (err > 0) {
			/* Command was sent, but response is not "OK" */
			switch(nrf_modem_at_err_type(err)) {
			case NRF_MODEM_AT_ERROR:
				/* Modem returned "ERROR" */
				printf("error");
				break;
			case NRF_MODEM_AT_CME_ERROR:
				/* Modem returned "+CME ERROR" */
				printf("cme error: %d", nrf_modem_at_err(err));
				break;
			case NRF_MODEM_AT_CMS_ERROR:
				/* Modem returned "+CMS ERROR" */
				printf("cms error: %d", nrf_modem_at_err(err));
				break;
			}
		}
		return err;
	}

	int foo(void)
	{
		/* Send AT+CFUN=1 */
		cfun_control(1);
		/* Send AT+CFUN=4 */
		cfun_control(4);
	}

Any return value other than zero indicates an error.
Negative values indicate that the Modem library has failed to send the AT command, and they represent an ``nrf_errno`` code that indicates the reason for the failure.
Positive values indicate that the modem has received the AT command and has responded with an error.
When a positive value is returned, the error type can be retrieved using the :c:func:`nrf_modem_at_err_type` helper function, and the error value (in case of CME or CMS errors) can be retrieved with the :c:func:`nrf_modem_at_err` helper function.

When possible, send unformatted AT commands instead of formatting the whole command as a string.
Avoiding formatting reduces the stack requirements for the call.

.. code-block:: c

	nrf_modem_at_printf("AT");			/* sends "AT", low stack usage */
	nrf_modem_at_printf("%s", "AT");	/* sends "AT", high stack usage */

	char buf[] = "AT";
	nrf_modem_at_printf(buf);			/* sends "AT", low stack usage */
	nrf_modem_at_printf("%s", buf);		/* sends "AT", high stack usage */

.. note::
   The application must use escape characters in AT commands as it would when formatting it using :c:func:`printf`.
   For example, the ``%`` character must be used with the escape character as ``%%``.

Reading data from an AT response
********************************

Use :c:func:`nrf_modem_at_scanf` to send an AT command to the modem and parse the response according to a specified format.
This function works similarly to :c:func:`scanf` from the standard C library, and it supports all the format specifiers supported by the :c:func:`scanf` implementation of the selected C library.

The following snippet shows how to use :c:func:`nrf_modem_at_scanf` to read the modem network registration status using ``AT+CEREG?``

.. code-block:: c

	void cereg_read(void)
	{
		int rc;
		int status;

		/* The `*` sub-specifier discards the result of the match.
		 * The data is read but it is not stored in any argument.
		 */
		rc = nrf_modem_at_scanf("AT+CEREG?", "+CEREG: %*d,%d", &status);

		/* Upon returning, `rc` contains the number of matches */
		if (rc == 1) {
			/* We have matched one argument */
			printf("Network registration status: %d\n", status);
		} else {
			/* No arguments where matched */
		}
	}

.. note::
   The :c:func:`nrf_modem_at_scanf` function has a stack usage of at least 512 bytes, which increases, like for all functions, with the number of arguments passed to the function.
   The actual stack usage depends on the :c:func:`vsscanf` implementation found in the C library that is compiled with the application.
   If the stack requirements for this function cannot be met by the calling thread, the application can instead call :c:func:`nrf_modem_at_cmd` and parse the response manually.

Sending an AT command and reading the response
**********************************************

The application can use :c:func:`nrf_modem_at_cmd` to send a formatted AT command to the modem and copy the AT response into the buffer that is supplied to the function.
The application can then parse the buffer as necessary, for example, by using the C library function :c:func:`sscanf`, thus achieving the combined functionality of :c:func:`nrf_modem_at_printf` and :c:func:`nrf_modem_at_scanf`.
Alternatively, the application can parse the response in any other way, as necessary.

This function works similarly to :c:func:`printf` from the standard C library, and it supports all the format specifiers supported by the :c:func:`printf` implementation of the selected C library.
The following snippet shows how to use the :c:func:`nrf_modem_at_cmd` function to change the function mode by using the ``AT+CFUN`` command and read the modem response:

.. code-block:: c

	void foo(void)
	{
		int err;
		char response[64];

		err = nrf_modem_at_cmd(response, sizeof(response), "AT+CFUN=%d", 1);
		if (err) {
			/* error */
		}

		/* buffer contains the whole response */
		printf("Modem response:\n%s", response);
	}

The application can use :c:func:`nrf_modem_at_cmd_async` to send a formatted AT command and receive the whole response asynchronously through the provided callback function.
Only one asynchronous command can be pending at any time.

The following snippet shows how to use the :c:func:`nrf_modem_at_cmd_async` function to change the function mode by using the ``AT+CFUN`` command and read the modem response:

.. code-block:: c

	void resp_callback(const char *at_response)
	{
		printf("AT response received:\n%s", at_response);
	}

	void foo(void)
	{
		int err;

		err = nrf_modem_at_cmd_async(resp_callback, "AT+CFUN=%d", 1);
		if (err) {
			/* error */
		}
	}

.. note::
   The callback function is executed in an interrupt service routine.
   The user is responsible for rescheduling any processing of the response as appropriate.

   When there is a pending response, all other functions belonging to the AT API will block until the response is received in the callback function.

.. note::
   The application must use escape characters in AT commands as it would when formatting it using :c:func:`printf`.
   For example, the ``%`` character must be used with the escape character as ``%%``.

Differences with nrf_modem_at_printf
====================================

Both functions can be used to send a formatted AT command to the modem, the main difference is how the AT response is handled.
The :c:func:`nrf_modem_at_cmd` function parses the modem AT response and returns an error accordingly.
In addition, it copies the whole modem AT response to the supplied buffer.
The :c:func:`nrf_modem_at_printf` function parses the modem AT response and returns an error accordingly.
However, the function does not make a copy of the AT response.

The application can use :c:func:`nrf_modem_at_printf` if it requires the result of the AT command (for example, ``OK`` or ``ERROR``) and :c:func:`nrf_modem_at_cmd` (or :c:func:`nrf_modem_at_scanf`) if it requires the contents of the AT response.

Differences with nrf_modem_at_scanf
===================================

The application can use :c:func:`nrf_modem_at_scanf` when it is convenient to parse the modem response based on a :c:func:`scanf` format.
In this case, the application need not provide any intermediate buffers and can instead parse the response directly into the provided arguments, thus avoiding any extra copy operations.

Conversely, :c:func:`nrf_modem_at_cmd` is the only function in the AT interface that copies the whole response of the modem from the shared memory into the provided input buffer, which is owned by the application.
Therefore, this function can be used when the application needs the whole AT command response, as received from the modem, or in those cases when the stack requirements of :c:func:`nrf_modem_at_scanf` are too high for the calling thread, or when parsing the response using a :c:func:`scanf` format is hard.

Custom AT commands
******************

The Modem library allows the application to implement custom AT commands.
When an AT command is sent by the application using the :c:func:`nrf_modem_at_cmd` function, if it matches any of the custom AT commands set by the application, the AT command is sent to a user-provided callback function instead of being sent to the modem.
The application can set a list of custom AT commands by calling the :c:func:`nrf_modem_at_cmd_custom_set` function with a list of custom commands defined in the :c:struct:`nrf_modem_at_cmd_custom` structure.
Only one list of custom commands can be registered with the Modem library.

When the callback function responds, the Modem library treats the contents of the provided :c:var:`buf` buffer as the modem response.
The following is the response format that must be the same as the modem's:

* Successful responses end with ``OK\r\n``.
* For error response, use ``ERROR\r\n``, ``+CME ERROR: <errorcode>``, or ``+CMS ERROR: <errorcode>`` depending on the error.

The following snippet shows how to set up and use a custom AT command:

.. code-block:: c

	#define AT_CMD_MAX_ARRAY_SIZE 32

	int my_at_cmd(char *buf, size_t len, char *at_cmd);
	{
		printf("Received +MYCOMMAND call: %s", at_cmd);

		/* Fill response buffer. */
		snprintf(buf, len, "+MYCOMMAND: %d\r\nOK\r\n", 1);

		return 0;
	}

	static struct nrf_modem_at_cmd_custom custom_at_cmds[] = {
		{ .cmd = "AT+MYCOMMAND", .callback = my_command_callback }
	};

	int foo(void)
	{
		int err;

		err = nrf_modem_at_cmd_custom_set(custom_at_cmds, 1);
		if (err) {
			/* error */
		}

		return 0;
	}

	void bar(void)
	{
		int err;
		char buf[AT_CMD_MAX_ARRAY_SIZE];

		err = nrf_modem_at_cmd(buf, sizeof(buf), "AT+MYCOMMAND=%d", 0);
		if (err) {
			/* error */
			return;
		}

		printf("Received AT response: %s", buf);
	}

.. note::
   The filter uses the callback of the first match found in the filter list.
   Hence, make sure to keep the filters accurately or order them accordingly.

Receiving AT notifications
**************************

The Modem library can dispatch incoming AT notifications from the modem to a user-provided callback function set by :c:func:`nrf_modem_at_notif_handler_set`.
Only one callback function can be registered with the Modem library.
Registering a new callback function will override any callback previously set.
The callback function can be unset by setting ``NULL`` as the callback.
If multiple parts of your application need to receive AT notifications, you must dispatch them from the callback function that you registered.

The following snippet shows how to setup an AT notification handler:

.. code-block:: c

	void notif_callback(const char *at_notification)
	{
		printf("AT notification received: %s\n", at_notification);
	}

	int foo(void)
	{
		int err;

		err = nrf_modem_at_notif_handler_set(notif_callback);
		if (err) {
			/* error */
		}

		return 0;
	}

The callback is invoked in an interrupt context.
The user is responsible for rescheduling the processing of AT notifications as appropriate.

In |NCS|, the :ref:`at_monitor_readme` library takes care of dispatching notifications to different parts of the application.

.. important::
   In |NCS| applications, many libraries use the :ref:`at_monitor_readme` library to register their own callback with the Modem library using the :c:func:`nrf_modem_at_notif_handler_set` function.
   If you are building an |NCS| application, do not use the :c:func:`nrf_modem_at_notif_handler_set` function to register your callback.
   Instead, use the :ref:`at_monitor_readme` library to dispatch AT notifications to where you need them in your application, and to ensure compatibility with other |NCS| libraries.
   The :ref:`at_monitor_readme` library also takes care of rescheduling the notifications to a thread context.

Thread safety
*************

The AT API is thread safe and can be used by multiple threads.
