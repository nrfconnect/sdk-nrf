.. _modem_key_mgmt:

Modem key management
####################

The modem key management library provides functions to manage the credentials stored in the nRF9160 LTE modem.
The library uses AT commands (`Credential storage management %CMNG`_) to add, update, and delete credentials.

Each set of keys and certificates that is stored in the modem is identified by a security tag (``sec_tag``).
You specify this tag when adding the credentials and use it when you update or delete them.
All related credentials share the same security tag.
You can use the library to check if a specific security tag exists.

To establish a connection, pass the security tag to the :ref:`nrfxlib:bsdlib` when creating a secure socket.

.. See :ref:`nrfxlib:security_tags` for more information about how security tags are used in the BSD library.

.. important::
   Security credentials usually exceed the default AT command response length.
   Therefore, you must set :option:`CONFIG_AT_CMD_RESPONSE_MAX_LEN` to a sufficiently high value when using this library.

API documentation
*****************

| Header file: :file:`include/modem_key_mgmt.h`
| Source files: :file:`lib/modem_key_mgmt/`

.. doxygengroup:: modem_key_mgmt
   :project: nrf
   :members:
