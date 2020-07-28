.. _ug_thread_configuring:

Configuring Thread in |NCS|
###########################

This page describes what is needed to start working with Thread in |NCS|:

.. contents::
    :local:
    :depth: 2

Required modules
****************

Thread requires the following Zephyr's modules to properly operate in |NCS|:

* :ref:`zephyr:ieee802154_interface` radio driver - This library is automatically enabled when working with OpenThread on Nordic Semiconductor's Development Kits.
* :ref:`zephyr:settings_api` subsystem - This is required to allow Thread to store settings in the non-volatile memory.

Mandatory configuration
***********************

To use the Thread protocol in |NCS|, set the following Kconfig options:

* :option:`CONFIG_NET_L2_OPENTHREAD` - This option enables the OpenThread stack required for the correct operation of the Thread protocol and allows you to use them.

* General setting options related to network configuration:

  * :option:`CONFIG_NETWORKING`
  * :option:`CONFIG_NET_SOCKETS` - This option can be omitted if OpenThread is meant to work only in the NCP architecture.

Optional configuration
**********************

Depending on your configuration needs, you can also set the following options:

* :option:`CONFIG_NET_SHELL` - This option enables Zephyr's :ref:`zephyr:shell_api` if you need to access OpenThread CLI.
* :option:`CONFIG_COAP` - This option enables Zephyr's :ref:`zephyr:coap_sock_interface` support.
* :option:`CONFIG_OPENTHREAD_COAP` - This option enables OpenThread's native CoAP API.

You can also change the default values for the following options:

* :option:`CONFIG_OPENTHREAD_CHANNEL` - By default set to ``11``.
  You can set any value ranging from ``11`` to ``26``.
* :option:`CONFIG_OPENTHREAD_PANID` - By default set to ``43981``.
  You can set any value ranging from ``0`` to ``65535``.

For other optional configuration options, see the following sections:

.. contents::
    :local:
    :depth: 2

Thread commissioning
====================

Thread commissioning is the process of adding new Thread devices to the network.
It involves two devices: a Commissioner that is already in the Thread network and a Joiner that wants to become a member of the network.

Configuring this process is optional, because the Thread :ref:`openthread_samples` in |NCS| use hardcoded network information.

If you want to manually enable the Thread network Commissioner role on a device, set the following Kconfig option to the provided value:

* :option:`CONFIG_OPENTHREAD_COMMISSIONER` to ``y``.

To enable the Thread network Joiner role on a device, set the following Kconfig option to the provided value:

* :option:`CONFIG_OPENTHREAD_JOINER` to ``y``.

You can also configure how the commissioning process is to be started:

* Automatically after Joiner's power up with the :option:`CONFIG_OPENTHREAD_JOINER_AUTOSTART` option, configured for the Joiner device.
* Started from the application.
* Triggered by Command Line Interface commands.
  In this case, the shell stack size must be increased to at least 3 KB by setting the following option:

  * :option:`CONFIG_SHELL_STACK_SIZE` to ``3072``.

For more details about the commissioning process, see `Thread Commissioning on OpenThread portal`_.

OpenThread stack logging options
================================

The OpenThread stack logging is handled with the following options:

* :option:`CONFIG_LOG` - This option enables Zephyr's :ref:`zephyr:logging_api`.
* :option:`CONFIG_OPENTHREAD_DEBUG` - This option enables logging for the OpenThread stack.

Both options must be enabled to allow logging.

This said, enabling logging is optional, because it is enabled by default for all Thread samples.
However, you must set one of the following logging levels to start receiving the logging output:

* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_CRIT` - critical error logging only.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_WARN` - enable warning logging in addition to critical errors.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_NOTE` - additionally enable notice logging.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_INFO` - additionally enable informational logging.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_DEBG` - additionally enable debug logging.

Zephyr L2 logging options
=========================

If you want to get logging output related to the Zephyr's L2 layer, enable one of the following Kconfig options:

* :option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_ERR` - Enables logging only for errors.
* :option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_WRN` - Enables logging for errors and warnings.
* :option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_INF` - Enables logging for informational messages, errors, and warnings.
* :option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_DBG` - Enables logging for debug messages, informational messages, errors, and warnings.

Choosing one of these options will enable writing the appropriate information in the L2 debug log.

Additionally, enabling :option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_DBG` allows you to set the :option:`CONFIG_OPENTHREAD_L2_DEBUG` option, which in turn has the following settings:

* :option:`CONFIG_OPENTHREAD_L2_DEBUG_DUMP_15_4`
* :option:`CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6`

These options enable dumping 802.15.4 or IPv6 frames (or both) in the debug log output.

You can disable writing to log with the :option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_OFF` option.

.. _thread_ug_device_type:

Switching device type
=====================

An OpenThread device can be configured to run as Full Thread Device (FTD) or Minimal Thread Device (MTD).
Both device types serve different roles in the Thread network.
An FTD can be both Router and End Device, while an MTD can only be an End Device.

You can configure the device type using the following Kconfig options:

* :option:`CONFIG_OPENTHREAD_FTD` - Enables the Full Thread Device (FTD) thread. This is the default configuration if none is selected.
* :option:`CONFIG_OPENTHREAD_MTD` - Enables the Minimal Thread Device (MTD) thread.

By default, when a Thread device is configured as MTD, it operates as Minimal End Device (MED).
You can choose to make it operate as Sleepy End Device (SED) by enabling the :option:`CONFIG_OPENTHREAD_MTD_SED` option.

For more information, see `Device Types on OpenThread portal`_.
