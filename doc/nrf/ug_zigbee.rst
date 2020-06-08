.. _ug_zigbee:

Working with Zigbee
###################

The |NCS| provides support for developing applications using the Zigbee protocol.

.. _zigbee_ug_intro:

Introduction
************

Zigbee is a portable, low-power software networking protocol that provides connectivity over an 802.15.4-based mesh network.
It also defines an application layer that provides interoperability among all vendors.

In combination with |NCS| and the integrated Zephyr RTOS, Zigbee allows for easy development of low-power connected solutions.

For more information about Zigbee, visit the `Zigbee Alliance`_ page and read `Zigbee Specification`_.

.. _zigbee_ug_supported_features:

Supported features
******************

The |NCS|'s Zigbee protocol uses the ZBOSS library, a third-party precompiled Zigbee stack.
It includes all mandatory features of the |zigbee_version| specification and provides an Application Programming Interface to access different services.
The stack comes with the following features:

* Complete implementation of the Zigbee core specification and Zigbee Pro feature set.
* Support for all device roles: Coordinator, Router, and End Device.
* Zigbee Cluster Library.
* Base Device Behavior.
* Devices, described in former Zigbee Home Automation and Light Link profiles.
* Zigbee Green Power Proxy Basic.
* Experimental support for ``ZB_ZCL_WWAH``.

See :ref:`nrfxlib:zboss` for more information.

.. _zigbee_ug_libs:

Required libraries and drivers
******************************

Zigbee requires the following modules to properly operate in |NCS|:

* :ref:`nrfxlib:zboss` available in nrfxlib, with the OSIF subsystem acting as the linking layer between the ZBOSS stack and |NCS|.
  OSIF implements a series of functions used by ZBOSS and is included in the |NCS|'s Zigbee subsystem.
  The files that handle the OSIF integration are located in :file:`nrf/subsys/zigbee/osif`.
* :ref:`zephyr:ieee802154_interface` radio driver - This library is automatically enabled when working with Zigbee on Nordic Semiconductor's Development Kits.

.. _zigbee_ug_configuration:

Mandatory configuration
***********************

To use the Zigbee protocol, set the :option:`CONFIG_ZIGBEE` Kconfig option.
Setting this option enables all the peripherals required for the correct operation of the Zigbee protocol and allows you to use them.

After that, you have to define the Zigbee device role for the Zigbee application or sample by setting one of the following Kconfig options:

* Router role: :option:`CONFIG_ZIGBEE_ROLE_ROUTER`
* End Device role: :option:`CONFIG_ZIGBEE_ROLE_END_DEVICE`
* Coordinator role: :option:`CONFIG_ZIGBEE_ROLE_COORDINATOR`

Setting any of these options enables the respective ZBOSS role library.
This is needed because End Devices use different libraries than Routers and Coordinators.

For instructions about how to set Kconfig options, see :ref:`configure_application`.

.. _zigbee_ug_sed:

Sleepy End Device behavior
==========================

The Sleepy End Device (SED) behavior is a Zigbee stack feature that enables the sleepy behavior for the end device.

By default, the end device regularly polls its parent for data.
When the SED behavior is enabled and no frames are available for reception after the last poll, the SED disables its radio until the next scheduled poll.
The Zigbee stack's own scheduler informs the application about periods of time when nothing is scheduled for any of the device roles.
This allows the stack to enter the sleep state during these periods, which also powers off some peripherals for the SED.

When the Zigbee stack thread goes to sleep, the Zigbee thread can enter the suspend state for the same amount of time as the stack's sleep.
The thread will be automatically resumed after the sleep period is over or on an event.

In the Zigbee samples in |NCS|, the sleepy behavior can be triggered by pressing a predefined button when the device is booting.
This action results in calling the ZBOSS API that activates this feature.
See the :ref:`light switch sample <zigbee_light_switch_sample>` for a demonstration.

.. note::
    This feature does not require enabling any additional options in Kconfig.

Power saving during sleep
-------------------------

With the sleepy behavior enabled, the unused part of RAM memory is powered off, which allows to lower the power consumption even more.
The sleep current of MCU can be lowered to about 1.8 uA by completing the following steps:

1. Turn off logger by setting :option:`CONFIG_LOG` to ``n``.
#. Turn off UART by setting :option:`CONFIG_SERIAL` to ``n``.
#. Enable Zephyr's tickless kernel by setting :option:`CONFIG_TICKLESS_KERNEL` to ``y``.
#. For current measurements for |nRF52840DK| or |nRF52833DK|, set **SW6** to ``nRF ONLY`` position to get the desired results.

Optional configuration
**********************

After enabling the Zigbee protocol and defining the Zigbee device role, you can enable additional options in Kconfig and modify `Stack configuration options`_.

You can enable the following additional configuration options:

* One of the following alternative options for selecting the channel on which the Zigbee device can operate:

  * :option:`CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_SINGLE` - Single mode is enabled by default.
    The default channel is set to 16.
    To set a different channel, edit the :option:`CONFIG_ZIGBEE_CHANNEL` option to the desired value.
  * :option:`CONFIG_ZIGBEE_CHANNEL_SELECTION_MODE_MULTI` - In this mode, you get all the channels enabled by default.
    To configure a custom set of channels in the range from 11 to 26, edit the :option:`CONFIG_ZIGBEE_CHANNEL_MASK` option.
    For example, you can set channels 13, 16, and 21.
    You must have at least one channel enabled with this option.
* :option:`CONFIG_ZIGBEE_VENDOR_OUI` - Represents MAC Address Block Large, and by default it is set to Nordic Semiconductor's MA-L block (f4-ce-36).
* :option:`CONFIG_ZIGBEE_CLI_LOG_ENABLED` - Enables logging of the incoming ZCL frames, and it is enabled by default.

Stack configuration options
===========================

Zigbee is initialized after Zephyr's kernel start.
The ZBOSS stack works on a separate Zephyr thread that is created and started with :cpp:func:`zigbee_enable`.
This function must be called by the application.

The ZBOSS thread can be configured using the following options:

* :option:`CONFIG_ZBOSS_DEFAULT_THREAD_PRIORITY` - Defines thread priority; set to 3 by default.
* :option:`CONFIG_ZBOSS_DEFAULT_THREAD_STACK_SIZE` - Defines the size of the thread stack; set to 2048 by default.

Custom logging per module
=========================

Logging is handled with the :option:`CONFIG_LOG` option.
This option enables logging for both the stack and Zephyr's :ref:`zephyr:logging_api` API.

Stack logs
----------

The stack logs are independent from Zephyr's :ref:`zephyr:logging_api` API.
To customize them, use the following options:

* :option:`CONFIG_ZBOSS_ERROR_PRINT_TO_LOG` - Allows the application to log ZBOSS error names; enabled by default.
* :option:`CONFIG_ZBOSS_TRACE_MASK` - Sets the modules from which ZBOSS will log the debug messages with :option:`CONFIG_ZBOSS_TRACE_LOG_LEVEL`; no module is set by default.

The stack logs are provided in a binary (hex dump) format.

Zephyr's logger options
-----------------------

Zephyr's :ref:`zephyr:logging_api` starts with the default ``ERR`` logging level (only errors reported).
This level is used by default by the application.

You can configure custom logger options for each Zigbee and ZBOSS module.
To do this, configure the related Kconfig option for one or more modules that you want to customize the logging for:

* :option:`CONFIG_ZBOSS_TRACE_LOG_LEVEL`
* :option:`CONFIG_ZBOSS_OSIF_LOG_LEVEL`
* :option:`CONFIG_ZIGBEE_CLI_LOG_LEVEL`
* :option:`CONFIG_ZIGBEE_HELPERS_LOG_LEVEL`

For each of the modules, you can set the following logging options:

* ``LOG_LEVEL_OFF`` - Turns off logging for this module.
* ``LOG_LEVEL_ERR`` - Enables logging only for errors.
* ``LOG_LEVEL_WRN`` - Enables logging for errors and warnings.
* ``LOG_LEVEL_INF`` - Enables logging for informational messages, errors, and warnings.
* ``LOG_LEVEL_DBG`` - Enables logging for debug messages, informational messages, errors, and warnings.

For example, setting :option:`CONFIG_ZBOSS_TRACE_LOG_LEVEL_INF` will enable logging of informational messages, errors, and warnings for the ZBOSS Trace module.

.. _zigbee_ug_available:

Available drivers, libraries, and samples
*****************************************

See :ref:`zigbee_samples` for the list of available Zigbee samples.

.. _zigbee_ug_tools:

Available Zigbee tools
**********************

When working with Zigbee in |NCS|, you can use the following tools during Zigbee application development:

* `nRF Sniffer for 802.15.4 based on nRF52840 with Wireshark`_ - Tool for analyzing network traffic during development.

Using Zigbee tools is optional.
