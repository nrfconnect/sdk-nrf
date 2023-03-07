.. _ug_thread_configuring:

Configuring Thread in the |NCS|
###############################

.. contents::
   :local:
   :depth: 2

This page describes the configuration you need to start working with Thread in the |NCS|.

The page also lists additional options you can use to configure your Thread application.
See :ref:`configure_application` for instructions on how to update the configuration for your application, permanently or temporarily.

.. _ug_thread_required_configuration:

Required configuration
**********************

Thread requires the following configuration when working in the |NCS|.
All :ref:`openthread_samples` in the |NCS| have these set by default.

.. _ug_thread_configuring_modules:

Enable Zephyr modules
=====================

Thread requires the following Zephyr modules to properly operate in the |NCS|:

* :ref:`zephyr:ieee802154_interface` radio driver - This library is automatically enabled when working with OpenThread on Nordic Semiconductor's development kits.
* :ref:`zephyr:settings_api` subsystem - This subsystem is required to allow Thread to store settings in the non-volatile memory.

.. _ug_thread_enable:
.. _ug_thread_configuring_basic:

Enable OpenThread in the |NCS|
==============================

To use the Thread protocol in the |NCS|, set the following Kconfig options:

* :kconfig:option:`CONFIG_NETWORKING` - This option enables the generic link layer and the IP networking support.
* :kconfig:option:`CONFIG_NET_L2_OPENTHREAD` - This option enables the OpenThread stack required for the correct operation of the Thread protocol and allows you to use it.
* :kconfig:option:`CONFIG_MPSL` - This option enables the :ref:`nrfxlib:mpsl` (MPSL) implementation, which provides services for both :ref:`single-protocol and multi-protocol implementations <ug_thread_architectures>`.
  This is automatically set for all samples in the |NCS| that use the :ref:`zephyr:ieee802154_interface` radio driver.

.. _ug_thread_select_libraries:
.. _ug_thread_configuring_basic_building:

Select OpenThread libraries
===========================

After enabling OpenThread in the |NCS|, you must choose which OpenThread libraries to use.
You can choose to either build the libraries from source or use :ref:`pre-built variants <ug_thread_prebuilt_libs>` of the libraries.

Building the OpenThread libraries from source gives you full flexibility in configuration.
Using pre-built variants can be useful for certification purposes.

* :kconfig:option:`CONFIG_OPENTHREAD_SOURCES` - This option enables building the OpenThread libraries from source.
  This option is selected by default.

  Building from source allows you to define :ref:`ug_thread_configuring_additional` one by one.
  By default, the :ref:`thread_ug_feature_sets` option is set to custom (:kconfig:option:`CONFIG_OPENTHREAD_USER_CUSTOM_LIBRARY`), which allows you to create your own OpenThread stack configuration.
  However, you can select other feature sets as a basis.

  When building the OpenThread libraries from source, you can also :ref:`update the pre-built OpenThread libraries <thread_ug_feature_updating_libs>`.

* :kconfig:option:`CONFIG_OPENTHREAD_LIBRARY` - This option enables OpenThread to use pre-built libraries.

  You must select one of the :ref:`thread_ug_feature_sets` by enabling :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER`, :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_FTD`, or :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD`.

  This disables building OpenThread from source files and links pre-built libraries instead.

.. _ug_thread_configuring_additional:

Additional configuration options
********************************

In addition to the required configuration, you can configure other features such as which Thread Specification to use and whether to enable hardware-accelerated cryptography.

Depending on your configuration needs, you can also set the following options:

* :kconfig:option:`CONFIG_NET_SOCKETS` - This option enables API similar to BSD Sockets on top of the native Zephyr networking API.
  This configuration is needed for managing networking protocols.
* :kconfig:option:`CONFIG_OPENTHREAD_SHELL` - This option enables OpenThread CLI (see `OpenThread CLI Reference`_).
* :kconfig:option:`CONFIG_COAP` - This option enables Zephyr's :ref:`zephyr:coap_sock_interface` support.
* :kconfig:option:`CONFIG_COAP_UTILS` - This option enables the :ref:`CoAP utils library <coap_utils_readme>`.
* :kconfig:option:`CONFIG_OPENTHREAD_COAP` - This option enables OpenThread's native CoAP API.
* :kconfig:option:`CONFIG_OPENTHREAD_CHANNEL` - By default set to ``11``.
  You can set any value ranging from ``11`` to ``26``.
* :kconfig:option:`CONFIG_OPENTHREAD_PANID` - By default set to ``43981``.
  You can set any value ranging from ``0`` to ``65535``.

See the following files for more options that you might want to change:

* :file:`zephyr/subsys/net/l2/openthread/Kconfig.features` - OpenThread stack features.
* :file:`zephyr/subsys/net/l2/openthread/Kconfig.thread` - Thread network configuration options.

.. note::
   You can find the default configuration for all :ref:`openthread_samples` in the :file:`nrf/subsys/net/openthread/Kconfig.defconfig` file.

.. _thread_ug_thread_specification_options:

Thread Specification options
============================

The OpenThread stack can be configured to operate in compliance with either the Thread 1.1 Specification, the :ref:`Thread 1.2 Specification <thread_ug_supported_features_v12>`, or the :ref:`Thread 1.3 Specification <thread_ug_supported_features_v13>`.
You can change the stack version by using the following Kconfig options:

* :kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_1` - Selects the Thread stack version that is compliant with the Thread 1.1 Specification.
* :kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_2` - Selects the Thread stack version that is compliant with the Thread 1.2 Specification.
* :kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_3` - Selects the Thread stack version that is compliant with the Thread 1.3 Specification.
  This option is enabled by default if no other option is selected.

By enabling support for Thread 1.2, you enable the following :ref:`thread_ug_supported_features_v12` in addition to the Thread 1.1 features:

* Coordinated Sampled Listening (CSL)
* Link Metrics Probing
* Multicast across Thread networks
* Thread Domain unicast addressing
* Enhanced Frame Pending
* Enhanced Keep Alive

By selecting support for Thread 1.3, you enable the following features in addition to the :ref:`Thread 1.2 features <thread_ug_supported_features>`:

* Service Registration Protocol (SRP) client

For a list of all supported features in the |NCS|, see the :ref:`thread_ug_feature_sets`.
For more information about Thread 1.2 features, see the `Thread 1.2 Base Features`_ document.

.. _ug_thread_configuring_eui64:

IEEE 802.15.4 EUI-64 configuration options
==========================================

.. include:: ../../includes/ieee802154_eui64_conf.txt

At the end of the configuration process, you can check the EUI-64 value using OpenThread CLI:

.. code-block:: console

   uart:~$ ot eui64
   8877665544332211
   Done

.. _ug_thread_configuring_crypto:

Hardware-accelerated cryptography options
=========================================

You can enable hardware-accelerated cryptography by using the :ref:`nrfxlib:nrf_security` (nrf_security).
To do this, modify the setting of the following Kconfig option:

* :kconfig:option:`CONFIG_OPENTHREAD_MBEDTLS` - Disable this option to disable the default Mbed TLS configuration for OpenThread.
  The nrf_security module is enabled by default when Mbed TLS for OpenThread is disabled.

For more configuration options, read the module documentation.

.. _ug_thread_configure_commission:

Thread commissioning options
============================

Thread commissioning is the process of adding new Thread devices to the network.
See :ref:`thread_ot_commissioning` for more information.

Configuring this process is optional, because the :ref:`openthread_samples` in the |NCS| use hardcoded network information.

If you want to manually enable the Thread network Commissioner role on a device, set the following Kconfig option to the provided value:

* :kconfig:option:`CONFIG_OPENTHREAD_COMMISSIONER` to ``y``.

To enable the Thread network Joiner role on a device, set the following Kconfig option to the provided value:

* :kconfig:option:`CONFIG_OPENTHREAD_JOINER` to ``y``.

You can also configure how the commissioning process is to be started.
The following options are available:

* Start automatically after the Joiner powers up.
  To configure this option, configure the :kconfig:option:`CONFIG_OPENTHREAD_JOINER_AUTOSTART` option for the Joiner device.
* Start from the application.
* Trigger by Command Line Interface commands.
  In this case, the shell stack size must be increased to at least 3 KB by setting the following option:

  * :kconfig:option:`CONFIG_SHELL_STACK_SIZE` to ``3168``.

For more details about the commissioning process, see `Thread Commissioning on OpenThread portal`_.

.. _thread_ug_logging_options:

OpenThread stack logging options
================================

You can enable the OpenThread stack logging for your project with the following options:

* :kconfig:option:`CONFIG_LOG` - This option enables Zephyr's :ref:`zephyr:logging_api`.
* :kconfig:option:`CONFIG_OPENTHREAD_DEBUG` - This option enables logging for the OpenThread stack.

Both options must be enabled to allow logging.
Use the :file:`overlay-logging.conf` configuration file to enable both options for the Thread samples in the |NCS|.

After setting these options, you can choose one of several :ref:`logging backends <ug_logging_backends>` available in Zephyr and supported in the |NCS|.
The :file:`overlay-logging.conf` configuration file enables :ref:`ug_logging_backends_rtt` as the logging backend by default.

.. note::
    If you are working with Thread samples, enabling logging and logging backend is optional.

Logging levels
--------------

Select one of the following logging levels to customize the logging output:

* :kconfig:option:`CONFIG_OPENTHREAD_LOG_LEVEL_CRIT` - This option enables critical error logging only.
* :kconfig:option:`CONFIG_OPENTHREAD_LOG_LEVEL_WARN` - This option enables warning logging in addition to critical errors.
* :kconfig:option:`CONFIG_OPENTHREAD_LOG_LEVEL_NOTE` - This option additionally enables notice logging.
* :kconfig:option:`CONFIG_OPENTHREAD_LOG_LEVEL_INFO` - This option additionally enables informational logging.
* :kconfig:option:`CONFIG_OPENTHREAD_LOG_LEVEL_DEBG` - This option additionally enables debug logging.

The more detailed logging level you select, the bigger logging buffer you need to have to see all messages.
Use the following Kconfig option for this purpose:

* :kconfig:option:`CONFIG_LOG_BUFFER_SIZE` - This option specifies the number of bytes dedicated to the logger internal buffer.

Zephyr L2 logging options
=========================

If you want to get logging output related to Zephyr's L2 layer, enable one of the following Kconfig options:

* :kconfig:option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_ERR` - Enables logging only for errors.
* :kconfig:option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_WRN` - Enables logging for errors and warnings.
* :kconfig:option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_INF` - Enables logging for informational messages, errors, and warnings.
* :kconfig:option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_DBG` - Enables logging for debug messages, informational messages, errors, and warnings.

Choosing one of these options enables writing the appropriate information in the L2 debug log.

Additionally, enabling :kconfig:option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_DBG` allows you to set the :kconfig:option:`CONFIG_OPENTHREAD_L2_DEBUG` option, which in turn has the following settings:

* :kconfig:option:`CONFIG_OPENTHREAD_L2_DEBUG_DUMP_15_4` - Enables dumping 802.15.4 frames in the debug log output.
* :kconfig:option:`CONFIG_OPENTHREAD_L2_DEBUG_DUMP_IPV6` - Enables dumping IPv6 frames in the debug log output.

You can disable writing to log with the :kconfig:option:`CONFIG_OPENTHREAD_L2_LOG_LEVEL_OFF` option.

.. _thread_ug_device_type:

Device type options
===================

You can configure OpenThread devices to run as a specific :ref:`device type <thread_ot_device_types>`.

Full Thread Device (FTD)
  Set :kconfig:option:`CONFIG_OPENTHREAD_FTD` to configure the device as FTD.
  This is the default configuration.

Minimal Thread Device (MTD)
  Set :kconfig:option:`CONFIG_OPENTHREAD_MTD` to configure the device as MTD.

  By default, the MTD operates as Minimal End Device (MED).
  To make it operate as Sleepy End Device (SED), set :kconfig:option:`CONFIG_OPENTHREAD_MTD_SED`.

.. _thread_ug_tfm_support:

Trusted Firmware-M support options
==================================

To configure your Thread application on the nRF5340 DK to run with Trusted Firmware-M, use the ``nrf5340dk_nrf5340_cpuapp_ns`` build target and enable the following Kconfig options:

* :kconfig:option:`CONFIG_BUILD_WITH_TFM`
* :kconfig:option:`CONFIG_OPENTHREAD_CRYPTO_PSA`

In the |NCS|, these options are enabled by default for the :ref:`application core <ug_nrf5340_intro_app_core>` of the :ref:`openthread_samples` that can be programmed with the ``nrf5340dk_nrf5340_cpuapp_ns`` build target.

For more Trusted Firmware-M documentation, see :ref:`ug_tfm` and the official `TF-M documentation`_.
