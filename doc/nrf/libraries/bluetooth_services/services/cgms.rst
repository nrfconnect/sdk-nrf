.. _cgms_readme:

GATT Continuous Glucose Monitoring Service (CGMS)
#################################################

.. contents::
   :local:
   :depth: 2

Overview
********

This library implements the Continuous Glucose Monitoring Service with the corresponding set of characteristics defined in the `Continuous Glucose Monitoring Service Specification`_.

Supported features
==================

All the mandatory CGMS features are supported.

Configuration
*************

Set the maximum number of glucose measurement records stored in the device using the :kconfig:option:`CONFIG_BT_CGMS_MAX_MEASUREMENT_RECORD` Kconfig option.
The value should be large enough to hold all records generated in a session.

Set the logging level of the CGMS library using the :kconfig:option:`CONFIG_BT_CGMS_LOG_LEVEL_CHOICE` Kconfig option.

Usage
*****

To use CGMS in your application, call the :c:func:`bt_cgms_init` function.
Then, call the :c:func:`bt_cgms_measurement_add` function to pass the measurement result of glucose concentration to CGMS.

API documentation
*****************

| Header file: :file:`include/bluetooth/services/cgms.h`
| Source files: :file:`subsys/bluetooth/services/cgms`

.. doxygengroup:: bt_cgms
   :project: nrf
   :members:
