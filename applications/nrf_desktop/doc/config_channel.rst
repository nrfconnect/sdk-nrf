.. _nrf_desktop_config_channel:

Configuration channel
#####################

.. contents::
   :local:
   :depth: 2

Use the configuration channel to exchange data between your host computer and nRF Desktop's HID device.

For example, among the types of data that you can send through the configuration channel are the following:

* Device configuration parameters, for example mouse sensor CPI.
* Firmware updates.
* LED effect display data, after it has been generated on the computer.

For instructions on how to install and use the configuration channel on a host computer, see the :ref:`nrf_desktop_config_channel_script`.

Behavior
********

The HID feature reports are used for transporting information between the host and the connected embedded device.
The cross-platform `HIDAPI library`_ is used for exchanging the reports.
The library supports both Bluetooth LE and USB.

The host computer can set configuration values of the embedded device.
It can also request fetching a value, for example in order to display it to the user.

Setting a configuration value
    .. msc::
       hscale = "1.3";
       Host,Device;
       Host>>Device      [label="set report: set value 1600"];
       Host<<Device      [label="get report: SUCCESS"];

    All exchanges, both setting and getting a report, are initiated by the host.
    Therefore, during a fetch operation, the host polls the device until the data is available.

Fetching a configuration value
    .. msc::
       hscale = "1.3";
       Host,Device;
       Host>>Device      [label="set report: request value fetch"];
       Host<<Device      [label="get report: PENDING"];
       Host<<Device      [label="get report: value 1600, SUCCESS"];

    Data from the host can be forwarded through a USB dongle to the connected device.
    The device must act as a Bluetooth LE peripheral in such case.

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
===========

The following table shows the format of requests and responses used to exchange data in the HID feature reports.

.. _nrf_desktop_table:

+-------------------------------------------------------------------+
| Feature report                                                    |
+-----------+-----------+----------+--------+-------------+---+-----+
| 0         | 1         | 2        | 3      | 4           | 5 | ... |
+===========+===========+==========+========+=============+===+=====+
| Report ID | Recipient | Event ID | Status | Data length | Data    |
+-----------+-----------+----------+--------+-------------+---------+

Each feature report contains the following components:

* Report ID - HID report identifier of the feature report used for transmitting data.
* Recipient - Identifier of the device to which the request is addressed.
  Needed to route requests in a multi-device setup.

     * A recipient value that equals ``0`` means that configuration channel frame is intended for a HID device that is directly connected.
     * Other recipient value means that the frame should be forwarded to the peripheral connected over Bluetooth LE.

* Event ID - Identifier of the value that should be set or fetched; consists of a module ID and an option ID.
* Status - Value used to exchange status of the request; also used by sender to specify the requested operation.
* Data length - Value that indicates how many bytes of data the request/response holds.
* Data - Arbitrary length data connected to the request/response.

.. note::
   Bluetooth LE HID Service removes the leading report ID byte, resulting in firmware obtaining a data frame 1 byte shorter.

   The USB HID class transmits the whole report, including the report ID byte.


Handling configuration channel in firmware
==========================================

To enable the configuration channel in the nRF Desktop firmware, set the :option:`CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE` Kconfig option.
This option also enables the mandatory :ref:`nrf_desktop_info`.

Make sure you also configure the following configuration channel elements:

* `Transport configuration`_
* `Listener configuration`_

Transport configuration
-----------------------

The HID configurator uses the HID feature reports to exchange the data.

Depending on the connection method:

* If the device is connected through USB, requests are handled by the :ref:`nrf_desktop_usb_state` in the functions :c:func:`get_report` and :c:func:`set_report`.
* If the device is connected over Bluetooth LE, requests are handled in :ref:`nrf_desktop_hids` in :c:func:`feature_report_handler`.
  The argument :c:data:`write` indicates whether the report is a GATT write (set report) or a GATT read (get report).

  Forwarding requests through a dongle to a connected peripheral is handled in :ref:`nrf_desktop_hid_forward`.
  The dongle, which is a Bluetooth LE central, uses the HID Client module to find the feature report of the paired device and access it in order to forward the configuration request.
  The report forwarding is based on recipient, which is assigned by :ref:`nrf_desktop_hid_forward`.
  The :ref:`nrf_desktop_config_channel_script` holds the mentioned recipient internally and uses it in configuration channel data frames.
  From the script user perspective, the device can be identified using type, board name or hardware ID.

.. note::
   If the Low Latency Packet Mode (LLPM) connection interval is in use, the Bluetooth peripheral can provide either HID input report or config channel response during single connection event.

   To prevent HID input report rate drop while forwarding config channel report set operation, nRF Desktop Dongle can forward the data using GATT write without response.
   In that case, the peripheral does not have to provide response instead of sending HID input report.

   The GATT write without response operation cannot be performed on HID feature report.
   To allow GATT write without response, the peripheral must provide an additional HID output report.
   Use the :option:`CONFIG_DESKTOP_CONFIG_CHANNEL_OUT_REPORT` Kconfig option in nRF Desktop peripheral configuration to add the mentioned HID output report.
   Disabling this option reduces the memory consumption.

The :c:struct:`config_event` is used to propagate the configuration channel data.
The configuration channel request received from host is propagated using the mentioned event with :c:member:`config_event.is_request` set to ``true``.
The application module that handles the request consumes the event and provides the response.
The response is provided as :c:struct:`config_event` with :c:member:`config_event.is_request` set to ``false``.
In case a request is not handled by any application module, the configuration channel transport will eventually receive it and generate an error response.

Listener configuration
----------------------

The configuration channel listener is an application module that provides a set of options that are accessible through the configuration channel.
For example, depending on listener, it can provide the CPI option from :ref:`nrf_desktop_motion` or the option for searching for new peer from :ref:`nrf_desktop_ble_bond`.
The host computer can use set or fetch operation for these options to access the option value.

On the firmware side, the configuration channel listener and its options are referenced with numbers, respectively module ID and option IDs.

On the host side, these IDs are translated to strings based on the registered listener and option names.
Details are described in the :ref:`nrf_desktop_config_channel_script`.

To register an application module as a configuration channel listener, complete the following steps:

1. Make sure that the application module is an :ref:`event_manager` listener.
#. Include the :file:`config_event.h` header.
#. Subscribe for the :c:struct:`config_event` using the :c:macro:`EVENT_SUBSCRIBE_EARLY` macro:

   .. code-block:: c

       EVENT_LISTENER(MODULE, event_handler);
       #if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
       EVENT_SUBSCRIBE_EARLY(MODULE, config_event);
       #endif

   The module should subscribe only if the configuration channel is enabled.

   .. note::
      The module must be an early subscriber to make sure it will receive the event before the configuration channel transports (:ref:`nrf_desktop_usb_state` and :ref:`nrf_desktop_hids`).
      Otherwise, the module may not receive the configuration channel requests at all.
      In that case an error responses will be generated by configuration channel transport.

#. Call :c:macro:`GEN_CONFIG_EVENT_HANDLERS` in the :ref:`event_manager` event handler function registered by the application module:

   .. code-block:: c

       static bool event_handler(const struct event_header *eh)
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

The configuration channel uses the :ref:`event_manager` events to propagate the configuration data.

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
