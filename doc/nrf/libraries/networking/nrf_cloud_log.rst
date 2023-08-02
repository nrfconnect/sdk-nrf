.. _lib_nrf_cloud_log:

nRF Cloud Log
#############

.. contents::
   :local:
   :depth: 2

This library is an enhancement to the :ref:`lib_nrf_cloud` library.
It enables applications to generate and transmit messages that comply with the Logs feature of nRF Cloud.
Log messages are compact JSON documents generated directly by an application if the nRF Cloud logging backend is not used.
If the nRF Cloud logging backend is enabled, Log messages encapsulate `Zephyr Logging`_ output.
The logging backend can either use JSON encoding or `Dictionary-based Logging`_ encoding.

Overview
********

This library provides an API for either REST-based or MQTT-based applications to send logs to nRF Cloud.
For MQTT-based applications, you can enable or disable logging as well as change the logging level remotely using the nRF Cloud portal or `nRF Cloud Patch Device State`_ REST API.
For REST-based applications, the enabled state and logging level can be controlled at compile time or at run time on the device, but not from the cloud.

Each JSON log message contains the following elements:

* ``appId`` - Set to ``LOG``.
* ``ts`` - Timestamp if accurate time is known, in the Unix epoch.
* ``seq`` - Sequence number to help order messages sent simultaneously, as well as to sort messages used when no timestamp is available.
* ``dom`` - Logging domain ID; usually 0.
* ``src`` - Name of the C function or subsystem which generated the log.
* ``lvl`` - Log level:

  * 0 = NONE (off)
  * 1 = ERR
  * 2 = WRN
  * 3 = INF
  * 4 = DBG

* ``msg`` - The message string.

The library sets the sequence number and timestamp automatically.

Log messages displayed by a device through a UART or RTT connection show timestamps that reset to ``0`` when the device resets.
Cloud-based log messages show timestamps with real date and time values for when each message was generated.
Using real date and time for timestamps is important because log messages are stored by default for up to 30 days.

The `nRF Cloud`_ website displays a device's log messages and enables the user to download historical log messages.
Log messages can also be retrieved using `the ListMessages REST API <nRF Cloud REST API ListMessages_>`_.

Supported features
==================

If the :ref:`lib_date_time` library is enabled and the current date and time are known, the timestamp field of the log is automatically set accordingly.
The sequence number is set to a monotonically-increasing value that resets to ``0`` when the device restarts.

Supported backends
==================

When so configured, this library includes a Zephyr logging backend that can transport log messages to nRF Cloud using MQTT or REST.
The logging backend can also use either JSON messages or dictionary-based compact binary messages.

Multiple JSON log messages are sent together as a JSON array to the `d2c/bulk device message topic <nRF Cloud MQTT Topics_>`_.
The nRF Cloud backend splits the array into individual JSON messages for display.
Using the bulk topic reduces the data transfer size as MQTT or REST message overhead is consolidated for multiple messages.

Multiple dictionary-based log messages are sent together as a binary message to the `d2c/bin device message topic <nRF Cloud MQTT Topics_>`_.
The nRF Cloud portal does not display the contents of the dictionary-based log messages in real time.
Instead, you must download a binary file containing the logs over a certain range of time, then decode the logs using a Python script and the dictionary built when the application was built.

Requirements
************

The device must be connected to nRF Cloud before calling the :c:func:`nrf_cloud_log_send` function.
The :c:func:`nrf_cloud_rest_log_send` function initiates the connection as needed.

Configuration
*************

Configure one or both of the following Kconfig options to enable direct log messages or the logging backend:

* :kconfig:option:`CONFIG_NRF_CLOUD_LOG_DIRECT`
* :kconfig:option:`CONFIG_NRF_CLOUD_LOG_BACKEND`

If only the first is enabled:

* Calls to the direct log message functions :c:func:`nrf_cloud_log_send()` and :c:func:`nrf_cloud_rest_log_send()` send messages direct to nRF Cloud immediately.
* The cloud logging backend is not available, and consequently, no Zephyr log messages are transmitted to the cloud.

If only the second is enabled:

* Only the logging backend is present and all enabled Zephyr log messages of the proper level are sent to the cloud.
* Direct log message functions are not available.

If both options are enabled, calls to the direct log message functions are passed to the logging backend instead.

Configure one of the following options to select the data transport method:

* :kconfig:option:`CONFIG_NRF_CLOUD_MQTT` or :kconfig:option:`CONFIG_NRF_CLOUD_REST`

Configure the message encoding:

* :kconfig:option:`CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_TEXT` or :kconfig:option:`CONFIG_LOG_BACKEND_NRF_CLOUD_OUTPUT_DICTIONARY`

See `Dictionary-based Logging`_ to learn how dictionary-based logging works, how the dictionary is built, and how to decode the binary log output.
Dictionary logs are compact binary log messages that require decoding using an offline script.
As such, dictionary logs are up to 60% smaller than JSON logs, but cannot be viewed in the nRF Cloud user interface in real time.
Instead, the user interface displays a link from which you can download a single binary file containing the logs.
To successfully decode dictionary logs, you must use the :file:`log_dictionary.json` file built by the build system at the same time as the firmware image.
If you modify the source code and build the firmware image again, the :file:`log_dictionary.json` file may change.
Keep track of each firmware image and the :file:`log_dictionary.json` file when a device runs different firmware images.

Configure the default log level to be sent to the cloud:

* :kconfig:option:`CONFIG_NRF_CLOUD_LOG_OUTPUT_LEVEL` set to ``0`` for NONE (to disable), ``1`` for ERR, ``2`` for WRN, ``3`` for INF, or ``4`` for DBG.

For fine run-time control of log levels for each logging source, configure the following:

* :kconfig:option:`CONFIG_LOG_RUNTIME_FILTERING`

See `Run-time Filtering`_ for more information.

Finally, configure these additional options:

* :kconfig:option:`CONFIG_LOG_MODE_DEFERRED`
* :kconfig:option:`CONFIG_LOG_PROCESS_THREAD_STACK_SIZE` set to ``4096``.
* :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` set to the maximum size of buffered log data before transmission to the cloud.
* :kconfig:option:`CONFIG_LOG_PROCESS_THREAD_SLEEP_MS` set to the maximum time log messages can be buffered before transmission to the cloud.
* :kconfig:option:`CONFIG_LOG_PRINTK` to ``n`` so that periodic messages from the :ref:`lte_lc_readme` library do not get sent to the cloud.

See :ref:`configure_application` for information on how to change configuration options.

Usage
*****

To use this library, complete the following steps:

1. Include the :file:`nrf_cloud_log.h` file.
#. If the :kconfig:option:`CONFIG_NRF_CLOUD_LOG_DIRECT` Kconfig option is enabled, call the :c:func:`nrf_cloud_log_send` function when connected to nRF Cloud using MQTT or :c:func:`nrf_cloud_rest_log_send` when using REST.
#. If the :kconfig:option:`CONFIG_NRF_CLOUD_LOG_BACKEND` option is enabled, use the normal Zephyr logging macros :c:macro:`LOG_ERR`, :c:macro:`LOG_WRN`, :c:macro:`LOG_INF`, or :c:macro:`LOG_DBG`, as well as the ``_HEXDUMP_ forms``.

Samples using the library
*************************

The following |NCS| samples use this library:

* :ref:`nrf_cloud_multi_service`
* :ref:`nrf_cloud_rest_device_message`

Limitations
***********

For REST-based applications, you can disable or set a log level for logs only at compile time.

Dependencies
************

This library uses the following |NCS| libraries:

* :ref:`lib_nrf_cloud`
* :ref:`lib_nrf_cloud_rest`
* :ref:`lib_date_time`

API documentation
*****************

| Header file: :file:`include/net/nrf_cloud_log.h`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/nrf_cloud_log.c`
| Source files: :file:`subsys/net/lib/nrf_cloud/src/nrf_cloud_log_backend.c`

.. doxygengroup:: nrf_cloud_log
   :project: nrf
   :members:
