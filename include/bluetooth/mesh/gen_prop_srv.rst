.. _bt_mesh_prop_srv_readme:

Generic Property Servers
########################

A Generic Property Server holds a list of Device Properties.
The following property servers are supported:

- Generic Manufacturer Property Server - Provides access to Manufacturer properties, which are read-only properties set by the device manufacturer.
- Generic Admin Property Server - Provides access to Admin properties, which can be read and written to.
- Generic User Property Server - Provides access to both Admin and Manufacturer properties on its element, but only if the Property owner allows it.
- Generic Client Property Server - Provides a list of all properties supported by the Generic Property Client on the same element.
  The Client Property Server does not actually own any properties, and does not need the user to provide access to any property values.

When developing, take the following into account:

* Only the administrator of the mesh network should have access to the Admin or Manufacturer servers.
* All other mesh nodes should operate on the User server.
* The Manufacturer and Admin servers:

  * May change the User property access rights for each individual Property they own at runtime.
  * Must provide a list of all Properties they own in the initialization of the model.

* The User Property server does not own any properties, but relies on Admin and Manufacturer servers on the same element to provide access to their properties.
  An element with a User Property Server and no Admin or Manufacturer Server is useless.
* The model keeps the user access field of each Admin and Manufacturer property in persistent storage.
  The property values themselves must be stored by the application.
* Your application should provide access to the property values through the callbacks of server getter (:cpp:member:`bt_mesh_prop_srv::get`) and setter (:cpp:member:`bt_mesh_prop_srv::set`).
* The set of Property IDs and their order cannot be changed.

States
======

The Generic Power OnOff Server model contains the following state:

Device Properties: :cpp:type:`bt_mesh_prop`
    A single server may own several Device Properties, which are accessed by the Property ID.
    Each Property holds a value and a user access parameter, which controls the Property's availability to a Generic User Property Server on the same element.

    The Device Properties have slightly different access restrictions depending on the type of server that owns them:

    * Manufacturer Property - The value can only be read.
    * Admin Property - The value can be read and written.
    * Client Property - Has no associated value, and is only a listing of the property IDs supported by the accompanying Generic Property Client.

Extended models
===============

The Generic Admin Property Server and Generic Manufacturer Property Server should both extend the Generic User Property Server.
This is not handled automatically by the model implementation.
For this reason, to ensure compliance with the Bluetooth Mesh Model specification, a Generic User Property Server must be instantiated in the same element in the device composition data as the Generic Admin or Generic Manufacturer Property server.

Persistent storage
==================

The Generic Manufacturer Property Server and Generic Admin Property Server models will store changes to the user access rights of their properties.
Any permanent changes to the property values themselves should be stored manually by the application.

API documentation
=================

| Header file: :file:`include/bluetooth/mesh/gen_prop_srv.h`
| Source file: :file:`subsys/bluetooth/mesh/gen_prop_srv.c`

.. doxygengroup:: bt_mesh_prop_srv
   :project: nrf
   :members:
