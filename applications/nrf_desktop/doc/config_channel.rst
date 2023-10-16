.. _nrf_desktop_config_channel:

Configuration channel
#####################

.. contents::
   :local:
   :depth: 2

The configuration channel lets you exchange data between the host computer and an nRF Desktop's HID device.
On the logical level, it creates a bridge between the application modules and the corresponding part of the host script.
If there are more compatible devices connected to the host, you can select which device will receive data.
The configuration channel allows a dongle type device to act as a proxy for Bluetooth® LE Peripheral devices.

Among the types of data that you can send through the configuration channel are the following:

* Device configuration parameters, for example mouse sensor CPI.
* Firmware updates.
* LED effect display data, after it has been generated on the computer.

For instructions on how to install and use the configuration channel tools that are provided in the |NCS| on a host computer, see the :ref:`nrf_desktop_config_channel_script`.

Transport overview
******************

The configuration channel activity is performed using a dedicated HID feature report.
The cross-platform `HIDAPI library`_ is used for exchanging the reports.
The library supports both Bluetooth® LE and USB.

On the HID feature report level, the host can set or get report, either to or from the connected device.
The host initiates all data exchange.

.. note::
   If the device supports multiple USB or Bluetooth® LE HID instances, only one USB HID instance and only one Bluetooth® LE HID instance can be used to configure the device.
   Other HID instances ignore the HID report set method and respond with the disconnected status on the HID report get method to indicate that the HID instance is not used for the configuration channel.

Transaction request and response
********************************

To maintain the communication pace between the host and the device, the configuration channel uses transactions.
Each transaction consists of a single request send from the host to the device, followed by a single response that provides the completion status of the requested activity.

* A request from the host to the device is a single HID feature report set operation.
* The response is a single HID feature report get operation.

Since the device works asynchronously from the host, it might happen that an activity requested by the host through the configuration channel is still pending when the host-side operation is done.

* If the received request completion status indicates that the request is still pending, the host must retry asking the device for status update after some time.
* If the received status indicates that the request was completed by the device, the host can send another request.

.. note::
   There can be only one pending configuration channel request.
   The host can send the following request only after it has received a response for the previous request.

Transaction types
*****************

The configuration channel has the following transaction types:

* set - Used to write configurable options.
* fetch - Used to read the values of options from the device.

Both transactions can be used to trigger a device to perform an activity, but the fetch transaction can ensure that such activity is completed.

Transaction forwarding
**********************

The nRF Desktop dongle can forward the request from the host to a peripheral.
In such case, the dongle also forwards the response from the peripheral to the host.
The :ref:`nrf_desktop_hid_forward` implements this functionality.

See the following diagrams for example transactions:

Setting a configuration value

    .. msc::
       hscale = "1.3";
       Host,Device;
       Host>>Device      [label="set report: set value 1600"];
       Host<<Device      [label="get report: SUCCESS"];

Fetching a configuration value

    .. msc::
       hscale = "1.3";
       Host,Device;
       Host>>Device      [label="set report: request value fetch"];
       Host<<Device      [label="get report: PENDING"];
       Host<<Device      [label="get report: value 1600, SUCCESS"];

Setting a configuration value of a device, forwarded by the dongle to a paired device

    .. msc::
       hscale = "1.3";
       Host,Dongle,Device;
       Host>>Dongle      [label="set report: set value 1600"];
       Dongle>>Device    [label="set report: set value 1600"];
       Host<<Dongle      [label="get report: PENDING"];
       Dongle<<Device    [label="get report: SUCCESS"];
       Host<<Dongle      [label="get report: SUCCESS"];

Data format
***********

Both request and response share the same data format, which is described in the following table.

.. _nrf_desktop_table:

+--------------------------------------------------------------------------------+
| Feature report                                                                 |
+-----------+-----------+-----------------------+--------+-------------+---+-----+
| 0         | 1         | 2                     | 3      | 4           | 5 | ... |
+===========+===========+=======================+========+=============+===+=====+
| Report ID | Recipient | Event ID              | Status | Data length | Data    |
|           |           | bits:                 |        |             |         |
|           |           +-----------+-----------+        |             |         |
|           |           | 0..3      | 4..7      |        |             |         |
|           |           +-----------+-----------+        |             |         |
|           |           | Option ID | Module ID |        |             |         |
+-----------+-----------+-----------+-----------+--------+-------------+---------+

See the following sections for a detailed description of each HID feature report component.

Report ID
=========

This is the HID report identifier of the feature report used for transmitting data.

The configuration channel uses a predefined HID feature report.

.. note::
   Bluetooth® LE HID Service removes the leading report ID byte.
   As a result, the firmware obtains a data frame shorter by one byte.

   The USB HID class transmits the whole report, including the report ID byte.

Recipient
=========

This is the identifier of the device to which the request is addressed.

This field is used to route requests in a multidevice setup.
The field can have the following values:

* ``0`` - The transaction is intended for a directly connected device.
* Other values - The transaction should be forwarded by the :ref:`nrf_desktop_hid_forward` to the peripheral connected over Bluetooth® LE.
  The recipients connected over Bluetooth® LE are discovered during the :ref:`device discovery <nrf_desktop_config_channel_device_discovery>`.

Event ID
========

This is the identifier for both module and option for the set or fetch operation.

The field is composed of the following subfields:

* Module ID - The application module that should handle the transaction, stored on the 4 most significant bits of the Event ID field.
* Option ID - The module option the transaction refers to, stored on the 4 least significant bits of the Event ID field.

The values of Module ID and Option ID are assigned during the application build time.
These values are obtained during the :ref:`device discovery <nrf_desktop_config_channel_device_discovery>` performed by the host.
The same application module can have different ID values, when it is compiled at a different configuration.

Status
======

The usage of the Status field is different for request and response operations.

Request
-------

In the case of a request, the value of Status field denotes a requested operation.
The request operations can be grouped as follows:

Generic operations
  There are two generic operations:

  * ``CONFIG_STATUS_SET``
  * ``CONFIG_STATUS_FETCH``

  These operations perform, respectively, set or fetch actions related to a given application module and option (as denoted by Event ID).
  The request is handled by the selected application module.
  The content and length of the associated data depends on module and option.

Module discovery operations
  The :ref:`nrf_desktop_info` handles module discovery operations.
  These operations are used during the :ref:`device discovery <nrf_desktop_config_channel_device_discovery>` to obtain information about the device.

  The Event ID field is not used and it should be set to ``0``.
  The following operations belong to this group:

  * ``CONFIG_STATUS_GET_MAX_MOD_ID`` - Obtain the maximum value of Module ID supported by the device.
    The maximum Module ID is returned as a single unsigned byte.
  * ``CONFIG_STATUS_GET_HWID`` - Obtain a unique Hardware ID of the device.
    The Hardware ID is represented as 8 bytes.
  * ``CONFIG_STATUS_GET_BOARD_NAME`` - Obtain the device's board name.
    The board name is part of the Zephyr board name (:kconfig:option:`CONFIG_BOARD`) from a beginning to the first underscore (``_``) character.
    For example, the ``nrf52840gmouse_nrf52840`` build target would return ``nrf52840gmouse`` as the board name.

  .. note::
     You must use :ref:`nrf_desktop_info` for every device that is configurable with the configuration channel.
     The module provides information that is necessary to identify the device.

Recipient discovery operations
  The :ref:`nrf_desktop_hid_forward` handles recipients discovery operations.
  These operations are performed to obtain IDs of Bluetooth® LE Peripherals connected to the device.

  The Event ID field is not used and it should be set to ``0``.
  The following operations belong to this group:

  * ``CONFIG_STATUS_INDEX_PEERS`` - Request Recipient ID re-evaluation.
    Response to this request returns no data.
    When performed, the device will map each connected Bluetooth® LE Peripheral to an integer.
  * ``CONFIG_STATUS_GET_PEER`` - Obtain Recipient ID of Bluetooth® LE Peripheral.
    Response to this request contains the following information:

    * Peripheral's Hardware ID on the first 8 bytes of response data.
    * Recipient ID assigned to the peripheral on the last meaningful byte of the response data.

    Performing the operation multiple times returns information about subsequent Bluetooth® LE Peripherals.
    This operation should be performed until Recipient ID is set to ``0xFF``, in which case there are no more peripherals.

Recipient discovery caching
  Apart from the recipient discovery operations, the :ref:`nrf_desktop_hid_forward` handles an additional request that can be used to retrieve a snapshot of currently connected recipients (``CONFIG_STATUS_GET_PEERS_CACHE``).
  The request can be used to detect changes in the set of connected Bluetooth® LE Peripherals.
  The request is supported in the firmware since the |NCS| v2.5.0, firmware built with older SDK releases does not support it.

  The Event ID field is not used and it should be set to ``0``.
  The response to this request contains 16 bytes of data, which represents peers cache values for Bluetooth® LE Peripherals.
  Every byte describes peers cache value of a single peripheral.
  The first byte describes the peripheral with a Recipient ID of ``1``, and rest bytes describe peripherals with subsequent Recipient IDs.

  The cache value related to a given Recipient ID is initialized to zero during dongle boot and incremented on both connection establishment and disconnection of the recipient.
  An odd number is reported for a connected recipient.
  An even number is reported for a disconnected recipient.

  If the cache value of a given recipient remains the same as reported before the recipient discovery, the discovered recipient is still connected.
  Otherwise, the host tools must identify the device and perform rediscovery if needed.
  See :ref:`discovering devices connected through dongle <nrf_desktop_config_channel_recipient_discovery>` for details.

Response
--------

In the case of a response, the value of the Status field indicates the state of the earlier request.
The following values are possible:

* ``CONFIG_STATUS_PENDING`` - The operation has not completed.
  The response is not ready and the data field should not be interpreted.
  The host should not send new requests before the operation completes.
* ``CONFIG_STATUS_SUCCESS`` - The operation was successful.
  The host tool can access data returned by the response.
  The host can send a new request.
* ``CONFIG_STATUS_TIMEOUT`` - The operation timed out on the device.
  The request was accepted by an application module but the response was not prepared in time.
  The host can send a new request.
* ``CONFIG_STATUS_REJECT`` - The operation was rejected.
  The host can send a new request.
* ``CONFIG_STATUS_WRITE_FAIL`` - Forwarding configuration channel transaction failed.
  The host can send a new request.
* ``CONFIG_STATUS_DISCONNECTED`` - The operation failed because a module or a device addressed by the request did not respond.
  This can happen when Bluetooth® LE Peripheral disconnects.
  The host can send a new request.

Data length
===========

Indicates how many meaningful bytes of data the request or the response holds.
As the HID feature report has a fixed length, this field indicates the size of the meaningful data.

Data
====

The piece of data of the length defined by Data length, related to the request or the response.

.. _nrf_desktop_config_channel_device_discovery:

Device discovery
****************

Before the device can be configured over the configuration channel, it needs to be discovered by the host scripts.
The discovery procedure identifies the device and gets information about its configurable application modules and their options.

If you are developing a custom host tool, you need to ensure your tool performs the following steps to discover the device:

1. Enumerate all of the connected HID devices.
#. Filter the enumerated devices using Vendor ID and Product ID specified for your devices.
#. For each applicable device, perform recipient discovery.
#. For each device, perform device identification.
#. For each device, perform module discovery.

.. tip::
   The host tools do not need to perform complete discovery of the device every time the device is connected.
   The Module ID and Options ID cannot change until firmware is updated on the device.
   The module descriptor can be cached by the host for a given device and firmware version.

.. _nrf_desktop_config_channel_recipient_discovery:

Recipient discovery
===================

The Recipient ID equal to ``0`` is used to communicate with any device directly connected to the host.

The configuration channel allows a dongle type device to act as a proxy for Bluetooth® LE Peripheral devices.
If you are developing a custom host tool, you need to ensure your tool performs the following steps to discover all of the peripherals that are connected to the dongle over Bluetooth® LE:

1. Retrieve the peers cache with the ``CONFIG_STATUS_GET_PEERS_CACHE`` request.
   The peers cache can be periodically fetched from the device and compared with the previously fetched values to detect changes in the set of peripherals connected through a dongle.
#. Trigger a Recipient ID re-evaluation with the ``CONFIG_STATUS_INDEX_PEERS`` request.
#. Obtain a list of connected devices by calling the ``CONFIG_STATUS_GET_PEER`` request.

Once the Recipient ID list is obtained, you can perform :ref:`nrf_desktop_config_channel_device_identification` and :ref:`nrf_desktop_config_channel_module_discovery` procedures on selected or all connected devices.
When sending the requests, use the chosen Recipient ID.

The assigned Recipient ID is valid only during the connection time between the remote device and the dongle.
If a remote device disconnects from the dongle, the recipient discovery must be repeated when the device reconnects.

.. tip::
   The recipient discovery is not needed if the host does not interact with remote devices connected through the dongle or if the device is a directly connected peripheral.

.. _nrf_desktop_config_channel_device_identification:

Device identification
=====================

The following requests provide the information about the device hardware:

* ``CONFIG_STATUS_GET_BOARD_NAME`` - For reading the board name.
* ``CONFIG_STATUS_GET_HWID`` - For the Hardware ID.
  The Hardware ID allows to differentiate devices of the same type (that have the same board name).

If you are developing a custom host tool, use the Recipient ID linked with the device that is being discovered.
Your tool should note which Recipient ID and HID instance on the host is associated with the board name and the Hardware ID.

.. _nrf_desktop_config_channel_module_discovery:

Module discovery
================

The host can access options provided by the application modules.
Before it gets access to the options, the host must identify which modules are available and what options are provided by them.

Both modules and their options are identified by a name, sent as a string during discovery procedure:

* The module of the given name is linked with a specific value of Module ID.
* The option of the given name is linked with a specific value of Option ID.

The Module ID and Option ID associated with the module and the option name can vary between devices, but they do not change between connections to the same device and firmware version.

If you are developing a custom host tool, you need to ensure your tool performs the following steps for the module discovery procedure:

1. Obtain the number of configurable application modules using the ``CONFIG_STATUS_GET_MAX_MOD_ID`` request.
   The response returns the highest value of Module ID available on the device.
#. Read the module descriptor of every application module (iterate Module ID from ``0`` up to and including the maximum supported Module ID).

For reading the module descriptor, the following conditions must be met:

* The module descriptor is read using fetch request with the current Module ID while Option ID is set to ``0``.
  The transaction must be performed multiple times, with responses for following requests containing strings that are part of the module descriptor.
* The end line character (``\n``) indicates the end of the descriptor.
  After the end line character is fetched, following requests should loop around and repeat descriptor strings.
* The module descriptor strings are provided in a predefined order, but the host should make no assumption about the descriptor string that will be provided by the device as the first one.
* The duplicated string is fetched after the host has received all of the strings related to the given module.
  The host can stop the option discovery procedure when a duplicated string is fetched.
* The module name should be provided as the first string in the module descriptor.
* The following strings should indicate the module option names.
  The first option on the descriptor must be identified with Option ID equal to ``1``.
  The following options will be identified by monotonically increasing Option ID values (second option by ``2``, third by ``3``, and so on).
* Once the descriptor is read, the module options can be accessed.
  When performing a set or fetch request on the option, the Event ID contains the Option ID and the Module ID of the module that owns the option.

The module descriptor can contain an optional ``module_variant`` option.
The nRF Desktop application modules come in various variants with different characteristics.
The ``module_variant`` option allows to differentiate which module variant was compiled into the firmware.
For example, :ref:`nrf_desktop_motion` uses this option to identify a motion sensor model.

Handling configuration channel in firmware
******************************************

To enable the configuration channel in the nRF Desktop firmware, set the :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE <config_desktop_app_options>` Kconfig option.
This option also enables the mandatory :ref:`nrf_desktop_info`.

Make sure you also configure the following configuration channel elements:

* `Transport configuration`_
* `Listener configuration`_

Transport configuration
=======================

The HID configurator uses the HID feature reports to exchange the data.

Depending on the connection method:

* If the device is connected through USB, data exchange between the device and the host is handled by the :ref:`nrf_desktop_usb_state` in the functions :c:func:`get_report` and :c:func:`set_report`.
* If the device is connected over Bluetooth® LE, data exchange between the device and the host is handled in :ref:`nrf_desktop_hids` in :c:func:`feature_report_handler`.
  The argument :c:data:`write` indicates whether the report is a GATT write (set report) or a GATT read (get report).

  Forwarding requests through a dongle to a connected peripheral is handled in :ref:`nrf_desktop_hid_forward`.
  The dongle, which is a Bluetooth® LE Central, uses the HID Client module to find the feature report of the paired device and access it in order to forward the request.
  The request forwarding is based on Recipient ID, which is assigned by the :ref:`nrf_desktop_hid_forward`.
  From the script user perspective, the device can be identified using type, board name or Hardware ID.

.. note::
   If the Low Latency Packet Mode (LLPM) connection interval is in use, the Bluetooth Peripheral can provide either a HID input report or a GATT write response during a single connection event.

   To prevent HID input report rate drop while forwarding config channel report set operation, nRF Desktop Dongle can forward the data using GATT write without response.
   In that case, the peripheral does not have to provide the GATT write response instead of sending the HID input report.

   The "GATT write without response" operation cannot be performed on the HID feature report.
   To allow the "GATT write without response", the Peripheral must provide an additional HID output report.
   Use the :ref:`CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT <config_desktop_app_options>` Kconfig option in the nRF Desktop peripheral configuration to add the mentioned HID output report.
   Disabling this option reduces the memory consumption.

The :c:struct:`config_event` is used to propagate the configuration channel data.
The configuration channel request received from the host is propagated using the mentioned event with :c:member:`config_event.is_request` set to ``true``.
The application module that handles the request consumes the event and provides the response.
The response is provided as :c:struct:`config_event` with :c:member:`config_event.is_request` set to ``false``.
In case a request is not handled by any application module, the configuration channel transport will eventually receive it and generate an error response.

Listener configuration
======================

The configuration channel listener is an application module that provides a set of options that are accessible through the configuration channel.
For example, depending on the listener, it can provide the CPI option from :ref:`nrf_desktop_motion` or the option for searching for a new peer from :ref:`nrf_desktop_ble_bond`.
The host computer can use set or fetch request to access these options.

On the firmware side, the configuration channel listener and its options are referenced with module ID and option ID numbers, respectively.

On the host side, these IDs are translated to strings based on the registered listener and option names.
Details are described in the :ref:`nrf_desktop_config_channel_script`.

To register an application module as a configuration channel listener, complete the following steps:

1. Make sure that the application module is an :ref:`app_event_manager` listener.
#. Include the :file:`config_event.h` header.
#. Subscribe for the :c:struct:`config_event` using the :c:macro:`APP_EVENT_SUBSCRIBE_EARLY` macro:

   .. code-block:: c

       APP_EVENT_LISTENER(MODULE, app_event_handler);
       #if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
       APP_EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
       #endif

   The module should subscribe only if the configuration channel is enabled.

   .. note::
      The module must be an early subscriber to make sure it will receive the event before the configuration channel transports (:ref:`nrf_desktop_usb_state` and :ref:`nrf_desktop_hids`).
      Otherwise, the module may not receive the configuration channel requests at all.
      In that case an error responses will be generated by configuration channel transport.

#. Call :c:macro:`GEN_CONFIG_EVENT_HANDLERS` in the :ref:`app_event_manager` event handler function registered by the application module:

   .. code-block:: c

       static bool app_event_handler(const struct app_event_header *aeh)
       {
           /* Functions used to handle other events. */
           ...

           GEN_CONFIG_EVENT_HANDLERS(STRINGIFY(MODULE), opt_descr,
                                     config_set, config_get);

           /* Functions used to handle other events. */
           ...
       }

   You must provide the following arguments to the macro:

   * Module name - String representing the module name (``STRINGIFY(MODULE)``).
   * Array with the names of the module's options (``opt_descr``):

     .. code-block:: c

         /* Creating enum to denote the module options is recommended,
          * because it makes code more readable.
          */
         enum test_module_opt {
             TEST_MODULE_OPT_FILTER_PARAM,
             TEST_MODULE_OPT_PARAM_BLE,
             TEST_MODULE_OPT_PARAM_WIFI,

             TEST_MODULE_OPT_COUNT
         };

         static const char * const opt_descr[] = {
             [TEST_MODULE_OPT_FILTER_PARAM] = "filter_param",
             [TEST_MODULE_OPT_PARAM_BLE] = "param_ble",
             [TEST_MODULE_OPT_PARAM_WIFI] = "param_wifi"
         };

   * Set operation handler (:c:func:`config_set`):

     .. code-block:: c

         static void config_set(const uint8_t opt_id, const uint8_t *data,
                                const size_t size)
         {
             switch (opt_id) {
             case TEST_MODULE_OPT_FILTER_PARAM:
                 /* Handle the data received under the "data" pointer.
                  * Number of received bytes is described as "size".
                  */
                 if (size != sizeof(struct filter_parameters)) {
                     LOG_WRN("Invalid size");
                 } else {
                     update_filter_params(data);
                 }
             break;

             case TEST_MODULE_OPT_PARAM_BLE:
                 /* Handle the data. */
                 ....
             break;

             /* Handlers for other option IDs. */
             ....

             default:
                 /* The option is not supported by the module. */
                 LOG_WRN("Unknown opt %" PRIu8, opt_id);
                 break;
             }
         }

   * Fetch operation handler (:c:func:`config_get`):

     .. code-block:: c

         static void config_get(const uint8_t opt_id, uint8_t *data, size_t *size)
         {
             switch (opt_id) {
             case TEST_MODULE_OPT_FILTER_PARAM:
                 /* Fill the buffer under the "data" pointer with
                  * requested data. Number of written bytes must be
                  * reflected by the value under the "size" pointer.
                  */
                 memcpy(data, filter_param, sizeof(filter_param));
                 *size = sizeof(filter_param);
                 break;

             case TEST_MODULE_OPT_PARAM_BLE:
                 /* Handle the request. */
                 ....
                 break;

             /* Handlers for other option IDs. */
             ....

             default:
                 /* The option is not supported by the module. */
                 LOG_WRN("Unknown opt: %" PRIu8, opt_id);
                 break;
             }
         }

.. note::
   A configuration channel listener can specify its variant by providing an option named :c:macro:`OPT_DESCR_MODULE_VARIANT`.
   On a fetch operation of this option, the module must provide an array of characters that represents the module variant.

   * The :ref:`nrf_desktop_motion` uses the module variant to specify the motion sensor model.
   * The :ref:`nrf_desktop_config_channel_script` uses the module variant to provide a separate description of the configurable module for every module variant.

For an example of a module that uses the configuration channel, see the following files:

* :file:`src/modules/ble_qos.c`
* :file:`src/modules/led_stream.c`
* :file:`src/modules/dfu.c`
* :file:`src/hw_interface/motion_sensor.c`

Dependencies
************

The configuration channel uses the :ref:`app_event_manager` events to propagate the configuration data.

Dependencies for the host software are described in the :ref:`nrf_desktop_config_channel_script`.

API documentation
*****************

The following API is used by the configuration channel transports.
The configurable application modules (configuration channel listeners) do not use it.

| Header file: :file:`applications/nrf_desktop/src/util/config_channel_transport.h`
| Source file: :file:`applications/nrf_desktop/src/util/config_channel_transport.c`

.. doxygengroup:: config_channel_transport
   :project: nrf
   :members:
