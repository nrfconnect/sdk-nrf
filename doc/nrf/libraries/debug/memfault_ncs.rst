.. _mod_memfault:

Memfault
########

.. contents::
   :local:
   :depth: 2

The Memfault module provides an integration of `Memfault SDK`_ into |NCS| and expands the functionality of the SDK with some options that are specific to |NCS|.
To get started with Memfault integration in |NCS|, see :ref:`ug_memfault`.

`Memfault SDK`_ is an SDK for embedded devices to make use of services that `Memfault`_ offers on their platform.
For official documentation for Memfault SDK and the Memfault platform in general, see `Memfault Docs`_.

.. note::
   When building applications with :ref:`Trusted Firmware-M <ug_tfm>` (TF-M), the faults resulting from memory access in secure regions are not caught by Memfault's fault handler.
   Instead, they are handled by TF-M.
   This means that those faults are not reported to the Memfault platform.

Configuration
*************

In general, configuration of `Memfault SDK`_ in |NCS| is done using Kconfig options.
Kconfig options are defined both in `Memfault SDK`_ and in the integration layer, which integrates the SDK into |NCS|.
Kconfig options of the integration layer are located in ``modules/memfault/Kconfig``.
These Kconfig options are differentiated by two distinct prefixes:

* ``CONFIG_MEMFAULT_`` - Prefix for options defined in Memfault SDK
* ``CONFIG_MEMFAULT_NCS_`` - Prefix for options defined in |NCS|

Additionally, |NCS| has an implementation of the :ref:`mds_readme`.
Use the :kconfig:option:`CONFIG_BT_MDS` option to enable it.
The MDS cannot be used concurrently with HTTP transport.

Configuration files
===================

.. memfault_config_files_start

If you just want to do a quick test with a sample, disable the :kconfig:option:`CONFIG_MEMFAULT_USER_CONFIG_ENABLE` option in the :file:`prj.conf` file to avoid adding the user configuration files.
Otherwise, follow the instructions below.

Memfault SDK requires three files in the include path during the build process.
Add a new folder called :file:`config` into your project and add the following three files:

* :file:`memfault_platform_config.h` - Sets Memfault SDK configurations that are not covered by Kconfig options.
* :file:`memfault_metrics_heartbeat_config.def` - Defines application-specific metrics.
* :file:`memfault_trace_reason_user_config.def` - Defines application-specific trace reasons.

For more information, see the `Memfault nRF Connect SDK integration guide`_.
You can use the files in the :ref:`memfault_sample` sample as a reference.
To have these configuration files in the include path, add the following to the :file:`CMakeLists.txt` file:

.. code-block:: console

   zephyr_include_directories(config)

.. memfault_config_files_end


Configuration options in Memfault SDK
=====================================

Following are some of the configuration options that Memfault SDK defines:

* :kconfig:option:`CONFIG_MEMFAULT_SHELL`
* :kconfig:option:`CONFIG_MEMFAULT_RAM_BACKED_COREDUMP`
* :kconfig:option:`CONFIG_MEMFAULT_RAM_BACKED_COREDUMP_SIZE`
* :kconfig:option:`CONFIG_MEMFAULT_COREDUMP_COLLECT_DATA_REGIONS`
* :kconfig:option:`CONFIG_MEMFAULT_COREDUMP_COLLECT_BSS_REGIONS`
* :kconfig:option:`CONFIG_MEMFAULT_HTTP_ENABLE`
* :kconfig:option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_INTERVAL_SECS`
* :kconfig:option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD_USE_DEDICATED_WORKQUEUE`
* :kconfig:option:`CONFIG_MEMFAULT_EVENT_STORAGE_SIZE`
* :kconfig:option:`CONFIG_MEMFAULT_CLEAR_RESET_REG`
* :kconfig:option:`CONFIG_MEMFAULT_METRICS`
* :kconfig:option:`CONFIG_MEMFAULT_METRICS_DEFAULT_SET_ENABLE`
* :kconfig:option:`CONFIG_MEMFAULT_HTTP_PERIODIC_UPLOAD`
* :kconfig:option:`CONFIG_MEMFAULT_ROOT_CERT_STORAGE_NRF9160_MODEM`

For more details on each option, use ``menuconfig`` or ``guiconfig``, and see the Kconfig sources in ``modules/lib/memfault-firmware-sdk/ports/zephyr/Kconfig``.

.. note::

   The Memfault shell is enabled by default, using the UART interface.
   If :ref:`lib_at_host` library and the Memfault module are enabled simultaneously, both will not behave as expected, as they both require the same UART interface.
   Therefore, it is recommended to enable only one of these at a time.
   To disable the Memfault shell, you need to disable the Kconfig options :kconfig:option:`CONFIG_MEMFAULT_SHELL` and :kconfig:option:`CONFIG_MEMFAULT_NRF_SHELL`.

Configuration options in |NCS|
==============================

The Kconfig options for Memfault that are defined in |NCS| provide some additional features compared to the options that are already implemented in Memfault SDK:

* :kconfig:option:`CONFIG_MEMFAULT_NCS_PROJECT_KEY`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_PROVISION_CERTIFICATES`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_INTERNAL_FLASH_BACKED_COREDUMP`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_LTE_METRICS`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_STACK_METRICS`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_BT_METRICS`

The |NCS| integration of `Memfault SDK`_ provides default values for some metadata that is required to identify the firmware when it is sent to Memfault cloud.
You can control the defaults by using the configuration options below:

* :kconfig:option:`CONFIG_MEMFAULT_NCS_DEVICE_ID`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_HW_VERSION`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_FW_TYPE`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_FW_VERSION_STATIC`
* :kconfig:option:`CONFIG_MEMFAULT_NCS_FW_VERSION_PREFIX`

The Kconfig options for :kconfig:option:`CONFIG_BT_MDS` are the following:

* :kconfig:option:`CONFIG_BT_MDS_MAX_URI_LENGTH`
* :kconfig:option:`CONFIG_BT_MDS_PERM_RW`
* :kconfig:option:`CONFIG_BT_MDS_PERM_RW_ENCRYPT`
* :kconfig:option:`CONFIG_BT_MDS_PIPELINE_COUNT`
* :kconfig:option:`CONFIG_BT_MDS_DATA_POLL_INTERVAL`

API documentation
*****************

| Header file: :file:`include/memfault_ncs.h`
| Source files: :file:`modules/memfault/`

.. doxygengroup:: memfault_ncs
   :project: nrf
   :members:
