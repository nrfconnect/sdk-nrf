.. _dult_readme:

Detecting Unwanted Location Trackers (DULT)
###########################################

.. contents::
   :local:
   :depth: 2

The Detecting Unwanted Location Trackers (DULT) library implements a set of functionalities required for :ref:`ug_dult` with the |NCS|.
The implementation is based on the official `DULT`_ specification, which lists a set of best practices and protocols for products with built-in location tracking capabilities.
Following the specification improves the privacy and safety of individuals by preventing the location tracking products from tracking users without their knowledge or consent.

Accessory non-owner service (ANOS)
**********************************

The DULT library implements the accessory non-owner service (ANOS), which is a GATT service that uses the accessory non-owner characteristic to communicate with other devices.
The ANOS uses UUID of ``15190001-12F4-C226-88ED-2AC5579F2A85``.
The accessory non-owner characteristic manages the `DULT Accessory Information`_ and the `DULT Non-owner controls`_ defined in the `DULT`_ specification.

.. _dult_configuration:

Configuration
*************

Set the :kconfig:option:`CONFIG_DULT` Kconfig option to enable the module.
This Kconfig option depends on the :kconfig:option:`CONFIG_BT` option that enables the Bluetooth® stack.

The following Kconfig options are also available for this module:

* :kconfig:option:`CONFIG_DULT_BATTERY` - This option enables support for battery information such as battery type and battery level.
  The battery information is an optional feature in the DULT specification.
  By default, this option is disabled.

  * :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_CRITICAL_THR`, :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_LOW_THR`, and :kconfig:option:`CONFIG_DULT_BATTERY_LEVEL_MEDIUM_THR` - These options allow to configure the mapping between a battery level expressed as a percentage value and battery levels defined in the `DULT`_ specification.
    The default values are set to ``10``, ``40``, and ``80`` respectively.
  * :kconfig:option:`CONFIG_DULT_BATTERY_TYPE_POWERED`, :kconfig:option:`CONFIG_DULT_BATTERY_TYPE_NON_RECHARGEABLE`, and :kconfig:option:`CONFIG_DULT_BATTERY_TYPE_RECHARGEABLE` - These options allow to choose the device's declared battery type.
    By default, the :kconfig:option:`CONFIG_DULT_BATTERY_TYPE_POWERED` is chosen.

* There are following ANOS configuration options for the DULT module:

  * :kconfig:option:`CONFIG_DULT_BT_ANOS_ID_PAYLOAD_LEN_MAX` - This option allows to configure the maximum length of the accessory-locating network identifier.
    The default value is set to ``18``. The identifier is defined by the accessory-locating network that the accessory belongs to.
  * :kconfig:option:`CONFIG_DULT_BT_ANOS_INDICATION_COUNT` - This option allows to configure the number of simultaneously processed GATT indications by the ANOS.
    The default value is set to ``2``.

See the Kconfig help for details.

Implementation details
**********************

The implementation uses :c:macro:`BT_GATT_SERVICE_DEFINE` to statically define and register the ANOS.
Because of that, the ANOS is still present in the GATT database after the DULT subsystem is disabled.
In the DULT subsystem disabled state, GATT operations on the ANOS are rejected.

The ANOS handles all requests received from the outer world.
In case of an application input needed to handle a GATT operation, the DULT subsystem calls the appropriate registered application callback.
For more details, see the :ref:`Integration steps <ug_integrating_dult>` section of the DULT integration guide.

API documentation
*****************

| Header file: :file:`include/dult.h`
| Source files: :file:`subsys/dult`

.. doxygengroup:: dult
   :project: nrf
   :members:
