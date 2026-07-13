.. _hids_readme:

GATT Human Interface Device (HID) Service
#########################################

.. contents::
   :local:
   :depth: 2

This library implements the Human Interface Device Service (HIDS) with the corresponding set of characteristics.
When initialized, it adds the HID Service and a set of characteristics, according to the `HID Service Specification`_ and the user requirements, to the Zephyr Bluetooth® stack database.

If enabled, a notification of Input Report characteristics is sent when the application calls the corresponding :c:func:`bt_hids_inp_rep_send` function.

You can register dedicated event handlers for most of the HIDS characteristics to be notified about changes in their values.

The HIDS library must be notified about the incoming connect and disconnect events using the dedicated API.
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

The HIDS library can handle multiple connected peers at the same time.
The server allocates context data for each connected client.
The context data is then used to store the values of HIDS characteristics.
These values are unique for each connected peer.
While sending notifications, you can also target a specific client by providing the connection instance that is associated with it.

HID Shorter Connection Intervals (SCI) support
**********************************************

The HID Shorter Connection Intervals (SCI) feature lets an HID host request shorter Bluetooth LE connection intervals for lower input latency.
This is done by requesting a specific HID SCI mode from the HID device.
Each mode specifies a set of connection parameters that the HID host can request.
The specific parameter values are device-dependent.

This feature is in experimental state.

There are four HID SCI modes:

* Default
* Fast
* Low Power (optional)
* Full Range

Each HID SCI mode has dedicated Kconfig options for connection interval, subrate, peripheral latency, and supervision timeout.
The default values follow the recommendations of the HID over GATT Profile specification.

Use the :kconfig:option:`CONFIG_BT_HIDS_SCI` Kconfig option to enable HID SCI on a HID device.

The feature requires the Zephyr :kconfig:option:`CONFIG_BT_SHORTER_CONNECTION_INTERVALS` option.

.. note::
   Currently, only one HID service can be present on a HID device supporting HID SCI.
   An attempt to initialize a second HID service will result in an error.

.. note::
   In case of multiple HID hosts connected to a single HID device supporting HID SCI, a separate HID SCI mode as well as a separate set of connection parameters is used for each connected host.

Additional Kconfig options:

* :kconfig:option:`CONFIG_BT_HIDS_SCI_LOW_POWER_MODE` - Enable HID SCI Low Power mode support.

Application integration
-----------------------

The HID host requests an HID SCI mode through the HID Control Point characteristic.
This starts the SCI mode change process, which consists of the following steps:

1. Register :c:member:`bt_hids_init_param.conn_cp_evt_handler` during HIDS initialization.
   The library invokes this callback with the matching :c:enum:`bt_hids_cp_evt` event and the connection object when the HID host writes an SCI mode request to the Control Point characteristic.
   The deprecated :c:member:`bt_hids_init_param.cp_evt_handler` callback is not invoked for SCI mode requests.
#. Inside the callback, the application can perform any additional necessary actions, such as performing Frame Space Update.
#. Request connection parameters for the target :c:enum:`bt_hids_sci_mode_value` using one of the following approaches:

   * Call the :c:func:`bt_hids_sci_mode_change_request` function to automatically request the parameter values specified by the Kconfig options.
   * Call the :c:func:`bt_hids_sci_mode_conn_rate_param_get` function to obtain the parameters for the mode.
     Optionally adjust them to select a subset of the allowed values for the mode.
     You can use the :c:func:`bt_hids_sci_mode_validate` API to check if the updated parameters are still valid for the selected HID SCI mode.
     Then call :c:func:`bt_conn_le_conn_rate_request` manually.

   The application should track the pending mode (for example, in per-connection context data) until the connection rate change completes.
#. In the :c:member:`bt_conn_cb.conn_rate_changed` callback, call the :c:func:`bt_hids_sci_mode_validate` function with the pending mode and the new connection parameters.
   If the validation succeeds, call the :c:func:`bt_hids_sci_mode_updated` function to update the cached SCI mode and notify the subscribed HID hosts.
   If the validation fails, the application decides how to recover (for example, ignore the event or try another mode).

See the :ref:`peripheral_hids_mouse` and :ref:`peripheral_hids_keyboard` samples (HID SCI configuration) for a reference implementation.

Report masking
**************

The HIDS library can consist of reports that are used to transmit differential data.
One example of such a report type is a mouse report.
In a mouse report, mouse movement data represents the position change along the X or Y axis and should only be interpreted once by the HID host.
After receiving a notification from the HID device, the client should not be able to get any non-zero value from the differential data part of the report by using the GATT Read Request on its characteristic.
This is achieved by report masking.
You can configure a relevant mask for a report to specify which part of the report is not to be stored as a characteristic value.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/hids.h`
| Source file: :file:`subsys/bluetooth/services/hids.c`

.. doxygengroup:: bt_hids
