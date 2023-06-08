.. _ug_thread_prebuilt_libs:
.. _thread_ug_prebuilt:

Pre-built libraries
###################

.. contents::
   :local:
   :depth: 2

The |NCS| provides a set of :ref:`nrfxlib:ot_libs`.
These pre-built libraries are available in nrfxlib and provide features and optional functionalities from the OpenThread stack.
You can use these libraries for building applications with support for the complete Thread Specification.

To use a pre-built library, configure OpenThread to use pre-built libraries by setting the :kconfig:option:`CONFIG_OPENTHREAD_LIBRARY` Kconfig option and select one of the provided :ref:`thread_ug_feature_sets`.

.. _thread_ug_feature_sets:

Feature sets
************

A feature set defines a combination of OpenThread features for a specific use case.

These feature sets are mainly used for pre-built libraries, but you can also use them when you :ref:`build your application libraries from source <ug_thread_configuring_basic_building>`.
The |NCS| provides the following feature sets:

* :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MASTER` - Enable the complete set of OpenThread features for the Thread Specification.
* :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_FTD` - Enable optimized OpenThread features for FTD.
* :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` - Enable optimized OpenThread features for MTD.
* :kconfig:option:`CONFIG_OPENTHREAD_USER_CUSTOM_LIBRARY` - Create a custom feature set for compilation when :ref:`building OpenThread libraries from source <ug_thread_configuring_basic_building>`.
  This option is the default.
  If you use pre-built libraries, you must choose a different feature set.

  .. note::
    When :ref:`building OpenThread libraries from source <ug_thread_configuring_basic>`, you can select another feature set as base to select several configuration options at once.
    You can then manually enable additional features, but you cannot disable features that are enabled by the chosen feature set.

The following table lists the supported features for each of these sets.
No tick indicates that there is no support for the given feature in the related configuration, while the tick signifies that the feature is selected (``=1`` value).
Features introduced with the Thread 1.2 Specification are at the bottom of the table and have "Thread 1.2" in parenthesis after the feature name.
For more information about Thread 1.2 features, see the `Thread 1.2 Base Features`_ document.

.. list-table::
    :widths: auto
    :header-rows: 1

    * - OpenThread feature
      - Master
      - FTD
      - MTD
      - Custom
    * - BORDER_AGENT
      -
      -
      -
      -
    * - BORDER_ROUTER
      -
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
      -
      -
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
      -
      -
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
      -
      -
      -
    * - MTD_NETDIAG
      - ✔
      -
      -
      -
    * - SERVICE
      - ✔
      -
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
    * - SRP_CLIENT
      - ✔
      - ✔
      - ✔
      -
    * - UDP_FORWARD
      - ✔
      -
      -
      -
    * - BACKBONE_ROUTER (Thread 1.2)
      -
      -
      -
      -
    * - CSL_RECEIVER (Thread 1.2)
      - ✔
      -
      - ✔
      -
    * - DUA (Thread 1.2)
      - ✔
      - ✔
      - ✔
      -
    * - LINK_METRICS_INITIATOR (Thread 1.2)
      - ✔
      - ✔
      - ✔
      -
    * - LINK_METRICS_SUBJECT (Thread 1.2)
      - ✔
      - ✔
      -
      -
    * - MLR (Thread 1.2)
      - ✔
      - ✔
      - ✔
      -

For the full list of configuration options that were used during compilation, including their default values, see the :file:`openthread_lib_configuration.txt` file within each library folder in the nrfxlib repository.

.. _thread_ug_customizing_prebuilt:

Customizing pre-built libraries
*******************************

Selecting a feature set allows you to use the respective OpenThread features in your application.
You might need to customize some configuration options to fit your use case though.

Be aware of the following limitations when customizing the configuration of a pre-built library:

* You can only update configuration options that are configurable at run time.
  If you change any options that are compiled into the library, your changes will be ignored.
* Changes to the configuration might impact the certification status of the pre-built libraries.
  See :ref:`ug_thread_cert_options` for more information.

The following list shows some of the configuration options that you might want to customize:

* :kconfig:option:`CONFIG_OPENTHREAD_FTD` or :kconfig:option:`CONFIG_OPENTHREAD_MTD` - Select the :ref:`device type <thread_ug_device_type>`.
  The :kconfig:option:`CONFIG_OPENTHREAD_NORDIC_LIBRARY_MTD` feature set supports only the MTD device type.
  The other feature sets support both device types.
* :kconfig:option:`CONFIG_OPENTHREAD_COPROCESSOR` and :kconfig:option:`CONFIG_OPENTHREAD_COPROCESSOR_RCP` - Select the OpenThread architecture to use.
  See :ref:`thread_architectures_designs_cp`.
* :kconfig:option:`CONFIG_OPENTHREAD_MANUAL_START` - Choose whether to configure and join the Thread network automatically.
  If you set this option to ``n``, also check and configure the network parameters that are used, for example:

  * :kconfig:option:`CONFIG_OPENTHREAD_CHANNEL`
  * :kconfig:option:`CONFIG_OPENTHREAD_NETWORKKEY`
  * :kconfig:option:`CONFIG_OPENTHREAD_NETWORK_NAME`
  * :kconfig:option:`CONFIG_OPENTHREAD_PANID`
  * :kconfig:option:`CONFIG_OPENTHREAD_XPANID`

.. _thread_ug_feature_updating_libs:

Updating pre-built OpenThread libraries
***************************************

You can update the :ref:`nrfxlib:ot_libs` in nrfxlib when using any Thread sample if you configure the sample to build the OpenThread stack from source with :kconfig:option:`CONFIG_OPENTHREAD_SOURCES`.
Use this functionality for :ref:`certification <ug_thread_cert>` of your configuration of the OpenThread libraries, for example.

You can install the libraries either with or without debug symbols.
Installing the libraries with debug symbols can be useful when debugging, but will take a significant amount of storage memory.
You can remove the symbols when updating with the :kconfig:option:`CONFIG_OPENTHREAD_BUILD_OUTPUT_STRIPPED` Kconfig option enabled.
The option is disabled by default.

.. note::
    When you select :kconfig:option:`CONFIG_OPENTHREAD_USER_CUSTOM_LIBRARY`, the location of the destination directory for the libraries depends on the chosen :ref:`nrf_security backend <nrf_security_readme>`, either :kconfig:option:`CONFIG_CC3XX_BACKEND` or :kconfig:option:`CONFIG_OBERON_BACKEND`.

Updating the libraries without debug symbols
============================================

There is a single command to update the libraries without debug symbols.
When using the command line, run the command in the project folder.
When using the |nRFVSC|, open a terminal and choose :guilabel:`nRF Terminal`, then run the command there.

Use the following command:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -t install_openthread_libraries -- -DOPENTHREAD_BUILD_OUTPUT_STRIPPED=y

This command builds two versions of the libraries, with and without debug symbols, and installs only the version without debug symbols.
|board_note_for_updating_libs|
The :kconfig:option:`CONFIG_OPENTHREAD_BUILD_OUTPUT_STRIPPED` Kconfig option will be disabled again after this command completes.

Updating the libraries with debug symbols
=========================================

There is a single command to update the libraries with debug symbols.
When using the command line, run the command in the project folder.
When using the |nRFVSC|, open a terminal and choose :guilabel:`nRF Terminal`, then run the command there.

Use the following command:

.. parsed-literal::
   :class: highlight

   west build -b nrf52840dk_nrf52840 -t install_openthread_libraries

|board_note_for_updating_libs|

.. |board_note_for_updating_libs| replace:: This command also builds the sample on the specified board.
   Make sure that the board you mention is compatible with the chosen sample.
