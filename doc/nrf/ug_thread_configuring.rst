.. _ug_thread_configuring:

Configuring Thread in |NCS|
###########################

This page describes what is needed to start working with Thread in |NCS|.

Required modules
****************

Thread requires the following Zephyr's modules to properly operate in |NCS|:

* :ref:`zephyr:ieee802154_interface` radio driver - This library is automatically enabled when working with OpenThread on Nordic Semiconductor's Development Kits.
* :ref:`zephyr:settings_api` subsystem - This is required to allow Thread to store settings in the non-volatile memory.

.. _ug_thread_configuring_basic:

Basic configuration
*******************

To use the Thread protocol in |NCS|, set the following Kconfig options:

* :option:`CONFIG_NETWORKING` - This option enables the generic link layer and the IP networking support.
* :option:`CONFIG_NET_L2_OPENTHREAD` - This option enables the OpenThread stack required for the correct operation of the Thread protocol and allows you to use them.

At this point, you can either:

* Define :ref:`additional configuration <ug_thread_configuring_additional>`
* Choose :ref:`thread_ug_feature_sets`

.. _ug_thread_configuring_additional:

Additional configuration
************************

Depending on your configuration needs, you can also set the following options:

* :option:`CONFIG_NET_SOCKETS` - This option enables API similar to BSD Sockets on top of the native Zephyr networking API.
  This is needed for managing networking protocols.
* :option:`CONFIG_NET_SHELL` - This option enables Zephyr's :ref:`zephyr:shell_api` if you need to access OpenThread CLI and use commands from `OpenThread CLI Reference`_.
* :option:`CONFIG_COAP` - This option enables Zephyr's :ref:`zephyr:coap_sock_interface` support.
* :option:`CONFIG_OPENTHREAD_COAP` - This option enables OpenThread's native CoAP API.

You can also change the default values in menuconfig for the options listed in the following files:

* :file:`subsys/net/l2/openthread/Kconfig.features` - OpenThread stack features.
* :file:`subsys/net/l2/openthread/Kconfig.thread` - Thread network configuration options.

This includes the following options:

* :option:`CONFIG_OPENTHREAD_CHANNEL` - By default set to ``11``.
  You can set any value ranging from ``11`` to ``26``.
* :option:`CONFIG_OPENTHREAD_PANID` - By default set to ``43981``.
  You can set any value ranging from ``0`` to ``65535``.

Default configuration reference
    The default configuration for all :ref:`openthread_samples` is defined in :file:`nrf/samples/openthread/openthread.conf`.

For other optional configuration options, see the following sections.

.. _ug_thread_configuring_crypto:

Hardware-accelerated cryptography
=================================

You can enable hardware-accelerated cryptography by using the :ref:`nrfxlib:nrf_security_readme`.
To do this, modify the setting of the following Kconfig option:

* :option:`CONFIG_OPENTHREAD_MBEDTLS` - Disable this option to disable the default mbedTLS configuration for OpenThread.
  The nrf_security module is enabled by default when mbedTLS for OpenThread is disabled.

For more configuration options, read the module documentation.

.. _thread_ug_thread_1_2:

Thread Specification v1.2 options
=================================

The OpenThread stack can be configured to operate in compliance with either Thread Specification v1.1 or :ref:`Thread Specification v1.2 <thread_ug_supported_features_v12>`.
You can change the stack version by using the following Kconfig options:

* :option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_1` - Selects the Thread stack version that is compliant with Thread Specification v1.1.
  This option is enabled by default if no other option is selected.
* :option:`CONFIG_OPENTHREAD_THREAD_VERSION_1_2` - Selects the Thread stack version that is compliant with Thread Specification v1.2.

By selecting support for the v1.2, you enable the following features in addition to the :ref:`v1.1 features <thread_ug_supported_features>`:

* Enhanced Frame Pending
* Enhanced Keep Alive
* Thread Domain Name

Moreover, the v1.2 also comes with the following features supported in experimental status:

* :option:`CONFIG_OPENTHREAD_DUA` - Enable Domain Unicast Addresses.
* :option:`CONFIG_OPENTHREAD_MLR` - Enable Multicast Listener Registration.
* :option:`CONFIG_OPENTHREAD_BACKBONE_ROUTER` - Enable Backbone Router.

.. note::
    To test Thread Specification v1.2 options, you can use the :ref:`Thread CLI sample <ot_cli_sample>` with the :ref:`experimental v1.2 extension <ot_cli_sample_thread_v12>`.

Thread commissioning
====================

Thread commissioning is the process of adding new Thread devices to the network.
It involves two devices: a Commissioner that is already in the Thread network and a Joiner that wants to become a member of the network.

Configuring this process is optional, because the :ref:`openthread_samples` in |NCS| use hardcoded network information.

If you want to manually enable the Thread network Commissioner role on a device, set the following Kconfig option to the provided value:

* :option:`CONFIG_OPENTHREAD_COMMISSIONER` to ``y``.

To enable the Thread network Joiner role on a device, set the following Kconfig option to the provided value:

* :option:`CONFIG_OPENTHREAD_JOINER` to ``y``.

You can also configure how the commissioning process is to be started:

* Automatically after Joiner's power up with the :option:`CONFIG_OPENTHREAD_JOINER_AUTOSTART` option, configured for the Joiner device.
* Started from the application.
* Triggered by Command Line Interface commands.

For more details about the commissioning process, see `Thread Commissioning on OpenThread portal`_.

.. _thread_ug_logging_options:

OpenThread stack logging options
================================

You can enable the OpenThread stack logging for your project with the following options:

* :option:`CONFIG_LOG` - This option enables Zephyr's :ref:`zephyr:logging_api`.
* :option:`CONFIG_OPENTHREAD_DEBUG` - This option enables logging for the OpenThread stack.

Both options must be enabled to allow logging.

After setting these options, you can choose one of several :ref:`logging backends <ug_logging_backends>` available in Zephyr and supported in |NCS|.

.. note::
    If you are working with Thread samples, enabling logging and logging backend is optional.
    By default, all Thread samples have logging enabled in the :file:`overlay-ot-defaults.conf` file, and are set to provide output at the informational level (:option:`CONFIG_OPENTHREAD_LOG_LEVEL_INFO`).

Logging levels
--------------

You can set one of the following logging levels to customize the logging output:

* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_CRIT` - This option enables critical error logging only.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_WARN` - This option enables warning logging in addition to critical errors.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_NOTE` - This option additionally enables notice logging.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_INFO` - This option additionally enables informational logging.
* :option:`CONFIG_OPENTHREAD_LOG_LEVEL_DEBG` - This option additionally enables debug logging.

The more detailed logging level you select, the more logging buffers you need to be able to see all messages, and the buffer size also needs to be increased.
Use the following Kconfig options for this purpose:

* :option:`CONFIG_LOG_STRDUP_BUF_COUNT` - This option specifies the number of logging buffers.
* :option:`CONFIG_LOG_STRDUP_MAX_STRING` - This option specifies the size of logging buffers.


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

An OpenThread device can be configured to run as one of the following device types, which serve different roles in the Thread network:

Full Thread Device (FTD)
    In this configuration, the device can be both Router and End Device.
    To enable this device type thread, set the following Kconfig option:

    * :option:`CONFIG_OPENTHREAD_FTD`

    This is the default configuration if none is selected.

Minimal Thread Device (MTD)
    In this configuration, the device can only be an End Device.
    To enable this device type thread, set the following Kconfig option:

    * :option:`CONFIG_OPENTHREAD_MTD`

    By default, when a Thread device is configured as MTD, it operates as Minimal End Device (MED).
    You can choose to make it operate as Sleepy End Device (SED) by enabling the :option:`CONFIG_OPENTHREAD_MTD_SED` option.

For more information, see `Device Types on OpenThread portal`_.

.. _thread_ug_feature_sets:

OpenThread configuration sets
*****************************

|NCS| provides predefined sets of features and optional functionalities from the OpenThread stack.
You can use these sets for configuration purposes or for building libraries with complete Thread specification support.

* :option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER` - Enable the complete set of OpenThread features.
* :option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_FTD` - Enable optimized OpenThread features for FTD.
* :option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` - Enable optimized OpenThread features for MTD.

Selecting these sets is not related to :ref:`thread_ug_device_type`.

The following table lists the supported features for each of these sets.

.. note::
    No tick means missing support for the given feature in the related configuration, while the tick is equal to ``=1`` value.

.. list-table::
    :widths: auto
    :header-rows: 1

    * - OpenThread feature
      - Master
      - Optimized_FTD
      - Optimized_MTD
    * - BORDER_AGENT
      - ✔
      -
      -
    * - BORDER_ROUTER
      - ✔
      -
      -
    * - CHILD_SUPERVISION
      - ✔
      - ✔
      - ✔
    * - COAP
      - ✔
      - ✔
      - ✔
    * - COAPS
      - ✔
      - ✔
      - ✔
    * - COMMISSIONER
      - ✔
      -
      -
    * - DIAGNOSTIC
      - ✔
      -
      -
    * - DNS_CLIENT
      - ✔
      - ✔
      - ✔
    * - DHCP6_SERVER
      - ✔
      -
      -
    * - DHCP6_CLIENT
      - ✔
      - ✔
      - ✔
    * - ECDSA
      - ✔
      - ✔
      - ✔
    * - IP6_FRAGM
      - ✔
      - ✔
      - ✔
    * - JAM_DETECTION
      - ✔
      - ✔
      - ✔
    * - JOINER
      - ✔
      - ✔
      - ✔
    * - LINK_RAW
      - ✔
      -
      -
    * - MAC_FILTER
      - ✔
      - ✔
      - ✔
    * - MTD_NETDIAG
      - ✔
      -
      -
    * - SERVICE
      - ✔
      - ✔
      -
    * - SLAAC
      - ✔
      - ✔
      - ✔
    * - SNTP_CLIENT
      - ✔
      - ✔
      - ✔
    * - UDP_FORWARD
      - ✔
      - ✔
      -

Replacing OpenThread Libraries
******************************

Each OpenThread example provides a possibility to replace libraries present in the nrfxlib when configured to build OpenThread stack from source with :option:`CONFIG_OPENTHREAD_SOURCES`.
After the sample is build execute the following to overwrite the exising nrfxlib libraries with currently built ones::

  west build -t install_openthread_libraries

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

Use the Arduino-style hardware reset, where the DTR signal is coupled to the RES pin through a 0.01[micro]F capacitor.
This causes the NCP to automatically reset whenever the serial port is opened.

.. note::
    This hardware reset method is not used in Nordic's solution.
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
