.. _bt_mesh_properties_readme:

Bluetooth mesh properties
##########################

.. contents::
   :local:
   :depth: 2

The Bluetooth SIG defines a list of Bluetooth mesh properties in the Bluetooth mesh device properties specification.
Each property has an assigned ID and an associated Bluetooth GATT characteristic.
The properties all represent values on a format defined by the associated characteristic.

Only the list of Property IDs, as specified in the Bluetooth Mesh Device Properties Specification v1.1 are defined in the API.
It's up to the application to implement property values that are compliant with the specified format.

API documentation
******************

| Header file: :file:`include/bluetooth/mesh/properties.h`

.. doxygengroup:: bt_mesh_property_ids
   :project: nrf
   :members:
