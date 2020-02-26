.. _at_notif_readme:

AT command notifications
########################

AT command notifications can be dispatched to registered modules.
Modules can register a callback function to receive AT command notifications as raw string.
Multiple instances, which can be identified by pointers to contexts, are also supported.
Modules can de-register the callback function to stop receiving notifications.

API documentation
*****************

| Header file: :file:`include/modem/at_notif.h`
| Source file: :file:`lib/at_notif/at_notif.c`

.. doxygengroup:: at_notif
   :project: nrf
   :members:
