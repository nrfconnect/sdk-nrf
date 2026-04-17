.. _bt_gatt:

Generic Attribute Profile (GATT)
################################

.. contents::
   :local:
   :depth: 2

The GATT layer manages the service database providing APIs for service registration and attribute declaration.

The GATT Client initiates commands and requests towards the GATT Server, and can receive responses, indications, and notifications sent by the server.
It is enabled through the :kconfig:option:`CONFIG_BT_GATT_CLIENT` Kconfig option.

The GATT Server accepts incoming commands and requests from the GATT Client, and sends responses, indications, and notifications to the client.

Services can be registered using the :c:func:`bt_gatt_service_register` function that takes the :c:struct:`bt_gatt_service` structure providing the list of attributes the service contains.
The helper macro :c:macro:`BT_GATT_SERVICE()` can be used to declare a service.

Attributes can be declared using the :c:struct:`bt_gatt_attr` structure or using one of the following helper macros:

* :c:macro:`BT_GATT_PRIMARY_SERVICE` - Declares a Primary Service.
* :c:macro:`BT_GATT_SECONDARY_SERVICE` - Declares a Secondary Service.
* :c:macro:`BT_GATT_INCLUDE_SERVICE` - Declares an Include Service.
* :c:macro:`BT_GATT_CHARACTERISTIC` - Declares a Characteristic.
* :c:macro:`BT_GATT_DESCRIPTOR` - Declares a Descriptor.
* :c:macro:`BT_GATT_ATTRIBUTE` - Declares an Attribute.
* :c:macro:`BT_GATT_CCC` - Declares a Client Characteristic Configuration.
* :c:macro:`BT_GATT_CEP` - Declares Characteristic Extended Properties.
* :c:macro:`BT_GATT_CUD` - Declares a Characteristic User Format.

Each attribute contain a ``uuid`` that describes their type, a ``read`` callback, a ``write`` callback, and a set of permissions.
Both read and write callbacks can be set to NULL if the attribute permission does not allow their respective operations.

.. note::
   32-bit UUIDs are not supported in GATT.
   All 32-bit UUIDs must be converted to 128-bit UUIDs when the UUID is contained in an ATT PDU.

.. note::
   Attribute ``read`` and ``write`` callbacks are called directly from the RX thread and thus, it is not recommended to block them for long periods of time.

Attribute value changes can be notified using the :c:func:`bt_gatt_notify` function.
Alternatively, you can use the :c:func:`bt_gatt_notify_cb` function that enables passing a callback to be called when it is necessary to know the exact instant when the data has been transmitted over the air.
Indications are supported by the :c:func:`bt_gatt_indicate` function.

Discover procedures can be initiated using the :c:func:`bt_gatt_discover` function that takes the :c:struct:`bt_gatt_discover_params` structure describing the type of discovery.
The parameters also serve as a filter when setting the ``uuid`` field.
Only matching attributes will be discovered.
In contrast, setting the field to NULL allows all attributes to be discovered.

.. note::
   Caching discovered attributes is not supported.

Read procedures are supported by the :c:func:`bt_gatt_read` function that takes the contents of the :c:struct:`bt_gatt_read_params` structure as parameters.
You can set one or more attributes in the parameters.
Setting multiple handles requires the :kconfig:option:`CONFIG_BT_GATT_READ_MULTIPLE` Kconfig option.

Write procedures are supported by the :c:func:`bt_gatt_write` function that takes the contents of the :c:struct:`bt_gatt_write_params` structure as parameters.
If the write operation does not require a response, you can use the :c:func:`bt_gatt_write_without_response` or :c:func:`bt_gatt_write_without_response_cb` function, with the later working similarly to the :c:func:`bt_gatt_notify_cb` function.

You can initiate subscriptions to a notification and an indication using the :c:func:`bt_gatt_subscribe` function that takes the contents of the :c:struct:`bt_gatt_subscribe_params` structure as parameters.
Multiple subscriptions to the same attribute are supported, so there might be multiple ``notify`` callbacks triggered for the same attribute.
To remove subscriptions, use the :c:func:`bt_gatt_unsubscribe` function.

.. note::
   When subscriptions are removed, the ``notify`` callback is called with the data set to NULL.

API Reference
*************

GATT
====

| Header file: :file:`include/bluetooth/host/gatt.h`

.. doxygengroup:: bt_gatt

GATT Server
===========

| Header file: :file:`include/bluetooth/host/gatt.h`

.. doxygengroup:: bt_gatt_server

GATT Client
===========

| Header file: :file:`include/bluetooth/host/gatt.h`

.. doxygengroup:: bt_gatt_client
