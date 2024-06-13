.. _sms_readme:

SMS
###

.. contents::
   :local:
   :depth: 2

The SMS library acts as a unique and global client in the system that can dispatch SMS notifications to many subscribers,
which makes it possible for more than one module to receive SMS messages.
In addition, the library has functions for sending SMS messages.

The module provides functions to register and unregister SMS listeners, which are the modules that need to send or receive SMS messages.
Each listener is identified by a unique handle and receives the SMS data and metadata through a callback function.
The module also provides a possibility to send SMS messages.

SMS listeners can be registered or unregistered at run time.
The SMS data payload is parsed and processed before it is given to the client.

The SMS module uses AT commands to register as an SMS client to the modem.
In addition, AT commands are also used to send SMS messages.
SMS notifications are received using AT commands, but those are not visible for the users of this module.
The module automatically acknowledges the SMS messages received on behalf of each listener.

The SMS module reads the SMS service center number using the ``AT+CSCA?`` AT command.
Most applications do not need to set a custom SMS service center number because the default value is enough.
However, if your application requires a custom SMS service center number, you can set it using the ``AT+CSCA`` AT command.
Because ``AT+CSCA`` AT command is not supported by the modem firmware, you can use the :ref:`lib_gcf_sms_readme` library.

Configuration
*************

Configure the following mandatory Kconfig options when using this library:

* :kconfig:option:`CONFIG_SMS` - Enables the SMS library.
* :kconfig:option:`CONFIG_SMS_SUBSCRIBERS_MAX_CNT` - Sets the maximum number of SMS subscribers.

Check and configure the optional :kconfig:option:`CONFIG_GCF_SMS` Kconfig option when using this library.
It enables the :ref:`lib_gcf_sms_readme` library for the ``AT+CSCA`` AT command support to allow a custom SMS service center number.

Limitations
***********

Only a single SMS client can be registered to the modem in the system.
If there is already an SMS client registered in the system (for example, using AT commands), the initialization of this module fails, and an error is returned.

API documentation
*****************

| Header file: :file:`include/modem/sms.h`
| Source file: :file:`lib/sms/sms.c`

.. doxygengroup:: sms
   :project: nrf
   :members:
