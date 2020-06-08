.. _ug_thread:

Working with Thread
###################

The |NCS| provides support for developing applications using the Thread protocol.

.. _thread_ug_intro:

Introduction
************

The |NCS|'s implementation is based on the OpenThread stack, which is integrated into Zephyr.
The integration with the stack and the radio driver is ensured by Zephyr's L2 layer, which acts as intermediary with Thread on the |NCS| side.

OpenThread is a portable and flexible open-source implementation of the Thread networking protocol, created by Nest in active collaboration with Nordic to accelerate the development of products for the connected home.

Among others, OpenThread has the following main advantages:

* A narrow platform abstraction layer that makes the OpenThread platform-agnostic.
* Small memory footprint.
* Support for system-on-chip (SoC), network co-processor (NCP) and radio co-processor (RCP) designs.
* Official Thread certification.

For more information about some aspects of Thread, see the following pages:

.. toctree::
   :maxdepth: 1

   ug_thread_stack_architecture.rst
   ug_thread_ot_integration.rst
   ug_thread_commissioning.rst

You can also find more at `OpenThread.io`_ and `Thread Group`_ pages.

.. _thread_ug_supported features:

Supported features
******************

The OpenThread implementation of the Thread protocol supports all features defined in the Thread 1.1.1 specification.
This includes:

* All Thread networking layers:

  * IPv6
  * 6LoWPAN
  * IEEE 802.15.4 with MAC security
  * Mesh Link Establishment
  * Mesh Routing

* All device roles
* Border Router support

Required modules
****************

Thread requires the following Zephyr's modules to properly operate in |NCS|:

* :ref:`zephyr:ieee802154_interface` radio driver - This library is automatically enabled when working with OpenThread on Nordic Semiconductor's Development Kits.
* :ref:`zephyr:settings_api` subsystem - This is required to allow Thread to store settings in the non-volatile memory.

Mandatory configuration
***********************

To use the Thread protocol in |NCS|, set the following Kconfig options:

* :option:`CONFIG_NET_L2_OPENTHREAD` - This option enables the OpenThread stack required for the correct operation of the Thread protocol and allows you to use them.
* :option:`CONFIG_CPLUSPLUS` - This option must be enabled, because OpenThread is implemented in C++.
* :option:`CONFIG_REBOOT` - This option is needed to ensure safe reboot.
* :option:`CONFIG_ENTROPY_GENERATOR` - Required by both OpenThread and radio driver.
* Options related to the Settings subsystem in the ``storage_partition`` partition of the internal flash:

  * :option:`CONFIG_SETTINGS`
  * :option:`CONFIG_FLASH`
  * :option:`CONFIG_FLASH_PAGE_LAYOUT`
  * :option:`CONFIG_FLASH_MAP`
  * :option:`CONFIG_MPU_ALLOW_FLASH_WRITE`
  * :option:`CONFIG_NVS`

* General setting options related to network configuration:

  * :option:`CONFIG_NETWORKING`
  * :option:`CONFIG_NET_UDP`
  * :option:`CONFIG_NET_SOCKETS`

IPv6 mandatory configuration
============================

The Thread protocol can only be used with IPv6.
IPv4 is not supported.

Enable the following options to make Thread work over IPv6:

* :option:`CONFIG_NET_IPV6`
* :option:`CONFIG_NET_CONFIG_NEED_IPV6`

Additionally, since Thread by default registers a considerable amount of IP addresses, the default IPv6 address count values must be increased.
Set the following options to the provided values:

* :option:`CONFIG_NET_IF_UNICAST_IPV6_ADDR_COUNT` to ``6``
* :option:`CONFIG_NET_IF_MCAST_IPV6_ADDR_COUNT` to ``8``

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

If you want to manually enable the Thread network Commissioner role on a device, set the following Kconfig options to the provided values:

* :option:`CONFIG_OPENTHREAD_COMMISSIONER` to ``y``.
* :option:`CONFIG_MBEDTLS_HEAP_SIZE` to ``10240``.

To enable the Thread network Joiner role on a device, set the following Kconfig options to the provided values:

* :option:`CONFIG_OPENTHREAD_JOINER` to ``y``.
* :option:`CONFIG_MBEDTLS_HEAP_SIZE` to ``10240``.

The MBEDTLS heap size needs to be increased for both Commissioner and Joiner, because the joining process is memory-consuming and requires at least 10 KB of RAM.

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

* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_ERROR` - Enables logging only for errors.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_WARNING` - Enables logging for errors and warnings.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_INFO` - Enables logging for informational messages, errors, and warnings.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_DEBUG` - Enables logging for debug messages, informational messages, errors, and warnings.

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

Available drivers, libraries, and samples
*****************************************

See :ref:`openthread_samples` for the list of available Thread samples.

Available Thread tools
**********************

When working with Thread in |NCS|, you can use the following tools during Thread application development:

* `nRF Thread Topology Monitor`_ - This desktop application helps to visualize the current network topology.
* `nRF Sniffer for 802.15.4 based on nRF52840 with Wireshark`_ - Tool for analyzing network traffic during development.

Using Thread tools is optional.

----

Copyright disclaimer
    |Google_CCLicense|
