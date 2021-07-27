.. _ug_thread_configuring:

Configuring Thread in |NCS|
###########################

.. contents::
   :local:
   :depth: 2

This page describes what is needed to start working with Thread in |NCS|.

See :ref:`configure_application` for instructions on how to update the configuration for your application, permanently or temporarily.

Required modules
****************

Thread requires the following Zephyr modules to properly operate in |NCS|:

* :ref:`zephyr:ieee802154_interface` radio driver - This library is automatically enabled when working with OpenThread on Nordic Semiconductor's development kits.
* :ref:`zephyr:settings_api` subsystem - This subsystem is required to allow Thread to store settings in the non-volatile memory.

.. _ug_thread_configuring_basic:

Enabling OpenThread in |NCS|
****************************

To use the Thread protocol in |NCS|, set the following Kconfig options:

* :kconfig:`CONFIG_NETWORKING` - This option enables the generic link layer and the IP networking support.
* :kconfig:`CONFIG_NET_L2_OPENTHREAD` - This option enables the OpenThread stack required for the correct operation of the Thread protocol and allows you to use it.
* :kconfig:`CONFIG_MPSL` - This option enables the :ref:`nrfxlib:mpsl` (MPSL) implementation, which provides services for both :ref:`single-protocol and multi-protocol implementations <ug_thread_architectures>`.

.. _ug_thread_configuring_basic_building:

Selecting OpenThread libraries
******************************

After enabling OpenThread in |NCS|, you must choose which OpenThread libraries to use.
You can choose to either build the libraries from source or use pre-built variants of the libraries.

Building the OpenThread libraries from source gives you full flexibility in configuration.
Using pre-built variants can be useful for certification purposes.

Configure OpenThread to build from source
  Set :kconfig:`CONFIG_OPENTHREAD_SOURCES` to build the libraries from source.
  This option is selected by default.

  This alternative allows you to define :ref:`ug_thread_configuring_additional` one by one.
  By default, the :ref:`thread_ug_feature_sets` option is set to custom (:kconfig:`CONFIG_OPENTHREAD_USER_CUSTOM_LIBRARY`), which allows you to create your own OpenThread stack configuration.
  However, you can select other feature sets as a basis.

  When building the OpenThread libraries from source, you can also :ref:`update the pre-built OpenThread libraries <thread_ug_feature_updating_libs>`.

Configure OpenThread to use pre-built libraries
  Set :kconfig:`CONFIG_OPENTHREAD_LIBRARY_1_1` to use pre-built libraries.
  Select one of the :ref:`thread_ug_feature_sets` by enabling :kconfig:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER`, :kconfig:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_FTD`, or :kconfig:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD`.

  This alternative disables building OpenThread from source files and links pre-built libraries instead.

.. _ug_thread_configuring_additional:

Additional configuration options
********************************

Depending on your configuration needs, you can also set the following options:

* :kconfig:`CONFIG_NET_SOCKETS` - This option enables API similar to BSD Sockets on top of the native Zephyr networking API.
  This configuration is needed for managing networking protocols.
* :kconfig:`CONFIG_NET_SHELL` - This option enables Zephyr's :ref:`zephyr:net_shell`.
  This configuration is needed for managing the network, based on Zephyr's IP stack, from the command line.
* :kconfig:`CONFIG_OPENTHREAD_SHELL` - This option enables OpenThread CLI (see `OpenThread CLI Reference`_).
* :kconfig:`CONFIG_COAP` - This option enables Zephyr's :ref:`zephyr:coap_sock_interface` support.
* :kconfig:`CONFIG_COAP_UTILS` - This option enables the :ref:`CoAP utils library <coap_utils_readme>`.
* :kconfig:`CONFIG_OPENTHREAD_COAP` - This option enables OpenThread's native CoAP API.
* :kconfig:`CONFIG_OPENTHREAD_CHANNEL` - By default set to ``11``.
  You can set any value ranging from ``11`` to ``26``.
* :kconfig:`CONFIG_OPENTHREAD_PANID` - By default set to ``43981``.
  You can set any value ranging from ``0`` to ``65535``.

See the following files for more options that you might want to change:

* :file:`zephyr/subsys/net/l2/openthread/Kconfig.features` - OpenThread stack features.
* :file:`zephyr/subsys/net/l2/openthread/Kconfig.thread` - Thread network configuration options.

.. note::
    You can find the default configuration for all :ref:`openthread_samples` in the :file:`nrf/samples/openthread/common/overlay-ot-defaults.conf` file.

For other optional configuration options, see the following sections.

.. _ug_thread_configuring_eui64:

IEEE 802.15.4 EUI-64 configuration
==================================

.. include:: /includes/ieee802154_eui64_conf.txt

At the end of the configuration process, you can check the EUI-64 value using OpenThread CLI:

.. code-block:: console

   uart:~$ ot eui64
   8877665544332211
   Done

.. _ug_thread_configuring_crypto:

Hardware-accelerated cryptography
=================================

You can enable hardware-accelerated cryptography by using the :ref:`nrfxlib:nrf_security`.
To do this, modify the setting of the following Kconfig option:

* :kconfig:`CONFIG_OPENTHREAD_MBEDTLS` - Disable this option to disable the default mbedTLS configuration for OpenThread.
  The nrf_security module is enabled by default when mbedTLS for OpenThread is disabled.

For more configuration options, read the module documentation.

.. _thread_ug_thread_1_2:

Thread 1.2 Specification options
================================

The OpenThread stack can be configured to operate in compliance with either the Thread 1.1 Specification  or the :ref:`Thread 1.2 Specification <thread_ug_supported_features_v12>`.
You can change the stack version by using the following Kconfig options:

* :kconfig:`CONFIG_OPENTHREAD_THREAD_VERSION_1_1` - Selects the Thread stack version that is compliant with the Thread 1.1 Specification.
  This option is enabled by default if no other option is selected.
* :kconfig:`CONFIG_OPENTHREAD_THREAD_VERSION_1_2` - Selects the Thread stack version that is compliant with the Thread 1.2 Specification.

By selecting support for Thread 1.2, you enable the following features in addition to the :ref:`Thread 1.1 features <thread_ug_supported_features>`:

* Enhanced Frame Pending
* Enhanced Keep Alive
* Thread Domain Name
* Coordinated Sampled Listening (CSL) Transmitter (for Full Thread Devices only)

Moreover, Thread 1.2 also comes with the following features that are supported for development, but not production:

* Domain Unicast Addresses - Set :kconfig:`CONFIG_OPENTHREAD_DUA` to enable this feature.
* Multicast Listener Registration - Set :kconfig:`CONFIG_OPENTHREAD_MLR` to enable this feature.
* Backbone Router - Set :kconfig:`CONFIG_OPENTHREAD_BACKBONE_ROUTER` to enable this feature.
* Link Metrics - Set :kconfig:`CONFIG_OPENTHREAD_LINK_METRICS_INITIATOR` to enable the link metrics initiator functionality.
  Set :kconfig:`CONFIG_OPENTHREAD_LINK_METRICS_SUBJECT` to enable the link metrics subject functionality.
* Coordinated Sampled Listening (CSL) Receiver - Set :kconfig:`CONFIG_OPENTHREAD_CSL_RECEIVER` to enable this feature.

.. note::
   The Link Metrics and Coordinated Sampled Listening features are not supported for nRF53 Series devices yet.
   The Backbone Router feature enables the functionality for the Thread Network side, but not for the Backbone side.

To test Thread 1.2 options, you can use the :ref:`ot_cli_sample` sample with the :ref:`ot_cli_sample_thread_v12`.



Thread commissioning options
============================

Thread commissioning is the process of adding new Thread devices to the network.
See :ref:`thread_ot_commissioning` for more information.

Configuring this process is optional, because the :ref:`openthread_samples` in |NCS| use hardcoded network information.

If you want to manually enable the Thread network Commissioner role on a device, set the following Kconfig option to the provided value:

* :kconfig:`CONFIG_OPENTHREAD_COMMISSIONER` to ``y``.

To enable the Thread network Joiner role on a device, set the following Kconfig option to the provided value:

* :kconfig:`CONFIG_OPENTHREAD_JOINER` to ``y``.

You can also configure how the commissioning process is to be started.
The following options are available:

* Start automatically after the Joiner powers up.
  To configure this option, configure the :kconfig:`CONFIG_OPENTHREAD_JOINER_AUTOSTART` option for the Joiner device.
* Start from the application.
* Trigger by Command Line Interface commands.
  In this case, the shell stack size must be increased to at least 3 KB by setting the following option:

  * :kconfig:`CONFIG_SHELL_STACK_SIZE` to ``3168``.

For more details about the commissioning process, see `Thread Commissioning on OpenThread portal`_.

.. _thread_ug_logging_options:

OpenThread stack logging options
================================

You can enable the OpenThread stack logging for your project with the following options:

* :kconfig:`CONFIG_LOG` - This option enables Zephyr's :ref:`zephyr:logging_api`.
* :kconfig:`CONFIG_OPENTHREAD_DEBUG` - This option enables logging for the OpenThread stack.

Both options must be enabled to allow logging.

After setting these options, you can choose one of several :ref:`logging backends <ug_logging_backends>` available in Zephyr and supported in |NCS|.

.. note::
    If you are working with Thread samples, enabling logging and logging backend is optional.
    By default, all Thread samples have logging enabled in the :file:`overlay-ot-defaults.conf` file and are configured to provide output at the informational level (:kconfig:`CONFIG_OPENTHREAD_LOG_LEVEL_INFO`).

Logging levels
--------------

Select one of the following logging levels to customize the logging output:

* :kconfig:`CONFIG_OPENTHREAD_LOG_LEVEL_CRIT` - This option enables critical error logging only.
* :kconfig:`CONFIG_OPENTHREAD_LOG_LEVEL_WARN` - This option enables warning logging in addition to critical errors.
* :kconfig:`CONFIG_OPENTHREAD_LOG_LEVEL_NOTE` - This option additionally enables notice logging.
* :kconfig:`CONFIG_OPENTHREAD_LOG_LEVEL_INFO` - This option additionally enables informational logging.
* :kconfig:`CONFIG_OPENTHREAD_LOG_LEVEL_DEBG` - This option additionally enables debug logging.

The more detailed logging level you select, the more logging buffers you need to be able to see all messages.
The buffer size must also be increased.
Use the following Kconfig options for this purpose:

* :kconfig:`CONFIG_LOG_STRDUP_BUF_COUNT` - This option specifies the number of logging buffers.
* :kconfig:`CONFIG_LOG_STRDUP_MAX_STRING` - This option specifies the size of logging buffers.


Zephyr L2 logging options
=========================

If you want to get logging output related to Zephyr's L2 layer, enable one of the following Kconfig options:

* :kconfig:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_ERR` - Enables logging only for errors.
* :kconfig:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_WRN` - Enables logging for errors and warnings.
* :kconfig:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_INF` - Enables logging for informational messages, errors, and warnings.
* :kconfig:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_DBG` - Enables logging for debug messages, informational messages, errors, and warnings.

Choosing one of these options enables writing the appropriate information in the L2 debug log.

Additionally, enabling :kconfig:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_DBG` allows you to set the :kconfig:`CONFIG_OPENTHREAD_L2_DEBUG` option, which in turn has the following settings:

* :kconfig:`CONFIG_OPENTHREAD_L2_DEBUG_DUMP_15_4` - Enables dumping 802.15.4 frames in the debug log output.
* :kconfig:`CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6` - Enables dumping IPv6 frames in the debug log output.

You can disable writing to log with the :kconfig:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_OFF` option.

.. _thread_ug_device_type:

Device type options
===================

You can configure OpenThread devices to run as a specific :ref:`device type <thread_ot_device_types>`.

Full Thread Device (FTD)
  Set :kconfig:`CONFIG_OPENTHREAD_FTD` to configure the device as FTD.
  This is the default configuration.

Minimal Thread Device (MTD)
  Set :kconfig:`CONFIG_OPENTHREAD_MTD` to configure the device as MTD.

  By default, the MTD operates as Minimal End Device (MED).
  To make it operate as Sleepy End Device (SED), enabling :kconfig:`CONFIG_OPENTHREAD_MTD_SED`.

.. _thread_ug_prebuilt:

Pre-built libraries
*******************

The |NCS| provides a set of :ref:`nrfxlib:ot_libs`.
These pre-built libraries are available in nrfxlib and provide features and optional functionalities from the OpenThread stack.
You can use these libraries for building applications with support for the complete Thread 1.1 Specification.

To use a pre-built library, configure OpenThread to use pre-built libraries by setting the :kconfig:`CONFIG_OPENTHREAD_LIBRARY_1_1` Kconfig option and select one of the provided :ref:`thread_ug_feature_sets`.

.. _thread_ug_feature_sets:

Feature sets
============

A feature set defines a combination of OpenThread features for a specific use case.
These feature sets are mainly used for pre-built libraries, but you can also use them for selecting several configuration options at once when you :ref:`build your application using OpenThread sources <ug_thread_configuring_basic_building>`.

The |NCS| provides the following feature sets:

* :kconfig:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER` - Enable the complete set of OpenThread features for the Thread 1.1 Specification.
* :kconfig:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_FTD` - Enable optimized OpenThread features for FTD (Thread 1.1).
* :kconfig:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` - Enable optimized OpenThread features for MTD (Thread 1.1).
* :kconfig:`CONFIG_OPENTHREAD_USER_CUSTOM_LIBRARY` - Create a custom feature set for compilation when :ref:`building using OpenThread sources <ug_thread_configuring_basic_building>`.
  This option is the default.
  If you select :kconfig:`CONFIG_OPENTHREAD_LIBRARY_1_1`, choose a different feature set.

  .. note::
    When :ref:`building OpenThread from source <ug_thread_configuring_basic>`, you can select another feature set as base.
    You can then manually enable additional features, but you cannot disable features that are selected by the feature set.

The following table lists the supported features for each of these sets.
No tick indicates that there is no support for the given feature in the related configuration, while the tick signifies that the feature is selected (``=1`` value).

.. list-table::
    :widths: auto
    :header-rows: 1

    * - OpenThread feature
      - Master
      - Optimized_FTD
      - Optimized_MTD
      - Custom
    * - BORDER_AGENT
      - ✔
      -
      -
      -
    * - BORDER_ROUTER
      - ✔
      -
      -
      -
    * - CHILD_SUPERVISION
      - ✔
      - ✔
      - ✔
      -
    * - COAP
      - ✔
      - ✔
      - ✔
      -
    * - COAPS
      - ✔
      - ✔
      - ✔
      -
    * - COMMISSIONER
      - ✔
      -
      -
      -
    * - DIAGNOSTIC
      - ✔
      -
      -
      -
    * - DNS_CLIENT
      - ✔
      - ✔
      - ✔
      -
    * - DHCP6_SERVER
      - ✔
      -
      -
      -
    * - DHCP6_CLIENT
      - ✔
      - ✔
      - ✔
      -
    * - ECDSA
      - ✔
      - ✔
      - ✔
      -
    * - IP6_FRAGM
      - ✔
      - ✔
      - ✔
      -
    * - JAM_DETECTION
      - ✔
      - ✔
      - ✔
      -
    * - JOINER
      - ✔
      - ✔
      - ✔
      -
    * - LINK_RAW
      - ✔
      -
      -
      -
    * - MAC_FILTER
      - ✔
      - ✔
      - ✔
      -
    * - MTD_NETDIAG
      - ✔
      -
      -
      -
    * - SERVICE
      - ✔
      - ✔
      -
      -
    * - SLAAC
      - ✔
      - ✔
      - ✔
      -
    * - SNTP_CLIENT
      - ✔
      - ✔
      - ✔
      -
    * - UDP_FORWARD
      - ✔
      - ✔
      -
      -

For the full list of configuration options that were used during compilation, including their default values, see the :file:`openthread_lib_configuration.txt` file within each library folder in the nrfxlib repository.

.. _thread_ug_customizing_prebuilt:

Customizing pre-built libraries
===============================

Selecting a feature set allows you to use the respective OpenThread features in your application.
You might need to customize some configuration options to fit your use case though.

Be aware of the following limitations when customizing the configuration of a pre-built library:

* You can only update configuration options that are configurable at run time.
  If you change any options that are compiled into the library, your changes will be ignored.
* Changes to the configuration might impact the certification status of the pre-built libraries.
  See :ref:`ug_thread_cert_options` for more information.

The following list shows some of the configuration options that you might want to customize:

* :kconfig:`CONFIG_OPENTHREAD_FTD` or :kconfig:`CONFIG_OPENTHREAD_MTD` - Select the :ref:`device type <thread_ug_device_type>`.
  The :kconfig:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` feature set supports only the MTD device type.
  The other feature sets support both device types.
* :kconfig:`CONFIG_OPENTHREAD_COPROCESSOR` and :kconfig:`CONFIG_OPENTHREAD_COPROCESSOR_NCP` - Select the OpenThread architecture to use.
  See :ref:`thread_architectures_designs_cp`.
* :kconfig:`CONFIG_OPENTHREAD_MANUAL_START` - Choose whether to configure and join the Thread network automatically.
  If you set this option to ``n``, also check and configure the network parameters that are used, for example:

  * :kconfig:`CONFIG_OPENTHREAD_CHANNEL`
  * :kconfig:`CONFIG_OPENTHREAD_NETWORKKEY`
  * :kconfig:`CONFIG_OPENTHREAD_NETWORK_NAME`
  * :kconfig:`CONFIG_OPENTHREAD_PANID`
  * :kconfig:`CONFIG_OPENTHREAD_XPANID`

.. _thread_ug_feature_updating_libs:

Updating pre-built OpenThread libraries
=======================================

You can update the :ref:`nrfxlib:ot_libs` in nrfxlib when using any Thread sample if you configure the sample to build the OpenThread stack from source with :kconfig:`CONFIG_OPENTHREAD_SOURCES`.
Use this functionality for :ref:`certification <ug_thread_cert>` of your configuration of the OpenThread libraries, for example.

.. note::
    The libraries destination directory can differ.
    When you selected :kconfig:`CONFIG_OPENTHREAD_USER_CUSTOM_LIBRARY`, the location depends on the chosen :ref:`nrf_security backend <nrfxlib:nrf_security_readme>`, either :kconfig:`CONFIG_CC3XX_BACKEND` or :kconfig:`CONFIG_OBERON_BACKEND`.

Updating libraries without debug symbols
----------------------------------------

You can install the release version of the latest nrfxlib libraries without the debug symbols.
This is handled with the :kconfig:`CONFIG_OPENTHREAD_BUILD_OUTPUT_STRIPPED` Kconfig option.
This option is disabled by default.

Run the following command to update the nrfxlib libraries:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -t install_openthread_libraries -- -DOPENTHREAD_BUILD_OUTPUT_STRIPPED=y

This command builds two versions of the libraries, with and without debug symbols, and installs only the version without debug symbols.
|board_note_for_updating_libs|
The :kconfig:`CONFIG_OPENTHREAD_BUILD_OUTPUT_STRIPPED` Kconfig option will be disabled again after this command completes.

Updating libraries to debug version
-----------------------------------

You can also install only the debug version of the current OpenThread libraries (from Zephyr).
This can be useful when debugging, but will take a significant amount of memory of the PC storage - it should be taken into account if one intends to commit those libraries to the repository.

To update the nrfxlib libraries with debug symbols, run the following command:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -t install_openthread_libraries

UART recommendations for NCP
****************************

Use the following recommended default UART settings for configuration based on :ref:`thread_architectures_designs_cp_ncp` architecture:

* Bit rate: 1000000
* Start bits: 1
* Data bits: 8
* Stop bits: 1
* No parity
* Flow Control: Hardware

Flow control
============

UART Hardware Flow Control is recommended in the Nordic solution.
Using Software Flow Control is neither recommended nor implemented.

Hardware reset
==============

Use the Arduino-style hardware reset, where the DTR signal is coupled to the RES pin through a 0.01 µF capacitor.
This causes the NCP to automatically reset whenever the serial port is opened.

.. note::
    This hardware reset method is not used in Nordic Semiconductor's solution.
    It is recommended to dedicate one of your host pins to control the RES pin on the NCP, so that you can easily perform a hardware reset if necessary.

Recommended UART signals
========================

The following UART signals are used in the Nordic solution:

* RX
* TX
* CTS
* RTS
* DTS (optional, not used)
* RES

.. |board_note_for_updating_libs| replace:: This command also builds the sample on the specified board.
   Make sure that the board you mention is compatible with the chosen sample.
