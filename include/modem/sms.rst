.. _sms_readme:

SMS subscriber
##############

The current modem firmware allows only one SMS client.
The SMS subscriber module acts as a unique and global client in the system that can dispatch SMS notifications to many subscribers, which makes it possible for more than one module to receive SMS messages.

The module provides functions to register and unregister SMS listeners, which are the modules that need to send or receive SMS messages.
Each listener is identified by a unique handle and receives the SMS data and metadata through a callback function.

SMS listeners can be registered or unregistered at run time.
The SMS data payload is given as raw data.
It is up to the listener to parse and process it.

The SMS module uses AT commands to register as SMS client.
SMS notifications are received using AT commands, but those are not visible for the users of this module.
The module automatically acknowledges received SMS messages on behalf of each listener.

The module does not use a dedicated socket.
It uses the shared AT socket that is provided by the :ref:`at_notif_readme` module.

Configuration
*************

Configure the following parameters when using this library:

* :option:`CONFIG_SMS_MAX_SUBSCRIBERS_CNT` - The maximum number of SMS subscribers.
* :option:`CONFIG_AT_CMD_RESPONSE_MAX_LEN` - The maximum size of the SMS message.
  This parameter is defined in the :ref:`at_cmd_readme` module.

Limitations
***********

Only one SMS client can be registered in the system.
If there already is an SMS client registered in the system (using AT commands, for example), the initialization of this module fails and an error is returned.

API documentation
*****************

| Header file: :file:`include/modem/sms.h`
| Source file: :file:`lib/sms/sms.c`

.. doxygengroup:: sms
   :project: nrf
   :members:
