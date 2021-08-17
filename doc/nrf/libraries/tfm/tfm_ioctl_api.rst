.. _lib_tfm_ioctl_api:

TF-M input/output control (IOCTL)
#################################

.. contents::
   :local:
   :depth: 2

The TF-M IOTCL library provides an API for platform-specific TF-M services.
These services are added by the ``platform`` partition.
The APIs are available for non-secure applications.

For more information on implementing TF-M in your application, see :ref:`ug_tfm`.

Functionality
*************

Platform-specific services are internally accessed through the :c:func:`tfm_platform_hal_ioctl` function.
Wrapper functions for these accesses are defined in :file:`tfm_ioctl_ns_api.c` and :file:`tfm_ioctl_s_api.c`.

The supported platform services are defined by :c:struct:`fm_platform_ioctl_request_types_t` in :file:`tfm_ioctl_api.h`.

.. literalinclude:: ../../../../include/tfm/tfm_ioctl_api.h
    :language: c
    :start-at: /** @brief Supported request types.
    :end-before: /** @brief Argument list for each platform read service.

Set the :option:``CONFIG_TFM_PARTITION_PLATFORM`` Kconfig option to enable the services.

See the :ref:`tfm_hello_world` sample for example usage.

Prerequisites
*************

This library requires that TF-M is installed on the device.


API documentation
*****************

| Header file: :file:`include/tfm/tfm_ioctl_api.h`
| Source files: :file:`modules/tfm/tfm/boards/src/`

.. doxygengroup:: tfm_ioctl_api
   :project: nrf
   :members:
