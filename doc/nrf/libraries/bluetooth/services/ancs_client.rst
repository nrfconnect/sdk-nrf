.. _ancs_client_readme:

Apple Notification Center Service (ANCS) Client
###############################################

.. contents::
   :local:
   :depth: 2

The ANCS Client library is used for implementation of the Apple Notification Center Service (ANCS) client.

.. note::

   Disclaimer: Nordic Semiconductor ASA can change the client implementation of the Apple Notification Center Service at any time.
   Apple can change the server implementations such as the ones found in iOS at any time, which may cause this client implementation to stop working.

Overview
********

The ANCS client can be used as a Notification Consumer (NC) that receives data notifications from a Notification Provider (NP).
The NP is typically an iOS device that is acting as a server.
For detailed information about the Apple Notification Center Service, see `Apple Developer Documentation <Apple Notification Center Service Specification_>`_.

In this library, iOS notifications are received through the GATT notifications.

.. note::

   The term *notification* is used in two different meanings:

   * An iOS notification is the data received from the Notification Provider.
   * A GATT notification is a way to transfer data with Bluetooth® Low Energy.

   *iOS notification* and *GATT notification* are used where required to avoid confusion.

Configuration
*************

Upon initializing the library, add different iOS notification attributes you want to receive for iOS notifications (see :c:func:`bt_ancs_register_attr`).

Once a connection is established with a central device, the library needs a service discovery to discover the ANCS server handles.
If this succeeds, the handles for the ANCS server must be assigned to an ANCS client instance using the :c:func:`bt_ancs_handles_assign` function.

The application can now subscribe to iOS notifications with :c:func:`bt_ancs_subscribe_notification_source`.

Usage
*****

The notifications arrive through the callback function of the notification source subscription.

Use the following functions to perform activities related to notifications:

* :c:func:`bt_ancs_request_attrs` - Request attributes for the notifications.

  The attributes arrive through the callback function of the data source subscription with the :c:enumerator:`BT_ANCS_COMMAND_ID_GET_NOTIF_ATTRIBUTES` Command ID.

* :c:func:`bt_ancs_request_app_attr` - Request attributes of the app that issued the notifications.

  The app attributes arrive through the callback function of the data source subscription with the :c:enumerator:`BT_ANCS_COMMAND_ID_GET_APP_ATTRIBUTES` Command ID.

* :c:func:`bt_ancs_notification_action` - Make the Notification Provider perform an action based on the provided notification.

.. msc::
    hscale = "2.0";
    Application, ANCS_Client;
    |||;
    Application=>ANCS_Client   [label = "bt_ancs_client_init(ancs_instance)"];
    Application=>ANCS_Client   [label = "bt_ancs_register_attr(attribute)"];
    ...;
    Application=>ANCS_Client   [label = "bt_ancs_handles_assign(dm, ancs_instance)"];
    Application=>ANCS_Client   [label = "bt_ancs_subscribe_notification_source(ancs_instance, callback)"];
    Application=>ANCS_Client   [label = "bt_ancs_subscribe_data_source(ancs_instance, callback)"];
    |||;
    ...;
    |||;
    Application<<=ANCS_Client  [label = "notification_source callback(notif)"];
    |||;
    ...;
    |||;
    Application=>ANCS_Client   [label = "bt_ancs_request_attrs(notif)"];
    Application<<=ANCS_Client  [label = "data_source callback(attr_response)"];
    |||;

Samples using the library
*************************

The :ref:`peripheral_ancs_client` sample uses this library.

Dependencies
************

There are no dependencies for using this library.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/ancs_client.h`
| Source files:

  * :file:`subsys/bluetooth/services/ancs_client.c`
  * :file:`subsys/bluetooth/services/ancs_attr_parser.c`
  * :file:`subsys/bluetooth/services/ancs_app_attr_get.c`

.. doxygengroup:: bt_ancs_client
   :project: nrf
   :members:
