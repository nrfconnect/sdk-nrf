.. _mod_memfault:

Memfault
########

.. contents::
   :local:
   :depth: 2

The Memfault module provides an integration of `Memfault SDK`_ into |NCS| and expands the functionality of the SDK with some options that are specific to |NCS|.


Overview
********

`Memfault SDK`_ is an SDK for embedded devices to make use of services that `Memfault`_ offers on their platform.

The SDK provides a functionality to collect debug information from devices in the form of coredumps, error traces, metrics, logs and more.
Information that is collected from the device can be sent to Memfault's cloud solution for further analysis.
Communication with the Memfault Cloud is handled by APIs available in Memfault SDK, while the integration code of the SDK in |NCS| provides some additional functionality.

For official documentation for Memfault SDK and the Memfault platform in general, see `Memfault Docs`_.

See the :ref:`memfault_sample` sample for an example of Memfault implementation in |NCS|.

Memfault in |NCS| currently supports the nRF9160-based build targets.


Using Memfault SDK
******************

The SDK is part of the West manifest in |NCS| and is automatically downloaded when running ``west update``.
By default, it is downloaded to ``<nRF Connect SDK path>/modules/lib/memfault-firmware-sdk/``.

To include Memfault in your build, enable the Kconfig option :kconfig:`CONFIG_MEMFAULT`.
The APIs in Memfault SDK can then be linked into your application.

In addition, you must configure a Memfault project key using :kconfig:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`.

To get access to all the benefits, like up to 100 free devices connected, register at the `Memfault registration page`_.


Configuration
*************

In general, configuration of `Memfault SDK`_ in |NCS| is done using Kconfig options.
Kconfig options are defined both in `Memfault SDK`_ and in the integration layer, which integrates the SDK into |NCS|.
Kconfig options of the integration layer are located in ``modules/memfault/Kconfig``.
These Kconfig options are differentiated by two distinct prefixes:

* ``CONFIG_MEMFAULT_`` - Prefix for options defined in Memfault SDK
* ``CONFIG_MEMFAULT_NCS_`` - Prefix for options defined in |NCS|


Configuration files
===================

.. memfault_config_files_start

If you just want to do a quick test with a sample, disable the :kconfig:`CONFIG_MEMFAULT_USER_CONFIG_ENABLE` option in the :file:`prj.conf` to avoid adding the user configuration files.
Otherwise, follow the instructions below.

Memfault SDK requires three files in the include path during the build process.
Add a new folder into your project called :file:`config` and add the following three files:

* ``memfault_platform_config.h`` - Sets Memfault SDK configurations that are not covered by Kconfig options
* ``memfault_metrics_heartbeat_config.def`` - Defines application-specific metrics
* ``memfault_trace_reason_user_config.def`` - Defines application-specific trace reasons

You can use the files in the :ref:`memfault_sample` sample as a reference.
To have these configuration files in the include path, add the following in the :file:`CMakeLists.txt` file:

.. code-block:: console

   zephyr_include_directories(config)

.. memfault_config_files_end


Configuration options in Memfault SDK
=====================================

Following are some of the configuration options that Memfault SDK define:

* :kconfig:`CONFIG_MEMFAULT_SHELL`
* :kconfig:`CONFIG_MEMFAULT_RAM_BACKED_COREDUMP`
* :kconfig:`CONFIG_MEMFAULT_RAM_BACKED_COREDUMP_SIZE`
* :kconfig:`CONFIG_MEMFAULT_COREDUMP_COLLECT_DATA_REGIONS`
* :kconfig:`CONFIG_MEMFAULT_COREDUMP_COLLECT_BSS_REGIONS`
* :kconfig:`CONFIG_MEMFAULT_HTTP_ENABLE`
* :kconfig:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS`
* :kconfig:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE`
* :kconfig:`CONFIG_MEMFAULT_EVENT_STORAGE_SIZE`
* :kconfig:`CONFIG_MEMFAULT_CLEAR_RESET_REG`
* :kconfig:`CONFIG_MEMFAULT_METRICS`
* :kconfig:`CONFIG_MEMFAULT_METRICS_DEFAULT_SET_ENABLE`
* :kconfig:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD`
* :kconfig:`CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM`

You can find more details on each option using ``menuconfig``, ``guiconfig``, and in the Kconfig sources in ``modules/lib/memfault-firmware-sdk/ports/zephyr/Kconfig``.

.. note::

   The Memfault shell is enabled by default, using the UART interface.
   If :ref:`lib_at_host` library and the memfault module are enabled simultaneously, both will not behave as expected, as they both require the UART same interface.
   Therefore, it is recommended to only enable one of these at the same time.
   To disable the Memfault shell, you need to disable the two configurations, ``CONFIG_MEMFAULT_SHELL`` and ``CONFIG_MEMFAULT_NRF_SHELL``.


Configuration options in |NCS|
==============================

The Kconfig options for Memfault that are defined in |NCS| provide some additional features compared to the options that are already implemented in Memfault SDK:

* :kconfig:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`
* :kconfig:`CONFIG_MEMFAULT_NCS_PROVISION_CERTIFICATES`
* :kconfig:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP`
* :kconfig:`CONFIG_MEMFAULT_NCS_LTE_METRICS`
* :kconfig:`CONFIG_MEMFAULT_NCS_STACK_METRICS`

The |NCS| integration of `Memfault SDK`_ provides default values for some metadata that are required to identify the firmware when it is sent to Memfault cloud.
These defaults can be controlled by using the configuration options below:

* :kconfig:`CONFIG_MEMFAULT_NCS_DEVICE_ID`
* :kconfig:`CONFIG_MEMFAULT_NCS_HW_VERSION`
* :kconfig:`CONFIG_MEMFAULT_NCS_FW_TYPE`
* :kconfig:`CONFIG_MEMFAULT_NCS_FW_VERSION_STATIC`
* :kconfig:`CONFIG_MEMFAULT_NCS_FW_VERSION_PREFIX`


API documentation
*****************

| Header file: :file:`include/memfault_ncs.h`
| Source files: :file:`modules/memfault/`

.. doxygengroup:: memfault_ncs
   :project: nrf
   :members:
