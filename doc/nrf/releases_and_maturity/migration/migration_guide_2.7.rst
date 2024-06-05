.. _migration_2.7:

Migration guide for |NCS| v2.7.0 (Working draft)
################################################

.. contents::
   :local:
   :depth: 3

This document describes the changes required or recommended when migrating your application from |NCS| v2.6.0 to |NCS| v2.7.0.

.. HOWTO

   Add changes in the following format:

   Component (for example, application, sample or libraries)
   *********************************************************

   .. toggle::

      * Change1 and description
      * Change2 and description

.. _migration_2.7_required:

Required changes
****************

The following changes are mandatory to make your application work in the same way as in previous releases.

Samples and applications
========================

This section describes the changes related to samples and applications.

Wi-Fi®
------

.. toggle::

   * :ref:`wifi_shell_sample` sample:

     * The parameters of the ``connect`` and ``ap enable`` commands have been updated.
       Check the updated parameters using the ``-h`` help option of the command.

Serial LTE Modem (SLM)
----------------------

.. toggle::

   The AT command parsing has been updated to utilize the :ref:`at_cmd_custom_readme` library.
   If you have introduced custom AT commands to the SLM, you need to update the command parsing to use the new library.
   See the :ref:`slm_extending` page for more information.

Peripheral samples
------------------

.. toggle::

   * :ref:`radio_test` sample:

     * The CLI command ``fem tx_power_control <tx_power_control>`` replaces ``fem tx_gain <tx_gain>`` .
       This change applies to the sample built with the :ref:`CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC <CONFIG_RADIO_TEST_POWER_CONTROL_AUTOMATIC>` set to ``n``.

Matter
------

  * With the inheritance of Zephyr's :ref:`zephyr:sysbuild` in the |NCS|, some changes are provided to the Matter samples and applications:

    * :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_BUILD` Kconfig option is deprecated and you need to use the ``SB_CONFIG_MATTER_FACTORY_DATA_GENERATE`` Kconfig option instead to enable or disable creating the factory data set during building a Matter sample.
      To enable factory data support on your device, you still need to set the :kconfig:option:`CONFIG_CHIP_FACTORY_DATA` to ``y``.
    * Factory data output files are now located in the ``<application_name>/zephyr/`` directory within the build directory.
    * :kconfig:option:`CONFIG_CHIP_FACTORY_DATA_MERGE_WITH_FIRMWARE` Kconfig option is deprecated in sysbuild and you need to use the ``SB_CONFIG_MATTER_FACTORY_DATA_MERGE_WITH_FIRMWARE`` Kconfig option instead to enable or disable merging the factory data HEX file with the final firmware HEX file.
    * ``SB_CONFIG_MATTER_OTA`` Kconfig option has been added to enable or disable generating Matter OTA package during the building process.
    * :kconfig:option:`CONFIG_CHIP_OTA_IMAGE_FILE_NAME` Kconfig option is deprecated and you need to use the ``SB_CONFIG_MATTER_OTA_IMAGE_FILE_NAME`` Kconfig option instead to define Matter OTA output filename.

  .. note::

    If you want to build a sample without using sysbuild, you need to use the old Kconfig options.

Libraries
=========

This section describes the changes related to libraries.

MQTT helper
-----------

.. toggle::

   * For applications using the :ref:`lib_mqtt_helper` library:

     * The ``CONFIG_MQTT_HELPER_CERTIFICATES_FILE`` Kconfig option is now replaced by :kconfig:option:`CONFIG_MQTT_HELPER_CERTIFICATES_FOLDER`.
       The new option is a folder path where the certificates are stored.
       The folder path must be relative to the root of the project.

       If you are using the :ref:`lib_mqtt_helper` library, you must update the Kconfig option to use the new option.

     * When using the :kconfig:option:`CONFIG_MQTT_HELPER_PROVISION_CERTIFICATES` Kconfig option, the certificate files must be in standard PEM format.
       This means that the PEM files must not be converted to string format anymore.

FEM abstraction layer
---------------------

.. toggle::

   * For applications using :ref:`fem_al_lib`:

     * The function :c:func:`fem_tx_power_control_set` replaces the function :c:func:`fem_tx_gain_set`.
       The function :c:func:`fem_default_tx_output_power_get` replaces the function :c:func:`fem_default_tx_gain_get`.

Modem library
-------------

.. toggle::


   * For applications using :ref:`nrf_modem_lib_readme`:
     The option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART_ZEPHYR` is now deprecated.
     Use the option :kconfig:option:`CONFIG_NRF_MODEM_LIB_TRACE_BACKEND_UART` instead.

.. _migration_2.7_recommended:

Recommended changes
*******************

The following changes are recommended for your application to work optimally after the migration.

Samples and applications
========================

* For applications using build types:

  * The :makevar:`CONF_FILE` used for :ref:`app_build_additions_build_types` is now deprecated and is being replaced with the :makevar:`FILE_SUFFIX` variable, inherited from Zephyr.
    You can read more about it in :ref:`app_build_file_suffixes`, :ref:`cmake_options`, and the :ref:`related Zephyr documentation <zephyr:application-file-suffixes>`.

    If your application uses build types, it is recommended to update the :file:`sample.yaml` to use the new variable instead of :makevar:`CONF_FILE`.

* For applications using child images:

  * With the inheritance of Zephyr's :ref:`sysbuild in the |NCS| <configuration_system_overview_sysbuild>`, the :ref:`ug_multi_image` are deprecated.

    If your application uses parent and child images, it is recommended to migrate your application to sysbuild before the multi-image builds are removed in one of the upcoming |NCS| releases.
    See the :ref:`documentation in Zephyr <zephyr:sysbuild>` for more information about sysbuild.

Matter
------

.. toggle::

   * For the Matter samples and applications:

      * All Partition Manager configuration files (:file:`pm_static` files) have been removed from the :file:`configuration` directory.
        Instead, a :file:`pm_static_<BOARD>` file has been created for each target board and placed in the samples' directories.
        Setting the ``PM_STATIC_YML_FILE`` argument in the :file:`CMakeLists.txt` file has been removed, as it is no longer needed.

      * Configuration files :file:`Kconfig.mcuboot.defaults`, :file:`Kconfig.hci_ipc.defaults` and :file:`Kconfig.multiprotocol_rpmsg.defaults` that stored a default configuration for the child images have been removed.
        This was done because of the sysbuild integration and child images deprecation.

        The Matter samples and applications have been migrated to use sysbuild, though you can still use the child images.
        To migrate an application from the previous to the new version and keep using child images, complete the following steps:

        1. Copy the content of the image configuration file :file:`prj.conf` located in the `sysbuild/<image_name>` directory (for example,  :file:`sysbuild/mcuboot`) to the :file:`prj.conf` file located in the :file:`child_image/<image_name>` directory.
        #. Copy the content of the board configuration file located in the :file:`sysbuild/<image_name>/boards` directory (for example, :file:`sysbuild/mcuboot/boards/nrf52840dk_nrf52840.conf`) to the board file located in the :file:`child_image/<image_name>/boards` directory.

      * All Partition Manager configuration files (:file:`pm_static` files) with the suffix ``release`` have been removed from all samples.
        Those files are now redundant, since the new build system allows using the file without the additional suffix if you use :makevar:`FILE_SUFFIX` and it is available in the project's directory.
        For example, if you add ``-DFILE_SUFFIX=release`` to the CMake arguments while building an |NCS| Matter sample on the ``nrf52840dk/nrf52840`` target, the file :file:`pm_static_nrf52840dk_nrf52840.yaml` will be used as a fallback.
        This means that the file :file:`pm_static_nrf52840dk_nrf52840_release.yaml` with the exact same contents is not needed anymore.
        The :makevar:`CONF_FILE` argument is deprecated, but if you want to keep using it within your project, you need to create the :file:`pm_static_nrf52840dk_nrf52840_release.yaml` file and copy the content of the :file:`pm_static_nrf52840dk_nrf52840.yaml` file to it.

Libraries
=========

This section describes the changes related to libraries.

LwM2M carrier library
---------------------

.. toggle::

   * Many event defines have received new values.
     If you are using the values directly in your application, you need to check the events listed in :file:`lwm2m_carrier.h`.
     The most likely place these changes are needed is :ref:`serial_lte_modem` application, where :ref:`SLM_AT_CARRIER` are relying on the value of the defines instead of the names.
