.. _at_cmd_readme:

AT command interface
####################

The socket interface has a global limitation of 8 socket instances that can be
open at the same time. So if an application need multiple socket protocols TCP,
UDP, TLS, AT, etc, the available 8 socket instances will be a limiting factor.
The AT command interface driver is designed to let multiple "clients", ie.
threads, software modules/components to share one AT socket instance.

The AT command driver will always parse the return code, if any (non
notification data) from the modem, ie. OK, ERROR, +CMS/+CME codes and remove
it from the string returned to the user. The return codes are returned as error
codes in the return code of the write function and also through the state
parameter that can be supplied. The state parameter must be used to
differentiate between +CMS and +CME errors as the error codes is overlapping.
Any subsequent write from other threads will be queued until all data (return
code + any payload) from the previous write is returned to the caller. This is
to make sure that the correct thread gets the correct data and return code as
there is no way to separate two "sessions" apart. The kernel thread priority
scheme will decide which thread will get write access if multiple threads are
queued.

There are two schemes for how immediately returned data from the modem is
delivered to the user, for instance from an AT+CNUM command. The user can either
submit a reference to a string buffer or a reference to a handler function in
the write function. In the case of the user string buffer, the driver will
remove the return code and then deliver the rest of the string in the buffer if
it's large enough. Data will only be returned to the user if the return code
is OK.

In the case of a handler function, the return will be removed and the rest of
the string will delivered to the handler function through a char pointer
parameter. Allocation and deallocation of the buffer is handled by the driver,
and the content should not be considered valid outside of the handler. Both
schemes are limited to the maximum reception size defined by
CONFIG_AT_CMD_RESPONSE_MAX_LEN.

Notifications are always handled by a callback function and is separate from
the one used to handle data returned immediately after sending a command. This
callback is set by @ref at_cmd_set_notification_handler.

API documentation
*****************

| Header file: :file:`include/at_cmd.h`
| Source file: :file:`lib/at_cmd.c`

.. doxygengroup:: at_cmd
   :project: nrf
   :members:
