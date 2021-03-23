.. _ancs_client_readme:

Apple Notification Center Service (ANCS) Client
###############################################

.. contents::
   :local:
   :depth: 2

.. note::

   Disclaimer: Nordic Semiconductor ASA can change this client implementation of the Apple Notification Center Service at any time.
   Apple can change the server implementations such as the ones found in iOS at any time, which may cause this client implementation to stop working.

The ANCS Client module implements the Apple Notification Center Service (ANCS) client.
This client can be used as a Notification Consumer (NC) that receives data notifications from a Notification Provider (NP).
The NP is typically an iOS device that is acting as a server.
For detailed information about the Apple Notification Center Service, see `Apple's iOS Developer Library <Apple Notification Center Service Specification_>`_.

The term "notification" is used in two different meanings:

* An iOS notification is the data received from the Notification Provider.

* A GATT notification is a way to transfer data with Bluetooth Low Energy.

In this module, iOS notifications are received through the GATT notifications.
The full term (iOS notification or GATT notification) is used where required to avoid confusion.

The ANCS Client is used in the :ref:`peripheral_ancs_client` sample.

Usage
*****

Upon initializing the module, you must add the different iOS notification attributes you would like to receive for iOS notifications (see :c:func:`bt_ancs_register_attr`).

Once a connection is established with a central device, the module needs a service discovery to discover the ANCS server handles.
If this succeeds, the handles for the ANCS server must be assigned to an ANCS client instance using the :c:func:`bt_ancs_handles_assign` function.

The application can now subscribe to iOS notifications with :c:func:`bt_ancs_subscribe_notification_source`.
The notifications arrive in the :c:enumerator:`BT_ANCS_EVT_NOTIF` event.
Use :c:func:`bt_ancs_request_attrs` to request attributes for the notifications.
The attributes arrive in the :c:enumerator:`BT_ANCS_EVT_NOTIF_ATTRIBUTE` event.
Use :c:func:`bt_ancs_request_app_attr` to request attributes of the app that issued the notifications.
The app attributes arrive in the :c:enumerator:`BT_ANCS_EVT_APP_ATTRIBUTE` event.
Use :c:func:`bt_ancs_notification_action` to make the Notification Provider perform an action based on the provided notification.

.. msc::
    hscale = "2.0";
    Application, ANCS_Client;
    |||;
    Application=>ANCS_Client   [label = "bt_ancs_client_init(ancs_instance, event_handler)"];
    Application=>ANCS_Client   [label = "bt_ancs_register_attr(attribute)"];
    ...;
    Application=>ANCS_Client   [label = "bt_ancs_handles_assign(dm, ancs_instance)"];
    Application=>ANCS_Client   [label = "bt_ancs_subscribe_notification_source(ancs_instance)"];
    Application=>ANCS_Client   [label = "bt_ancs_subscribe_data_source(ancs_instance)"];
    |||;
    ...;
    |||;
    Application<<=ANCS_Client  [label = "BT_ANCS_EVT_NOTIF"];
    |||;
    ...;
    |||;
    Application=>ANCS_Client   [label = "bt_ancs_request_attrs(notif)"];
    Application<<=ANCS_Client  [label = "BT_ANCS_EVT_NOTIF_ATTRIBUTE"];
    |||;

API documentation
*****************

| Header file: :file:`include/bluetooth/services/ancs_client.h`
| Source files: :file:`subsys/bluetooth/services/ancs_client.c` and :file:`subsys/bluetooth/services/ancs_attr_parser.c` and :file:`subsys/bluetooth/services/ancs_app_attr_get.c`

.. doxygengroup:: bt_ancs_client
   :project: nrf
   :members:
