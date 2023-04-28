.. _hids_readme:

GATT Human Interface Device (HID) Service
#########################################

.. contents::
   :local:
   :depth: 2

This module implements the Human Interface Device Service with the corresponding set of characteristics.
When initialized, it adds the HID Service and a set of characteristics, according to the `HID Service Specification`_ and the user requirements, to the Zephyr Bluetooth® stack database.

If enabled, a notification of Input Report characteristics is sent when the application calls the corresponding :c:func:`bt_hids_inp_rep_send` function.

You can register dedicated event handlers for most of the HIDS characteristics to be notified about changes in their values.

The HIDS module must be notified about the incoming connect and disconnect events using the dedicated API.
This is done to synchronize the connection state of HIDS with the top module that uses it.

.. note::

   Some systems require security to use the HID Service.
   To ensure interoperability, enable the :kconfig:option:`CONFIG_BT_HIDS_DEFAULT_PERM_RW_ENCRYPT` Kconfig option.

The HID Service is used in the following samples:

 * :ref:`peripheral_hids_mouse`
 * :ref:`peripheral_hids_keyboard`

Service Changed support
***********************

You can change the structure of the HID Service dynamically, even during an ongoing connection, by using the Service Changed feature supported by the Zephyr Bluetooth stack.
You must uninitialize the currently used HIDS instance, prepare a new description of the service structure, and initialize a new instance.
In such case, the Bluetooth stack automatically notifies connected peers about the changed service.

Multiclient support
*******************

The HIDS module can handle multiple connected peers at the same time.
The server allocates context data for each connected client.
The context data is then used to store the values of HIDS characteristics.
These values are unique for each connected peer.
While sending notifications, you can also target a specific client by providing the connection instance that is associated with it.

Report masking
**************

The HIDS module can consist of reports that are used to transmit differential data.
One example of such a report type is a mouse report.
In a mouse report, mouse movement data represents the position change along the X or Y axis and should only be interpreted once by the HID host.
After receiving a notification from the HID device, the client should not be able to get any non-zero value from the differential data part of the report by using the GATT Read Request on its characteristic.
This is achieved by report masking.
You can configure a relevant mask for a report to specify which part of the report is not to be stored as a characteristic value.

API documentation
*****************

| Header file: :file:`include/hids.h`
| Source file: :file:`subsys/bluetooth/services/hids.c`

.. doxygengroup:: bt_hids
   :project: nrf
   :members:
