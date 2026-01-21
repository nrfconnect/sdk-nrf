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

You can enable the Thread protocol in the |NCS| by using the Zephyr networking layer or by passing Thread frames directly to the nRF 802.15.4 radio driver.

* To use the Thread protocol with Zephyr networking layer, enable the following Kconfig options:

  * :kconfig:option:`CONFIG_NETWORKING` - This option enables a generic link layer and the IP networking support.
  * :kconfig:option:`CONFIG_NET_L2_OPENTHREAD` - This option enables the OpenThread stack required for operating the Thread protocol effectively.

* To use the Thread protocol and nRF 802.15.4 radio directly, disable the previous Kconfig options and enable the :kconfig:option:`CONFIG_OPENTHREAD` option.
  This enables the OpenThread stack, allowing for direct use of the nRF 802.15.4 radio.

To learn more about available architectures, see the :ref:`openthread_stack_architecture` user guide.

Additionally, you can set the :kconfig:option:`CONFIG_MPSL` Kconfig option.
It enables the :ref:`nrfxlib:mpsl` (MPSL) implementation, which provides services for both :ref:`single-protocol and multi-protocol implementations <ug_thread_architectures>`.
This is automatically set for all |NCS| samples that use the :ref:`zephyr:ieee802154_interface` radio driver.

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

  You must select one of the :ref:`thread_ug_feature_sets` by enabling :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER`, :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_FTD`, :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD`, or :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_RCP`.

  This disables building OpenThread from source files and links pre-built libraries instead.

.. _ug_thread_configuring_additional:

Additional configuration options
********************************

In addition to the required configuration, you can configure other features such as which Thread Specification to use and whether to enable hardware-accelerated cryptography.

Depending on your configuration needs, you can also set the following options:

* :kconfig:option:`CONFIG_NET_SOCKETS` - This option enables an API similar to BSD Sockets on top of the native Zephyr networking API.
  This configuration is needed for managing networking protocols.
  This configuration is available only if Zephyr networking layer is enabled.
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
* :file:`nrf/modules/openthread/Kconfig.features.nrf` - Thread network configuration dedicated to nRF Connect purposes.

.. note::
   You can find the default configuration for all :ref:`openthread_samples` in the :file:`nrf/subsys/net/openthread/Kconfig.defconfig` file.

.. _thread_configuring_messagepool:

Message pool configuration
**************************

OpenThread uses a message pool to manage memory for message buffers.
Memory for the message pool can be statically allocated by the OpenThread stack or managed by the platform.
You can use the :kconfig:option:`CONFIG_OPENTHREAD_PLATFORM_MESSAGE_MANAGEMENT` Kconfig option to enable platform message management.

Message buffer size and number of message buffers in the pool can be configured with the :kconfig:option:`CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE` and :kconfig:option:`CONFIG_OPENTHREAD_NUM_MESSAGE_BUFFERS` Kconfig options, respectively.
By default, the message buffer size is set to ``128``, and the number of message buffers is set to ``96`` for a Full Thread Device and ``64`` for a Minimal Thread Device.

.. note::
   When using :ref:`thread_ug_prebuilt`, changing the :kconfig:option:`CONFIG_OPENTHREAD_PLATFORM_MESSAGE_MANAGEMENT` Kconfig option will have no effect.
   Additionally, the :kconfig:option:`CONFIG_OPENTHREAD_MESSAGE_BUFFER_SIZE` Kconfig option has to be set to the same value that is used in the pre-built library.

.. _thread_ug_thread_specification_options:

Thread Specification options
============================

The OpenThread stack can be configured to operate in compliance with either the Thread 1.1 Specification, the :ref:`Thread 1.2 Specification <thread_ug_supported_features_v12>`, the :ref:`Thread 1.3 Specification <thread_ug_supported_features_v13>`, or the :ref:`Thread 1.4 Specification <thread_ug_supported_features_v14>`.
You can change the stack version by using the following Kconfig options:

* :kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_1` - Selects the Thread stack version that is compliant with the Thread 1.1 Specification.
* :kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_2` - Selects the Thread stack version that is compliant with the Thread 1.2 Specification.
* :kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_3` - Selects the Thread stack version that is compliant with the Thread 1.3 Specification.
* :kconfig:option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_4` - Selects the Thread stack version that is compliant with the Thread 1.4 Specification.
  This option is enabled by default if no other option is selected.

By selecting support for Thread 1.2, you enable the following :ref:`thread_ug_supported_features_v12` in addition to the Thread 1.1 features:

* Coordinated Sampled Listening (CSL)
* Link Metrics Probing
* Multicast across Thread networks
* Thread Domain unicast addressing
* Enhanced Frame Pending
* Enhanced Keep Alive

By selecting support for Thread 1.3, you enable the following :ref:`thread_ug_supported_features_v13` in addition to the :ref:`thread_ug_supported_features_v12`:

* Service Registration Protocol (SRP) client

By selecting support for Thread 1.4, you enable the following :ref:`thread_ug_supported_features_v14` in addition to the :ref:`thread_ug_supported_features_v13` and :ref:`thread_ug_supported_features_v12`:

* Enhanced Internet Connectivity
* Enhanced Network Diagnostics

For a list of all supported features in the |NCS|, see the :ref:`thread_ug_feature_sets`.

.. _ug_thread_configuring_eui64:

IEEE 802.15.4 EUI-64 configuration options
==========================================

An IEEE EUI-64 address consists of two parts:

* Company ID - a 24-bit MA-L (MAC Address Block Large), formerly called OUI (Organizationally Unique Identifier)
* Extension identifier - a 40-bit device unique identifier

You can configure the EUI-64 for a device in the following ways depending on chosen architecture:

  .. tabs::

     .. tab:: Zephyr networking layer enabled

        Use the default
          By default, the company ID is set to Nordic Semiconductor's MA-L (``f4-ce-36``).
          The extension identifier is set to the DEVICEID from the factory information configuration registers (FICR).

        Replace the company ID
          You can enable the :kconfig:option:`CONFIG_IEEE802154_VENDOR_OUI_ENABLE` Kconfig option to replace Nordic Semiconductor's company ID with your own company ID.
          Specify your company ID with the :kconfig:option:`CONFIG_IEEE802154_VENDOR_OUI` option.

          The extension identifier is set to the default, namely the DEVICEID from FICR.

        Replace the full EUI-64
          You can provide the full EUI-64 value by programming certain user information configuration registers (UICR).
          nRF52 Series devices use the CUSTOMER registers block, while nRF53 Series devices use the OTP registers block

          To use the EUI-64 value from the UICR, enable the :kconfig:option:`CONFIG_NRF5_UICR_EUI64_ENABLE` Kconfig option and set :kconfig:option:`CONFIG_NRF5_UICR_EUI64_REG` to the base of the two consecutive registers that contain your EUI-64 value.

          The following example shows how to replace the full EUI-64 on the nRF52840 device:

          1. Enable the :kconfig:option:`CONFIG_IEEE802154_NRF5_UICR_EUI64_ENABLE` Kconfig option.

          #. Specify the offset for the UICR registers in :kconfig:option:`CONFIG_IEEE802154_NRF5_UICR_EUI64_REG`.
             This example uses UICR->CUSTOMER[0] and UICR->CUSTOMER[1], which means that you can keep the default value ``0``.

          #. Build and program your application erasing the whole memory.
             Make sure to replace *serial_number* with the serial number of your debugger:

              .. parsed-literal::
               :class: highlight

                west build -b nrf52840dk/nrf52840 -p always
                west flash --snr *serial_number* --erase

          #. Program the registers UICR->CUSTOMER[0] and UICR->CUSTOMER[1] with your EUI-64 value (replace *serial_number* with the serial number of your debugger):

              .. parsed-literal::
               :class: highlight

                nrfutil device x-write --serial-number *serial_number* --address 0x10001080 --value 0x11223344
                nrfutil device x-write --serial-number *serial_number* --address 0x10001084 --value 0x55667788
                nrfutil device reset --reset-kind=RESET_PIN

             If you used a different value for :kconfig:option:`CONFIG_IEEE802154_NRF5_UICR_EUI64_REG`, you must use different register addresses.

             At the end of the configuration process, you can check the EUI-64 value using the OpenThread CLI as follows:

              .. code-block:: console

               uart:~$ ot eui64
               8877665544332211
               Done

     .. tab:: Zephyr networking layer disabled

        Use the default
          By default, the company ID is set to Nordic Semiconductor's MA-L (``f4-ce-36``).
          The extension identifier is set to the DEVICEID from the factory information configuration registers (FICR).

        Replace the company ID
          You can enable the :kconfig:option:`CONFIG_IEEE802154_VENDOR_OUI_ENABLE` Kconfig option to replace Nordic Semiconductor's company ID with your own company ID.
          Specify your company ID with the :kconfig:option:`CONFIG_NRF5_VENDOR_OUI` option.

          The extension identifier is set to the default, namely the DEVICEID from FICR.

        Replace the full EUI-64
          You can provide the full EUI-64 value by programming certain user information configuration registers (UICR).
          nRF52 Series devices use the CUSTOMER registers block, while nRF53 Series devices use the OTP registers block.

          To use the EUI-64 value from the UICR, enable the :kconfig:option:`CONFIG_NRF5_UICR_EUI64_ENABLE` Kconfig option and set :kconfig:option:`CONFIG_NRF5_UICR_EUI64_REG` to the base of the two consecutive registers that contain your EUI-64 value.

          The following example shows how to replace the full EUI-64 on the nRF52840 device:

          1. Enable the :kconfig:option:`CONFIG_NRF5_UICR_EUI64_ENABLE` Kconfig option.

          #. Specify the offset for the UICR registers in :kconfig:option:`CONFIG_NRF5_UICR_EUI64_REG`.
             This example uses UICR->CUSTOMER[0] and UICR->CUSTOMER[1], which means that you can keep the default value ``0``.

          #. Build and program your application erasing the whole memory.
             Make sure to replace *serial_number* with the serial number of your debugger:

              .. parsed-literal::
               :class: highlight

               west build -b nrf52840dk/nrf52840 -p always
               west flash --snr *serial_number* --erase

          #. Program the registers UICR->CUSTOMER[0] and UICR->CUSTOMER[1] with your EUI-64 value (replace *serial_number* with the serial number of your debugger):

              .. parsed-literal::
                :class: highlight

                nrfutil device x-write --serial-number *serial_number* --address 0x10001080 --value 0x11223344
                nrfutil device x-write --serial-number *serial_number* --address 0x10001084 --value 0x55667788
                nrfutil device reset --reset-kind=RESET_PIN

             If you used a different value for :kconfig:option:`CONFIG_NRF5_UICR_EUI64_REG`, you must use different register addresses.

             At the end of the configuration process, you can check the EUI-64 value using OpenThread CLI:

              .. code-block:: console

                uart:~$ ot eui64
                8877665544332211
                Done


.. _ug_thread_configuring_crypto:

Cryptographic support
=====================

By default, the OpenThread stack uses the :ref:`PSA Crypto API <ug_psa_certified_api_overview_crypto>` for cryptographic operations.
The support is implemented through the nRF Security library, which provides hardware-accelerated cryptographic functionality on selected Nordic Semiconductor SoCs.
For more information, see the :ref:`psa_crypto_support` page.

.. _ug_thread_configuring_mbedtls:

Mbed TLS support
================

By default, the OpenThread stack uses the Mbed TLS library for X.509 certificate manipulation and the SSL protocols.
The cryptographic support is handled through PSA Crypto API, as mentioned in `Cryptographic support`_.

The `Mbed TLS`_ protocol features can be handled using the :kconfig:option:`OPENTHREAD_MBEDTLS_CHOICE` Kconfig option.

.. note::
   The :kconfig:option:`OPENTHREAD_MBEDTLS_CHOICE` Kconfig option has not been tested and is not recommended for use with the |NCS|.

For more information about the open source Mbed TLS implementation in the |NCS|, see the `sdk-mbedtls`_ repository.
For more information about the OpenThread security in |NCS|, see the :ref:`ug_ot_thread_security` page.

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

  When you set the :kconfig:option:`CONFIG_OPENTHREAD_JOINER` Kconfig option, the :kconfig:option:`CONFIG_SHELL_STACK_SIZE` Kconfig option is automatically increased to ``3168``, meaning the shell stack size is set to 3 KB.

You can also configure how the commissioning process is to be started.
The following options are available:

* Provisioning starts automatically after the Joiner powers up.
  To configure this option, configure the :kconfig:option:`CONFIG_OPENTHREAD_JOINER_AUTOSTART` option for the Joiner device.
* Provisioning is started when the application makes a call to the OpenThread API.
* Provisioning is started by using Command Line Interface commands.

For more details about the commissioning process, see `Thread Commissioning on OpenThread portal`_.

.. _thread_ug_logging_options:

OpenThread stack logging options
================================

You can enable the OpenThread stack logging for your project with the following options:

* :kconfig:option:`CONFIG_LOG` - This option enables Zephyr's :ref:`zephyr:logging_api`.
* :kconfig:option:`CONFIG_OPENTHREAD_DEBUG` - This option enables logging for the OpenThread stack.

Both options must be enabled to allow logging.
Use the ``logging`` snippet to enable both options for the Thread samples in the |NCS|.

After setting these options, you can choose one of several :ref:`logging backends <ug_logging_backends>` available in Zephyr and supported in the |NCS|.
The ``logging`` snippet enables :ref:`ug_logging_backends_rtt` as the logging backend by default.

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

Thread currently supports Trusted Firmware-M (TF-M) on the nRF54L15 DK.

To configure your Thread application to run with Trusted Firmware-M, use the ``nrf54l15dk/nrf54l15/cpuapp/ns`` board target and follow the instructions in :ref:`ug_tfm_building`.
