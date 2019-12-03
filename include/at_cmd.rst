.. _at_cmd_readme:

AT command interface
####################

The socket interface has a global limitation of 8 socket instances that can be open at the same time.
So if an application needs multiple socket protocols like TCP, UDP, TLS, AT, and so on, the available 8 socket instances become a limiting factor.
The AT command interface is designed to let multiple threads share a single AT socket instance.

The AT command interface always parses the return code from the modem.
Non-notification data such as OK, ERROR, and +CMS/+CME is removed from the string that is returned to the user.
The return codes are returned as error codes in the return code of the write functions (:cpp:type:`at_cmd_write` and :cpp:type:`at_cmd_write_with_callback`) and also through the state parameter that can be supplied.
The state parameter must be used to differentiate between +CMS and +CME errors as the error codes are overlapping.
Any subsequent writes from other threads are queued until all the data (return code + any payload) from the previous write is returned to the caller.
This is to make sure that the correct thread gets the correct data and return code, because it is not possible to distinguish between two separate sessions.
The kernel thread priority scheme determines the thread to get write access if multiple threads are queued.

There are two schemes by which data returned immediately from the modem (for instance, the modem response for an AT+CNUM command) is delivered to the user.
The user can call the write function by submitting either of the following input parameters in the write function:

* A reference to a string buffer
* A reference to a handler function

In the case of a string buffer, the AT command interface removes the return code and then delivers the rest of the string in the buffer if the buffer is large enough.
Data is returned to the user if the return code is OK.

In the case of a handler function, the return code is removed and the rest of the string is delivered to the handler function through a char pointer parameter.
Allocation and deallocation of the buffer is handled by the AT command interface, and the content should not be considered valid outside of the handler.

Both schemes are limited to the maximum reception size defined by :option:`CONFIG_AT_CMD_RESPONSE_MAX_LEN`.

Notifications are always handled by a callback function.
This callback function is separate from the one that is used to handle data returned immediately after sending a command.
This callback is set by :cpp:type:`at_cmd_set_notification_handler`.

API documentation
*****************

| Header file: :file:`include/at_cmd.h`
| Source file: :file:`lib/at_cmd/at_cmd.c`

.. doxygengroup:: at_cmd
   :project: nrf
   :members:
