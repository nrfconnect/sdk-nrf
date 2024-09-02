.. _bms_readme:

GATT Bond Management Service (BMS)
##################################

.. contents::
   :local:
   :depth: 2

This library implements the Bond Management Service with the corresponding set of characteristics defined in the `Bond Management Service Specification`_.

You can configure the service to support your desired feature set of bond management operations.
All the BMS features in the "LE transport only" mode are supported:

 * Delete the bond of the requesting device.
 * Delete all bonds on the Server.
 * Delete all bonds on the Server except the one of the requesting device.

You can enable each feature when initializing the library.

Authorization
*************

You can require authorization to access each BMS feature.

When required, the Client's request to execute a bond management operation must contain the authorization code.
The Server compares the code with its local version and accepts the request only if the codes match.

If you use at least one BMS feature that requires authorization, you need to provide a callback with comparison logic for the authorization codes.
You can set this callback when initializing the library.

Deleting the bonds
******************

The Server deletes bonding information on Client's request right away when there is no active Bluetooth® Low Energy connection associated with a bond.
Otherwise, the Server removes the bond for a given peer when it disconnects.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/bms.h`
| Source file: :file:`subsys/bluetooth/services/bms.c`

.. doxygengroup:: bt_bms
   :project: nrf
   :members:
