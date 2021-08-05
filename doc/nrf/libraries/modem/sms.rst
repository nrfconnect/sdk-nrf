.. _sms_readme:

SMS subscriber
##############

.. contents::
   :local:
   :depth: 2

The SMS subscriber module acts as a unique and global client in the system that can dispatch SMS notifications to many subscribers, which makes it possible for more than one module to receive SMS messages.

The module provides functions to register and unregister SMS listeners, which are the modules that need to send or receive SMS messages.
Each listener is identified by a unique handle and receives the SMS data and metadata through a callback function.
The module also provides a possibility to send SMS messages.

SMS listeners can be registered or unregistered at run time.
The SMS data payload is parsed and processed before it is given to the client.

The SMS module uses AT commands to register as an SMS client to the modem.
In addition, AT commands are also used to send SMS messages.
SMS notifications are received using AT commands, but those are not visible for the users of this module.
The module automatically acknowledges the SMS messages received on behalf of each listener.

The module does not use a dedicated socket.
It uses the shared AT socket that is provided by the :ref:`at_notif_readme` module.

The current modem firmware allows only one SMS client.

Configuration
*************

Configure the following Kconfig options when using this library:

* :kconfig:`CONFIG_SMS` - Enables the SMS subscriber library.
* :kconfig:`CONFIG_SMS_SUBSCRIBERS_MAX_CNT` - Sets the maximum number of SMS subscribers.
* :kconfig:`CONFIG_AT_CMD_RESPONSE_MAX_LEN` - Defines the maximum size of the AT command response, which might limit the size of the received SMS message. Values over 512 bytes will not restrict the size of the received message as the maximum data length of the SMS is 140 bytes. This parameter is defined in the :ref:`at_cmd_readme` module.

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
