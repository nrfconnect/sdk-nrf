.. _ug_matter_device_adding_bt_services:

Adding Bluetooth LE services to Matter application
##################################################

.. contents::
   :local:
   :depth: 2

You can integrate a variety of Bluetooth® Low Energy (LE) services to your Matter-enabled application.
The Bluetooth LE services can run next to the Matter stack and the Bluetooth LE Arbiter class can advertise them in a specific prioritization order.

You can add a Bluetooth LE service in one of the following ways:

* Taking advantage of :ref:`Bluetooth services <lib_bluetooth_services>` and their related samples in the |NCS|.
  You can use the library functions for configuring services and follow the available samples to learn how to integrate the services into your application.
* Using `Bluetooth SIG's Assigned Numbers`_ documentation with predefined characteristics to create your custom implementation.
* Writing the custom Bluetooth LE service from scratch.

This guide describes the first case and uses :ref:`nus_service_readme` as an example.

.. _ug_matter_device_adding_bt_services_ble_arbiter:

Advertising arbiter for Bluetooth LE services
*********************************************

The Bluetooth LE Arbiter is a class implemented in the Zephyr platform within the Matter SDK.
It enables easier coexistence of application components that want to advertise their Bluetooth LE services, for example during :ref:`ug_matter_network_topologies_commissioning_stages_discovery`.

When several application services are active, they can define a request to the Arbiter with the desired priority.
If the service with the highest priority stops advertising, the Arbiter automatically selects the next service in the queue.
If the service that requests advertising has a higher priority than the service that is running when the request is submitted, the advertising is restarted using parameters defined in the new request.

In the Matter implementation in the |NCS|, the following services are configured by default for all Matter applications:

* Commissioning for Matter (priority ``0``)
* DFU over Bluetooth LE (priority ``uint8 max``)

Lower number has a higher priority.
If you implement a custom Bluetooth LE service, you need to prioritize it with the Bluetooth LE Arbiter.

For more information about what the class offers, see the `Bluetooth LE Arbiter's header file`_.

.. _ug_matter_device_adding_bt_services_adding:

Adding support for a Bluetooth LE service
*****************************************

To add support for a Bluetooth LE service implemented in the |NCS|, complete the following steps:

1. Check the documentation of the :ref:`Bluetooth service <lib_bluetooth_services>` for its characteristics and functions.
#. Add the application code that instantiates and fills the Bluetooth LE Arbiter's ``Request`` structure, including its ``priority`` field:

   .. code-block:: cpp

      struct Request : public sys_snode_t
      {
         uint8_t priority;                     ///< Advertising request priority. Lower value means higher priority
         uint32_t options;                     ///< Advertising options: bitmask of BT_LE_ADV_OPT_XXX constants from Zephyr
         uint16_t minInterval;                 ///< Minimum advertising interval in 0.625 ms units
         uint16_t maxInterval;                 ///< Maximum advertising interval in 0.625 ms units
         Span<const bt_data> advertisingData;  ///< Advertising data fields
         Span<const bt_data> scanResponseData; ///< Scan response data fields
         OnAdvertisingStarted onStarted;       ///< (Optional) Callback invoked when the request becomes top-priority.
         OnAdvertisingStopped onStopped;       ///< (Optional) Callback invoked when the request stops being top-priority.
      };

#. Add the application code that calls the Bluetooth LE Arbiter's ``InsertRequest`` function, with the reference to the ``Request`` structure, when the service needs to be advertised:

   .. code-block:: cpp

      CHIP_ERROR InsertRequest(Request & request);

   This adds the service to the Arbiter queue.

.. _ug_matter_device_adding_bt_services_example:

Example: Bluetooth LE with Nordic UART Service implementation
=============================================================

The :ref:`Matter door lock sample <matter_lock_sample>` provides an optional configuration of :ref:`matter_lock_sample_ble_nus`, which you can use as an example implementation of a Bluetooth LE service.
Once enabled, you can use this configuration to declare NUS commands specific to the door lock sample and use them to control the device remotely through Bluetooth LE.

The following steps correspond to the implementation steps above, with the NUS as example:

1. *Check the documentation:* The NUS characteristics are listed on the :ref:`nus_service_readme` documentation page.
#. *Add the application code that instantiates and fills the struct:* Using the information from the documentation page, `bt_nus_service.cpp`_ and `bt_nus_service.h`_ are created.
   The ``Init`` method in the CPP file contains the implementation of the Bluetooth LE Arbiter's ``Request`` structure.
#. *Add the application code that calls the ``InsertRequest`` function:* In the CPP file, the service is started with the call to ``StartServer``, which contains a reference to ``InsertRequest``.
