.. _ug_matter_device_optimizing_memory:

Optimizing memory usage in Matter applications
##############################################

.. contents::
   :local:
   :depth: 2

You can use different approaches to optimizing the memory usage of your Matter application, both on the |NCS| side and in the Matter SDK.

Reducing memory usage on the |NCS| side
***************************************

See the :ref:`app_memory` guide for information about how to reduce memory usage for the |NCS| generally and for specific subsystems in particular, including Bluetooth® LE, Matter, and Thread.

.. _ug_matter_device_optimizing_memory_logs:

Cutting off log regions for Matter SDK modules
**********************************************

The Matter SDK, included in the |NCS| as one of the submodule repositories using a `dedicated Matter fork`_, provides a custom mechanism for optimizing memory usage in a Matter application.
This solution defines a series of logging modules, each of which is a logical section of code that is a source of log messages and can include one or more files.
For the complete list of modules, see the Matter SDK's `LogModule enumeration`_.

You can reduce the memory usage of each of these modules in the following ways:

* Define a custom logging level for each module by setting one of the `Matter SDK logging levels <Matter SDK's Logging header_>`_ (Error, Progress, Detail, or Automation) in :file:`src/chip_project_config.h`,  located in your application's directory.
  For example, the following snippet shows the Bluetooth LE module set to logging at the `Detail` level:

  .. code-block:: c

     CHIP_CONFIG_LOG_MODULE_Ble_DETAIL 1

* Turn off logging for any of the modules by setting its respective enabler to ``0`` in the Matter SDK's `LogModule enumeration`_.
  For example, the following snippet shows the Bluetooth LE module cut off from logging its entries:

  .. code-block:: c

     CHIP_CONFIG_LOG_MODULE_Ble 0
