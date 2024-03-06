.. _bt_mesh_prop_srv_readme:

Generic Property Servers
########################

.. contents::
   :local:
   :depth: 2

A Generic Property Server holds a list of Device Properties.
The following property servers are supported:

- Generic Manufacturer Property Server - Provides access to Manufacturer Properties, which are read-only properties set by the device manufacturer.
- Generic Admin Property Server - Provides access to Admin Properties, which can be read and written to.
- Generic User Property Server - Provides access to both Admin and Manufacturer Properties on its element, but only if the property owner allows it.
- Generic Client Property Server - Provides a list of all properties supported by the Generic Property Client on the same element.
  The Client Property Server does not actually own any properties, and does not need the user to provide access to any property values.

When developing, take the following into account:

* Only the administrator of the mesh network should have access to the admin or manufacturer servers.
* All other mesh nodes should operate on the user server.
* The manufacturer and admin servers:

  * May change the user property access rights for each individual property they own at runtime.
  * Must provide a list of all properties they own in the initialization of the model.

* The User Property Server does not own any properties, but relies on Admin Property Server and Manufacturer Property Server on the same element to provide access to their properties.
  An element with a User Property Server, and no Admin or Manufacturer Property Server is useless.
* The model keeps the user access field of each Admin and Manufacturer Property in persistent storage.
  The property values themselves must be stored by the application.
* Your application should provide access to the property values through the callbacks of server getter (:c:member:`bt_mesh_prop_srv.get`) and setter (:c:member:`bt_mesh_prop_srv.set`).
* The set of Property IDs and their order cannot be changed.

States
======

The Generic Power OnOff Server model contains the following state:

Device Properties: :c:struct:`bt_mesh_prop`
    A single server may own several Device Properties, which are accessed by the Property ID.
    Each property holds a value and a user access parameter, which controls the property's availability to a Generic User Property Server on the same element.

    The Device Properties have slightly different access restrictions, depending on the type of server that owns them:

    * Manufacturer Property - The value can only be read.
    * Admin Property - The value can be read and written.
    * Client Property - Has no associated value, and is only a listing of the Property IDs supported by the accompanying Generic Property Client.

Extended models
===============

Generic Admin Property Server and Generic Manufacturer Property Server should both extend the Generic User Property Server.
This is not handled automatically by the model implementation.
For this reason, to ensure compliance with the Bluetooth® Mesh model specification, a Generic User Property Server must be instantiated in the same element in the device composition data as the Generic Admin or Generic Manufacturer Property Server.

Persistent storage
==================

The Generic Manufacturer Property Server and Generic Admin Property Server models will store changes to the user access rights of their properties.
Any permanent changes to the property values themselves should be stored manually by the application.

If :kconfig:option:`CONFIG_BT_SETTINGS` is enabled, the Generic Admin Property Server stores all its states persistently using a configurable storage delay to stagger storing.
See :kconfig:option:`CONFIG_BT_MESH_STORE_TIMEOUT`.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_prop_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_prop_srv.c`

.. doxygengroup:: bt_mesh_prop_srv
   :project: nrf
   :members:
