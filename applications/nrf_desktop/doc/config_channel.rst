.. _nrf_desktop_config_channel:

Configuration channel
#####################

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

The following table shows the format of requests used to exchange data in the HID feature reports.

.. _nrf_desktop_table:

+-------------------------------------------------------------------+
| Feature report                                                    |
+-----------+-----+-----+----------+--------+-------------+---+-----+
| 0         | 1   | 2   | 3        | 4      | 5           | 6 | ... |
+===========+=====+=====+==========+========+=============+===+=====+
| Report ID | Recipient | Event ID | Status | Data length | Data    |
+-----------+-----------+----------+--------+-------------+---------+

Each feature report contains the following components:

* Report ID - HID report identifier of the feature report used for transmitting data.
* Recipient - Product identifier of the device to which the request is addressed.
  Needed to route requests in a multi-device setup.
* Event ID - Identifier of the value that should be set or fetched; consists of a module ID and an option ID.
* Status - Value used to exchange status of the request; also used by sender to ask for fetch.
* Data length - Value that indicates how many bytes of data the request holds.
* Data - Arbitrary length data connected to the request.

.. note::
   Bluetooth LE HID Service removes the leading report ID byte, resulting in firmware obtaining a data frame 1 byte shorter.

   The USB HID class transmits the whole report, including the report ID byte.


Handling configuration channel in firmware
==========================================

To enable the configuration channel in the nRF Desktop firmware, set the ``CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE`` Kconfig option.
This option also enables the mandatory :ref:`nrf_desktop_info`.

Make sure you also configure the following configuration channel elements:

* `Transport configuration`_
* `Listener configuration`_

Transport configuration
-----------------------

The HID configurator uses the HID feature reports to exchange the data.

Depending on the connection method:

* If the device is connected through USB, requests are handled by the :ref:`nrf_desktop_usb_state` in the functions :cpp:func:`get_report` and :cpp:func:`set_report`.
* If the device is connected over Bluetooth LE, requests are handled in :ref:`nrf_desktop_hids` in :cpp:func:`feature_report_handler`.
  The argument :c:data:`write` indicates whether the report is a GATT write (set report) or a GATT read (get report).

  Forwarding requests through a dongle to a connected peripheral is handled in :ref:`nrf_desktop_hid_forward`.
  The dongle, which is a Bluetooth LE central, uses the HID Client module to find the feature report of the paired device and access it in order to forward the configuration request.
  The report forwarding is based on the peripheral device PID.

Listener configuration
----------------------

The listener can provide a set of options that are accessible through the configuration channel.
For example, depending on listener, it can provide the CPI option from :ref:`nrf_desktop_motion` or the option for searching for new peer from :ref`ble_bond`.
The host computer can use report set or report get for these options to access the option value.

On the firmware side, the configuration channel listener and its options are referenced with numbers, respectively module ID and option IDs.

On the host side, these IDs are translated to strings based on the registered listener and option names.
Details are described in the :ref:`nrf_desktop_config_channel_script`.

To register an application module as a configuration channel listener, complete the following steps:

1. Make sure that the application module is an :ref:`event_manager` listener.
#. Include the :file:`config_event.h` header.
#. Subscribe for the ``config_event`` and ``config_fetch_request_event`` using the :c:macro:`EVENT_SUBSCRIBE` macro:

   .. code-block:: c

       EVENT_LISTENER(MODULE, event_handler);
       #if CONFIG_DESKTOP_CONFIG_CHANNEL_ENABLE
       EVENT_SUBSCRIBE(MODULE, config_event);
       EVENT_SUBSCRIBE(MODULE, config_fetch_request_event);
       #endif

   The module should subscribe only if the configuration channel is enabled.
#. Call :c:macro:`GEN_CONFIG_EVENT_HANDLERS` in the :ref:`event_manager` event handler function registered by the application module:

   .. code-block:: c

       static bool event_handler(const struct event_header *eh)
       {
           /* Functions used to handle other events. */
           ...

           GEN_CONFIG_EVENT_HANDLERS(STRINGIFY(MODULE), opt_descr,
                         config_set, config_get, false);

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

   * Set report handler (:cpp:func:`config_set`):

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

   * Get report handler (:cpp:func:`config_get`):

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

   * Boolean indicating if the module is the final subscriber for the configuration channel events.
     It should be set to ``false`` for every subscriber, except for :ref:`nrf_desktop_info`.

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

| Header file: :file:`applications/nrf_desktop/src/util/config_channel.h`
| Source file: :file:`applications/nrf_desktop/src/util/config_channel.c`

.. doxygengroup:: config_channel_transport
   :project: nrf
   :members:
